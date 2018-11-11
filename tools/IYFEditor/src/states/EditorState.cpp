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
#include "graphics/Renderer.hpp"
#include "threading/ThreadProfiler.hpp"
#include "threading/ThreadProfilerCore.hpp"
#include "assets/AssetManager.hpp"
#include "assets/loaders/MeshLoader.hpp"
#include "localization/TextLocalization.hpp"
#include "tools/MaterialEditor.hpp"
#include "tools/AssetCreatorWindow.hpp"
#include "tools/AssetUpdateManager.hpp"

#include "imgui.h"
#include "tinyfiledialogs.h"
#include "EditorUI.hpp"

#include "ImGuiImplementation.hpp"
#include "core/DefaultWorld.hpp"
#include "imgui_internal.h"
#include "utilities/IntegerPacking.hpp"
#include "utilities/ImGuiUtils.hpp"

#include "glm/gtx/string_cast.hpp"

// TODO move the test to tests
#include "graphics/shaderGeneration/VulkanGLSLShaderGenerator.hpp"

// TODO something smarter, like measuring the actual size
#define ADD_REMOVE_COMPONENT_BUTTON_WIDTH 60

namespace iyf::editor {
// TODO implement TOOLTIPS, like in imgui_demo.cpp:
//    ImGui::BeginTooltip();
//    ImGui::PushTextWrapPos(450.0f);
//    ImGui::TextUnformatted(desc);
//    ImGui::PopTextWrapPos();
//    ImGui::EndTooltip();

EditorState::EditorState(iyf::Engine* engine) : GameState(engine),
        newLevelDialogRequested(false), levelName("test_level_name"), levelDescription("test_level_desc"), isPickPlaceMode(false), 
        pickOrPlaceModeId(0), worldType(0), entityName("a"), cameraMode(CameraMode::Stationary), materialFamilyEditorOpen(true), materialEditorOpen(false),
        currentlyPickedAssetType(0), maxClipboardElements(5), logWindow(engine) {
    
    pickPlaceModeDrawFunction = drawNothing;
    debugDataUnit = DebugDataUnit::Mebibytes;
    editorUI->doSomething();
}

EditorState::~EditorState() { }

void EditorState::initialize() {
    IYFT_PROFILE(EditorInitialize, iyft::ProfilerTag::Editor)
    currentProject = engine->getProject();
    
    assetUpdateManager = std::make_unique<AssetUpdateManager>(engine);
    assetUpdateManager->initialize();
    
    // TODO because of frequent file system acccess, quite a few of these should happen in a separate thread 
    // TODO load the default world
    EntitySystemManagerCreateInfo ci(engine);
    ci.setEditorMode(true);
    world = new DefaultWorld("defaultWorld", engine->getConfiguration(), ci);
    world->initialize();
    world->addCamera(true);
    world->addCamera(false);
    
    materialFamilyEditor = std::make_unique<MaterialFamilyEditor>(engine, engine->getRenderer(), this);
    materialEditor = std::make_unique<MaterialEditor>();
    
    assetBrowserPathChanged = true;
    assetDirUpdated = false;
    
    // TODO move this to tests
    {
        MaterialFamilyDefinition def = con::GetMaterialFamilyDefinition(MaterialFamily::Toon);
        {
            VirtualFileSystemSerializer output("toon.iyfpl", File::OpenMode::Write);
            def.serialize(output);
        }
        
        MaterialFamilyDefinition empty;
        {
            VirtualFileSystemSerializer input("toon.iyfpl", File::OpenMode::Read);
            empty.deserialize(input);
        }
        
        LOG_D(def.getName() << " " << empty.getName());
        LOG_D(def.getSupportedLanguages().size() << " " << empty.getSupportedLanguages().size());
        LOG_D("Code matches? " << (def.getLightProcessingCode()[0] == empty.getLightProcessingCode()[0]) << "\n" << empty.getLightProcessingCode()[0]);
        
        const std::string newName = empty.getName() + "1";
        empty.setName(newName);
        
        iyf::VulkanGLSLShaderGenerator vkGen(engine->getFileSystem());
        
        const ShaderGenerationResult vsResult = vkGen.generateShader(RendererType::ForwardClustered, ShaderStageFlagBits::Vertex, def);
        const ShaderGenerationResult fsResult = vkGen.generateShader(RendererType::ForwardClustered, ShaderStageFlagBits::Fragment, def);
        
        assert(vsResult);
        assert(fsResult);
        
        LOG_V("VERTEX SHADER for " << def.getName() << ":\n" << vsResult.getContents());
//         LOG_V("VERTEX SHADER for " << empty.getName() << ":\n" << vkGen.generateShader(ShaderStageFlagBits::Vertex, empty).getContents());
        
        LOG_V("FRAGMENT SHADER for " << def.getName() << ":\n" << fsResult.getContents());
//         LOG_V("FRAGMENT SHADER for " << empty.getName() << ":\n" << vkGen.generateShader(ShaderStageFlagBits::Fragment, empty).getContents());
        
        ShaderCompilationSettings compilationSettings;
        compilationSettings.logAssembly = false;
        compilationSettings.optimizationLevel = ShaderOptimizationLevel::Performance;
        
        const VertexDataLayout chosenLayout = VertexDataLayout::MeshVertexColored;
        const std::int64_t vertexDataLayoutID = static_cast<std::int64_t>(chosenLayout);
        const VertexDataLayoutDefinition& vertexDataLayoutDefinition = con::GetVertexDataLayoutDefinition(chosenLayout);
        
        compilationSettings.macros.emplace_back(ShaderMacro::VertexDataLayout, vertexDataLayoutID);
        compilationSettings.macros.emplace_back(ShaderMacro::NormalMappingMode, static_cast<std::int64_t>(vertexDataLayoutDefinition.getSupportedNormalMappingMode()));
        compilationSettings.macros.emplace_back(ShaderMacro::NormalMapTextureAvailable);
        
        ShaderCompilationResult vsCompilationResult =  vkGen.compileShader(ShaderStageFlagBits::Vertex, vsResult.getContents(), "vertShader", compilationSettings);
        ShaderCompilationResult fsCompilationResult = vkGen.compileShader(ShaderStageFlagBits::Fragment, fsResult.getContents(), "fragShader", compilationSettings);
        
        if (vsCompilationResult) {
            LOG_V("Vertex shader compiled successfully");
        } else {
            LOG_V("Failed to compile the vertex shader. Error: " << vsCompilationResult.getErrorsAndWarnings());
        }
        
        if (fsCompilationResult) {
            LOG_V("Fragment shader compiled successfully");
        } else {
            LOG_V("Failed to compile the fragment shader. Error: " << fsCompilationResult.getErrorsAndWarnings());
        }
    }
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(MaterialRenderMode::COUNT); ++i) {
        renderModeNames.push_back(LOC_SYS(MaterialRenderModeNames[i]));
    }
    
