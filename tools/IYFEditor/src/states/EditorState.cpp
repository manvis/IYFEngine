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

#include "states/EditorState.hpp"

#include <thread>
#include <chrono>
#include <cstdio>

#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "core/InputState.hpp"
#include "core/Logger.hpp"
#include "core/Project.hpp"
#include "graphics/GraphicsSystem.hpp"
#include "graphics/MeshComponent.hpp"
#include "assets/AssetManager.hpp"
#include "assets/loaders/MeshLoader.hpp"
#include "localization/TextLocalization.hpp"

#include "imgui.h"
#include "tinyfiledialogs.h"
#include "EditorUI.hpp"

#include "ImGuiImplementation.hpp"
#include "core/DefaultWorld.hpp"
#include "imgui_internal.h"
#include "utilities/IntegerPacking.hpp"

#include "glm/gtx/string_cast.hpp"

// TODO move the test to tests
#include "graphics/shading/VulkanGLSLShaderGenerator.hpp"

/// How long the size of a file needs to remain stable to be considered safe for writing.
/// Used to determine if the file is safe to import
const float MIN_STABLE_FILE_SIZE_DURATION_SECONDS = 0.25f;

// TODO something smarter, like measuring the actual size
#define ADD_REMOVE_COMPONENT_BUTTON_WIDTH 60

namespace iyf::editor {

static void WidthAtLeastTwiceHeight(ImGuiSizeCallbackData* data) {
    if (data->DesiredSize.x < data->DesiredSize.y * 2) {
        data->DesiredSize.x = data->DesiredSize.y * 2;
    }
}

// TODO implement TOOLTIPS, like in imgui_demo.cpp:
//    ImGui::BeginTooltip();
//    ImGui::PushTextWrapPos(450.0f);
//    ImGui::TextUnformatted(desc);
//    ImGui::PopTextWrapPos();
//    ImGui::EndTooltip();

EditorState::EditorState(iyf::Engine* engine) : GameState(engine),
        newLevelDialogRequested(false), levelName("test_level_name"), levelDescription("test_level_desc"), isPickPlaceMode(false), 
        pickOrPlaceModeId(0), worldType(0), entityName("a"), cameraMode(CameraMode::Stationary), pipelineEditorOpen(true), 
        assetWindowOpen(true), firstUpdatePending(false),  currentlyPickedAssetType(0), maxClipboardElements(5), logWindow(engine) {
    
    filesToProcess.reserve(40);
    filesToRemove.reserve(40);
    
    pickPlaceModeDrawFunction = drawNothing;
    debugDataUnit = DebugDataUnit::Mebibytes;
    editorUI->doSomething();
}

EditorState::~EditorState() { }

void EditorState::initialize() {
    currentProject = engine->getProject();
    
    FileSystemWatcher::CreateInfo fsci;
    fsci.handler = std::bind(&EditorState::fileSystemWatcherCallback, this, std::placeholders::_1);
    fsci.writeChangesToLog = true;
    
    FileSystemWatcher::MonitoredDirectory md;
    md.path = currentProject->getRootPath() / con::ImportPath;
    md.recursive = true;
    md.monitoredEvents = FileSystemEventFlags::All;
    
    fsci.monitoredDirectories.push_back(std::move(md));
    
    fileSystemThreadRunning = true;
    
    lastFileSystemUpdate = std::chrono::steady_clock::now();
    fileSystemWatcher = FileSystemWatcher::MakePlatformFilesystemWatcher(fsci);
    fileSystemWatcherThread = std::thread([this](){
        while (fileSystemThreadRunning) {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> delta = now - lastFileSystemUpdate;
            lastFileSystemUpdate = now;
            
            fileSystemWatcher->poll();
            updateProjectFiles(delta.count());
            
            // TODO maybe sleep here?
        }
    });
    
    // TODO because of frequent file system acccess, quite a few of these should happen in a separate thread 
    // TODO load the default world
    EntitySystemManagerCreateInfo ci(engine);
    ci.setEditorMode(true);
    world = new DefaultWorld("defaultWorld", engine->getConfiguration(), ci);
    world->initialize();
    world->addCamera(true);
    world->addCamera(false);
    
    pipelineEditor = std::make_unique<ShadingPipelineEditor>(engine, engine->getRenderer(), this);
    
    FileSystem* fileSystem = engine->getFileSystem();
    
    firstUpdatePending = true;// TODO implement me pipelineNames = fileSystem->getDirectoryContents(iyf::con::PipelinePath);
    
//     // TODO move this to tests
//     {
//         MaterialPipelineDefinition def = DefaultMaterialPipelineDefinitions[static_cast<std::size_t>(DefaultMaterialPipeline::Toon)];
//         {
//             File output("toon.iyfpl", File::OpenMode::Write);
//             def.serialize(output);
//         }
//         
//         MaterialPipelineDefinition empty;
//         {
//             File input("toon.iyfpl", File::OpenMode::Read);
//             empty.deserialize(input);
//         }
//         
//         LOG_D(def.name << " " << empty.name);
//         LOG_D(def.languages.size() << " " << empty.languages.size());
//         LOG_D("Code matches? " << (def.lightProcessingCode[0] == empty.lightProcessingCode[0]) << "\n" << empty.lightProcessingCode[0]);
//         
//         empty.name.append("1");
//         
//         iyf::VulkanGLSLShaderGenerator vkGen(engine);
//         vkGen.generateFragmentShader(ComponentsReadFromTexture(0), def, true, true);
//         vkGen.generateFragmentShader(ComponentsReadFromTexture(0), empty, true, true);
//     }
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(MaterialRenderMode::COUNT); ++i) {
        renderModeNames.push_back(LOC_SYS(MaterialRenderModeNames[i]));
    }
//    std::uint32_t i1 = util::BytesToInt32(1, 2, 3, 4);
//    std::uint32_t i2 = util::BytesToInt32({1, 2, 3, 4});
//    std::array<std::uint8_t, 4> i3;
//    std::array<std::uint8_t, 4> i4;
//    util::Int32ToBytes(i1, i3);
//    util::Int32ToBytes(i2, i4);
//    LOG_D(i1 << "\n\t" << i2 << "\n\t" 
//          << static_cast<std::uint32_t>(i3[0]) << " " << static_cast<std::uint32_t>(i3[1]) << " " << static_cast<std::uint32_t>(i3[2]) << " " << static_cast<std::uint32_t>(i3[3]) << "\n\t"
//          << static_cast<std::uint32_t>(i4[0]) << " " << static_cast<std::uint32_t>(i4[1]) << " " << static_cast<std::uint32_t>(i4[2]) << " " << static_cast<std::uint32_t>(i4[3]))
}

void EditorState::dispose() {
    if (world != nullptr) {
        world->dispose();
        delete world;
    }
    
    fileSystemThreadRunning = false;
    fileSystemWatcherThread.join();
    
    currentProject = nullptr;
}

