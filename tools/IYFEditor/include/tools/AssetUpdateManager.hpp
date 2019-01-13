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

#ifndef IYF_ASSET_UPDATE_MANAGER_HPP
#define IYF_ASSET_UPDATE_MANAGER_HPP

#include <mutex>
#include <thread>
#include <future>
#include <chrono>
#include <map>

#include "core/filesystem/cppFilesystem.hpp"
#include "core/filesystem/FileSystemEvent.hpp"

#include "utilities/hashing/Hashing.hpp"

namespace iyf {
class Engine;
class FileSystemWatcher;
}

namespace iyf::editor {
class ConverterManager;
class ConverterState;

/// The type of the AssetOperation
///
/// We don't do moves. It's simpler to mark an asset as deleted, followed by a creation of a new asset
enum class AssetOperationType : std::uint32_t {
    Created, Updated, Deleted, Moved
};

struct AssetOperation {
    fs::path destination;
    StringHash nameHash;
    AssetOperationType type;
    std::chrono::steady_clock::time_point timePoint;
    bool isDirectory;
};

class AssetUpdateManager {
public:
    AssetUpdateManager(Engine* engine);
    ~AssetUpdateManager();
    
    void initialize();
    void dispose();
    
    bool update();
    void forceReimport(const fs::path& path);
private:
    void watcherCallback(std::vector<FileSystemEvent> eventList);
    
    /// Asynchronously proocesses the asset changes and, if everything succeeded, returns a non-nullptr
    /// function that must be run in the main thread in order to update the AssetManager state.
    std::function<void()> executeAssetOperation(std::string path, AssetOperation op) const;
    
    Engine* engine;
    fs::path importsDir;
    
    std::unique_ptr<ConverterManager> converterManager;
    
    std::unique_ptr<FileSystemWatcher> fileSystemWatcher;
    std::thread fileSystemWatcherThread;
    std::chrono::steady_clock::time_point lastFileSystemUpdate;
    
    std::map<fs::path, AssetOperation> assetOperations;
    std::pair<fs::path, AssetOperation> currentlyProcessedAsset;
    
    std::mutex assetOperationMutex;
    std::future<std::unique_ptr<ConverterState>> assetConverterInitFuture;
    std::future<std::function<void()>> assetProcessingFuture;
    
    std::atomic<bool> watcherThreadRunning;
    bool isInit;
};
}

#endif // IYF_ASSET_UPDATE_MANAGER_HPP