    // Include the last one
    for (std::size_t i = 0; i <= static_cast<std::size_t>(AssetType::COUNT); ++i) {
        LocalizationHandle lh = LH(con::AssetTypeToTranslationString(static_cast<AssetType>(i)).c_str());
        assetTypeNames.push_back(LOC_SYS(lh));
    }
    currentlyPickedAssetType = static_cast<int>(AssetType::COUNT);
    
    currentlyOpenDir = con::ImportPath();
    
//     profilerResults = iyft::ProfilerResults::LoadFromFile("profilerResults.profres");
    profilerZoom = 1.0f;
    profilerOpen = false;
    
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
    IYFT_PROFILE(EditorDispose, iyft::ProfilerTag::Editor)
    if (world != nullptr) {
        world->dispose();
        delete world;
    }
    
    assetUpdateManager->dispose();
    
    currentProject = nullptr;
}

void EditorState::step() {
    IYFT_PROFILE(EditorStep, iyft::ProfilerTag::Editor)
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
    IYFT_PROFILE(EditorFrame, iyft::ProfilerTag::Editor)
    iyf::InputState* is = engine->getInputState();
    iyf::GraphicsAPI* api = engine->getGraphicsAPI();
    
    engine->getRenderer()->retrieveDataFromIDBuffer();
    
    if (hoveredItemIDFuture.valid()) {
        std::uint32_t id = hoveredItemIDFuture.get();
        
        if (id == std::numeric_limits<std::uint32_t>::max()) {
            deselectCurrentItem();
        } else {
            changeSelection(id);
            LOG_V("Picked item ID: " << id);
        }
    }
    
//    throw std::runtime_error("wrt");
    
    is->setMouseRelativeMode(cameraMode == CameraMode::Free);

    ImGuiImplementation* impl = engine->getImGuiImplementation();
    impl->requestRenderThisFrame();
    
    //std::cerr << sizeof(size_t) << std::endl;
//     ImGui::ShowDemoWindow();
    
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
            ImGui::MenuItem("Profiler", nullptr, &profilerOpen);
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
//         const ImVec2 mousePos = ImGui::GetMousePos();
//         ImGui::Text("Mouse: %f;%f", mousePos.x, mousePos.y);
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
    
    assetDirUpdated = assetUpdateManager->update();
    logWindow.show(engine->getLogString());
    // TODO implement family editor
    
    if (materialFamilyEditorOpen) {
        //materialFamilyEditor->show(&materialFamilyEditorOpen);
    }
    
    if (materialEditorOpen) {
        materialEditor->show(&materialEditorOpen);
    }
    
    showAssetWindow();
    showMaterialEditorWindow();
    showWorldEditorWindow();
    showEntityEditorWindow();
    showDebugWindow();
    showProfilerWindow();
    
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
    
    if (world != nullptr) {
        if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) && is->isMouseClicked(MouseButton::Left)) {
            int mouseX = is->getMouseX();
            int mouseY = is->getMouseY();
            
            glm::uvec2 windowSize = api->getWindowSize();
            if (mouseX >= 0 && mouseX <= static_cast<int>(windowSize.x) && mouseY >= 0 && mouseY <= static_cast<int>(windowSize.y)) {
                //world->rayPick(mouseX, mouseY);
                assert(engine->getRenderer()->isPickingEnabled());
                assert(!hoveredItemIDFuture.valid());
                hoveredItemIDFuture = engine->getRenderer()->getHoveredItemID();
            }
        }
        
        world->setInputProcessingPaused(cameraMode != CameraMode::Free);
        world->update(delta);
    }
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
    IYFT_PROFILE(EditorCreateWorld, iyft::ProfilerTag::Editor)
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
    iyf::StringHash hashedName = HS(modeName);
    
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
    IYFT_PROFILE(MaterialEditorDrawing, iyft::ProfilerTag::Editor)
    // TODO needs to be GENERATED for each family, but I need to implement the family
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