void EditorState::step() {
    //LOG_D("COMPONENT NAME TEST" << con::ComponentNames[0].size() << " " << con::ComponentNames[1].size());
    iyf::InputState* is = engine->getInputState();
    
    // TODO proper key mods
    if((is->isKeyPressed(SDL_SCANCODE_LCTRL) && is->isKeyPressed(SDL_SCANCODE_Q)) || is->isKeyPressed(SDL_SCANCODE_Q)) {
        engine->quit();
    }
    
    // TODO replace with onPress/onRelease listener
    static bool pressedDebugToggle = false;
    // Currently, F1 toggles between free camera and "flying" camera mode, like in Blender
    if (!pressedDebugToggle && is->isKeyPressed(SDL_SCANCODE_F1)) {
        pressedDebugToggle = true;
        
        cameraMode = (cameraMode == CameraMode::Free) ? CameraMode::Stationary : CameraMode::Free;
        clearActivePickOrPlaceMode();//editorMode = EditorMode::None;
    }
    
    if (pressedDebugToggle && !is->isKeyPressed(SDL_SCANCODE_F1)) {
        pressedDebugToggle = false;
    }
}

void EditorState::frame(float delta) {
    iyf::InputState* is = engine->getInputState();
    iyf::GraphicsAPI* api = engine->getGraphicsAPI();
    
//    throw std::runtime_error("wrt");
    
    is->setMouseRelativeMode(cameraMode == CameraMode::Free);
    world->setInputProcessingPaused(cameraMode != CameraMode::Free);
    
    if (world != nullptr) {
        world->update(delta);
        
        if (!ImGui::IsAnyItemHovered() && is->isMouseClicked(MouseButton::Left)) {
            int mouseX = is->getMouseX();
            int mouseY = is->getMouseY();
            
            if (mouseX >= 0 && mouseX <= static_cast<int>(api->getScreenWidth()) && mouseY >= 0 && mouseY <= static_cast<int>(api->getScreenHeight())) {
                world->rayPick(mouseX, mouseY);
            }
        }
    }

    ImGuiImplementation* impl = engine->getImGuiImplementation();
    impl->requestRenderThisFrame();
    
    //std::cerr << sizeof(size_t) << std::endl;
    //ImGui::ShowTestWindow();
    
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New World", "CTRL+N")) {
                newLevelDialogRequested = true;
            }
            
            if (ImGui::MenuItem("Open World", "CTRL+O")) {
                //engine->quit();
            }
            
            // TODO NEW WORLD!!!
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Save", "CTRL+S")) {
                //engine->quit();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Close Project", "CTRL+W")) {
                engine->popState();
            }
            
            if (ImGui::MenuItem("Quit", "CTRL+Q")) {
                engine->quit();
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            /*if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}*/
            ImGui::EndMenu();
        }
        
        // I do not think we need an explicit world menu any more worlds should be contained in the project
        // and "export project to runtime" (TODO) or sth should be a replacement for this menu
