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

#ifndef IYF_TYPE_MANAGER_HPP
#define IYF_TYPE_MANAGER_HPP

#include "assets/Asset.hpp"
#include "assets/AssetHandle.hpp"
#include "assets/metadata/Metadata.hpp"
#include "core/interfaces/GarbageCollecting.hpp"

#include <chrono>

namespace iyft {
class ThreadPool;
}

namespace iyf {
class AssetManager;

struct LoadedAssetData {
    LoadedAssetData(const Metadata& metadata, Asset& assetData, std::pair<std::unique_ptr<char[]>, std::int64_t> rawData) 
        : assetData(assetData), metadata(metadata), rawData(std::move(rawData)) {}
    
    Asset& assetData;
    const Metadata& metadata;
    std::pair<std::unique_ptr<char[]>, std::int64_t> rawData;
    
    virtual ~LoadedAssetData() {}
};

class TypeManager : public GarbageCollecting {
public:
    TypeManager(AssetManager* manager);
    
    virtual AssetType getType() = 0;
    virtual ~TypeManager() {}
    
protected:
    friend class AssetManager;
    
    /// \brief Reload the specified asset from disk.
    ///
    /// \warning This function can only be used if the Engine is running in editor mode.
    ///
    /// \throws std::logic_error if the engine is running in game mode.
    virtual bool refresh(StringHash nameHash, const fs::path& path, const Metadata& meta, std::uint32_t id) = 0;
    
    /// Load an asset that has not been loaded yet
    virtual std::pair<Asset*, AssetHandleRefCounter*> load(StringHash nameHash, const fs::path& path, const Metadata& meta, std::uint32_t& idOut, bool isAsync) = 0;
    
    /// Fetch a handle to an asset that has already been loaded
    virtual std::pair<Asset*, AssetHandleRefCounter*> fetch(std::uint32_t id) = 0;
    
    /// Return a handle that corresponds to a "missing" asset
    virtual std::pair<Asset*, AssetHandleRefCounter*> getMissingAssetHandle() = 0;
    
    friend class AssetManager;
    
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
    
    virtual void notifyMove(std::uint32_t id, StringHash sourceNameHash, StringHash destinationNameHash) = 0;
    
    void notifyRemoval(StringHash nameHash);
    void logLeakedAsset(std::size_t id, StringHash nameHash, std::uint32_t count);
    
    AssetManager* manager;
    iyft::ThreadPool* longTermWorkerPool;
};

}

#endif // IYF_TYPE_MANAGER_HPP