void EditorState::showProfilerWindow() {
    IYFT_PROFILE(EditorProfilerWindow, iyft::ProfilerTag::Editor)
    
    if (!profilerOpen) {
        return;
    }
    
    ImGui::SetNextWindowSize(ImVec2(1200,750), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Profiler", &profilerOpen)) {
        
        if (IYFT_PROFILER_STATUS == iyft::ProfilerStatus::EnabledAndNotRecording) {
            if (ImGui::Button("Start Recording")) {
                IYFT_PROFILER_SET_RECORDING(true);
            }
        } else if (IYFT_PROFILER_STATUS == iyft::ProfilerStatus::EnabledAndRecording) {
            if (ImGui::Button("Stop Recording")) {
                IYFT_PROFILER_SET_RECORDING(false);
                profilerResults = std::make_unique<iyft::ProfilerResults>(std::move(iyft::GetThreadProfiler().getResults()));
            }
        } else {
            assert(0);
        }
        
        ImGui::SameLine();
        ImGui::SliderFloat("Zoom", &profilerZoom, iyft::ProfilerResults::GetMinScale(), iyft::ProfilerResults::GetMaxScale());
        
        if (profilerResults != nullptr) {
            profilerResults->drawInImGui(profilerZoom);
        } else {
            ImGui::Text("Results not yet loaded or recorded.");
        }
    }
    
    ImGui::End();
}

