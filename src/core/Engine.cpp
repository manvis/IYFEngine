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
#include <vector>
#include <chrono>
#include <atomic>

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

#include "io/DefaultFileSystem.hpp"

#include "core/Constants.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "core/GameState.hpp"
#include "core/InputState.hpp"
#include "logging/Logger.hpp"
#include "core/Project.hpp"

#include "configuration/Configuration.hpp"
#include "localization/TextLocalization.hpp"
#include "sound/SoundAPI.hpp"
#include "assets/AssetManager.hpp"

#include "threading/ThreadProfiler.hpp"
#include "threading/ThreadPool.hpp"

#include "graphics/materials/MaterialDatabase.hpp"
#include "graphics/clusteredRenderers/ClusteredRenderer.hpp"

#include "../../VERSION.hpp"

namespace iyf {
inline bool ExtractCommandValue(const char* argument, std::string_view& value) {
    const std::string_view argValue(argument);
    
    if (argValue.empty()) {
        value = argValue;
        return false;
    }
    
    const std::size_t lastChar = argValue.size() - 1;
    const bool startsWithQuotes = argValue[0] == '\"';
    const bool endsWithQuotes = argValue[lastChar] == '\"';
    
    if (startsWithQuotes != endsWithQuotes) {
        std::cout << "Argument needs to be quoted from both sides or not quoted at all\n";
        return false;
    }
    
    if (startsWithQuotes) {
        if (argValue.size() <= 2) {
            value = std::string_view();
        } else {
            value = argValue.substr(1, argValue.size() - 2);
        }
    } else {
        value = argValue;
    }
    
    return true;
}

inline bool IsCommand(std::size_t length, const char* arg) {
    return length >= 3 && arg[0] == '-' && arg[1] == '-';
}

enum class StackOperation {
    NoOperation,
    Push,
    Pop
};

enum class CommandLineOptionID : std::uint16_t {
    Version,
    Project,
    NewProject
};

enum class CommandLineOptionType : std::uint8_t {
    Any,
    EditorOnly,
    GameOnly,
};

struct CommandLineOption {
    CommandLineOption(CommandLineOptionID id, std::uint8_t paramCount = 0, CommandLineOptionType type = CommandLineOptionType::Any)
    : id(id), paramCount(paramCount), type(type) {}
    
    CommandLineOptionID id;
    std::uint8_t paramCount;
    CommandLineOptionType type;
};

const std::unordered_map<std::string_view, CommandLineOption> COMMAND_LINE_OPTIONS = {
    {"version", CommandLineOption(CommandLineOptionID::Version)},
    {"project", CommandLineOption(CommandLineOptionID::Project, 1, CommandLineOptionType::EditorOnly)},
    {"new-project", CommandLineOption(CommandLineOptionID::NewProject, 4, CommandLineOptionType::EditorOnly)},
};

struct EngineInternalData {
    EngineInternalData() : ticks(std::chrono::milliseconds(TICKS)), frameID(0), pendingStackOperation(StackOperation::NoOperation) {}
    
    // Timing
    std::chrono::time_point<std::chrono::steady_clock> currentTime;
    std::chrono::time_point<std::chrono::steady_clock> previousTime;
    std::chrono::time_point<std::chrono::steady_clock> logicTime;
    const std::chrono::nanoseconds ticks;
    
    std::atomic<std::uint64_t> frameID;
    