//        if (ImGui::BeginMenu("World")) {
//            /*if (ImGui::MenuItem("World settings", "CTRL+SHIFT+P")) {
//                
//            }
//            
//            ImGui::Separator();*/
//            
//            // TODO move serialization where it's actually needed
//            if (ImGui::MenuItem("Validate", "CTRL+SHIFT+V")) {
////                iyf::FileReader fr("test.hexm");
////                grid->deserialize(fr);
//            }
//            
//            if (ImGui::MenuItem("Export and package", "CTRL+SHIFT+E")) {
////                iyf::FileWriter fw("test.hexm");
////                grid->serialize(fw);
//            }
//            
//            ImGui::EndMenu();
//        }
        
        if (ImGui::BeginMenu("Window")) {
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Preferences")) {
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
    
    if ((is->isKeyPressed(SDL_SCANCODE_LCTRL) && is->isKeyPressed(SDL_SCANCODE_N)) || newLevelDialogRequested) {
        ImGui::OpenPopup("Create a New World");
        cameraMode = CameraMode::Stationary;
        newLevelDialogRequested = false;
    }
    
    if (ImGui::BeginPopupModal("Create a New World", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Level ID", levelName, con::MaxLevelNameLength);//These should only handles that will be used to retrieve a localized string
        ImGui::InputText("Info ID", levelDescription, con::MaxLevelDescriptionLength);
        
        ImGui::Separator();
        ImGui::Text("World type");
        
        createNewWorldTypePicker();
        
        if (ImGui::CollapsingHeader("World specific settings")) {
            createNewWorldParameterEditor();
        }

        if (ImGui::Button("Create", ImVec2(120,0))) {
            bool errorsFound = false;
            errorStr = "ERRORS:\n";
            
            if (strlen(levelName) == 0) {
                errorsFound = true;
                errorStr += "Level ID not set\n";
            }
            
            if (strlen(levelDescription) == 0) {
                errorsFound = true;
                errorStr += "Info ID not set\n";
            }
            
            validateNewWorldParameters(errorsFound);
            
            if (!errorsFound) {
                createNewWorld();
                
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::OpenPopup("Errors were found");
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120,0))) {
            ImGui::CloseCurrentPopup();
        }
        
        if (ImGui::BeginPopupModal("Errors were found", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", errorStr.c_str());
            
            if (ImGui::Button("Close", ImVec2(120,0))) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::SetNextWindowPos(ImVec2(10, 30));
//     ImGui::SetNextWindowBgAlpha(0.7f);
    if (ImGui::Begin("Info Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        
        if (world != nullptr) {
            GraphicsSystem* graphicsSystem = dynamic_cast<GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
            if (graphicsSystem != nullptr) {
                if (graphicsSystem->isViewingFromEditorCamera()) {
                    ImGui::Text("CAM: Editor Camera");
                } else {
                    ImGui::Text("CAM: %s", world->getEntityByID(graphicsSystem->getActiveCameraID()).getName().c_str());
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Reset")) {
                        graphicsSystem->setViewingFromEditorCamera(true);
                    }
                }
            }
        }
        
        ImGui::Separator();
        ImGui::Text("Asset Clipboard");
        
        // TODO probably would be better if this was drawn next to the mouse as a floating menu
        // TODO add a way to choose the active one
        bool isFirst = true;
        for (AssetData d : assetClipboard) {
            ImGui::AssetKey(d.fileName.c_str(), static_cast<AssetType>(d.type), d.keyData, isFirst);
            ImGui::SameLine();
            isFirst = false;
        }
        // Eat the SameLine we didn't use
        ImGui::NewLine();
        
        if (assetClipboard.size() > 0) {
            ImGui::Text("%s", assetClipboard[0].fileName.c_str());
        }
//        if (world != nullptr) {
//            ImGui::Text(world->getPosString().c_str());
//        }
        /*ImGui::Separator();
        ImGui::Text("Controls:\n\tQ - quit\n\tF1 - toggle between menu and free camera modes");
        ImGui::Text(world->getPosString().c_str());*/
        
        // TODO Draw empty circle when camera is free
//        if (editorMode != EditorMode::None) {
//            ImDrawList* drawList = ImGui::GetWindowDrawList();//
//            drawList->PushClipRectFullScreen();
//            drawList->AddCircle(ImVec2(engine->getConfiguration()->getIntValue("width") * 0.5f, 
//                                       engine->getConfiguration()->getIntValue("height") * 0.5f),
//                                5.0f, ImColor(ImVec4(0.0f, 0.0f, 0.0f, 0.8f)), 12, 2.0f);
//            
//            drawList->PopClipRect();
//        }
        
        
        // There's a one frame delay before this kicks in, but I think it's acceptible
        if (isPickPlaceMode) {
            pickPlaceModeDrawFunction();
        }
        ImGui::End();
    } else {
        ImGui::End();
    }
    
    logWindow.show(engine->getLogString());
    // TODO implement pipeline editor
    
    if (pipelineEditorOpen) {
        pipelineEditor->show(&pipelineEditorOpen);
    }
    
    showAssetWindow();
    showMaterialEditorWindow();
    showWorldEditorWindow();
    showEntityEditorWindow();
    showDebugWindow();
    
    ImGui::Begin("test");
    if (ImGui::Button("PerfTest")) {
        MeshLoader ml(engine);
        MeshLoader::MemoryRequirements mr = ml.getMeshMemoryRequirements("data/test/inData.iyfm");
        LOG_D(mr.vertexSize << " " << mr.indexSize << " " << mr.boneCount)
        // And what about offsets, limits, submeshes, etc?
        std::vector<char> vbo;
        vbo.resize(mr.vertexSize);
        
        std::vector<char> ibo;
        ibo.resize(mr.indexSize);
        
        std::vector<Bone> bones;
        bones.resize(mr.boneCount);
        
        MeshLoader::LoadedMeshData lmd;
        ml.loadMesh("data/test/inData.iyfm", lmd, vbo.data(), ibo.data(), &bones);
        Animation anim;
        ml.loadAnimation("data/test/inData_Twisty.iyfa", anim);
    }
    ImGui::End();
    
    createWorldSpecificEditor();
    
    ImGui::GetIO().MouseDrawCursor = (cameraMode == CameraMode::Stationary);
}

void EditorState::pause() {
    //
}

void EditorState::resume() {
    //
}

void EditorState::createNewWorldTypePicker() {
    ImGui::RadioButton("Regular world", &worldType, 0);
}

void EditorState::createNewWorldParameterEditor() {
    createDefaultNewWorldParameterEditor();
}

void EditorState::createDefaultNewWorldParameterEditor() {
    ImGui::Text("No specific settings exist for this world");
}

void EditorState::validateNewWorldParameters(bool& errorsFound) {
    validateDefaultNewWorldParameters(errorsFound);
}

void EditorState::validateDefaultNewWorldParameters(bool& errorsFound) {
    // TODO actual validation
    errorsFound = false;
}

void EditorState::createNewWorld() {
    if (world != nullptr) {
        world->dispose();
        delete world;
    }
    
    EntitySystemManagerCreateInfo ci(engine);
    ci.setEditorMode(true);
    world = new DefaultWorld("defaultWorld", engine->getConfiguration(), ci);
    world->initialize();
}

void EditorState::clearActivePickOrPlaceMode() {
    isPickPlaceMode = false;
    pickPlaceModeDrawFunction = drawNothing;
}

void EditorState::handlePickOrPlaceMode(const char* modeName, bool buttonPressed, bool& buttonReleased, std::function<void()> handleLogic, std::function<void()> handleDraw) {
    iyf::hash32_t hashedName = HS(modeName);
    
    if (buttonPressed) {
        pickOrPlaceModeId = hashedName;
        isPickPlaceMode = true;
        cameraMode = CameraMode::Free;
        buttonReleased = true; // To prevent an accidental
    } else if(isPickPlaceMode && !buttonReleased && pickOrPlaceModeId == hashedName) {
        handleLogic();
        
        pickPlaceModeDrawFunction = handleDraw;
    } else {
        buttonReleased = false;
    }
}

void EditorState::showUnableToInstanceTooltip(const std::string& tooltip) {
    ImGui::Text("N/A");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", tooltip.c_str());
        ImGui::EndTooltip();
    }
}

void EditorState::showMaterialEditorWindow() {
    // TODO needs to be GENERATED for each pipeline, but I need to implement the pipeline
    // editor before I can do that.
    // TODO allow using colors (with color pickers) and (maybe?) sliders (e.g., for roughness and metallic parameters)
    if (materialComponents.size() != 4) {
        materialComponents.resize(4);
    }
    
    ImGui::SetNextWindowSize(ImVec2(300,500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Material editor", nullptr);
    
    ImGui::AlignTextToFramePadding();
    static char materialName[80] = "";
    ImGui::InputText("Name", materialName, IM_ARRAYSIZE(materialName));
    
    float buttonWidth = 20.0f;
    ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - buttonWidth);
    ImGui::Button("+", ImVec2(buttonWidth, 0));
    
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("Create a copy of this material.");
        ImGui::EndTooltip();
    }
    
    ImGui::Columns(2);
    draw2DColorDataSlot(materialComponents[0], "Albedo", con::ColorChannelCountFlagBits::Four);
    ImGui::NextColumn();
    
    ImGui::Text("Albedo");
    ImGui::NextColumn();
    
    //---------------------------------------------
    draw2DColorDataSlot(materialComponents[1], "Metallic", con::ColorChannelCountFlagBits::One);
    ImGui::NextColumn();
    
    ImGui::Text("Metallic");
    ImGui::NextColumn();
    
    //---------------------------------------------
    
    draw2DColorDataSlot(materialComponents[2], "Roughness", con::ColorChannelCountFlagBits::One);
    ImGui::NextColumn();
    
    ImGui::Text("Roughness");
    ImGui::NextColumn();
    
    //---------------------------------------------
    
    draw2DColorDataSlot(materialComponents[3], "Roughness", con::ColorChannelCountFlagBits::Two);
    ImGui::NextColumn();
    
    ImGui::Text("Normal Map");
    ImGui::NextColumn();
    
    ImGui::Columns();
    
    ImGui::End();
}

void EditorState::showDebugWindow() {
    ImGui::SetNextWindowSize(ImVec2(300,500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Engine Debug", nullptr);
    
    int unit = static_cast<int>(debugDataUnit);
    ImGui::Combo("Data type", &unit, "B\0KiB\0MiB\0\0");
    debugDataUnit = static_cast<DebugDataUnit>(unit);
    
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Mesh Assets")) {
        AssetManager* assetManager = engine->getAssetManager();
        ImGui::Text("Registered Assets: %lu", assetManager->getRegisteredAssetCount());
        ImGui::Text("Loaded Assets: %lu", assetManager->getLoadedAssetCount());
        
        ImGui::Separator();
        
        const MeshTypeManager* meshManager = reinterpret_cast<const MeshTypeManager*>(assetManager->getTypeManager(AssetType::Mesh));
        if (meshManager->isSingleBufferMode()) {
            const auto& buffers = meshManager->getVertexDataBuffersWithRanges();
            
            printBufferInfo("Shared buffer", buffers);
        } else {
            const auto& vertexBuffers = meshManager->getVertexDataBuffersWithRanges();
            const auto& indexBuffers = meshManager->getIndexDataBuffersWithRanges();
            
            printBufferInfo("Vertex buffer", vertexBuffers);
            printBufferInfo("Index buffer", indexBuffers);
        }
        
        ImGui::TreePop();
    }
    
    ImGui::Separator();
    
    bool physicsDebug = (world == nullptr) ? false : world->isPhysicsDebugDrawn();
    ImGui::Checkbox("Draw physics debug", &physicsDebug);
    
    if (world != nullptr) {
        world->setPhysicsDebugDrawn(physicsDebug);
    }
    
    ImGui::Text("Drawing stats");
    if (world != nullptr) {
        GraphicsSystem* graphicsSystem = static_cast<GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
        const auto& visible = graphicsSystem->getVisibleComponents();
        
        ImGui::Text("Visible opaque meshes %zu", visible.opaqueMeshEntityIDs.size());
        ImGui::Text("Visible transparent meshes %zu", visible.transparentMeshEntityIDs.size());
    }
    
    ImGui::End();
}

void EditorState::printBufferInfo(const char* name, const std::vector<MeshTypeManager::BufferWithRanges>& buffers) {
    ImGui::Text("%s count: %lu", name, buffers.size());
    
    for (std::size_t i = 0; i < buffers.size(); ++i) {
        char buff[128];
        sprintf(buff, "%s %lu", name, i);
        
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(buff)) {
            double percentage = ((double)(buffers[i].freeRanges.getFreeSpace().count()) / (double)(buffers[i].freeRanges.getTotalSpace().count())) * 100.0;
            
            std::uint64_t freeSpace;
            std::uint64_t totalSpace;
            const char* unitName;
            
            switch (debugDataUnit) {
            case DebugDataUnit::Bytes:
                freeSpace = buffers[i].freeRanges.getFreeSpace().count();
                totalSpace = buffers[i].freeRanges.getTotalSpace().count();
                unitName = "B";
                
                break;
            case DebugDataUnit::Kibibytes:
                freeSpace = Kibibytes(buffers[i].freeRanges.getFreeSpace()).count();
                totalSpace = Kibibytes(buffers[i].freeRanges.getTotalSpace()).count();
                unitName = "KiB";
                
                break;
            case DebugDataUnit::Mebibytes:
                freeSpace = Mebibytes(buffers[i].freeRanges.getFreeSpace()).count();
                totalSpace = Mebibytes(buffers[i].freeRanges.getTotalSpace()).count();
                unitName = "MiB";
                
                break;
            }
            
            ImGui::Text("%.2f%% (%lu%s of %lu%s) free", percentage, freeSpace, unitName, totalSpace, unitName);
            ImGui::TreePop();
        }
    }
}

void EditorState::showWorldEditorWindow() {
//     ImGui::ShowTestWindow();
//    
    if (world == nullptr) {
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(300,500), ImGuiCond_FirstUseEver);
    ImGui::Begin("World editor", nullptr);
    
    ImGui::Text("The world tree");
    if (ImGui::TreeNodeEx("World", ImGuiTreeNodeFlags_DefaultOpen)) {
//        if (ImGui::IsItemClicked()) {
//            LOG_D("Y")
//        }
        
        auto& entities = world->getEntityHierarchy();

        // TODO multi select
        // TODO this does NOT support hierarchies
        for (auto& entity : entities) {
            if (ImGui::Selectable(entity.first.c_str(), entity.second.isSelected())) {
                auto previouslySelected = entities.find(selectedEntityName);
                
                if (previouslySelected != entities.end()) {
                    (*previouslySelected).second.setSelected(false);
                }
                
                selectedEntityName = entity.first;
                assert(entity.first.length() + 1 <= con::MaxEntityNameLength);
                std::memcpy(entityName, entity.first.c_str(), entity.first.length() + 1);
                //entityName = entity.first;
                entity.second.setSelected(true);
                // Clear badName flag, if it's set
                badName = false;
            }

            // TODO either this, or add right click menus
            // TODO add parent-child relationships once World supports them
//            if (ImGui::IsItemHovered()) {
//                ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 40);
//
//                if (ImGui::SmallButton("+")) {
//                    // TODO create a copy of the entity in the world world
//                }
//
//                if (ImGui::IsItemHovered()) {
//                    ImGui::BeginTooltip();
//                    ImGui::Text("Create a copy of this entity in the world");
//                    ImGui::EndTooltip();
//                }
//
//                ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - 20);
//
//                if (ImGui::SmallButton("-")) {
//                    // TODO delete entity from world
//                }
//
//                if (ImGui::IsItemHovered()) {
//                    ImGui::BeginTooltip();
//                    ImGui::Text("Remove this entity from the world");
//                    ImGui::EndTooltip();
//                }
//            }
        }
        
        // TODO these will probably fit better somewhere else
//        if (ImGui::TreeNode("Materials")) {
//            ImGui::TreePop();
//        }
        
        ImGui::TreePop();
    }
    
    ImGui::Separator();
    
    ImGui::Text("Instantiate new entities: ");
    
    ImGui::Columns(2);
    
    if (ImGui::AssetLock("StaticMesh", AssetType::Mesh, 0)) {
        const AssetData& asset = assetClipboard[0];
        
        world->addStaticMesh(hash32_t(asset.id));
    }
    
    ImGui::NextColumn();
    ImGui::Text("Static mesh entity");
    
    ImGui::NextColumn();
    if (ImGui::AssetLock("DynamicMesh", AssetType::Mesh, 0)) {
        const AssetData& asset = assetClipboard[0];
        
        world->addDynamicMesh(hash32_t(asset.id));
    }
    
    ImGui::NextColumn();
    ImGui::Text("Dynamic mesh entity");
    
    ImGui::NextColumn();
    
    ImGui::Separator();
    
    if (ImGui::AssetLock("SerializeMesh", AssetType::Mesh, 0)) {
        const AssetData& asset = assetClipboard[0];
        
        AssetManager* assetManager = engine->getAssetManager();
        VirtualFileSystemSerializer fw("mesh_metadata", File::OpenMode::Write);
        assetManager->serializeMetadata(hash32_t(asset.id), fw);
    }
    
    ImGui::NextColumn();
    
    ImGui::Text("Serialize mesh metadata");
    
    ImGui::NextColumn();
    
    
//    if (ImGui::AssetLock("StaticMesh", AssetType::Mesh, 0)) {
//        //
//    }
    
    ImGui::Columns();
    
    ImGui::End();
}

void EditorState::showEntityEditorWindow() {
//     bool stw = true;
//     ImGui::ShowTestWindow(&stw);
    
    // TODO imgui drag float does not do bounds checking on manually entered values. Fix it.
    // TODO this whole thing is bad. It still thinks only a single component type exists per system
    ImGui::SetNextWindowSize(ImVec2(300,500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Entity editor", nullptr);
    
    if (world == nullptr) {
        ImGui::Text("No Worlds are loaded at the moment.");
        ImGui::End();
        
        return;
    }
    
    for (const auto& c : componentsToRemove) {
        bool result = world->removeComponent(c.first, c.second);
        assert(result);
    }
    componentsToRemove.clear();

    // TODO this does not support hierarchies
    auto& entities = world->getEntityHierarchy();
    auto currentSelection = entities.find(selectedEntityName);
    
    if (currentSelection == entities.end()) {
        ImGui::Text("No Entities are selected.");
        ImGui::End();
        
        return;
    }
    
    Entity& entity = currentSelection->second.getEntity();
    const EntityState& state = entity.getState();
    
    if (ImGui::InputText("Name", entityName, con::MaxEntityNameLength)) {//, ImGuiInputTextFlags_EnterReturnsTrue
//         //LOG_D("REACTED")
//         if (entityName != currentSelection->first) {
//             auto existingName = entities.find(entityName);
//             
//             if (existingName == entities.end()) {
//                 selectedEntityName = entityName;
//                 entities.erase(currentSelection);
//                 
        // TODO PERFORM A RENAME HERE
//                 auto insertionResult = entities.insert({entityName, {entity, true}});
//                 assert(insertionResult.second);
//                 
//                 currentSelection = insertionResult.first;
//                 
//                 badName = false;
//             } else {
//                 badName = true;
//             }
//         }
    }
    
    ImGui::SameLine();
    ImGui::Text("(INFO)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("EntitySystemManager info\n\nID: %" PRIu32 "\nVERSION: %" PRIu32, entity.getKey().getID(), entity.getKey().getVersion());
    }

    if (badName) {
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "WARNING: The name \"%s\" is used by another Entity and can't be used.", entityName);
        ImGui::PushTextWrapPos();
    }

    TransformationComponent& transformation = entity.getTransformation();
    
    bool isStatic = transformation.isStatic();
    if (ImGui::Checkbox("Static", &isStatic)) {
        transformation.setStatic(isStatic);
    }
    
    ImGui::Separator();
    
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("Components");
    }
    
    if (ImGui::BeginPopup("Components")) {
        const SystemArray& systems = world->getSystems();
        
        for (std::size_t i = 0; i < systems.size(); ++i) {
            auto& system = systems[i];
            if (ImGui::BeginMenu(LOC_SYS(system->getManagedComponentTypeName()).c_str())) {
                auto availableComponents = system->getAvailableComponents(entity.getKey().getID());
                
                for (std::size_t j = 0; j < system->getSubTypeCount(); ++j) {
                    if (ImGui::MenuItem(LOC_SYS(con::ComponentNames[i][j]).c_str(), nullptr, false, !(availableComponents[j]))) {
                        world->attachComponent(entity.getKey(), ComponentType(static_cast<ComponentBaseType>(i), j));
                    }
                }
                
                ImGui::EndMenu();
            }
        }
        
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 1.0f);
    ImGui::BeginChild("ComponentList");//, ImVec2(-1, -1), true);//, ImVec2(contentHeight, contentHeight), true);
    
    //ImGui::Text("Added components");
    
    // All Entities must have transformations, so we create the editor without any checking.
    showTransformationEditor(entity, transformation);
    
    ImGui::Separator();
    
    showComponentEditors(entity, state);
//        con::AssetType t = con::AssetType::Audio;
//        ImGui::AssetKey("t1", t, 0);ImGui::SameLine();ImGui::AssetKey("t3", t, 0);
//        ImGui::AssetLock("t2", t, 0);
    // Animations are not yet shareable
//        ImGui::AssetKey("t1", con::AssetType::Animation, 0);
//        ImGui::AssetKey("t2", con::AssetType::Audio, 0);
//        ImGui::AssetKey("t3", con::AssetType::Font, 0);
//        ImGui::AssetKey("t4", con::AssetType::Mesh, 0);
//        ImGui::AssetKey("t5", con::AssetType::Other, 0);
//        ImGui::AssetKey("t6", con::AssetType::Texture, 0);
    
    ImGui::EndChild();
    ImGui::PopStyleVar();
    
    // Reset for next frame
    transformationChanged = false;
    
    ImGui::End();
}

void EditorState::showTransformationEditor(Entity& entity, TransformationComponent& transformation) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Transformation component");
    
    glm::vec3 position = transformation.getPosition();
    if (ImGui::DragFloat3("Position", &position[0], 0.01f)) {
        transformationChanged = true;
        transformation.setPosition(position);
    }
    
    glm::quat rotation = transformation.getRotation();
    glm::vec3 angles = glm::eulerAngles(rotation);
    angles = glm::degrees(angles);
    if (ImGui::DragFloat3("Rotation", &angles[0], 0.01f)) {
        transformationChanged = true;
        rotation = glm::quat(glm::radians(angles));
        transformation.setRotation(rotation);
    }
    
    if (ImGui::Button("ROT45X+", ImVec2(-1, 0))) {
        transformation.rotate(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    }
    
    ImGui::SameLine();
    ImGui::Text("(Q)");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Quaternion: (%f, %f, %f, %f)", rotation.x, rotation.y, rotation.z, rotation.w);
    }
    
    glm::vec3 scale = transformation.getScale();
    if (ImGui::DragFloat3("Scale", &scale[0])) {
        transformationChanged = true;
        transformation.setScale(scale);
    }
}

void EditorState::showComponentEditors(Entity& entity, const EntityState& entityState) {
    if (entityState.hasComponentsOfType(ComponentBaseType::Graphics)) {
        showGraphicsComponentEditors(entity);
    }
    
    if (entityState.hasComponentsOfType(ComponentBaseType::Physics)) {
        showPhysicsComponentEditors(entity);
    }
}

void EditorState::beginComponentEditor(Entity& entity, const ComponentType& type) {
    std::string name = LOC_SYS(con::ComponentNames[static_cast<std::uint32_t>(type.getBaseType())][type.getSubType()]);
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", name.c_str());
    
    float buttonWidth = ADD_REMOVE_COMPONENT_BUTTON_WIDTH;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth);
    
    std::string removeButtonName = "Remove##Remove" + name;
    if (ImGui::Button(removeButtonName.c_str(), ImVec2(buttonWidth, 0))) {
        componentsToRemove.push_back({entity.getKey(), type});
    }
}

