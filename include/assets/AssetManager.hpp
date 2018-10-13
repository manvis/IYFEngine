// The IYFEngine
//
// Copyright (C) 2015-2018, Manvydas Šliamka
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
// conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
// of conditions and the following disclaimer in the documentation and/or other materials
// provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
// used to endorse or promote products derived from this software without specific prior
// written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
// WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP

#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/interfaces/GarbageCollecting.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/ChunkedVector.hpp"
#include "assets/metadata/Metadata.hpp"
#include "assets/Asset.hpp"
#include "assets/AssetHandle.hpp"
#include "graphics/MaterialPipelineDefinition.hpp"
#include "threading/ThreadPool.hpp"

#include <unordered_map>
#include <array>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <future>

namespace iyf {
class Engine;
class MeshLoader;

class AssetManager;

namespace con {
extern const std::chrono::milliseconds MinAsyncLoadWindow;
extern const std::chrono::milliseconds MaxAsyncLoadWindow;
}

class TypeManagerBase : public GarbageCollecting {
public:
    TypeManagerBase(AssetManager* manager);
    
    virtual AssetType getType() = 0;
    virtual ~TypeManagerBase() {}
    
    /// \brief Reload the specified asset from disk.
    ///
    /// \warning This function can only be used if the Engine is running in editor mode.
    ///
    /// \throws std::logic_error if the engine is running in game mode.
    virtual bool refresh(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t id) = 0;
protected:
    friend class AssetManager;
    
    enum class AssetsToEnableResult {
        HasAssetsToEnable, /// < The TypeManager has assets that are ready to be enabled
        NoAssetsToEnable, /// < The TypeManager doesn't have any assets that need to be enabled
        Busy /// < The TypeManager has assets that need to be enabled, but can't do it. E.g., a GPU upload buffer for this frame has been filled.
    };
    
    /// Tells the AssetManager that this TypeManager batches async loaded assets and executeBatchOperations() needs to be called on it
    /// to perform actual data uploads or processing.
    virtual bool canBatchAsyncLoadedAssets() const {
        return false;
    }
    
    /// Used by some classes derived from the TypeManager to process all asynchronously loaded data in one go. E.g., to avoid multiple expensive synchronization 
    /// or data upload operations.
    ///
    /// This call happens every frame. If nothing was added into the batch, the TypeManager should be smart enough to avoid any expensive operations.
    virtual void executeBatchOperations() {}
    
    /// If canBatchAsyncLoadedAssets() returns true and executeBatchOperations() may take a while, this function should return the expected run
    /// duration of the executeBatchOperations() function.
    virtual std::chrono::nanoseconds estimateBatchOperationDuration() {
        return std::chrono::nanoseconds(0);
    }
    
    /// \brief Enable a single asset that has been loaded asynchronously.
    virtual void enableAsyncLoadedAsset(bool canBatch) = 0;
    
    /// \brief Checks if the TypeManager can enable any Assets.
    virtual AssetsToEnableResult hasAssetsToEnable() const = 0;
    
    /// Our friend AssetManager calls this function once it finishes building the manifest. The "missing" assets
    /// are treated like any other assets and require the presence of a manifest to be loaded.
    virtual void initMissingAssetHandle() = 0;
    
    void logLeakedAsset(std::size_t id, hash32_t nameHash, std::uint32_t count);
    void notifyRemoval(hash32_t nameHash);
    
    virtual void notifyMove(std::uint32_t id, hash32_t sourceNameHash, hash32_t destinationNameHash) = 0;
    
    AssetManager* manager;
    iyft::ThreadPool* longTermWorkerPool;
};

struct LoadedAssetData {
    LoadedAssetData(const Metadata& metadata, Asset& assetData, std::pair<std::unique_ptr<char[]>, std::int64_t> rawData) 
        : assetData(assetData), metadata(metadata), rawData(std::move(rawData)) {}
    
    Asset& assetData;
    const Metadata& metadata;
    std::pair<std::unique_ptr<char[]>, std::int64_t> rawData;
    