void EditorState::showDebugWindow() {
    IYFT_PROFILE(EditorDebugWindow, iyft::ProfilerTag::Editor)
    
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
    IYFT_PROFILE(EditorBufferInfo, iyft::ProfilerTag::Editor)
    
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
    IYFT_PROFILE(WorldEditorWindow, iyft::ProfilerTag::Editor)
    
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

        // TODO multi select?
        // TODO this does NOT support hierarchies
        for (auto& entity : entities) {
            if (ImGui::Selectable(entity.first.c_str(), entity.second.isSelected())) {
                changeSelection(entity);
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
    
    ImGui::Text("Drop Mesh Here");
    if (ImGui::BeginDragDropTarget()) {
        const char* payloadName = util::GetPayloadNameForAssetType(AssetType::Mesh);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName)) {
            assert(payload->DataSize == sizeof(DragDropAssetPayload));
            
            DragDropAssetPayload payloadDestination;
            std::memcpy(&payloadDestination, payload->Data, sizeof(DragDropAssetPayload));
            
            world->addStaticMesh(payloadDestination.getNameHash());
        }
        ImGui::EndDragDropTarget();
    }
//     if (ImGui::AssetLock("StaticMesh", AssetType::Mesh, 0)) {
//         const AssetData& asset = assetClipboard[0];
//         
//         world->addStaticMesh(hash32_t(asset.id));
//     }
    
    ImGui::NextColumn();
    ImGui::Text("Static mesh entity");
    
    ImGui::NextColumn();
//     if (ImGui::AssetLock("DynamicMesh", AssetType::Mesh, 0)) {
//         const AssetData& asset = assetClipboard[0];
//         
//         world->addDynamicMesh(hash32_t(asset.id));
//     }
    
    ImGui::Text("Drop Mesh Here");
    if (ImGui::BeginDragDropTarget()) {
        const char* payloadName = util::GetPayloadNameForAssetType(AssetType::Mesh);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName)) {
            assert(payload->DataSize == sizeof(DragDropAssetPayload));
            
            DragDropAssetPayload payloadDestination;
            std::memcpy(&payloadDestination, payload->Data, sizeof(DragDropAssetPayload));
            
            world->addDynamicMesh(payloadDestination.getNameHash());
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::NextColumn();
    ImGui::Text("Dynamic mesh entity");
    
    ImGui::Columns();
    
    ImGui::End();
}

void EditorState::deselectCurrentItem() {
    if (world == nullptr) {
        return;
    }
    
    auto& entities = world->getEntityHierarchy();
    auto previouslySelected = entities.find(selectedEntityName);
    
    if (previouslySelected != entities.end()) {
        (*previouslySelected).second.setSelected(false);
    }
    
    selectedEntityName.clear();
    badName = false;
}

void EditorState::changeSelection(std::uint32_t entityID) {
    if (world == nullptr) {
        return;
    }
    
    std::string name = world->getEntityByID(entityID).getName();
    
    auto& entities = world->getEntityHierarchy();
    auto pickedItem = entities.find(name);
    
    assert(pickedItem != entities.end());
    
    changeSelection(*pickedItem);
}