void EditorState::endComponentEditor() {
    //
}

void EditorState::showGraphicsComponentEditors(Entity& entity) {
    ComponentType meshComponentType = ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Mesh);
    if (entity.hasComponent(meshComponentType)) {
        beginComponentEditor(entity, meshComponentType);
        showMeshComponentEditor(entity);
        endComponentEditor();
    }
    
    ComponentType cameraComponentType = ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Camera);
    if (entity.hasComponent(cameraComponentType)) {
        beginComponentEditor(entity, cameraComponentType);
        showCameraComponentEditor(entity);
        endComponentEditor();
    }
}

void EditorState::showMeshComponentEditor(Entity& entity) {
    // TODO update RenderDataKey if mesh or materials change
    // System* graphicsSystem = dynamic_cast<System*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    GraphicsSystem* graphicsSystem = dynamic_cast<GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    assert(graphicsSystem != nullptr);
    // TODO fetch directly from the Entity
    MeshComponent& renderData = graphicsSystem->getComponent<MeshComponent>(entity.getKey().getID());
    
    const auto& mesh = renderData.getMesh();
    AssetManager* assetManager = engine->getAssetManager();
    
    ImGui::Text("Mesh file: %s", assetManager->getAssetPath(mesh->getNameHash()).c_str());
    
    AssetHandle<Mesh> mh = assetManager->getMissingAsset<Mesh>(iyf::AssetType::Mesh);
    
    MaterialRenderMode mode = renderData.getRenderMode();
    int tempMode = static_cast<int>(mode);
    
    // TODO create a SINGLE std::function and reuse it
    ImGui::Combo("Render mode", &tempMode, 
                 [](void* in, int idx, const char** out_text){
                   std::string* strs = static_cast<std::string*>(in);

                   *out_text = strs[idx].c_str();
                   return true;
                }, const_cast<void*>(static_cast<const void*>(renderModeNames.data())), renderModeNames.size());
    
    if (static_cast<MaterialRenderMode>(tempMode) != renderData.getRenderMode()) {
        renderData.setRenderMode(static_cast<MaterialRenderMode>(tempMode));
    }
    
    ImGui::Text("BOUNDS\nINITIAL: %s\nFINAL: %s", renderData.getPreTransformBoundingVolume().print().c_str(), renderData.getCurrentBoundingVolume().print().c_str());
}