    virtual ~LoadedAssetData() {}
};

struct AsyncLoadInfo {
    AsyncLoadInfo(std::future<std::unique_ptr<LoadedAssetData>> future, std::uint64_t estimatedSize) 
        : future(std::move(future)), estimatedSize(estimatedSize) {}
    /// The actual future of the asynchronously performed load operation
    std::future<std::unique_ptr<LoadedAssetData>> future;
    
    /// Used to determine if the TypeManager should try to upload more data or not (e.g., Mesh and Texture type managers
    /// use this value to check if the data will fit into the staging buffer this frame).
    std::uint64_t estimatedSize;
};

/// TypeManager handles the actual loading and unloading of //
/// \todo is the default chunk size ok?
template <typename T, size_t chunkSize = 8192>
class TypeManager : public TypeManagerBase {
protected:
    static const std::uint32_t ClearedAsset = std::numeric_limits<std::uint32_t>::max();
    
public:
    static_assert(std::is_base_of<Asset, T>::value, "All assets need to be derived from the Asset base class");
    static_assert(std::is_default_constructible<T>::value, "All assets need to be default constructible");
    
    TypeManager(AssetManager* manager, std::size_t initialFreeListSize = 1024) : TypeManagerBase(manager) {
        freeList.reserve(initialFreeListSize);
    }
    
    virtual ~TypeManager() { }
    
    /// Load an asset that has not been loaded yet
    inline AssetHandle<T> load(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t& idOut, bool isAsync);
    /// Fetch a handle to an asset that has already been loaded
    inline AssetHandle<T> fetch(std::uint32_t id);
    /// Return a handle that corresponds to a "missing" asset
    inline AssetHandle<T> getMissingAssetHandle();
    
    virtual void collectGarbage(GarbageCollectionRunPolicy policy = GarbageCollectionRunPolicy::FullCollection) override {
        // Pointer increments are signifficantly faster than lookups via [] operator. We can't
        // use the iterator here because we need to know the ids of the elements that need to be freed
        std::size_t chunkCount = counts.getChunkCount();
        std::size_t id = 0;
        
        for (std::size_t c = 0; c < chunkCount; ++c) {
            std::atomic<std::uint32_t>* current = counts.getChunkStart(c);
            std::atomic<std::uint32_t>* end = (c + 1 == chunkCount) ? (&counts[counts.size() - 1] + 1) : counts.getChunkEnd(c);
            
            // Free all assets with reference count equal to 0 and notify the parent AssetManager that
            // these assets should no longer be in lookup map
            if (policy == GarbageCollectionRunPolicy::FullCollection) {
                while (current != end) {
                    if ((*current) == 0) {
                        notifyRemoval(assets[id].getNameHash());
                        
                        freeList.push_back(id);
                        performFree(assets[id]);
                        assets[id].setLoaded(false);
                        
                        // Set this to a special value in order to prevent repeated clearing
                        *current = ClearedAsset;
                    }
                    
                    current++;
                    id++;
                }
            // Free all assets and log those that got leaked (still have live references)
            } else if (policy == GarbageCollectionRunPolicy::FullCollectionDuringDestruction) {
                while (current != end) {
                    T& asset = assets[id];
                    
                    if ((*current) != 0 && (*current) != ClearedAsset) {
                        logLeakedAsset(id, asset.getNameHash(), *current);
                    }
                    
                    // No need to clear already cleared values
                    if ((*current) != ClearedAsset) {
                        performFree(asset);
                    }
                    
                    current++;
                    id++;
                }
            }
        }
    }
protected:
    virtual void performFree(T& assetData) = 0;
    
    /// A concrete implementation of this function should read the file from disk into memory and perform any processing that 
    /// can be done without (or with minimal) synchronization. The second part is especially important because this function may be
    /// called at almost any time, from almost any thread.
    ///
    /// Assigning to assetData is considered safe because assets aren't supposed to be touched by other code while Asset::isLoaded()
    /// is false.
    ///
    /// \return everything that is required by enableAsset();
    virtual std::unique_ptr<LoadedAssetData> readFile(hash32_t nameHash, const fs::path& path, const Metadata& meta, T& assetData) = 0;
    
