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

#ifndef IYF_ASSET_MANAGER_HPP
#define IYF_ASSET_MANAGER_HPP

#include "assets/Asset.hpp"
#include "assets/AssetHandle.hpp"
#include "assets/metadata/Metadata.hpp"
#include "assets/typeManagers/TypeManager.hpp"
#include "utilities/NonCopyable.hpp"

#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace iyf {
class Engine;
class MeshLoader;

class AssetManager;
class TypeManager;

namespace con {
extern const std::chrono::milliseconds MinAsyncLoadWindow;
extern const std::chrono::milliseconds MaxAsyncLoadWindow;
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
    static inline StringHash ComputeNameHash(const fs::path& sourcePath) {
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
    inline AssetHandle<T> load(StringHash nameHash, bool async) {
        auto manifestLock = editorMode ? std::unique_lock<std::mutex>(manifestMutex) : std::unique_lock<std::mutex>();
        std::lock_guard<std::mutex> assetLock(loadedAssetListMutex);
        
        const auto assetReference = loadedAssets.find(nameHash);
        
        assert(manifest.find(nameHash) != manifest.end());
        const ManifestElement& asset = manifest[nameHash];
        
        TypeManager* typeManager = typeManagers[static_cast<std::size_t>(asset.type)].get();
        assert(typeManager != nullptr);
        assert(typeManager->getType() == asset.type);
        
        if (assetReference == loadedAssets.end()) {
            std::uint32_t id;
            
            auto result = typeManager->load(nameHash, asset.path, asset.metadata, id, async);
            loadedAssets[nameHash] = {asset.type, id};
            return AssetHandle<T>(static_cast<T*>(result.first), result.second);
        } else {
            auto result = typeManager->fetch(assetReference->second.second);
            return AssetHandle<T>(static_cast<T*>(result.first), result.second);
        }
    }
    
    template <typename T>
    inline AssetHandle<T> getMissingAsset(AssetType type) {
        TypeManager* typeManager = typeManagers[static_cast<std::size_t>(type)].get();
        
        assert(typeManager != nullptr);
        assert(typeManager->getType() == type);
        
        auto result = typeManager->getMissingAssetHandle();
        return AssetHandle<T>(static_cast<T*>(result.first), result.second);
    }
    
    template <typename T>
    inline AssetHandle<T> getSystemAsset(const std::string& name) {
        // System assets should have already been loaded by this point, which it's safe to use a synchronous load here.
        return load<T>(getSystemAssetNameHash(name), false);
    }
    
    inline StringHash getSystemAssetNameHash(const std::string& name) {
        return HS("raw/system/" + name);
    }
    
    /// Checks for hash collisions. Should be called before performing asset conversion.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in game mode.
    ///
    /// \return A path to the colliding file or a nullopt if no hash collisions have been detected.
    std::optional<fs::path> checkForHashCollision(StringHash nameHash, const fs::path& checkPath) const;
    
    /// \brief Obtains a copy of a Metadata object that corresponds to the file with the specified nameHash or an std::nullopt if
    /// the nameHash wasn't found in the manifest.
    ///
    /// \remark This method is always thread safe. However, when running in editor mode, it is possible that the Metadata object is
    /// no longer relevant by the time you get to read it (e.g., the file may have been replaced, updated or deleted)
    inline std::optional<Metadata> getMetadataCopy(StringHash nameHash) const {
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
    inline std::optional<fs::path> getAssetPathCopy(StringHash nameHash) const {
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
    inline const Metadata* getMetadata(StringHash nameHash) const {
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
    inline const fs::path* getAssetPath(StringHash nameHash) const {
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
    bool serializeMetadata(StringHash nameHash, Serializer& file);
    
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
    
    /// Obtain a const observer pointer to a specific TypeManager object. This method is typically used
    /// to retrieve debug data directly from type managers.
    ///
    /// The returned pointer is guaranteed to stay valid until Engine::quit() is called.
    inline const TypeManager* getTypeManager(AssetType type) const {
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
    /// Called by checkForHashCollision(). Needed to avoid a deadlock when checking for hash collisions during asset move.
    std::optional<fs::path> checkForHashCollisionImpl(StringHash nameHash, const fs::path& checkPath) const;
    
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
    void appendAssetToManifest(StringHash nameHash, const fs::path& path, const Metadata& metadata);
    
    /// \brief Removes a file from the manifest. If the asset in question has already been loaded, replaces it with a "missing" one.
    ///
    /// \todo many things written here are no longer relevant or true.
    ///
    /// \warning This function can only be used if the Engine that was passed to the constructor is running in editor mode.
    /// 
    /// \throws std::logic_error if this AssetManager was constructed using an Engine instance running in game mode.
    ///
    /// \param nameHash Hash of the path to the file that you want to remove from the manifest
    void removeAssetFromManifest(StringHash nameHash);
    
    friend class Engine;
    /// Builds the manifest from all converted assets that reside in the asset folder for the current platform and have corresponding metadata.
    void buildManifestFromFilesystem();
    
    friend class TypeManager;
    void notifyRemoval(StringHash handle) {
        std::lock_guard<std::mutex> lock(loadedAssetListMutex);
        std::size_t count = loadedAssets.erase(handle);
        assert(count != 0);
    }
    
    void loadSystemAssets();
    
    Engine* engine;
    
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
    
    /// Manifest maps StringHash file name hashes to unhashed filenames + metadata that can be useful to know before loading
    std::unordered_map<StringHash, ManifestElement> manifest;
    
    /// A pair of an asset type (to choose the appropriate TypeManager) and id in that manager
    using AssetTypeID = std::pair<AssetType, std::uint32_t>;
    
    /// Maps hashed paths of loaded Asset objects to AssetTypeID pairs
    std::unordered_map<StringHash, AssetTypeID> loadedAssets;
    
    std::array<std::unique_ptr<TypeManager>, static_cast<std::size_t>(AssetType::ANY)> typeManagers;
    
    std::chrono::milliseconds asyncLoadWindow;
    
    bool editorMode;
    bool isInit;
    
    static const std::unordered_map<std::string, AssetType> ExtensionToType;
};
}


#endif // IYF_ASSET_MANAGER_HPP

