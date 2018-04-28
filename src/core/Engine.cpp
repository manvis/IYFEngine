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

#include "core/Engine.hpp"

#define TICKS 16

#include <iostream>
#include <string>
#include <stdexcept>

#include <SDL2/SDL.h>

#include "graphics/GraphicsAPI.hpp"
#if defined(BUILD_WITH_VULKAN)
#   include "graphics/vulkan/VulkanAPI.hpp"
#   undef None // Vulkan headers pull in XLib, that pulls in the constant we need to undef
#   undef Bool
#elif defined (BUILD_WITH_OPENGL)
#   include "graphics/gl/OpenGLBackend.hpp"
#endif

#include <imgui.h>
#include "ImGuiImplementation.hpp"

#include "core/Constants.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "core/GameState.hpp"
#include "core/InputState.hpp"
#include "core/Logger.hpp"
#include "core/Project.hpp"

#include "core/configuration/Configuration.hpp"
#include "localization/TextLocalization.hpp"
#include "sound/SoundAPI.hpp"
#include "assets/AssetManager.hpp"

#include "graphics/clusteredRenderers/ClusteredRenderer.hpp"

#include "../../VERSION.hpp"

namespace iyf
{
Engine::Engine(char* argv0, bool editorMode) : graphicsDelta(0L), ticks(std::chrono::milliseconds(TICKS)), pendingStackOperation(StackOperation::NoOperation), argv0(argv0) {
    engineMode = editorMode ? EngineMode::Editor : EngineMode::Game;

    init();
}

void Engine::setProject(std::unique_ptr<Project> project) {
    if (isGameMode()) {
        throw std::logic_error("Can't change the project if the Engine is running in game mode");
    }
    
    if (project == nullptr) {
        throw std::runtime_error("The Project was nullptr");
    }
    
    // Remove the assets of the previous project. They should have been unloaded
    // when the last EditorState was popped. If they weren't, we will crash.
    assetManager->removeNonSystemAssetsFromManifest();
    assetManager->removeNonSystemPipelines();
    
    // TODO load all assets into the manager here
    
    this->project = std::move(project);
    fileSystem->setResourcePathsForProject(this->project.get());
}

void Engine::init() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        // Just throw an exception here. We can't log this one since
        // the file system may not have been initialized
        throw std::runtime_error("Couldn't initialize SDL");
    }

    fileSystem = std::unique_ptr<FileSystem>(new FileSystem(isEditorMode(), argv0));
    
    // Set the project and activate it in the filesystem. When running in game mode, this is the first and only
    // project change. If running in editor mode, the Project will soon get overriden, however, we still need
    // to set a default project in order to find system asset paths.
    //
    // Don't use setProject() here. It only works in editor mode and requires the asset manager to be initialized.
    project = std::make_unique<Project>(fileSystem->getBaseDirectory());
    fileSystem->setResourcePathsForProject(project.get());
    
    // TODO set log direcotory here
    LOG_V("Sanitizer(s) that the engine was built with: " << ACTIVE_SANITIZER_NAME);
    
    SDL_version linked;
    SDL_GetVersion(&linked);
    LOG_V("SDL version:" << static_cast<std::uint32_t>(linked.major) << "." << 
                            static_cast<std::uint32_t>(linked.minor) << "." << 
                            static_cast<std::uint32_t>(linked.patch) << "\n\t" <<
         "SDL revision:" << SDL_GetRevision());
    
    //Can't use_make unique because the constructor is private and accessed via a friend declaration.
    fs::path userConfiguration = fileSystem->getPreferenceDirectory();
    userConfiguration /= "config.lua";
    
    LOG_D("User's configuration file path: " << userConfiguration);
//     std::cout << "ccc" << std::flush;
    std::vector<ConfigurationFilePath> configPaths;
    configPaths.reserve(2);
    configPaths.emplace_back("EngineBaseConfig.lua", ConfigurationFilePath::PathType::Virtual);
    configPaths.emplace_back(userConfiguration, ConfigurationFilePath::PathType::Real);
    
    config = std::unique_ptr<Configuration>(new Configuration(configPaths, Configuration::Mode::Editable, nullptr));
    useDebugAndValidation = config->getValue(HS("debug_and_validation"), ConfigurationValueFamily::Engine);