    /// A concrete implementation of this function should "enable" a loaded asset by finishing all preparations (e.g., uploading data 
    /// to the GPU) and setting Asset::setLoaded() to true. Always called on the main thread.
    ///
    /// \param canBatch If true, the TypeManager is allowed to use a batcher and delay the actual upload operation until executeBatchOperations()
    /// is called
    virtual void enableAsset(std::unique_ptr<LoadedAssetData> loadedAssetData, bool canBatch) = 0;
    
    /// Used by TypeManagers that perform batching. Must return a value that's equal to the final data upload size.
    virtual std::uint64_t estimateUploadSize(const Metadata&) const {
        return 0;
    }
    
    virtual void enableAsyncLoadedAsset(bool canBatch) final override {
        // This should only be called after hasAssetsToEnable
        assert(!toEnable.empty());
        assert(toEnable.front().future.valid());
        assert(toEnable.front().future.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
        
        std::unique_ptr<LoadedAssetData> data = toEnable.front().future.get();
        
        Asset& assetData = data->assetData;
        assert(assetData.isLoaded());
        
        enableAsset(std::move(data), canBatch);
        
        toEnable.pop_front();
    }
    
    /// \remark This default implementation doesn't handle the AssetsToEnableResult::Busy case because every TypeManager defines
    /// "busy" in a different way and some can't be busy at all.
    virtual AssetsToEnableResult hasAssetsToEnable() const override {
        if (toEnable.empty()) {
            return AssetsToEnableResult::NoAssetsToEnable;
        }
        
        // TODO Technically, depending on the size of the files and how work gets assigned to threads, it's possible that
        // a later operation will finish before an earlier one. Iterating over and checking many elements every frame  (e.g.,
        // when loading a World) doesn't seem to be a particularly good idea as well. If this proves to be a problem, I'll 
        // need to look for a smarter solution, e.g., use a syncronized queue that receives a LoadedAssetData object whenever
        // readFile() finishes. However, the current solution seems to be good enough for now.
        assert(toEnable.front().future.valid());
        if (toEnable.front().future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            return AssetsToEnableResult::HasAssetsToEnable;
        } else {
            return AssetsToEnableResult::NoAssetsToEnable;
        }
    }
    
    virtual bool refresh(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t id) final override {
        return refreshImpl(nameHash, path, meta, id);
    }
    
    inline bool refreshImpl(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t id);
    
    virtual void notifyMove(std::uint32_t id, [[maybe_unused]] hash32_t sourceNameHash, hash32_t destinationNameHash) override {
        T& asset = assets[id];
        assert(asset.getNameHash() == sourceNameHash);
        
        asset.setNameHash(destinationNameHash);
    }
    
    std::vector<std::uint32_t> freeList;
    ChunkedVector<std::atomic<std::uint32_t>, chunkSize> counts;
    ChunkedVector<T, chunkSize> assets;
    
    std::deque<AsyncLoadInfo> toEnable;
    
    /// A value that can safely be used for missing assets.
    AssetHandle<T> missingAssetHandle;
};

template <typename T, size_t chunkSize>
AssetHandle<T> TypeManager<T, chunkSize>::load(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t& idOut, bool isAsync) {
    // Find a free slot in the freeList or start using a new slot at the end
    std::uint32_t id;
    
    if (freeList.empty()) {
        assets.emplace_back();
        
        id = assets.size() - 1;
        counts.emplace_back(0);
        
        assets[id].setNameHash(nameHash);
        
        if (isAsync) {
            toEnable.emplace_back(longTermWorkerPool->addTaskWithResult(&TypeManager<T, chunkSize>::readFile, this, nameHash, path, meta, assets[id]), estimateUploadSize(meta));
        } else {
            std::unique_ptr<LoadedAssetData> loadedFile = readFile(nameHash, path, meta, assets[id]);
            enableAsset(std::move(loadedFile), false);
            assert(assets[id].isLoaded());
        }
        
        idOut = id;
        
        return AssetHandle<T>(&assets[id], &counts[id]);
    } else {
        id = freeList.back();
        freeList.pop_back();
        
        assets[id].setNameHash(nameHash);
        
        if (isAsync) {
            toEnable.emplace_back(longTermWorkerPool->addTaskWithResult(&TypeManager<T, chunkSize>::readFile, this, nameHash, path, meta, assets[id]), estimateUploadSize(meta));
        } else {
            std::unique_ptr<LoadedAssetData> loadedFile = readFile(nameHash, path, meta, assets[id]);
            enableAsset(std::move(loadedFile), false);
            assert(assets[id].isLoaded());
        }
        
        // Check if the asset has been cleared successfully
        assert(counts[id] == ClearedAsset);
        counts[id] = 0;
        
        idOut = id;
        
        return AssetHandle<T>(&assets[id], &counts[id]);
    }
}

template <typename T, size_t chunkSize>
AssetHandle<T> TypeManager<T, chunkSize>::fetch(std::uint32_t id) {
    return AssetHandle<T>(&assets[id], &counts[id]);
}

template <typename T, size_t chunkSize>
AssetHandle<T> TypeManager<T, chunkSize>::getMissingAssetHandle() {
    return missingAssetHandle;
}

/// The AssetManager is an intermediary between asset users, such as Entity component objects, and TypeManager objects
/// that perform the actual loading, unloading and reference counting of specific asset types.
///
/// \todo Async loading
class AssetManager : private NonCopyable {
public:
    AssetManager(Engine* engine);
    ~AssetManager();
    