void EditorState::changeSelection(EntityHierarchy::value_type& entity) {
    if (world == nullptr) {
        return;
    }
    
    auto& entities = world->getEntityHierarchy();
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

void EditorState::showEntityEditorWindow() {
    IYFT_PROFILE(EntityEditorWindow, iyft::ProfilerTag::Editor)
    
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
    IYFT_PROFILE(TransformationEditorWindow, iyft::ProfilerTag::Editor)
    
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
    IYFT_PROFILE(ComponentEditorWindow, iyft::ProfilerTag::Editor)
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
    
    auto path = assetManager->getAssetPathCopy(mesh->getNameHash());
    
    if (path) {
        ImGui::Text("Mesh file: %s", (*path).c_str());
    } else {
        assert("Missing mesh not handled");
    }
    
    AssetHandle<Mesh> mh = assetManager->getMissingAsset<Mesh>(iyf::AssetType::Mesh);
    
    MaterialRenderMode mode = renderData.getRenderMode();
    int tempMode = static_cast<int>(mode);
    
    // TODO create a SINGLE std::function and reuse it
    ImGui::Combo("Render mode", &tempMode, util::StringVectorGetter, &renderModeNames, renderModeNames.size());
    
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
    
    float exposure = camera.getExposure();
    if (ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.01f, 20.0f)) {
        camera.setExposure(exposure);
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
    IYFT_PROFILE(EditorAssetWindow, iyft::ProfilerTag::Editor)
    FileSystem* filesystem = engine->getFileSystem();
    AssetManager* assetManager = engine->getAssetManager();
    
    // TODO implement search (select statements are done)
    // TODO implement custom ordering (needs more work)
    // TODO implement author, license and tag adding, removal and editing
    // TODO previews
    ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_FirstUseEver);
//     ImGui::SetNextWindowSizeConstraints(ImVec2(600, 200), ImVec2(FLT_MAX, FLT_MAX), WidthAtLeastTwiceHeight);
    if (ImGui::Begin("Assets")) {
        
//         const float rounding = 3.0f;
//         const float spacing = 10.0f; // TODO how to get exact spacing?
//         
//         float contentHeight = ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y;
//         
// //        ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, rounding);
//         ImGui::BeginChild("AssetList", ImVec2(ImGui::GetWindowContentRegionWidth() - contentHeight - spacing, 0), false);

        const bool showUpNavigation = (currentlyOpenDir != con::ImportPath());
        ImGui::Text("Current path: %s", currentlyOpenDir.c_str());
        
        ImGui::Separator();
        
        ImGui::PushItemWidth(200.0f);
        
        const int typeCount = static_cast<int>(AssetType::COUNT) + 1;
        bool filterUpdate = ImGui::Combo("Asset Type", &currentlyPickedAssetType, util::StringVectorGetter, &assetTypeNames, typeCount, typeCount);

        ImGui::PopItemWidth();
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        
        if (ImGui::Button("Create New")) {}
        
        bool needToOpenCreatorWindow = false;
        if (ImGui::BeginPopupContextItem(nullptr, 0)) {
            
            if (ImGui::Selectable("Directory")) {
                assert(assetCreatorWindow == nullptr);
                assetCreatorWindow = std::make_unique<AssetCreatorWindow>(engine, currentlyOpenDir.generic_string());
                needToOpenCreatorWindow = true;
            }
            
            ImGui::Separator();
            
            if (ImGui::Selectable("Material")) {
                // TODO implement me
                LOG_W("NOT YET IMPLEMENTED");
//                 assert(assetCreatorWindow == nullptr);
//                 assetCreatorWindow = std::make_unique<AssetCreatorWindow>(engine, currentlyOpenDir.generic_string(), AssetType::Material);
//                 needToOpenCreatorWindow = true;
            }
            
            if (ImGui::Selectable("Material Template")) {
                assert(assetCreatorWindow == nullptr);
                assetCreatorWindow = std::make_unique<AssetCreatorWindow>(engine, currentlyOpenDir.generic_string(), AssetType::MaterialTemplate);
                needToOpenCreatorWindow = true;
            }
            
            ImGui::EndPopup();
        }
        
        if (assetCreatorWindow != nullptr) {
            if (needToOpenCreatorWindow) {
                assetCreatorWindow->openPopup();
            }
            
            if (assetCreatorWindow->show()) {
                assetCreatorWindow = nullptr;
            }
        }
        
        //assert(currentlyPickedAssetType >= 0 && currentlyPickedAssetType <= static_cast<int>(AssetType::Custom));
        // TODO or dir contents changed
        if (filterUpdate || assetBrowserPathChanged || assetDirUpdated) {
            assetList.clear();
            
            auto paths = filesystem->getDirectoryContents(currentlyOpenDir);
            
            const AssetType assetFilterType = static_cast<AssetType>(currentlyPickedAssetType);
            for (const auto& p : paths) {
                const fs::path finalPath = currentlyOpenDir / p;
                
                // con::ImportPath is not supposed to be a part of the hash
                const fs::path pathToHash = finalPath.lexically_relative(con::ImportPath());
                
                PHYSFS_Stat stat;
                filesystem->getFileSystemStatistics(finalPath, stat);
                
                AssetListItem listItem;
                listItem.path = finalPath;
                listItem.hash = HS(pathToHash.generic_string());
                if (stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_DIRECTORY) {
                    listItem.isDirectory = true;
                    listItem.imported = false;
                } else if (stat.filetype == PHYSFS_FileType::PHYSFS_FILETYPE_REGULAR) {
                    if (p.extension() == con::ImportSettingsExtension()) {
                        continue;
                    }
                    
                    if (assetFilterType != AssetType::ANY) {
                        const AssetType currentFileType = AssetManager::GetAssetTypeFromExtension(p);
                        if (currentFileType != assetFilterType) {
                            continue;
                        }
                    }
                    
                    listItem.isDirectory = false;
                    
                    // TODO should I check for presence of the import settings file and if everything is up-to-date here?
                    auto result = assetManager->getMetadataCopy(listItem.hash);
                    if (result) {
                        listItem.imported = true;
                        listItem.metadata = std::move(*result);
                    } else {
                        listItem.imported = false;
                    }
                }
                
                assetList.insert(std::move(listItem));
            }
            
            assetBrowserPathChanged = false;
            assetDirUpdated = false;
        }
        
        if (ImGui::TreeNode("Search and Filtering")) {
            ImGui::Text("TODO implement");
            ImGui::TreePop();
        }
        
        ImGui::BeginChild("assetDataColumns", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::VerticalSeparator();
        ImGui::Columns(4, "assetListColumns");
        ImGui::Separator();
        
        ImGui::Text("Path");
        ImGui::NextColumn();
        
        ImGui::Text("Name Hash");
        ImGui::NextColumn();
        
        ImGui::Text("Is Imported");
        ImGui::NextColumn();
        
        ImGui::Text("Type");
        ImGui::NextColumn();
        
        ImGui::Separator();
        
        if (showUpNavigation) {
            if (ImGui::Selectable("..", false, ImGuiSelectableFlags_SpanAllColumns)) {
                currentlyOpenDir = currentlyOpenDir.parent_path();
                assetBrowserPathChanged = true;
            }
            
            ImGui::NextColumn();
            
            ImGui::Text("N/A");
            ImGui::NextColumn();
            
            ImGui::Text("N/A");
            ImGui::NextColumn();
            
            ImGui::Text("Parent Directory");
            ImGui::NextColumn();
        }
        
        for (const auto& a : assetList) {
            const std::string fileName = a.path.filename().generic_string();
            if (ImGui::Selectable(fileName.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                if (a.isDirectory) {
                    currentlyOpenDir /= a.path.filename();
                    assetBrowserPathChanged = true;
                }
            }
            
            if (!a.isDirectory && ImGui::BeginPopupContextItem()) {
                ImGui::TextDisabled("Item: %s", fileName.c_str());
                
                ImGui::Separator();
                
                const AssetType type = AssetManager::GetAssetTypeFromExtension(a.path);
                if ((type == AssetType::MaterialTemplate) && ImGui::Selectable("Edit")) {
                    materialEditorOpen = true;
                    materialEditor->setPath(a.path);
                }
                
                if (a.imported) {
                    if (ImGui::Selectable("Re-import Asset")) {
                        // TODO implement me
                        LOG_W("NOT YET IMPLEMENTED")
                    }
                } else {
                    if (ImGui::Selectable("Import Asset")) {
                        // TODO implement me
                        LOG_W("NOT YET IMPLEMENTED")
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::Selectable("Open in File Browser")) {
                    filesystem->openInFileBrowser(filesystem->getRealDirectory(a.path.parent_path()));
                }
                
                if (ImGui::Selectable("Open in External Editor")) {
                    filesystem->openInFileBrowser(filesystem->getRealDirectory(a.path));
                }
                
                if (ImGui::Selectable("Delete")) {
                    nextAssetToDelete = NextAssetToDelete(filesystem->getRealDirectory(a.path), false);
                }
                
                ImGui::EndPopup();
            } else if (a.isDirectory && ImGui::BeginPopupContextItem()) {
                if (ImGui::Selectable("Open in File Browser")) {
                    filesystem->openInFileBrowser(filesystem->getRealDirectory(a.path));
                }
                
                if (ImGui::Selectable("Delete")) {
                    nextAssetToDelete = NextAssetToDelete(filesystem->getRealDirectory(a.path), true);
                }
                ImGui::EndPopup();
            }
            
            const AssetType type = AssetManager::GetAssetTypeFromExtension(a.path);
            
            if (a.imported && !a.isDirectory) {
                if (ImGui::BeginDragDropSource()) {
                    ImGui::Text("File: %s\n%s asset", fileName.c_str(), con::AssetTypeToTranslationString(type).c_str());
                    DragDropAssetPayload payload(a.hash, type);
                    ImGui::SetDragDropPayload(util::GetPayloadNameForAssetType(type), &payload, sizeof(payload));
                    ImGui::EndDragDropSource();
                }
            }
            
            ImGui::NextColumn();
            
            if (a.isDirectory) {
                ImGui::Text("N/A");
                ImGui::NextColumn();
                
                ImGui::Text("N/A");
                ImGui::NextColumn();
                
                ImGui::Text("Directory");
                ImGui::NextColumn();
            } else {
                ImGui::Text("%lu", a.hash.value());
                ImGui::NextColumn();
                
                ImGui::Text(a.imported ? "Yes" : "No");
                ImGui::NextColumn();
                
                ImGui::Text("%s", LOC_SYS(LH(con::AssetTypeToTranslationString(type).c_str())).c_str());
                ImGui::NextColumn();
            }
        }
        
        ImGui::Columns(1, "assetListColumns");
        
        if (nextAssetToDelete.needToOpenModal) {
            nextAssetToDelete.needToOpenModal = false;
            ImGui::OpenPopup("Confirm Deletion");
        }
        
        if (ImGui::BeginPopupModal("Confirm Deletion")) {
            const bool isDir = nextAssetToDelete.isDirectory;
            ImGui::Text("Are you sure you want to delete the %s?\nThis operation cannot be undone.", (isDir ? "directory" : "file"));
            
            ImGui::Spacing();
            
            if (ImGui::Button("Delete")) {
                const fs::path& realPath = nextAssetToDelete.path;
                
                if (isDir) {
                    iyf::ErrorCode ec;
                    if (fs::remove_all(realPath, ec) == static_cast<std::uintmax_t>(-1)) {
                        LOG_W("Failed to delete " << realPath);
                    }
                } else {
                    iyf::ErrorCode ec;
                    if (!fs::remove(realPath, ec)) {
                        LOG_W("Failed to delete " << realPath);
                    }
                }
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        
        ImGui::Separator();

        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(200, 200), ImVec2(FLT_MAX, FLT_MAX), util::SquareConstraint);
    if (ImGui::Begin("Last Selected Asset Info")) {
        ImGui::Text("TODO Implement previews");
        ImGui::End();
    }
}

void ImGuiLog::show(const std::string& logStr) {
    IYFT_PROFILE(EditorLogWindow, iyft::ProfilerTag::Editor)
    
    // TODO rest of the functionality and DOCK at the bottom somehow?
    GraphicsAPI* api = engine->getGraphicsAPI();
    
    const glm::uvec2 swapImageSize = api->getSwapchainImageSize();
    float width = swapImageSize.x * 0.7f;
    float height = swapImageSize.y * 0.2f;
    float posY = swapImageSize.y - height;
    
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