//     std::cout << "ccc" << std::flush;
    
    // TODO these next config lines should go to the test cases
    auto editor = config->makeConfigurationEditor();
    editor->setValue("the_meaning_of_life", ConfigurationValueFamily::Other, 42.0f);
    editor->commit(false);
    
    double returnedValue = config->getValue(ConfigurationValueHandle(HS("the_meaning_of_life"), ConfigurationValueFamily::Other));
    LOG_D("Returned value: " << returnedValue);
    config->serialize();
    
    std::string locale = config->getValue(ConfigurationValueHandle(HS("text_locale"), ConfigurationValueFamily::Localization));
    TextLocalizer::LoadResult result = SystemLocalizer().loadStringsForLocale(fileSystem.get(), con::SystemStringPath, locale, false);
    if (result != TextLocalizer::LoadResult::LoadSuccessful) {
        LOG_E("Failed to load system strings. Error: " << SystemLocalizer().loadResultToErrorString(result));
        throw std::runtime_error("Failed to load system strings (check log)");
    }
    
    if (!SystemLocalizer().executePendingSwap()) {
        throw std::runtime_error("Failed to swap in loaded strings");
    }
    
    // Can't use make_unique because the constructor is private and accessed via a friend declaration.
    inputState = std::unique_ptr<InputState>(new InputState(this, config.get()));

    LOG_D(LOC_SYS(LH("test"), "Alpha", 1));
// TODO joystick management
//	SDL_GameController *ctrl;
//	SDL_Joystick *joy;
//
//    int i;
//
//    for (i = 0; i < SDL_NumJoysticks(); ++i) {
//        if (SDL_IsGameController(i)) {
//            std::stringstream ss;
//            ss << "Index " << i << " is a compatible controller, named " << SDL_GameControllerNameForIndex(i) << "\n";
//            ctrl = SDL_GameControllerOpen(i);
//            ss << "Controller " << i << " is mapped as " << SDL_GameControllerMapping(ctrl);
//
//            Logger::log(ss.str());
//        } else {
//            std::stringstream ss;
//            ss << "Index " << i << " is not a compatible controller.";
//
//            Logger::log(ss.str());
//            ss.str( std::string() );
//            ss.clear();
//
//            joy = SDL_JoystickOpen(i);
//
//            if (joy) {
//                ss << "Opened Joystick " << i << "\n";
//                ss << "Name: " << SDL_JoystickNameForIndex(0) << "\n";
//                ss << "Number of Axes: " << SDL_JoystickNumAxes(joy) << "\n";
//                ss << "Number of Buttons: " << SDL_JoystickNumButtons(joy) << "\n";
//                ss << "Number of Balls: " << SDL_JoystickNumBalls(joy);
//
//                Logger::log(ss.str());
//            } else {
//                ss << "Couldn't open Joystick " << i;
//                Logger::log(ss.str());
//            }
//
//            // Close if opened
//            if (SDL_JoystickGetAttached(joy)) {
//                SDL_JoystickClose(joy);
//            }
//        }
//    }

    running = true;
    
#if defined(BUILD_WITH_VULKAN)
    graphicsAPI = std::unique_ptr<VulkanAPI>(new VulkanAPI(this, useDebugAndValidation, config.get()));
#elif defined (BUILD_WITH_OPENGL)
//   graphicsAPI = std::unique_ptr<OpenGLAPI>(new OpenGLAPI(this, useDebugAndValidation, config.get()));
#endif

    // Initialize graphics
    graphicsAPI->initialize();
    window = graphicsAPI->getWindow();
    
    // Initialize the asset manager (depends on the graphics API and the filesystem to load the system assets)
    assetManager = std::make_unique<AssetManager>(this);
    assetManager->initialize();
    
    // Initialize the renderer
    // TODO choose a renderer based on configuration
    renderer = std::unique_ptr<ClusteredRenderer>(new ClusteredRenderer(this, graphicsAPI.get()));
    renderer->initialize();
    
    // Initialize imgui. Can't use_make unique because the constructor is private and accessed via a friend declaration.
    imguiImpl = std::unique_ptr<ImGuiImplementation>(new ImGuiImplementation(this));
    imguiImpl->initialize();
    
    // Initialize sounds
    // TODO SoundAPI should only be a base class
    // TODO pass engine and config
    soundAPI = std::unique_ptr<SoundAPI>(new SoundAPI(this));
    soundAPI->initialize();
    
    // Set cursor
    //SDL_SetWindowGrab(window, SDL_TRUE);
    if (isGameMode()) {
        int tx, ty;
        SDL_GetRelativeMouseState(&tx, &ty);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    } else {
        SDL_ShowCursor(0);
    }

    int x;
    int y;
    SDL_GetMouseState(&x, &y);
}