    /// Determines the AssetType by checking the file's extension. If the format isn't in the map of supported formats, AssetType::Custom
    /// will be returned.
    ///
    /// \remark This function is supposed to be used on files you want to import and not imported files.
    ///
    /// \warning This function DOES NOT access the contents of the file. This leads to some limitations. E.g., since animations are
    /// imported from mesh files, this function will return AssetType::Mesh, even if the file contains nothing but an animation library.
    static AssetType GetAssetTypeFromExtension(const fs::path& pathToFile);
    
    /// Computes the hash of the provided path. If the path starts with con::ImportPath, it is stripped before computing the hash.
    static inline hash32_t ComputeNameHash(const fs::path& sourcePath) {
        if (*sourcePath.begin() == con::ImportPath) {
            return HS(sourcePath.lexically_relative(con::ImportPath).generic_string());
        } else {
            return HS(sourcePath.generic_string());
        }
    }
    
    /// \brief Sets the async load window. See getAsyncLoadWindow() for more info about what it is.
    ///
    /// The loadWindow is clamped using std::clamp(loadWindow, con::MinAsyncLoadWindow, con::MaxAsyncLoadWindow)
    void setAsyncLoadWindow(std::chrono::milliseconds loadWindow);
    
    /// \brief Returns the current async load window.
    ///
    /// An async load window is the maximum amount of time that the AssetManager is allowed to spend enabling loaded assets
    /// every frame.
    ///
    /// Enabling an asset usually involves some extra work, e.g., enabling a mesh uploads its data to the GPU. If a window is
    /// too low, loading may take forever, but you'll have a stable and high framerate. If a window is too high, loading will
    /// take a lot less time, however the framerate may degrade to a slide show. Both high and low load windows may be useful.
    /// If you are showing nothing but a loading bar, a high async load window makes sense. On the other hand, if you're masking
    /// loading with an endless corridor or an elevator ride, a lower value should be used to maintain an interactive framerate.
    ///
    /// \returns The async load window.
    std::chrono::milliseconds getAsyncLoadWindow() const;
    
    /// Creates and initializes all type managers and loads all system assets that will be stored in memory
    /// persistently
    void initialize();
    
    /// Unloads all system assets and disposes of type managers
    void dispose();
    
    /// \todo Running full collection every frame becomes extemely wasteful when many assets are loaded.
    /// Running it every several frames or seconds (once again, with many objects) gives slowdown spikes.
    /// More GarbageCollectionRunPolicy types should be created and they should allow for partial collection.
    /// Moreover, AssetManager should be smarter and call collectGarbage() on TypeManager objects selectively.
    ///
    /// \warning This class does not and should not inherit from GarbageCollecting interface because it makes its
    /// own decisions on GarbageCollectionRunPolicy.
    void collectGarbage();
    
    /// Enables the assets that were loaded asynchronously, if any.
    void enableLoadedAssets();
    
