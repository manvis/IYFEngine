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

#ifndef IYF_CHUNKED_VECTOR_TYPE_MANAGER_HPP
#define IYF_CHUNKED_VECTOR_TYPE_MANAGER_HPP

#include "assets/typeManagers/TypeManager.hpp"
#include "utilities/ChunkedVector.hpp"
#include "threading/ThreadPool.hpp"
#include "logging/Logger.hpp"

#include <future>
#include <memory>

namespace iyf {
struct AsyncLoadInfo {
    AsyncLoadInfo(std::future<std::unique_ptr<LoadedAssetData>> future, std::uint64_t estimatedSize) 
        : future(std::move(future)), estimatedSize(estimatedSize) {}
    /// The actual future of the asynchronously performed load operation
    std::future<std::unique_ptr<LoadedAssetData>> future;
    
    /// Used to determine if the TypeManager should try to upload more data or not (e.g., Mesh and Texture type managers
    /// use this value to check if the data will fit into the staging buffer this frame).
    std::uint64_t estimatedSize;
};

/// \todo is the default chunk size ok?
template <typename T, size_t chunkSize = 8192>
class ChunkedVectorTypeManager : public TypeManager {
protected:
    static const std::uint32_t ClearedAsset = std::numeric_limits<std::uint32_t>::max();
    
public:
    static_assert(std::is_base_of<Asset, T>::value, "All assets need to be derived from the Asset base class");
    static_assert(std::is_default_constructible<T>::value, "All assets need to be default constructible");
    
    ChunkedVectorTypeManager(AssetManager* manager, std::size_t initialFreeListSize = 1024) : TypeManager(manager), missingAssetHandle(AssetHandle<T>::CreateInvalid()) {
        freeList.reserve(initialFreeListSize);
    }
    
    virtual ~ChunkedVectorTypeManager() { }
    
    virtual std::pair<Asset*, AssetHandleRefCounter*> load(StringHash nameHash, const Path& path, const Metadata& meta, std::uint32_t& idOut, bool isAsync) final override {
        // Find a free slot in the freeList or start using a new slot at the end
        std::uint32_t id;
        
        if (freeList.empty()) {
            assets.emplace_back();
            
            id = assets.size() - 1;
            counts.emplace_back(0);
            
            assets[id].setNameHash(nameHash);
            
            if (isAsync) {
                toEnable.emplace_back(longTermWorkerPool->addTaskWithResult(&ChunkedVectorTypeManager<T, chunkSize>::readFile, this, nameHash, path, meta, assets[id]), estimateUploadSize(meta));
            } else {
                std::unique_ptr<LoadedAssetData> loadedFile = readFile(nameHash, path, meta, assets[id]);
                enableAsset(std::move(loadedFile), false);
                assert(assets[id].isLoaded());
            }
            
            idOut = id;
            
            if (isLoggingCreations()) {
                logAssetCreation(id, nameHash, false, isAsync);
            }
            
            return std::make_pair(&assets[id], &counts[id]);
        } else {
            id = freeList.back();
            freeList.pop_back();
            
            assets[id].setNameHash(nameHash);
            
            if (isAsync) {
                toEnable.emplace_back(longTermWorkerPool->addTaskWithResult(&ChunkedVectorTypeManager<T, chunkSize>::readFile, this, nameHash, path, meta, assets[id]), estimateUploadSize(meta));
            } else {
                std::unique_ptr<LoadedAssetData> loadedFile = readFile(nameHash, path, meta, assets[id]);
                enableAsset(std::move(loadedFile), false);
                assert(assets[id].isLoaded());
            }
            
            // Check if the asset has been cleared successfully
            assert(counts[id] == ClearedAsset);
            counts[id] = 0;
            
            idOut = id;
            
            return std::make_pair(&assets[id], &counts[id]);
        }
    }
    
    virtual std::pair<Asset*, AssetHandleRefCounter*> fetch(std::uint32_t id) final override {
        if (isLoggingCreations()) {
            logAssetCreation(id, assets[id].getNameHash(), false, false);
        }
        
        return std::make_pair(&assets[id], &counts[id]);
    }
    
    virtual std::pair<Asset*, AssetHandleRefCounter*> getMissingAssetHandle() final override {
        throw std::runtime_error("IMPLEMENT ME");
        //return missingAssetHandle;
    }
    
    /// A concrete implementation of this function should read the file from disk into memory and perform any processing that 
    /// can be done without (or with minimal) synchronization. The second part is especially important because this function may be
    /// called at almost any time, from almost any thread.
    ///
    /// Assigning to assetData is considered safe because assets aren't supposed to be touched by other code while Asset::isLoaded()
    /// is false.
    ///
    /// \return everything that is required by enableAsset();
    virtual std::unique_ptr<LoadedAssetData> readFile(StringHash nameHash, const Path& path, const Metadata& meta, T& assetData) = 0;
    
    /// A concrete implementation of this function should "enable" a loaded asset by finishing all preparations (e.g., uploading data 
    /// to the GPU) and setting Asset::setLoaded() to true. Always called on the main thread.
    ///
    /// \param canBatch If true, the TypeManager is allowed to use a batcher and delay the actual upload operation until executeBatchOperations()
    /// is called
    virtual void enableAsset(std::unique_ptr<LoadedAssetData> loadedAssetData, bool canBatch) = 0;
    
    virtual void performFree(T& assetData) = 0;
    
    virtual void collectGarbage(GarbageCollectionRunPolicy policy = GarbageCollectionRunPolicy::FullCollection) override {
        // Pointer increments are signifficantly faster than lookups via [] operator. We can't
        // use the iterator here because we need to know the ids of the elements that need to be freed
        std::size_t chunkCount = counts.chunkCount();
        std::size_t id = 0;
        
        for (std::size_t c = 0; c < chunkCount; ++c) {
            std::atomic<std::uint32_t>* current = counts.getChunkStart(c);
            std::atomic<std::uint32_t>* end = (c + 1 == chunkCount) ? (&counts[counts.size() - 1] + 1) : counts.getChunkEnd(c);
            
            // Free all assets with reference count equal to 0 and notify the parent AssetManager that
            // these assets should no longer be in lookup map
            if (policy == GarbageCollectionRunPolicy::FullCollection) {
                while (current != end) {
                    if ((*current) == 0) {
                        if (isLoggingRemovals()) {
                            logAssetRemoval(id, assets[id].getNameHash());
                        }
                        
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
    
    virtual bool refresh(StringHash nameHash, const Path& path, const Metadata& meta, std::uint32_t id) final override {
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
    
    virtual void notifyMove(std::uint32_t id, [[maybe_unused]] StringHash sourceNameHash, StringHash destinationNameHash) override {
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

}

#endif // IYF_CHUNKED_VECTOR_TYPE_MANAGER_HPP