Engine::~Engine() {
    // State cleanup
    while (!stateStack.empty()) {
        stateStack.back()->dispose();
        stateStack.pop_back();
    }
    
    imguiImpl->dispose();
    imguiImpl = nullptr;

//  Lua::dispose();
//  ShaderProvider::dispose();
//  DeferredFBOmanager::dispose();
    
    renderer->dispose();
    renderer = nullptr;
    
    assetManager->dispose();
    assetManager = nullptr;
    
    graphicsAPI->dispose();
    graphicsAPI = nullptr;
    
    soundAPI->dispose();
    soundAPI = nullptr;
    
    fileSystem = nullptr;
    
    SDL_Quit();
}

bool Engine::isRunning() const {
    return running;
}

void Engine::quit() {
    running = false;
}

void Engine::executeMainLoop() {
    currentTime = std::chrono::steady_clock::now();
    previousTime = currentTime;
    logicTime = currentTime;
    
    while (isRunning()) {
        if (pendingStackOperation == StackOperation::Push) {
            // Pause the current state
            if (!stateStack.empty()) {
                stateStack.back()->pause();
            }

            // Storing and initializing the new state
            stateStack.push_back(std::move(tempState));
            stateStack.back()->init();
        } else if (pendingStackOperation == StackOperation::Pop) {
            // Clear data from the current state
            if (!stateStack.empty()) {
                GameState *p = stateStack.back().get();
                p->dispose();
                stateStack.pop_back();
            }

            // Resume previous (if any)
            if (!stateStack.empty()) {
                stateStack.back()->resume();
            }
        }
        
        pendingStackOperation = StackOperation::NoOperation;
        
        if (stateStack.empty()) {
            LOG_I("No more states in the stack. Quitting.")
            break;
        }
        
        // Swap string maps if the language has changed
        if (GameLocalizer().executePendingSwap()) {
            LOG_V("Changed the string locale to: " << GameLocalizer().getLocale());
        }
        
        inputState->pollInput(); //TODO really here?
        assetManager->collectGarbage(); // TODO really here? TODO Make it smarter. Iterating through all assets every frame is NOT smart
        
        currentTime = std::chrono::steady_clock::now();
        
        std::chrono::duration<float> secondsDelta = currentTime - previousTime;
        graphicsDelta = secondsDelta.count();
        frame(graphicsDelta);
        previousTime = currentTime;
        
        std::chrono::nanoseconds logicDelta = std::chrono::steady_clock::now() - logicTime;

        // Catch up with logic (physics), if graphics are too slow
        while (logicDelta >= ticks) {
            logicDelta -= ticks;
            logicTime += ticks;

            step();
        }
    }
}

void Engine::step() {
    // TODO Is this really the best place for such updates?
    fetchLogString();
    stateStack.back()->step();
}

void Engine::frame(float delta) {
    graphicsAPI->startFrame();
    stateStack.back()->frame(delta);
    graphicsAPI->endFrame();
}

bool Engine::pushState(std::unique_ptr<GameState> gameState) {
    if (pendingStackOperation != StackOperation::NoOperation) {
        return false;
    }
    
    tempState = std::move(gameState);
    pendingStackOperation = StackOperation::Push;
    return true;
}

bool Engine::popState() {
    if (pendingStackOperation != StackOperation::NoOperation) {
        return false;
    }
    
    pendingStackOperation = StackOperation::Pop;
    return true;
}

void Engine::fetchLogString() {
    LoggerOutput* output = DefaultLog().getOutputObserver();
    LogSplitter* splitter = static_cast<LogSplitter*>(output);
    StringLoggerOutput* strLog = static_cast<StringLoggerOutput*>(splitter->getObserverToLog1());
    
    log += strLog->getAndClear();
}

//void Engine::setUpFramebuffer() {
//
//}

//int Engine::getWidth()
//{
//    return windowSizeX;
//}
//
//int Engine::getHeight()
//{
//    return windowSizeY;
//}

}