    /// \brief Either loads an asset or retrieves a handle to an already loaded one.
    ///
    /// Asset loads can be synchronous (async is false) or asynchronous (async is true).
    ///
    /// If an asset is loaded synchronously, it becomes available and safe to use immediately after this function returns.
    /// If an asset is loaded asynchronously, it cannot be used until Asset::isLoaded() returns true. Depending on the asset type, may take a
    /// while.
    ///
    /// \warning TODO FIXME Synchronous loading may introduce race conditions under certain circumstances
    ///
    /// \param nameHash hashed path to an asset
    /// \return An AssetHandle
    template <typename T>
    inline AssetHandle<T> load(hash32_t nameHash, bool async) {
        return loadImpl<T>(nameHash, async);
    }
    
    template <typename T>
    inline AssetHandle<T> getMissingAsset(AssetType type) {
        TypeManager<T>* typeManager = dynamic_cast<TypeManager<T>*>(typeManagers[static_cast<std::size_t>(type)].get());
        
        assert(typeManager != nullptr);
        assert(typeManager->getType() == type);
        
        return typeManager->getMissingAssetHandle();
    }
    
    template <typename T>
    inline AssetHandle<T> getSystemAsset(const std::string& name) {
        // System assets should have already been loaded by this point, which it's safe to use a synchronous load here.
        return load<T>(getSystemAssetNameHash(name), false);
    }
    
    inline hash32_t getSystemAssetNameHash(const std::string& name) {
        return HS("raw/system/" + name);
    }
    
    /// Checks for hash collisions. Should be called before performing asset conversion.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in game mode.
    ///
    /// \return A path to the colliding file or a nullopt if no hash collisions have been detected.
    std::optional<fs::path> checkForHashCollision(hash32_t nameHash, const fs::path& checkPath) const;
    
    /// \brief Obtains a copy of a Metadata object that corresponds to the file with the specified nameHash or an std::nullopt if
    /// the nameHash wasn't found in the manifest.
    ///
    /// \remark This method is always thread safe. However, when running in editor mode, it is possible that the Metadata object is
    /// no longer relevant by the time you get to read it (e.g., the file may have been replaced, updated or deleted)
    inline std::optional<Metadata> getMetadataCopy(hash32_t nameHash) const {
        auto manifestLock = editorMode ? std::unique_lock<std::mutex>(manifestMutex) : std::unique_lock<std::mutex>();
        
        auto result = manifest.find(nameHash);
        if (result != manifest.end()) {
            return result->second.metadata;
        } else {
            return std::nullopt;
        }
    }
    
    /// \brief Obtains a copy of a path that corresponds to the file with the specified nameHash or an std::nullopt if the nameHash 
    /// wasn't found in the manifest.
    ///
    /// \remark This method is always thread safe. However, when running in editor mode, it is possible that the path is
    /// no longer relevant by the time you get to read it (e.g., the file may have been deleted)
    inline std::optional<fs::path> getAssetPathCopy(hash32_t nameHash) const {
        auto manifestLock = editorMode ? std::unique_lock<std::mutex>(manifestMutex) : std::unique_lock<std::mutex>();
        
        auto result = manifest.find(nameHash);
        if (result != manifest.end()) {
            return result->second.path;
        } else {
            return std::nullopt;
        }
    }
    
    /// \breif Obtains a const pointer to a Metadata object or a nullptr if the nameHash wasn't found in the manifest.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in game mode.
    ///
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in editor mode.
    inline const Metadata* getMetadata(hash32_t nameHash) const {
        if (editorMode) {
            throw std::logic_error("This method can't be used when the engine is running in editor mode.");
        }
        
        auto result = manifest.find(nameHash);
        if (result != manifest.end()) {
            return &result->second.metadata;
        } else {
            return nullptr;
        }
    }
    
    /// \breif Obtains a const pointer to a file path or a nullptr if the nameHash wasn't found in the manifest.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in game mode.
    ///
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in editor mode.
    inline const fs::path* getAssetPath(hash32_t nameHash) const {
        if (editorMode) {
            throw std::logic_error("This method can't be used when the engine is running in editor mode.");
        }
        
        auto result = manifest.find(nameHash);
        if (result != manifest.end()) {
            return &result->second.path;
        } else {
            return nullptr;
        }
    }
    
    inline Engine* getEngine() {
        return engine;
    }
    