void EditorState::showCameraComponentEditor(Entity& entity) {
    Camera& camera = entity.getComponent<Camera>();
    
    float verticalFOV = glm::degrees(camera.getVerticalFOV());
    if (ImGui::DragFloat("FOV (vertical)", &verticalFOV, 1.0f, 1.0f, 179.0f, u8"%.3f°")) {
        camera.setVerticalFOV(glm::radians(verticalFOV));
    }
    
    float near = camera.getNearDistance();
    float far = camera.getFarDistance();
    
    bool nearChanged = ImGui::DragFloat("Near plane distance", &near, 0.01f, 0.0f, FLT_MAX);
    bool farChanged = ImGui::DragFloat("Far plane distance", &far, 0.01f, 0.0f, FLT_MAX);
    
    if (nearChanged || farChanged) {
        camera.setClippingDistance(near, far);
    }
    
    GraphicsSystem* graphicsSystem = dynamic_cast<GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    assert(graphicsSystem != nullptr);
    
    ImGui::Text("UP: %s", glm::to_string(camera.getUp()).c_str());
    ImGui::Text("DIR: %s", glm::to_string(camera.getForward()).c_str());
    ImGui::Text("RIGHT: %s", glm::to_string(camera.getRight()).c_str());
    
    if (ImGui::Button("View from this camera", ImVec2(-1, 0))) {
        graphicsSystem->setActiveCameraID(entity.getKey().getID());
        graphicsSystem->setViewingFromEditorCamera(false);
    }
    
    if (ImGui::Button("Toggle draw frustum", ImVec2(-1, 0))) {
        graphicsSystem->setDrawingFrustum(true);
        graphicsSystem->setDrawnCameraFrustumID(entity.getKey().getID());
    }
    
    if (ImGui::Button("PRINT_TEST", ImVec2(-1, 0))) {
        glm::mat4 VM = glm::lookAt(glm::vec3(0.0, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        LOG_D(glm::to_string(VM) << "--------------------------\n\t" << glm::to_string(camera.getViewMatrix()) << "+++++++++++++++++++++\n\t" << glm::to_string(glm::mat4_cast(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))));
    }
}

void EditorState::showPhysicsComponentEditors(Entity& entity) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Rigid body component");
    
    float buttonWidth = ADD_REMOVE_COMPONENT_BUTTON_WIDTH;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - buttonWidth);
    
    if (ImGui::Button("Remove##RemoveRigidBody", ImVec2(buttonWidth, 0))) {
        // TODO remove entities
        LOG_D("RigidBody")
    }
    
    if (transformationChanged) {
        // TODO react to this?
    }
}

