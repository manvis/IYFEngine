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

#ifndef IYF_EDITOR_WELCOME_STATE_HPP
#define IYF_EDITOR_WELCOME_STATE_HPP

#include <memory>
#include <future>
#include <mutex>
#include <string>
#include <array>

#include "core/GameState.hpp"
#include "core/Project.hpp"

namespace iyf {
class Engine;
class World;
}

namespace iyf::editor {
class EditorWelcomeState : public iyf::GameState {
public:
    EditorWelcomeState(Engine* engine);
    virtual ~EditorWelcomeState();
    
    virtual void dispose() final override;

    virtual void step() final override;
    virtual void frame(float delta) final override;

    virtual void pause() final override;
    virtual void resume() final override;

private:
    virtual void initialize() final override;
    void updateCreationProgress(const std::string& progress);
    void updateProjectOpenDate(std::size_t id);
    void writeProjectList();
    
    std::unique_ptr<World> world;
    
    struct ProjectData {
        std::string name;
        std::string path;
        std::string lastOpenText;
        std::uint64_t lastOpen;
        
        bool friend operator<(const ProjectData& a, const ProjectData& b) {
            return a.lastOpen < b.lastOpen;
        }
    };
    
    enum class UserDataValidationResult {
        MissingFirstName,
        MissingEmail,
        InvalidEmail,
        Success
    };
    
    struct UserData {
        UserData() : firstName{}, middleName{}, lastName{}, nickname{}, job{}, email{} { }
        
        std::array<char, 128> firstName;
        std::array<char, 128> middleName;
        std::array<char, 128> lastName;
        std::array<char, 128> nickname;
        std::array<char, 128> job;
        std::array<char, 256> email;
        
        UserDataValidationResult isValid() const;
        void save(Configuration* config) const;
    };
    
    UserData userData;
    UserData userDataEdit;
    std::string userDataValidationErrorDescription;
    
    std::vector<ProjectData> lastLoadedProjects;
    
    enum class NewProjectResult {
        Created,
        Cancelled
    };
    using ResultPathPair = std::pair<NewProjectResult, std::string>;
    std::future<Project::CreationResult> projectCreationFuture;
    std::future<ResultPathPair> projectDirectoryPickFuture;
    std::future<bool> openProjectAsyncTask;
    
    bool projectLoadRequested;
    bool pendingUserSetup;
    std::size_t projectToLoad;
    std::string messageText;
    std::mutex progressTextMutex;
    std::string progressText;
    std::string projectLoadResult;
    
    std::array<char, 128> nameBuffer;
    std::array<char, 2048> pathBuffer;
};
}

#endif // IYF_EDITOR_WELCOME_STATE_HPP
