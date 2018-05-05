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

#ifndef EDITORSTATE_HPP
#define EDITORSTATE_HPP

#include "core/GameState.hpp"
#include "core/World.hpp"
#include "graphics/Camera.hpp"

#include "tools/ShadingPipelineEditor.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "AssetList.hpp"
#include "core/Constants.hpp"

#include <future>
#include <vector>
#include <list>
#include <set>

#include "core/filesystem/FileSystemWatcher.hpp"
#include "utilities/hashing/Hashing.hpp"

#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {
    class Engine;
    class Renderer;
    class FileSystem;
    class Project;
}

namespace iyf::editor {
class EditorUI;
    
class ImGuiLog {
public:
    ImGuiLog(Engine* engine) : engine(engine), lastLogLength(0) {}
    void show(const std::string& logStr);
    
protected:
    Engine* engine;
    size_t lastLogLength;
};

struct DragDropAssetPayload {
    hash32_t nameHash;
    AssetType type;
};

struct AssetListItem {
    hash32_t hash;
    bool isDirectory;
    bool imported;
    fs::path path;
    Metadata metadata;
    
    inline bool operator<(const AssetListItem& other) const {
        if (isDirectory != other.isDirectory) {
            return isDirectory > other.isDirectory;
        } else {
            return path < other.path;
        }
    }
};

/// The type of the AssetOperation
///
/// We don't do moves. It's simpler to mark an asset as deleted, followed by a creation of a new asset
enum class AssetOperationType : std::uint32_t {
    Created, Updated, Deleted, Moved
};

struct AssetOperation {
    hash32_t nameHash;
    AssetOperationType type;
    std::chrono::steady_clock::time_point timePoint;
    bool isDirectory;
};

class EditorState : public GameState {
public:    
    EditorState(Engine* engine);
    ~EditorState();

    virtual void dispose() override;

    virtual void step() override;
    virtual void frame(float delta) override;

    virtual void pause() override;
    virtual void resume() override;

protected:
    Project* currentProject;
    std::unique_ptr<EditorUI> editorUI;
//    
//    void createHexWorldEditor();
//    
//    void createHexTypeListItem(int id, const char* name, int& chosenHexType, const char* info, bool canChangeColor = true);
//    void createEventHandlerListItem(const char* name, char* buff, std::size_t buffSize);
//
    virtual void initialize() override;
// TODO move this to an asset manager when that is online.FILE CHANGE MONITORING
//  GENERAL---------------------------------------------------------------------
// WORLD CREATION---------------------------------------------------------------
    bool newLevelDialogRequested;
    char levelName[con::MaxLevelNameLength];
    char levelDescription[con::MaxLevelDescriptionLength];
    
    std::string errorStr;
    
    virtual void createNewWorldTypePicker();
    virtual void createNewWorldParameterEditor();
    virtual void validateNewWorldParameters(bool& errorsFound);
    virtual void createNewWorld();

    virtual void createDefaultNewWorldParameterEditor();
    virtual void validateDefaultNewWorldParameters(bool& errorsFound);
// MENU-------------------------------------------------------------------------
//    virtual void 
// PICK AND PLACE FUNCTIONALITY-------------------------------------------------
    bool isPickPlaceMode;
    
    iyf::hash32_t pickOrPlaceModeId;
    std::function<void()> drawNothing = [](){};
    std::function<void()> pickPlaceModeDrawFunction;
    void handlePickOrPlaceMode(const char* modeName, bool buttonPressed, bool& buttonReleased, std::function<void()> handleLogic, std::function<void()> handleDraw);
    void clearActivePickOrPlaceMode();
//    std::stack<> assetStack;
// WORLD AND ENTITIES-----------------------------------------------------------
    World* world;
    int worldType;
    std::string selectedEntityName;
    
    char entityName[con::MaxEntityNameLength];
    bool badName;
    /// Some systems need to update their components if transformation changes
    bool transformationChanged;
    
    virtual void createWorldSpecificEditor() {}
    
    void showMaterialEditorWindow();
    void showWorldEditorWindow();
    void showEntityEditorWindow();
    
    // Various component editors --------------------------------------------------------------
    virtual void showComponentEditors(Entity& entity, const EntityState& entityState);
    virtual void beginComponentEditor(Entity& entity, const ComponentType& type);
    virtual void endComponentEditor();
    