    StackOperation pendingStackOperation;
    std::unique_ptr<GameState> tempState;
    std::vector<std::unique_ptr<GameState>> stateStack;
};

Engine::Engine(int argc, char* argv[], EngineMode engineMode) : graphicsDelta(0L), engineMode(engineMode), argv0(argv[0]), skipRunning(false), returnValue(0) {
    IYFT_PROFILER_NAME_THREAD("Main");
    
    internalData = std::make_unique<EngineInternalData>();
    
    skipRunning = !parseCommandLine(argc, argv);
    init();
}

bool Engine::parseCommandLine(int argc, char* argv[]) {
//     for (int i = 0; i < argc; ++i) {
//         LOG_V(argv[i]);
//     }
    bool needToQuit = false;
    
    if (argc > 1) {
        int versionCommand = -1;
        int projectDir = -1;
        int newProjectDir = -1;
        
        for (int i = 1; i < argc; ++i) {
            const char* arg = argv[i];
            const std::size_t length = std::strlen(arg);
            
            if (IsCommand(length, arg)) {
                const std::string_view view(arg + 2);
                
                const auto iter = COMMAND_LINE_OPTIONS.find(view);
                if (iter != COMMAND_LINE_OPTIONS.end()) {
                    if (iter->second.type == CommandLineOptionType::EditorOnly && engineMode != EngineMode::Editor) {
                        std::cout << "You can't use this command line option \"" << arg << "\" with and IYFEngine executable that wasn't compiled in editor mode.\n";
                        returnValue = 1;
                        return false;
                    }
                    
                    if (iter->second.type == CommandLineOptionType::GameOnly && engineMode != EngineMode::Game) {
                        std::cout << "You can't use this command line option \"" << arg << "\" with and IYFEngine executable that wasn't compiled in game mode.\n";
                        returnValue = 1;
                        return false;
                    }
                    
                    switch (iter->second.id) {
                        case CommandLineOptionID::Version:
                            versionCommand = i;
                            break;
                        case CommandLineOptionID::Project:
                            projectDir = i;
                            break;
                        case CommandLineOptionID::NewProject:
                            newProjectDir = i;
                            break;
                        default:
                            LOG_E("Handler for option \"{}\" is not implemented", arg);
                            throw std::runtime_error("Handler for option not implemented.");
                    }
                    
                    if (i + iter->second.paramCount >= argc) {
                        std::cout << "Option: " << arg << " is missing parameters\n";
                        returnValue = 1;
                        return false;
                    }
                    
                    for (int x = i + 1; x <= i + iter->second.paramCount; ++x) {
                        const char* par = argv[x];
                        const std::size_t parLength = std::strlen(par);
                        
                        if (IsCommand(parLength, par)) {
                            std::cout << "Option: " << arg << " is missing parameters. Next option was found too early.\n";
                            returnValue = 1;
                            return false;
                        }
                    }
                    
                    i += iter->second.paramCount;
                } else {
                    std::cout << "Unknown command line option: " << arg << "\n";
                    returnValue = 1;
                    return false;
                }
            } else {
                std::cout << "One or more options formatted incorrectly. Check " << arg << " and what comes after it\n";
                returnValue = 1;
                return false;
            }
        }
        
        if (engineMode == EngineMode::Editor) {
            if (projectDir != -1 && newProjectDir != -1) {
                std::cout << "Options --project and --new-project are mutually exclusive\n";
                returnValue = 1;
                return false;
            }
        }
        
        if (versionCommand != -1) {
            const auto& enVer = con::EngineVersion;
            std::cout << "IYFEditor. Powered by IYFEngine. Version: " 
                << enVer.getMajor() << "." << enVer.getMinor() << "." << enVer.getPatch() << "\n";
        }
        
        // Must be last!
        if (engineMode == EngineMode::Editor) {
            if (projectDir != -1) {
                std::string_view path;
                ExtractCommandValue(argv[projectDir + 1], path);
                project = std::make_unique<Project>(path);
            }
            
            if (newProjectDir != -1) {
                std::string_view path;
                ExtractCommandValue(argv[newProjectDir + 1], path);
                
                std::string_view name;
                ExtractCommandValue(argv[newProjectDir + 2], name);
                
                std::string_view company;
                ExtractCommandValue(argv[newProjectDir + 3], company);
                
                std::string_view locale;
                ExtractCommandValue(argv[newProjectDir + 4], locale);
                
                const auto creationResult = Project::Create(path, std::string(name), std::string(company), nullptr, std::string(locale));
                if (creationResult != Project::CreationResult::CreatedSuccessfully) {
                    std::cout << "Failed to create the new project:\n";
                    switch (creationResult) {
                        case Project::CreationResult::EmptyPath:
                            std::cout << "The project directory parameter cannot be empty.";
                            break;
                        case Project::CreationResult::EmptyName:
                            std::cout << "The project name parameter cannot be empty.";
                            break;
                        case Project::CreationResult::NotADirectory:
                            std::cout << "The project directory does not exist or it exists, but it is not a directory (e.g., you specified a file).";
                            break;
                        case Project::CreationResult::NonEmptyDirectory:
                            std::cout << "The final project directory exists, but it is not empty.";
                            break;
                        case Project::CreationResult::FolderCreationFailed:
                            std::cout << "Failed to create required project folders.";
                            break;
                        case Project::CreationResult::ProjectFileCreationFailed:
                            std::cout << "Failed to create the Project file.";
                            break;
                        case Project::CreationResult::CreatedSuccessfully:
                            break;
                    }
                    std::cout << "\n";
                    
                    returnValue = 1;
                    return false;
                }
                
                Path finalPath = path;
                finalPath /= name;
                
                project = std::make_unique<Project>(finalPath);
            }
            
            if (projectDir == -1 && newProjectDir == -1) {
                return false;
            }
        }
    } else if (engineMode == EngineMode::Editor) {
        std::cout << "Can't start. IYFEngine compiled in editor mode requires at least one command. E.g. a project path may be specified using --project\n";
        return false;
    }
    
    return !needToQuit;
}

void Engine::init() {
    if (skipRunning) {
        return;
    }

    assert((engineMode == EngineMode::Editor && project != nullptr) || (engineMode == EngineMode::Game && project == nullptr));
    if (engineMode == EngineMode::Game) {
        project = std::make_unique<Project>(fileSystem->getBaseDirectory());
    }
    
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        // Just throw an exception here. We can't log this one since
        // the file system may not have been initialized
        throw std::runtime_error("Couldn't initialize SDL");
    }
    
