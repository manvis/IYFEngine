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

#include "tools/AssetUpdateManager.hpp"

#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/Project.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/FileSystemWatcher.hpp"

#include "assets/AssetManager.hpp"
#include "assetImport/ConverterManager.hpp"

#include "threading/ThreadPool.hpp"
#include "threading/ThreadProfiler.hpp"

#include "imgui.h"

// If defined, all received file events be logged
//#define IYF_EDITOR_LOG_RECEIVED_FILE_EVENT_LIST

/// How long the size of a file needs to remain stable to be considered safe for writing.
/// Used to determine if the file is safe to import
const float MIN_STABLE_FILE_SIZE_DURATION_SECONDS = 0.25f;

namespace iyf::editor {
AssetUpdateManager::AssetUpdateManager(Engine* engine) : engine(engine) {}
AssetUpdateManager::~AssetUpdateManager() {}

void AssetUpdateManager::initialize() {
    if (isInit) {
        return;
    }
    
    importsDir = engine->getProject()->getRootPath() / con::ImportPath;
    
    FileSystemWatcherCreateInfo fsci;
    fsci.handler = std::bind(&AssetUpdateManager::watcherCallback, this, std::placeholders::_1);
    fsci.writeChangesToLog = false;
    
    MonitoredDirectory md;
    md.path = importsDir;
    md.recursive = true;
    md.monitoredEvents = FileSystemEventFlags::All;
    
    fsci.monitoredDirectories.push_back(std::move(md));
    
    watcherThreadRunning = true;
    
    lastFileSystemUpdate = std::chrono::steady_clock::now();
    fileSystemWatcher = FileSystemWatcher::MakePlatformFilesystemWatcher(fsci);
    fileSystemWatcherThread = std::thread([this](){
        IYFT_PROFILER_NAME_THREAD("FileSystemWatcher");
        
        while (watcherThreadRunning) {
            auto now = std::chrono::steady_clock::now();
//             std::chrono::duration<float> delta = now - lastFileSystemUpdate;
            lastFileSystemUpdate = now;
            
            // This checks the file system state and sends all changes to the watcherCallback() function
            fileSystemWatcher->poll();
            
            // TODO Should sleep take more/less time?
            IYFT_PROFILE(FileSystemWatcherSleep, iyft::ProfilerTag::AssetConversion);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    const FileSystem* fs = engine->getFileSystem();
    converterManager = std::make_unique<ConverterManager>(fs, "");
    const fs::path platformDataPath = converterManager->getAssetDestinationPath(con::GetCurrentPlatform());
    const fs::path realPlatformDataPath = fs->getRealDirectory(platformDataPath.generic_string());
    LOG_V("Converted assets for the current platform will be written to: " << realPlatformDataPath);
    
    isInit = true;
}

void AssetUpdateManager::watcherCallback(std::vector<FileSystemEvent> eventList) {
#ifdef IYF_EDITOR_LOG_RECEIVED_FILE_EVENT_LIST
    for (const auto& e : eventList) {
        switch (e.getType()) {
        case FileSystemEventFlags::Created:
            LOG_D("-FS-CREATED:" << "\n\tOrigin: " << (e.getOrigin() == FileSystemEventOrigin::Directory ? "directory" : "file")
                                 << "\n\tSource: " << e.getSource());
            break;
        case FileSystemEventFlags::Deleted:
            LOG_D("-FS-DELETED:" << "\n\tOrigin: " << (e.getOrigin() == FileSystemEventOrigin::Directory ? "directory" : "file") 
                                 << "\n\tSource: " << e.getSource());
            break;
        case FileSystemEventFlags::Modified:
            LOG_D("-FS-MODIFIED:" << "\n\tOrigin: " << (e.getOrigin() == FileSystemEventOrigin::Directory ? "directory" : "file")
                                  << "\n\tSource: " << e.getSource());
            break;
        case FileSystemEventFlags::Moved:
            LOG_D("-FS-MOVED:" << "\n\tOrigin: " << (e.getOrigin() == FileSystemEventOrigin::Directory ? "directory" : "file")
                               << "\n\tSource: " << e.getSource() << "\n\tDestination: " << e.getDestination());
            break;
        default:
            throw std::runtime_error("Invalid event type");
        }
    }
#endif // IYF_EDITOR_LOG_RECEIVED_FILE_EVENT_LIST
    
    std::lock_guard<std::mutex> lock(assetOperationMutex);
    for (const auto& e : eventList) {
        const bool isDirectory = e.getOrigin() == FileSystemEventOrigin::Directory;
        
        const fs::path finalSourcePath = e.getSource().lexically_relative(importsDir);
        const fs::path finalDestinationPath = e.getDestination().lexically_relative(importsDir);
        
        // TODO updated settings files (e.g. via version control) should probably trigger a re-import as well
        if (!isDirectory && finalSourcePath.extension() == con::ImportSettingsExtension) {
            continue;
        }
        
        const hash32_t hashedName = HS(finalSourcePath.generic_string());
        
        const char* t = "InvalidOP";
        switch (e.getType()) {
        case FileSystemEventFlags::Created:
            t = "Created";
            assetOperations[finalSourcePath] = {fs::path(), hashedName, AssetOperationType::Created, lastFileSystemUpdate, isDirectory};
            break;
        case FileSystemEventFlags::Deleted:
            t = "Deleted";
            assetOperations[finalSourcePath] = {fs::path(), hashedName, AssetOperationType::Deleted, lastFileSystemUpdate, isDirectory};
            break;
        case FileSystemEventFlags::Modified:
            t = "Modified";
            assetOperations[finalSourcePath] = {fs::path(), hashedName, AssetOperationType::Updated, lastFileSystemUpdate, isDirectory};
            break;
        case FileSystemEventFlags::Moved:
            t = "Moved";
            
            if (isDirectory) {
                assetOperations[finalSourcePath] = {finalDestinationPath, hashedName, AssetOperationType::Moved, lastFileSystemUpdate, isDirectory};
            } else {
                assetOperations[finalSourcePath] = {fs::path(), hashedName, AssetOperationType::Deleted, lastFileSystemUpdate, isDirectory};
                assetOperations[finalDestinationPath] = {fs::path(), HS(finalDestinationPath.generic_string()), AssetOperationType::Created, lastFileSystemUpdate, isDirectory};
            }
            
            break;
        default:
            throw std::runtime_error("Invalid event type");
        }
        //TODO test current (just to make sure I didn't screw it up) and stop turning moves into creations and deletions
        LOG_V(t << " a " << (isDirectory ? "directory" : " file") << "; SRC " << finalSourcePath << "; DST " << finalDestinationPath);
    }
}

std::function<void()> AssetUpdateManager::executeAssetOperation(fs::path path, AssetOperation op) const {
    AssetManager* assetManager = engine->getAssetManager();
    if (op.isDirectory) {
        // TODO implement deletions, moves (deletion folowed by creation, maybe some lookup method for old assets)
        // and folder operations
        switch (op.type) {
            case AssetOperationType::Created:
            case AssetOperationType::Updated:
                // We don't track folders as assets
                return []{};
            case AssetOperationType::Deleted: {
                const fs::path sourcePath = con::ImportPath / path;
                return [assetManager, sourcePath]{
                    assetManager->requestAssetDeletion(sourcePath, true);
                };
            }
            case AssetOperationType::Moved: {
                const fs::path sourcePath = con::ImportPath / path;
                const fs::path destinationPath = con::ImportPath / op.destination;
                return [assetManager, sourcePath, destinationPath] {
                    assetManager->requestAssetMove(sourcePath, destinationPath, true);
                };
            }
        }
        
        LOG_W("Directory import operations are not yet implemented")
        
    } else {
        switch (op.type) {
        case AssetOperationType::Created:
        case AssetOperationType::Updated: {
            // No need to prepend con::ImportPath here. The hash does not contain it and computeNameHash would only strip it.
            const hash32_t nameHash = AssetManager::ComputeNameHash(path);
            
            const fs::path sourcePath = con::ImportPath / path;
            auto collisionCheckResult = assetManager->checkForHashCollision(nameHash, sourcePath);
            if (collisionCheckResult) {
                LOG_W("Failed to import " << path << ". Detected a hash collision with " << *collisionCheckResult);
                return nullptr;
            }
            
            const PlatformIdentifier platform = con::GetCurrentPlatform();
            std::unique_ptr<ConverterState> converterState = converterManager->initializeConverter(sourcePath, platform);
            
            if (converterState != nullptr) {
                if (converterManager->convert(*converterState)) {
                    const fs::path finalPath = converterManager->makeFinalPathForAsset(sourcePath, converterState->getType(), platform);
                    const AssetType assetType = converterState->getType();
                    
                    return [assetManager, assetType, finalPath] {
                        assetManager->requestAssetRefresh(assetType, finalPath);
                    };
                }
            }
            
            break;
        }
        case AssetOperationType::Deleted: {
            const fs::path sourcePath = con::ImportPath / path;
            const fs::path settingsPath = fs::path(sourcePath).concat(con::ImportSettingsExtension);
            
            const FileSystem* fs = engine->getFileSystem();
            if (fs->exists(settingsPath)) {
                fs->remove(settingsPath);
            } else {
                LOG_V("Failed to find the import settings file: " << settingsPath << ". Assuming it was deleted by the user.");
            }
            
            const PlatformIdentifier platform = con::GetCurrentPlatform();
            const fs::path finalPath = converterManager->makeFinalPathForAsset(sourcePath, AssetManager::GetAssetTypeFromExtension(sourcePath), platform);
            
            return [assetManager, finalPath]{
                assetManager->requestAssetDeletion(finalPath, false);
            };
        }
        case AssetOperationType::Moved:
            // TODO This isn't a good solution, moves can be done a lot more efficiently, but it's easy
            throw std::runtime_error("Move ops should have been turned into deletions followed by creations by this point");
        }
    }
    
    throw std::runtime_error("Unknown file or directory operation");
}

bool AssetUpdateManager::update() {
    IYFT_PROFILE(EditorFileOpWindow, iyft::ProfilerTag::Editor)
    
    const bool valid = assetProcessingFuture.valid();
    if (valid && assetProcessingFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        std::function<void()> notificationFunction = assetProcessingFuture.get();
        if (notificationFunction == nullptr) {
            LOG_W("Failed to process changes. File: " << currentlyProcessedAsset.first.generic_string());
        } else {
            notificationFunction();
        }
    }
    
    std::lock_guard<std::mutex> lock(assetOperationMutex);
    const std::size_t operationCount = assetOperations.size();
    const bool pendingOperations = !assetOperations.empty();
    const auto now = std::chrono::steady_clock::now();
    if (!valid && pendingOperations) {
        for (auto it = assetOperations.begin(); it != assetOperations.end();) {
            const std::chrono::duration<float> duration = now - it->second.timePoint;
            
            if (duration.count() >= MIN_STABLE_FILE_SIZE_DURATION_SECONDS) {
                iyft::ThreadPool* tp = engine->getLongTermWorkerPool();
                
                // TODO technically, most asset import stuff is thread safe. I should look into importing multiple
                // assets at the same time.
                currentlyProcessedAsset = *it;
                //assetProcessingFuture = std::async(std::launch::async, &AssetUpdateManager::executeAssetOperation, this, it->first, it->second);
                assetProcessingFuture = tp->addTaskWithResult(&AssetUpdateManager::executeAssetOperation, this, it->first, it->second);
                
                assetOperations.erase(it);
                break;
            }
            
            ++it;
        }
    }
    
    const bool popupShouldBeOpen = pendingOperations || valid;
    
    const char* modalName = "Asset operations in progress";
    if (popupShouldBeOpen && !ImGui::IsPopupOpen(modalName)) {
        ImGui::OpenPopup(modalName);
    }
    
    bool result = false;
    
    if (ImGui::BeginPopupModal(modalName, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Processing %s, %lu more item(s) remain(s)", currentlyProcessedAsset.first.generic_string().c_str(), operationCount);
        if (!popupShouldBeOpen) {
            result = true;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    return result;
//     GraphicsAPI* api = engine->getGraphicsAPI();
//     AssetManager* manager = engine->getAssetManager();
//     
//     float width = api->getRenderSurfaceWidth() * 0.3f;
//     float height = api->getRenderSurfaceHeight() * 0.2f;
//     float posY = api->getRenderSurfaceHeight() - height;
//     
// // //     static int test = 0;
// // //     if (test == 5) {
// //         ImGui::OpenPopup("Blocker");
// // //         test++;
// // //     }
// //     if (ImGui::BeginPopupModal("Blocker")) {
// //         if (ImGui::Button("HRY")) {
// //             ImGui::CloseCurrentPopup();
// //         }
// //         ImGui::EndPopup();
// //     }
//     
//     ImGui::SetNextWindowPos(ImVec2(api->getRenderSurfaceWidth() * 0.7f, posY), ImGuiCond_Always);
//     ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
//     ImGui::Begin("Pending File Operations", nullptr);
//     
//     {
//         std::lock_guard<std::mutex> lock(assetOperationMutex);
//         int count = assetOperations.size();
//         ImGui::Text("Pending operation count: %i", count);
//         
//         ImGui::BeginChild("Pending Operation List");
//         if (count > 0) {
//             ImGui::Columns(2);
//             ImGui::Separator();
//             for (auto it = assetOperations.begin(); it != assetOperations.end(); ) {
//                 const auto& op = *it;
//                 
//                 // Try to fetch the metadata for the asset
//                 auto metadataCopy = manager->getMetadataCopy(op.second.nameHash);
//                 const bool metadataExists = metadataCopy ? true : false;
//                 
//                 // If we didn't get metadata and the operation was a deletion, the user removed a file that hasn't been imported in the
//                 // first place. Therefore, we can safely ignore the operation.
//                 if (!metadataExists && op.second.type == AssetOperationType::Deleted) {
//                     it = assetOperations.erase(it);
//                     continue;
//                 }
//                 
//                 ImGui::Text("Path (hash): %s; (%u)", op.first.generic_string().c_str(), op.second.nameHash.value());
//                 
//                 const std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(now - op.second.timePoint);
//                 switch (op.second.type) {
//                     case AssetOperationType::Created:
//                         ImGui::Text("Type: created");
//                         ImGui::Text("Time: %lis ago", duration.count());
//                         break;
//                     case AssetOperationType::Updated:
//                         ImGui::Text("Type: updated");
//                         ImGui::Text("Time: %lis ago", duration.count());
//                         break;
//                     case AssetOperationType::Deleted:
//                         ImGui::Text("Type: deleted");
//                         ImGui::Text("Time: %lis ago", duration.count());
//                         break;
//                 }
//                 ImGui::NextColumn();
//                 
//                 if (ImGui::Button("Execute")) {
//                     LOG_D("Button: " << op.first);
//                     // TODO import settings, separate thread, applies to creation and update
//                     // TODO notify assetManager upon completion
//                     //auto converter = converterManager->initializeConverter(op.first, con::GetCurrentPlatform());
//                 }
//                 ImGui::NextColumn();
//                 ImGui::Separator();
//                 
//                 ++it;
//             }
//             ImGui::Columns();
//         }
//         ImGui::EndChild();
//     }
//     
//     ImGui::End();
}

void AssetUpdateManager::dispose() {
    watcherThreadRunning = false;
    fileSystemWatcherThread.join();
    
    isInit = false;
}
}