void EditorState::showColliderComponentEditor(Entity& entity) {
    //
}

void EditorState::draw2DColorDataSlot(std::pair<std::uint32_t, std::string>& contents, const char* label, con::ColorChannelCountFlagBits channels, bool allowColor) {
    // TODO make allowColor matter
    if (contents.first != 0) {
        std::uint32_t keyData = util::BytesToInt32(static_cast<std::uint8_t>(channels), static_cast<std::uint8_t>(con::ColorDataTypeFlagBits::Texture2D), 0, 0);
        
        if (ImGui::AssetKeyWithLock(label, AssetType::Texture, keyData)) {
            contents.first = 0;
        }
        
        ImGui::SameLine();
        ImGui::Text("%s", contents.second.c_str());
    } else {
        std::uint32_t keyData = util::BytesToInt32(static_cast<std::uint8_t>(channels), static_cast<std::uint8_t>(con::ColorDataTypeFlagBits::Texture2D), 0, 0);
        
        if (ImGui::AssetLock(label, AssetType::Texture, keyData) && !assetClipboard.empty()) {
            const AssetData& asset = assetClipboard[0];
            
            std::array<std::uint8_t, 4> keyData;
            util::Int32ToBytes(asset.keyData, keyData);
            
            bool isTexture = static_cast<AssetType>(asset.type) == AssetType::Texture;
            bool channelCountMatches = static_cast<con::ColorChannelCountFlagBits>(keyData[0]) == channels;
            bool is2D = static_cast<con::ColorDataTypeFlagBits>(keyData[1]) == con::ColorDataTypeFlagBits::Texture2D;
            
            if (isTexture && is2D && channelCountMatches) {
                contents.first = asset.id;
                contents.second = asset.fileName;

                assetClipboard.pop_front();
            }
        }
    }
}