    VirtualFileSystem::argv = argv0;
    fileSystem = std::unique_ptr<VirtualFileSystem>(new VirtualFileSystem());
    fileSystem->initialize(project.get(), isEditorMode());
    
    // TODO set log direcotory here
    LOG_V("Sanitizer(s) that the engine was built with: {}", ACTIVE_SANITIZER_NAME);
    
    SDL_version linked;
    SDL_GetVersion(&linked);
    LOG_V("SDL version: {}.{}.{}\n\tSDL revision: {}",
          static_cast<std::uint32_t>(linked.major), static_cast<std::uint32_t>(linked.minor), static_cast<std::uint32_t>(linked.patch), SDL_GetRevision());
    
    //Can't use_make unique because the constructor is private and accessed via a friend declaration.
    Path userConfiguration = fileSystem->getPreferenceDirectory();
    userConfiguration /= "config.cfg";
    
    LOG_D("User's configuration file path: {}", userConfiguration);

    std::vector<ConfigurationPath> configPaths;
    configPaths.reserve(2);
    configPaths.emplace_back("EngineBaseConfig.cfg", &VirtualFileSystem::Instance());
    configPaths.emplace_back(userConfiguration, &DefaultFileSystem::Instance());
    
    config = std::unique_ptr<Configuration>(new Configuration(configPaths, Configuration::Mode::Editable, nullptr));
    useDebugAndValidation = config->getValue(HS("debugAndValidation"), ConfigurationValueNamespace::Engine);
    
    std::string locale = config->getValue(ConfigurationValueHandle(HS("textLocale"), con::GetConfigurationValueNamespaceNameHash(ConfigurationValueNamespace::Localization)));
    TextLocalizer::LoadResult result = SystemLocalizer().loadStringsForLocale(fileSystem.get(), con::SystemStringPath(), locale, false);
    if (result != TextLocalizer::LoadResult::LoadSuccessful) {
        LOG_E("Failed to load system strings. Error: {}", SystemLocalizer().loadResultToErrorString(result));
        throw std::runtime_error("Failed to load system strings (check log)");
    }
    
    // TODO I need a smarter way to pick the number of workers
    longTermWorkerPool = std::make_unique<iyft::ThreadPool>(2);
    frameWorkerPool = std::make_unique<iyft::ThreadPool>(2);
    
    if (!SystemLocalizer().executePendingSwap()) {
        throw std::runtime_error("Failed to swap in loaded strings");
    }
    
    // Can't use make_unique because the constructor is private and accessed via a friend declaration.
    inputState = std::unique_ptr<InputState>(new InputState(this, config.get()));

    LOG_D("{}", LOC_SYS(LH("test"), "Alpha", 1));
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
    
    if (!graphicsAPI->isInitialized()) {
        throw std::runtime_error("Failed to initialize the GraphicsAPI");
    }
    
    window = graphicsAPI->getWindow();
    
    // Initialize the asset manager (depends on the graphics API and the filesystem to load the system assets)
    assetManager = std::make_unique<AssetManager>(this);
    assetManager->initialize();
    
