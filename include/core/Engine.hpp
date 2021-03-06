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

#ifndef IYF_ENGINE_HPP
#define IYF_ENGINE_HPP

#include <memory>
#include <string>

struct SDL_Window;

namespace iyft {
class ThreadPool;
}

namespace iyf {

class GameState;
class GraphicsAPI;
class ImGuiImplementation;
class InputState;
class Configuration;
class AssetManager;
class MaterialDatabase;
class SoundAPI;
class VirtualFileSystem;
class Renderer;
class Project;
struct EngineInternalData;

enum class EngineMode {
    Editor,
    Game
};

class Engine {
public:
    Engine(int argc, char* argv[], EngineMode engineMode);
    ~Engine();

    void executeMainLoop();

    bool isRunning() const;
    
    inline EngineMode getMode() const {
        return engineMode;
    }
    
    inline bool isEditorMode() const {
        return engineMode == EngineMode::Editor;
    }
    
    inline bool isGameMode() const {
        return engineMode == EngineMode::Game;
    }

    /// \brief Finish the main loop and quit after finishing this frame.
    ///
    /// Tells the engine that it's time to stop the main loop and clean up all existing iyf::GameState objects in the state stack,
    /// unload all resources, close the window and libraries, etc.
    ///
    /// \remark This does not "kill" the Engine. The current frame is guaranteed to run until the end.
    void quit();

    /// Pushes a new state onto the stack. The current frame will continue until the end. Before the start of the next frame, 
    /// the current GameState will be paused by calling GameState::pause() and the newly pushed GameState will become the active 
    /// one. This function calls GameState::init() to initialize the GameState if it has not yet been initialized.
    /// For performance reasons and, if your initialization logic allows it, you may wish to initialize your GameState in a 
    /// separate thread before passing it to this function.
    ///
    /// \warning Only one stack operation can be performed per frame. Make sure you check the return value to avoid any
    /// surprises.
    ///
    /// \return Will the operation be performed or not?
    bool pushState(std::unique_ptr<GameState> gameState);
    
    /// Pops the currently active state from the GameState stack. The current frame will continue until the end. Before the
    /// start of the next frame, the current state will be disposed of by calling GameState::dispose() and a previous state
    /// will be resumed by calling GameState::resume()
    /// 
    /// \warning Only one stack operation can be performed per frame. Make sure you check the return value to avoid any
    /// surprises.
    ///
    /// \return Will the operation be performed or not?
    bool popState();

    SDL_Window* getWindow() const {
        return window;
    }

    /// \brief Returns delta time between this frame and the previous one in seconds.
    ///
    /// \return delta time in seconds
    float getGraphicsDelta() const {
        return graphicsDelta;
    }

    /// \brief Get a non-owning pointer to the iyf::InputState.
    ///
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::InputState.
    ///
    InputState* getInputState() const {
        return inputState.get();
    }

    /// \brief Get a non-owning pointer to the main iyf::Configuration instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::Configuration
    Configuration* getConfiguration() const {
        return config.get();
    }

    /// \brief Get a non-owning pointer to a concrete iyf::GraphicsAPI instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::GraphicsAPI
    GraphicsAPI* getGraphicsAPI() const {
        return graphicsAPI.get();
    }
    
    /// \brief Get a non-owning pointer to a concrete iyf::Renderer instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::Renderer
    Renderer* getRenderer() const {
        return renderer.get();
    }
    
    /// \brief Get a non-owning pointer to a concrete iyf::VirtualFileSystem instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::VirtualFileSystem
    VirtualFileSystem* getFileSystem() const {
        return fileSystem.get();
    }
    
    /// \brief Get a non-owning pointer to an iyf::Project instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::Project
    Project* getProject() const {
        return project.get();
    }
    
    /// \brief Get a non-owning pointer to an iyf::ImGuiImplementation instance
    /// 
    /// The pointer is guaranteed to stay valid until the last GameState is disposed of after calling quit().
    ///
    /// \return pointer to iyf::ImGuiImplementation
    ImGuiImplementation* getImGuiImplementation() const {
        return imguiImpl.get();
    }
    
    AssetManager* getAssetManager() const {
        return assetManager.get();
    }
    
    MaterialDatabase* getMaterialDatabase() const {
        return materialDatabase.get();
    }
    
    /// The long term worker iyft::ThreadPool should be used for tasks that may take multiple frames to complete, e.g., network
    /// requests, asset loading, etc.
    ///
    /// \return A pool of long term worker threads.
    iyft::ThreadPool* getLongTermWorkerPool() const {
        return longTermWorkerPool.get();
    }
    
    /// The frame worker iyft::ThreadPool should be used for tasks that take a fraction of the frame to complete, e.g., particle
    /// simulation or animation may be divided into multiple tasks, sent to the pool and waited for.
    /// 
    /// \return A pool of short term worker threads.
    iyft::ThreadPool* getFrameWorkerPool() const {
        return frameWorkerPool.get();
    }

    const std::string& getLogString() {
        return log;
    }

    void clearLogString() {
        log.clear();
    }
    
    std::uint64_t getFrameID() const;
    
    std::uint8_t getReturnValue() const {
        return returnValue;
    }
    
//        int getWidth();
//        int getHeight();

//        std::shared_ptr<Font> getDefaultFont();
private:
    bool parseCommandLine(int argc, char* argv[]);
    void init();

    void step();
    void frame(float delta);

    // Configuration
    std::unique_ptr<Configuration> config;
    
    // File system
    std::unique_ptr<VirtualFileSystem> fileSystem;

    void fetchLogString();

    // Graphics backend
    std::unique_ptr<GraphicsAPI> graphicsAPI;
    std::unique_ptr<Renderer> renderer;

    // Sound backend
    std::unique_ptr<SoundAPI> soundAPI;

    // Assets
    std::unique_ptr<AssetManager> assetManager;
    
    // Materials
    std::unique_ptr<MaterialDatabase> materialDatabase;
    
    // Threading
    std::unique_ptr<iyft::ThreadPool> longTermWorkerPool;
    std::unique_ptr<iyft::ThreadPool> frameWorkerPool;

    // SDL video
    SDL_Window* window;
    //int windowSizeX, windowSizeY;
    //bool fullscreen;
//   	int msaa;

    // Input
    std::unique_ptr<InputState> inputState;

    float graphicsDelta;
    
    bool running;
    EngineMode engineMode;
    bool useDebugAndValidation;

    std::unique_ptr<Project> project;

    std::string log;
    std::unique_ptr<ImGuiImplementation> imguiImpl;
    
    /// Contains data that I don't want to put in the header for some reason (e.g., it requires inclusion of a big and expensive C++ header)
    std::unique_ptr<EngineInternalData> internalData;
    
    char* argv0;
    bool skipRunning;
    std::uint8_t returnValue;
};

}

#endif // IYF_ENGINE_HPP