    inline bool isEditorMode() const {
        return editorMode;
    }
    
    inline bool isGameMode() const {
        return !editorMode;
    }
    
    /// Writes metadata of a single asset to file
    ///
    /// \return true if written successfully, false if not (e.g., when no asset with the specified nameHash exists)
    bool serializeMetadata(hash32_t nameHash, Serializer& file);
    
    /// \return Number of unique assets that are listed in the manifest and can be loaded
    std::size_t getRegisteredAssetCount() const {
        auto manifestLock = editorMode ? std::unique_lock<std::mutex>(manifestMutex) : std::unique_lock<std::mutex>();
        return manifest.size();
    }
    
    /// \return Number of unique assets that are currently loaded
    std::size_t getLoadedAssetCount() const {
        std::unique_lock<std::mutex> lock(loadedAssetListMutex);
        return loadedAssets.size();
    }
    
    /// Obtain a const observer pointer to a specific TypeManagerBase object. This method is typically used
    /// to retrieve debug data directly from type managers.
    ///
    /// The returned pointer is guaranteed to stay valid until Engine::quit() is called.
    inline const TypeManagerBase* getTypeManager(AssetType type) const {
        return typeManagers[static_cast<std::size_t>(type)].get();
    }
//--------------- Editor API Start
    /// Used by the editor Reimports and reloads the asset from disk if it's loaded.
    ///
    /// \param type The type of the asset. Could be determined by examining the path, howerver, it's always present when requestAssetRefresh needs to be called
    /// \param path The destination path of the imported file
    void requestAssetRefresh(AssetType type, const fs::path& path);
   
    /// Used by the editor to delete a specific asset (or a folder of assets)
    /// 
    /// \throws std::logic_error if this AssetManager was constructed using an Engine running in game mode.
    void requestAssetDeletion(const fs::path& path, bool isDir);
    
    /// Used by the editor to rename or move a specific asset (or a folder of assets)
    /// 
    /// \throws std::logic_error if this AssetManager was constructed using an Engine running in game mode.
    void requestAssetMove(const fs::path& sourcePath, const fs::path& destinationPath, bool isDir);
    
    /// Removes all non-system assets from the manifest. Typically used when closing a Project. For performance reasons, 
    /// this function should only be called after all Entity objects that use non-system assets have been unloaded.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in editor mode.
    ///
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in game mode.
    void removeNonSystemAssetsFromManifest();
//---------------- Editor API end
    struct ManifestElement {
        fs::path path;
        AssetType type;
        bool systemAsset;
        Metadata metadata;
    };
private:
    /// \todo an empty handle would be nicer than an assert
    template <typename T>
    inline AssetHandle<T> loadImpl(hash32_t nameHash, bool isAsync) {
        auto manifestLock = editorMode ? std::unique_lock<std::mutex>(manifestMutex) : std::unique_lock<std::mutex>();
        std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
        
        const auto assetReference = loadedAssets.find(nameHash);
        
        assert(manifest.find(nameHash) != manifest.end());
        const ManifestElement& asset = manifest[nameHash];
        
        TypeManager<T>* typeManager = dynamic_cast<TypeManager<T>*>(typeManagers[static_cast<std::size_t>(asset.type)].get());
        
        if (assetReference == loadedAssets.end()) {
            std::uint32_t id;
            
            AssetHandle<T> loadedHandle = typeManager->load(nameHash, asset.path, asset.metadata, id, isAsync);
            loadedAssets[nameHash] = {asset.type, id};
            return loadedHandle;
        } else {
            return typeManager->fetch(assetReference->second.second);
        }
    }
    
    /// Called by checkForHashCollision(). Needed to avoid a deadlock when checking for hash collisions during asset move.
    std::optional<fs::path> checkForHashCollisionImpl(hash32_t nameHash, const fs::path& checkPath) const;
    