void EditorState::showAssetWindow() {
    // TODO implement search (select statements are done)
    // TODO implement custom ordering (needs more work)
    // TODO implement author, license and tag adding, removal and editing
    // TODO previews
    ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(600, 200), ImVec2(FLT_MAX, FLT_MAX), WidthAtLeastTwiceHeight);
    if (assetWindowOpen) {
        ImGui::Begin("Assets", &assetWindowOpen);
        
        const float rounding = 3.0f;
        const float spacing = 10.0f; // TODO how to get exact spacing?
        
        float contentHeight = ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y;
        
//        ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, rounding);
        ImGui::BeginChild("AssetList", ImVec2(ImGui::GetWindowContentRegionWidth() - contentHeight - spacing, 0), false);

        ImGui::PushItemWidth(200.0f);
        
        const int typeCount = static_cast<int>(AssetType::COUNT) - 1;
        bool listUpdate = ImGui::Combo("Asset Type", &currentlyPickedAssetType,
                [](void*, int idx, const char** out_text) {
                    // Checking if we're in bounds
                    if (idx < 0 || idx >= static_cast<int>(AssetType::COUNT)) {
                        return false;
                    }

                    *out_text = con::AssetTypeToTranslationString(static_cast<AssetType>(idx)).c_str();
                    return true;
                },
            nullptr, typeCount, typeCount);

        ImGui::PopItemWidth();
        //assert(currentlyPickedAssetType >= 0 && currentlyPickedAssetType <= static_cast<int>(AssetType::Custom));

        if ((listUpdate || firstUpdatePending)) {
            // TODO fill the asset list. OR even better, replace it with an asset browser
            //assetList = std::move(project->getAssetList(static_cast<AssetType>(currentlyPickedAssetType), Project::OrderAssetsBy::FileName));
            firstUpdatePending = false;
        }
        
        float buttonWidth = 80.0f;
        ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - buttonWidth - 10);
        if (ImGui::Button("Columns", ImVec2(buttonWidth, 0))) {
            // TODO implement add/remove columns
        }
        
        if (ImGui::TreeNode("Search and Filtering")) {
            ImGui::Text("TODO implement");
            ImGui::TreePop();
        }
        
        // TODO maybe use something like this for headers?
        // https://github.com/ocornut/imgui/issues/513#issuecomment-240388455
        // Or this?
        // https://github.com/ocornut/imgui/issues/513#issuecomment-177487376
        ImGui::Columns(7, "listColumns");
        ImGui::Separator();

        ImGui::Text("Key");
        ImGui::NextColumn();
        
        ImGui::Text("ID");
        ImGui::NextColumn();

        ImGui::Text("Name");
        ImGui::NextColumn();

        ImGui::Text("Type");
        ImGui::NextColumn();

        ImGui::Text("Authors");
        ImGui::NextColumn();

        ImGui::Text("Licenses");
        ImGui::NextColumn();

        ImGui::Text("Tags");
        ImGui::NextColumn();

        ImGui::Separator();

        int i = 0;
        for (const auto& a : assetList.assets) {
            ImGui::PushID(i);

    //        char label[32];
    //        std::snprintf(label, 32, "%u", a.id);
    //        if (ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns)) {
    ////                    selected = i;
    //        }

            if (ImGui::AssetKey("key", static_cast<AssetType>(a.type), a.keyData)) {
                // Overflowing, remove last
                if (assetClipboard.size() >= maxClipboardElements) {
                    assetClipboard.pop_back();
                }
                
                assetClipboard.push_front(a);
            }
//            ImGui::SameLine();
//            ImGui::AssetLock("lock", static_cast<AssetType>(a.type), a.keyData);
//            ImGui::SameLine();
//            ImGui::AssetKeyWithLock("kl", static_cast<AssetType>(a.type), a.keyData);
            
            ImGui::NextColumn();
            
            ImGui::Text("%u", a.id);
            if (ImGui::BeginPopupContextItem("item context menu")) {
                ImGui::Text("HI");
                ImGui::EndPopup();
            }

            ImGui::NextColumn();

            ImGui::Text("%s", a.fileName.c_str());
            ImGui::NextColumn();

            // TODO Localize, then use these translation strings to get localized values
            ImGui::Text("%s", con::AssetTypeToTranslationString(a.type).c_str());
            ImGui::NextColumn();

            ImGui::Text("%s", a.authors.c_str());
            ImGui::NextColumn();

            ImGui::Text("%s", a.licenses.c_str());
            ImGui::NextColumn();

            ImGui::Text("%s", a.tags.c_str());
            ImGui::NextColumn();

            ImGui::PopID();
            i++;
        }

        ImGui::Columns(1, "listColumns");
        ImGui::Separator();
        
        ImGui::EndChild();
//        ImGui::PopStyleVar();
        
        ImGui::SameLine();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, rounding);
        ImGui::BeginChild("AssetDisplay", ImVec2(contentHeight, contentHeight), true);
        
        ImGui::Text("TODO Implement previews");
//        con::AssetType t = con::AssetType::Audio;
//        ImGui::AssetKey("t1", t, 0);ImGui::SameLine();ImGui::AssetKey("t3", t, 0);
//        ImGui::AssetLock("t2", t, 0);
        // Animations are not yet shareable
//        ImGui::AssetKey("t1", con::AssetType::Animation, 0);
//        ImGui::AssetKey("t2", con::AssetType::Audio, 0);
//        ImGui::AssetKey("t3", con::AssetType::Font, 0);
//        ImGui::AssetKey("t4", con::AssetType::Mesh, 0);
//        ImGui::AssetKey("t5", con::AssetType::Other, 0);
//        ImGui::AssetKey("t6", con::AssetType::Texture, 0);
        
        ImGui::EndChild();
        ImGui::PopStyleVar();
        
        ImGui::End();
    }
}

void EditorState::updateProjectFiles(float delta) {
    assert (fileSystemWatcherThread.get_id() == std::this_thread::get_id());
    
    filesToProcess.clear();
    
//     LOG_D("Iterating newFiles " << newFiles.size());
//     std::size_t fn = 0;
    // Check if a recently added file is still changing by being written to or copied.
    for (auto i = newFiles.begin(); i != newFiles.end();) {
        
//         LOG_D(fn << " " << i->path);
//         fn++;
        std::size_t newSize = fs::file_size(i->path);
        
        //LOG_D(i->path << " " << newSize)
        
        if (newSize == i->lastSize && i->stableSizeDuration.count() >= MIN_STABLE_FILE_SIZE_DURATION_SECONDS) {
            // File should be safe for reading, remove it from the list of files we observe and add it to a vector of files to process.
            filesToProcess.emplace_back(i->path);
            i = newFiles.erase(i);
        } else if (newSize == i->lastSize) {
            // Size is stable for now
            i->stableSizeDuration += std::chrono::duration<float>(delta);
            ++i;
        } else {
            // Size isn't stable (or no longer stable). (Re)set stable size duration and size itself.
            i->lastSize = newSize;
            i->stableSizeDuration = std::chrono::duration<float>(0.0f);
            ++i;
        }
    }
    
    if (filesToProcess.empty() && filesToRemove.empty()) {
        return;
    }
    
    for (const auto& f : filesToRemove) {
        LOG_D("Deleting file: " << f)
        //project->removeAsset(f);
    }
    
    for (const auto& f : filesToProcess) {
        LOG_D("Processing file: " << f)
        //project->addPendingAssetImport(f);
    }
    
    filesToRemove.clear();
}