    void showTransformationEditor(Entity& entity, TransformationComponent& transformation);
    
    virtual void showGraphicsComponentEditors(Entity& entity);
    virtual void showMeshComponentEditor(Entity& entity);
    virtual void showCameraComponentEditor(Entity& entity);
    std::vector<std::string> renderModeNames;
    
    virtual void showPhysicsComponentEditors(Entity& entity);
    virtual void showColliderComponentEditor(Entity& entity);
    
    /// We can't erase an element immediately because that would crash the Editor. This vector
    /// contains a list of Component objects that need to be removed before the component editors
    /// are shown again
    std::vector<std::pair<EntityKey, ComponentType>> componentsToRemove;
    // ----------------------------------------------------------------------------------------
    
    std::vector<std::pair<std::uint32_t, std::string>> materialComponents;
    void draw2DColorDataSlot(std::pair<std::uint32_t, std::string>& contents, const char* label, con::ColorChannelCountFlagBits channels, bool allowColor = true);
// CAMERA-----------------------------------------------------------------------
    enum class CameraMode {
        LockedToPlayer, Free, Stationary
    };
    
    CameraMode cameraMode;
// SCRIPTING -------------------------------------------------------------------
//    using ArgDesc = std::pair<std::string, std::string>;
    
// PIPELINE EDITING ------------------------------------------------------------
    std::unique_ptr<ShadingPipelineEditor> pipelineEditor;
    bool pipelineEditorOpen;
// FILE MANAGEMENT -------------------------------------------------------------
    void showAssetWindow();
    void showFileOperationWindow();
    
    void fileSystemWatcherCallback(FileSystemWatcher::EventList eventList);
    void fileSystemWatcherNewFileCallback(FileSystemEvent event);
    
    void updateProjectFiles(float delta);

    fs::path importsDir;
    std::set<AssetListItem> assetList;
    bool assetBrowserPathChanged;
    int currentlyPickedAssetType;
    std::vector<std::string> assetTypeNames;
    fs::path currentlyOpenDir;
    
    std::mutex assetOperationMutex;
    std::map<fs::path, AssetOperation> assetOperations;
    
    void showUnableToInstanceTooltip(const std::string& tooltip);
    std::deque<AssetData> assetClipboard;
    const std::size_t maxClipboardElements;
    
    struct ChangeTracker {
        ChangeTracker(const std::string& path) : path(path), lastSize(0), stableSizeDuration(0) {}
        
        std::string path;
        std::size_t lastSize;
        std::chrono::duration<float> stableSizeDuration;
    };
    /// When files are created or modified, their contents are not immediately available
    /// for safe reading (e.g., another program may be writing them or they are still being copied).
    /// This vector is used to track the sizes of newly added files for some time. 
    /// Once they stop changing, safe-ish reading can commence.
    std::list<ChangeTracker> newFiles;
    
    /// List of original files that have been removed from the imports directory since the last update() call. These names will be hashed and 
    /// used to first find, then remove both the assets that were imported from the original files and their corresponding database entries.
    std::vector<std::string> filesToRemove;
    
    /// List of files that have been deemed "safe to process", based on potentially horrible guesswork. These file names will be
    /// sent to the converterManager and, if the import succeeds, added to the project database.
    /// This list is reset and re-processed on every update()
    std::vector<std::string> filesToProcess;
    
    std::chrono::steady_clock::time_point lastFileSystemUpdate;
    
    std::atomic<bool> fileSystemThreadRunning;
    std::mutex fileSystemCallbackMutex;
    std::unique_ptr<FileSystemWatcher> fileSystemWatcher;
    std::thread fileSystemWatcherThread;
// LOGGING AND DEBUG ------------------------------------------------------------
    ImGuiLog logWindow;
    enum class DebugDataUnit : int {
        Bytes,
        Kibibytes,
        Mebibytes
    };
    
    void showDebugWindow();
    void printBufferInfo(const char* name, const std::vector<MeshTypeManager::BufferWithRanges>& buffers);
    DebugDataUnit debugDataUnit;
};
}


#endif /* EDITORSTATE_HPP */