    /// \brief Adds a file to the manifest.
    ///
    /// \todo many things written here are no longer relevant or true.
    ///
    /// \warning Checking for nameHash collisions is the responsibility of the caller. Calling appendAssetToManifest() will 
    /// overrwrite any existing ManifestElement identified by the same nameHash, which will likely lead to undefined behaviour.
    /// It's especially dangerous if the asset types differ or the assets are already loaded.
    /// 
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in editor mode.
    ///
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in game mode.
    /// 
    /// \param nameHash Hashed path to the file that you want to add to the manifest.
    /// \param path Path to the file that you want to add to the manifest
    /// \param metadata additional metadata
    void appendAssetToManifest(hash32_t nameHash, const fs::path& path, const Metadata& metadata);
    
    /// \brief Removes a file from the manifest. If the asset in question has already been loaded, replaces it with a "missing" one.
    ///
    /// \todo many things written here are no longer relevant or true.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in editor mode.
    /// 
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in game mode.
    ///
    /// \param nameHash Hash of the path to the file that you want to remove from the manifest
    void removeAssetFromManifest(hash32_t nameHash);
    
    friend class Engine;
    /// Builds the manifest from all converted assets that reside in the asset folder for the current platform and have corresponding metadata.
    void buildManifestFromFilesystem();
    
    friend class TypeManagerBase;
    void notifyRemoval(hash32_t handle) {
        std::lock_guard<std::mutex> lock(loadedAssetListMutex);
        std::size_t count = loadedAssets.erase(handle);
        assert(count != 0);
    }
    
    void loadSystemAssets();
    
    Engine* engine;
    
//    std::vector<Pipeline> pipelines;
//    std::vector<SubMeshData> WTH; // DATA ranges??? For defrag and reuse
//    std::vector<Texture> MLML;
    
    /// If the Engine is running in editor mode, the manifest may be updated and/or read from multiple threads. In this case,
    /// a mutex is needed prevent data races.
    ///
    /// On the other hand, std containers can be safely read from multiple threads. Therefore, if the Engine is running in game 
    /// mode and the manifest is read-only (which it becomes immediately after it is loaded), the mutex isn't required.
    ///
    /// \remark Now that all manifest access is more or less synchronous and happens in well defined places, this mutex is mostly
    /// useless, but I'm keeping it just in case.
    mutable std::mutex manifestMutex;
    
    /// Assets can be loaded both synchronously and asynchronously and the loaded asset map may be modified from multiple
    /// threads.
    mutable std::mutex loadedAssetListMutex;
    
    /// Manifest maps hash32_t file name hashes to unhashed filenames + metadata that can be useful to know before loading
    std::unordered_map<hash32_t, ManifestElement> manifest;
    
    /// A pair of an asset type (to choose the appropriate TypeManager) and id in that manager
    using AssetTypeID = std::pair<AssetType, std::uint32_t>;
    
    /// Maps hashed paths of loaded Asset objects to AssetTypeID pairs
    std::unordered_map<hash32_t, AssetTypeID> loadedAssets;
    
    std::array<std::unique_ptr<TypeManagerBase>, static_cast<std::size_t>(AssetType::ANY)> typeManagers;
    
    /// A vector of MaterialPipelineDefinition objects that correspond to all pipeline families that can be used
    /// by the Meshes
    std::unordered_map<hash32_t, MaterialPipelineDefinition> availablePipelines;
    
    /// Used only when running in editor mode. This thread regularly polls the FileSystemWatcher and performs
    /// imports of new or modified assets
    std::thread importManagementThread;
    
    std::chrono::milliseconds asyncLoadWindow;
    
    bool editorMode;
    bool isInit;
    
    static const std::unordered_map<std::string, AssetType> ExtensionToType;
};

template <typename T, size_t chunkSize>
inline bool TypeManager<T, chunkSize>::refreshImpl(hash32_t nameHash, const fs::path& path, const Metadata& meta, std::uint32_t id) {
    if (manager->isGameMode()) {
        throw std::logic_error("This method can't be used when the engine is running in game mode.");
    }
    
    // Everything must happen synchronously
    performFree(assets[id]);
    std::unique_ptr<LoadedAssetData> loadedFile = readFile(nameHash, path, meta, assets[id]);
    enableAsset(std::move(loadedFile), false);
    assets[id].setNameHash(nameHash);
    
    return true;
}

inline void TypeManagerBase::notifyRemoval(hash32_t nameHash) {
    manager->notifyRemoval(nameHash);
}

}


#endif /* ASSETMANAGER_HPP */