void EditorState::fileSystemWatcherCallback(FileSystemWatcher::EventList eventList) {
    assert (fileSystemWatcherThread.get_id() == std::this_thread::get_id());
    
//     for (const auto& e : eventList) {
//         switch (e.getType()) {
//         case FileSystemEventFlags::Created:
//             LOG_D("-FS-CRE:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName());
//             break;
//         case FileSystemEventFlags::Deleted:
//             LOG_D("-FS-DEL:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName());
//             break;
//         case FileSystemEventFlags::Modified:
//             LOG_D("-FS-MOD:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName());
//             break;
//         case FileSystemEventFlags::Moved:
//             LOG_D("-FS-MOV:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName() << "\n\t" << e.getDestinationDirectory() << e.getDestinationName());
//             break;
//         case FileSystemEventFlags::Opened:
//             LOG_D("-FS-OPE:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName());
//             break;
//         case FileSystemEventFlags::Closed:
//             LOG_D("-FS-CLO:" << "\n\t" << (e.getOrigin() == FileSystemEventOrigin::Directory ? "Dir" : "Fil") << "\n\t"
//                              << e.getSourceDirectory() << e.getSourceName());
//             break;
//         }
//     }
    
//     for (const FileSystemEvent& c : eventList) {
//         if (c.getType() == FileSystemEventFlags::Created) {
//             LOG_D("Import directory monitoring. CREATED: " << c.getSourceDirectory() << c.getSourceName())
//             
//             // TODO can hash collisions happen here or is this case handled in addNewAsset()?
//             newFiles.emplace_back(c.getSourceDirectory() + c.getSourceName());
//             LOG_D("nfSize " << newFiles.size());
//         } else if (c.getType() == FileSystemEventFlags::Deleted) {
//             LOG_D("Import directory monitoring. DELETED: " << c.getSourceDirectory() << c.getSourceName())
// 
//             filesToRemove.emplace_back(c.getSourceDirectory() + c.getSourceName());
//         } else if (c.getType() == FileSystemEventFlags::Modified) {
//             LOG_D("Import directory monitoring. MODIFIED: " << c.getSourceDirectory() << c.getSourceName())
//             
//             std::string sourcePath = c.getSourceDirectory() + c.getSourceName();
//             
//             // Check if the file is contained in newFiles. If it is, it means it's being written to
//             // and should not be messed with. If it's not - time to re-import
//             auto result = std::find_if(std::begin(newFiles), std::end(newFiles), [&sourcePath](const ChangeTracker& a) {
//                 return a.path == sourcePath;
//             });
//             
//             if (result == std::end(newFiles)) {
//                 filesToRemove.emplace_back(sourcePath);
//                 newFiles.emplace_back(c.getDestinationDirectory() + c.getDestinationName());
//             }
//             
//         } else if (c.getType() == FileSystemEventFlags::Moved) {
//             if (c.getSourceDirectory().empty()) {
//                 // If a source path is empty, a file has just arrived. It MAY be a new one, or a replacement for an old one
//                 std::string fullPath = c.getDestinationDirectory() + c.getDestinationName();
//                 std::string strippedPath = project->stripProjectBasePath(fullPath);
//                 
//                 //std::string relativePath = ;
//                 auto nameHash = HS(strippedPath.c_str());
//                 
//                 sqlite3_bind_int64(selectImportPath, 1, nameHash.value());
//                 int collisionResult = sqlite3_step(selectImportPath);
// 
//                 // SQLITE_DONE == hash is not in DB yet, can safely insert
//                 if (collisionResult == SQLITE_DONE) {
//                    newFiles.emplace_back(fullPath);
//                     LOG_D("Import directory monitoring. NEW FILE MOVED INTO: " << c.getDestinationDirectory() << c.getDestinationName())
//                 } else { // SQLITE_ROW == hash already in DB, file needs to either be re-imported (if names match) OR we have a hash collision if they don't 
//                     const unsigned char* collidingPath = sqlite3_column_text(selectImportPath, 0);
// 
//                     if (std::string(reinterpret_cast<const char*>(collidingPath)) == strippedPath) {
//                         filesToRemove.emplace_back(fullPath);
//                         newFiles.emplace_back(fullPath);
//                         
//                         LOG_D("Import directory monitoring. REPLACEMENT FILE MOVED INTO: " << c.getDestinationDirectory() << c.getDestinationName())
//                     } else {
//                         // TODO Handle a hash collision gracefully or at very least localize this message.
//                         LOG_E("Hash collision: \"" << fullPath << "\" collides with \"" << collidingPath << "\"")
//                         throw std::runtime_error("Hash collision occured");
//                     }
//                 }
// 
//                 sqlite3_reset(selectImportPath);
//             } else if (c.getDestinationDirectory().empty()) { // If the destination path is empty, a file needs to be treatead as if it has been deleted
//                 filesToRemove.emplace_back(c.getDestinationDirectory() + c.getDestinationName());
//                 LOG_D("Import directory monitoring. FILE REMOVED VIA MOVE FROM: " << c.getSourceDirectory() << c.getSourceName())
//             } else if (c.getSourceDirectory() == c.getDestinationDirectory()) { // If neither path is empty and both are equal, we have a rename
//                 LOG_D("Import directory monitoring. RENAMED FROM: " << c.getSourceName() << " TO: " << c.getDestinationName())
//                 
//                 // TODO actual renaming instead of re-import
//                 filesToRemove.emplace_back(c.getSourceDirectory() + c.getSourceName());
//                 newFiles.emplace_back(c.getDestinationDirectory() + c.getDestinationName());
//             } else { // Two different directories should not appear, since (at least for now) we're only tracking the root of the imports folder.
//                 assert(false);
//             }
//         }
//     }
}

void ImGuiLog::show(const std::string& logStr) {
    // TODO rest of the functionality and DOCK at the bottom somehow?
    GraphicsAPI* api = engine->getGraphicsAPI();
    
    float width = api->getRenderSurfaceWidth();
    float height = api->getRenderSurfaceHeight() * 0.2f;
    float posY = api->getRenderSurfaceHeight() - height;
    
    ImGui::SetNextWindowPos(ImVec2(0, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::Begin("Log", nullptr);
    if (ImGui::Button("Clear")) {
        engine->clearLogString();
    }
//    ImGui::SameLine();
//    bool copy = ImGui::Button("Copy");
//    ImGui::SameLine();
//    Filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
//    if (copy) ImGui::LogToClipboard();
//
//    if (Filter.IsActive())
//    {
//        const char* buf_begin = Buf.begin();
//        const char* line = buf_begin;
//        for (int line_no = 0; line != NULL; line_no++)
//        {
//            const char* line_end = (line_no < LineOffsets.Size) ? buf_begin + LineOffsets[line_no] : NULL;
//            if (Filter.PassFilter(line, line_end))
//                ImGui::TextUnformatted(line, line_end);
//            line = line_end && line_end[1] ? line_end + 1 : NULL;
//        }
//    }
//    else
//    {
        ImGui::TextUnformatted(logStr.c_str());
//    }
//
    if (lastLogLength != logStr.length()) {
        ImGui::SetScrollHere(1.0f);
        lastLogLength = logStr.length();
    }
        
//    ScrollToBottom = false;
    ImGui::EndChild();
    ImGui::End();
}

}
