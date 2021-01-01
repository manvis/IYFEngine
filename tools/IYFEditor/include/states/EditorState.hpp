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

#ifndef IYF_EDITORSTATE_HPP
#define IYF_EDITORSTATE_HPP

#include "core/GameState.hpp"
#include "core/World.hpp"
#include "graphics/Camera.hpp"

#include "tools/MaterialFamilyEditor.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "AssetList.hpp"
#include "utilities/DragDropAssetPayload.hpp"
#include "core/Constants.hpp"
#include "core/interfaces/InputListener.hpp"

#include <future>
#include <vector>
#include <list>
#include <set>

#include "utilities/hashing/Hashing.hpp"

namespace iyf {
class Engine;
class Renderer;
class FileSystem;
class Project;
}

namespace iyft {
class ProfilerResults;
}

namespace iyf::editor {
class EditorUI;
class MaterialEditor;
class AssetUpdateManager;
class AssetCreatorWindow;
class MaterialInstanceEditor;

class ImGuiLog {
public:
    ImGuiLog(Engine* engine) : engine(engine), lastLogLength(0) {}
    void show(const std::string& logStr);
    
protected:
    Engine* engine;
    size_t lastLogLength;
};

struct AssetListItem {
    StringHash hash;
    bool isDirectory;
    bool imported;
    Path path;
    Metadata metadata;
    
    inline bool operator<(const AssetListItem& other) const {
        if (isDirectory != other.isDirectory) {
            return isDirectory > other.isDirectory;
        } else {
            return path < other.path;
        }
    }
};

struct NextAssetToDelete {
    NextAssetToDelete() : isDirectory(false), needToOpenModal(false) {}
    NextAssetToDelete(Path path, bool isDirectory) : path(std::move(path)), isDirectory(isDirectory), needToOpenModal(true) {}
    
    Path path;
    bool isDirectory;
    bool needToOpenModal;
};

class EditorState : public GameState, public InputListener {
public:    
    EditorState(Engine* engine);
    ~EditorState();

    virtual void dispose() override;

    virtual void step() override;
    virtual void frame(float delta) override;

    virtual void pause() override;
    virtual void resume() override;
    
    virtual void onMouseMoved(int cursorXPos, int cursorYPos, int deltaX, int deltaY, bool windowHasFocus) final override;
    virtual void onMouseWheelMoved(int deltaX, int deltaY) final override;
    virtual void onMouseButtonDown(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) final override;
    virtual void onMouseButtonUp(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) final override;
    virtual void onKeyPressed(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) final override;
    virtual void onKeyReleased(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) final override;
    virtual void onTextInput(const char* text) final override;

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
    
    iyf::StringHash pickOrPlaceModeId;
    std::function<void()> drawNothing = [](){};
    std::function<void()> pickPlaceModeDrawFunction;
    void handlePickOrPlaceMode(const char* modeName, bool buttonPressed, bool& buttonReleased, std::function<void()> handleLogic, std::function<void()> handleDraw);
    void clearActivePickOrPlaceMode();
    std::future<std::uint32_t> hoveredItemIDFuture;
    
//    std::stack<> assetStack;
// WORLD AND ENTITIES-----------------------------------------------------------
    World* world;
    int worldType;
    std::string selectedEntityName;
    
    char entityName[con::MaxEntityNameLength];
    bool badName;
    /// Some systems need to update their components if transformation changes
    bool transformationChanged;
    
    void changeSelection(std::uint32_t entityID);
    void changeSelection(EntityHierarchy::value_type& entity);
    void deselectCurrentItem();
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

// PROFILING -------------------------------------------------------------------
    std::unique_ptr<iyft::ProfilerResults> profilerResults;
    float profilerZoom;
    bool profilerOpen;
    
    void showProfilerWindow();
// MATERIAL FAMILY AND SHADER EDITING -------------------------------------------------
    std::unique_ptr<MaterialFamilyEditor> materialFamilyEditor;
    bool materialFamilyEditorOpen;
    
    std::unique_ptr<MaterialEditor> materialTemplateEditor;
    bool materialTemplateEditorOpen;
    
    
    std::unique_ptr<MaterialInstanceEditor> materialInstanceEditor;
    bool materialInstanceEditorOpen;
// FILE MANAGEMENT -------------------------------------------------------------
    void showAssetWindow();
    void updateProjectFiles();
    
    std::set<AssetListItem> assetList;
    bool assetBrowserPathChanged;
    bool assetDirUpdated;
    int currentlyPickedAssetType;
    std::vector<std::string> assetTypeNames;
    Path currentlyOpenDir;
    
    void showUnableToInstanceTooltip(const std::string& tooltip);
    std::deque<AssetData> assetClipboard;
    const std::size_t maxClipboardElements;
    
    std::mutex fileSystemCallbackMutex;
    std::unique_ptr<AssetUpdateManager> assetUpdateManager;
    std::unique_ptr<AssetCreatorWindow> assetCreatorWindow;
    
    NextAssetToDelete nextAssetToDelete;
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


#endif // IYF_EDITORSTATE_HPP