    materialDatabase = std::make_unique<MaterialDatabase>(this);
    materialDatabase->initialize();
    
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
    if (skipRunning) {
        return;
    }
    
    graphicsAPI->waitUntilDone();
    
    // State cleanup
    auto& stateStack = internalData->stateStack;
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
    
    materialDatabase->dispose();
    materialDatabase = nullptr;
    
    graphicsAPI->dispose();
    graphicsAPI = nullptr;
    
    soundAPI->dispose();
    soundAPI = nullptr;
    
    longTermWorkerPool = nullptr;
    frameWorkerPool = nullptr;
    
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
    if (skipRunning) {
        return;
    }
    
    auto& currentTime = internalData->currentTime;
    auto& previousTime = internalData->previousTime;
    auto& logicTime = internalData->logicTime;
    const std::chrono::nanoseconds ticks = internalData->ticks;
    
    currentTime = std::chrono::steady_clock::now();
    previousTime = currentTime;
    logicTime = currentTime;
    
    while (isRunning()) {
        // Mark the start of the next frame
        IYFT_PROFILER_NEXT_FRAME
        
        IYFT_PROFILE(Frame, iyft::ProfilerTag::Core)
        auto& stateStack = internalData->stateStack;
        
        if (internalData->pendingStackOperation == StackOperation::Push) {
            // Pause the current state
            if (!stateStack.empty()) {
                stateStack.back()->pause();
            }

            // Storing and initializing the new state
            stateStack.push_back(std::move(internalData->tempState));
            stateStack.back()->init();
        } else if (internalData->pendingStackOperation == StackOperation::Pop) {
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
        
        internalData->pendingStackOperation = StackOperation::NoOperation;
        
        if (stateStack.empty()) {
            LOG_I("No more states in the stack. Quitting.")
            break;
        }
        
        // Swap string maps if the language has changed
        if (GameLocalizer().executePendingSwap()) {
            LOG_V("Changed the string locale to: {}", GameLocalizer().getLocale());
        }
        
        inputState->pollInput(); //TODO really here?
        graphicsAPI->getDeviceMemoryManager()->beginFrame();
        assetManager->collectGarbage(); // TODO really here? TODO Make it smarter. Iterating through all assets every frame is NOT smart
        assetManager->enableLoadedAssets();
        
        currentTime = std::chrono::steady_clock::now();
        
        std::chrono::duration<float> secondsDelta = currentTime - previousTime;
        graphicsDelta = secondsDelta.count();
        frame(graphicsDelta);
        previousTime = currentTime;
        
        std::chrono::nanoseconds logicDelta = std::chrono::steady_clock::now() - logicTime;
        
        internalData->frameID++;

        // Catch up with logic (physics), if graphics are too slow
        while (logicDelta >= ticks) {
            // TODO making sure this is in sync with physics steps would be beneficial
            IYFT_PROFILE(LogicUpdate, iyft::ProfilerTag::Core)
            logicDelta -= ticks;
            logicTime += ticks;

            step();
        }
    }
}

void Engine::step() {
    // TODO Is this really the best place for such updates?
    fetchLogString();
    internalData->stateStack.back()->step();
}

void Engine::frame(float delta) {
    graphicsAPI->startFrame();
//     LOG_D("{}", graphicsAPI->getCurrentSwapImage())
    internalData->stateStack.back()->frame(delta);
    graphicsAPI->endFrame();
}

bool Engine::pushState(std::unique_ptr<GameState> gameState) {
    if (internalData->pendingStackOperation != StackOperation::NoOperation) {
        return false;
    }
    
    internalData->tempState = std::move(gameState);
    internalData->pendingStackOperation = StackOperation::Push;
    return true;
}

bool Engine::popState() {
    if (internalData->pendingStackOperation != StackOperation::NoOperation) {
        return false;
    }
    
    internalData->pendingStackOperation = StackOperation::Pop;
    return true;
}

void Engine::fetchLogString() {
    LoggerOutput* output = DefaultLog().getOutputObserver();
    if (!output->logsToBuffer()) {
        throw std::runtime_error("No buffer to fetch the log from");
    }
    
    log += output->getAndClearLogBuffer();
}

std::uint64_t Engine::getFrameID() const {
    return internalData->frameID;
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
