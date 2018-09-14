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

#include "states/EditorWelcomeState.hpp"
#include "states/EditorState.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/GraphicsAPI.hpp"
#include "core/DefaultWorld.hpp"
#include "core/InputState.hpp"
#include "core/configuration/Configuration.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "utilities/Regexes.hpp"

#include "TestState.hpp"

#include "tinyfiledialogs.h"
#include "imgui.h"
#include "ImGuiImplementation.hpp"

#include <thread>
#include <chrono>
#include <iomanip>

#include "core/filesystem/cppFilesystem.hpp"

namespace iyf::editor {
const std::string AUTO_LOAD_PROJECT_CONFIG_NAME = "auto_load_last";

const std::size_t MAX_HISTORY_ELEMENTS = 10;
const std::string EDITOR_BUSY_POPUP_NAME = "Editor Busy";
const std::string NEW_PROJECT_POPUP_NAME = "New Project";
const std::string PROJECT_CREATION_POPUP_NAME = "Creating a project";
const std::string USER_SETUP_MODAL_NAME = "Create or Edit a User";
const std::string PROJECT_OPEN_POPUP_NAME = "Opening project";

const ImGuiWindowFlags MODAL_POPUP_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

const std::array<std::string, MAX_HISTORY_ELEMENTS> PREVIOUS_PROJECT_NAMES = {
    "previously_opened_project_0",
    "previously_opened_project_1",
    "previously_opened_project_2",
    "previously_opened_project_3",
    "previously_opened_project_4",
    "previously_opened_project_5",
    "previously_opened_project_6",
    "previously_opened_project_7",
    "previously_opened_project_8",
    "previously_opened_project_9",
};

const std::array<std::string, MAX_HISTORY_ELEMENTS> PREVIOUS_PROJECT_OPEN_TIME_NAMES = {
    "previously_opened_project_time_0",
    "previously_opened_project_time_1",
    "previously_opened_project_time_2",
    "previously_opened_project_time_3",
    "previously_opened_project_time_4",
    "previously_opened_project_time_5",
    "previously_opened_project_time_6",
    "previously_opened_project_time_7",
    "previously_opened_project_time_8",
    "previously_opened_project_time_9",
};

static inline std::string timeSinceEpochToString(const std::chrono::milliseconds durationMS) {
    const std::chrono::system_clock::time_point timePoint(durationMS);
    const time_t timeVal = std::chrono::system_clock::to_time_t(timePoint);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&timeVal), "%F %T");
    
    return ss.str();
}

static inline std::string timeSinceEpochToString(std::uint64_t time) {
    const std::chrono::milliseconds durationMS(time);
    
    return timeSinceEpochToString(durationMS);
}

EditorWelcomeState::~EditorWelcomeState() {}

EditorWelcomeState::UserDataValidationResult EditorWelcomeState::UserData::isValid() const {
    if (std::strlen(firstName.data()) == 0) {
        return UserDataValidationResult::MissingFirstName;
    }
    
    if (std::strlen(email.data()) == 0) {
        return UserDataValidationResult::MissingEmail;
    }
    
    if (!std::regex_match(email.data(), iyf::SystemRegexes().EmailValidationRegex)) {
        return UserDataValidationResult::InvalidEmail;
    }
    
    return UserDataValidationResult::Success;
}

void EditorWelcomeState::UserData::save(Configuration* config) const {
    auto editor = config->makeConfigurationEditor();
    editor->setValue("user_first_name", ConfigurationValueFamily::Editor, std::string(firstName.data()));
    editor->setValue("user_middle_name", ConfigurationValueFamily::Editor, std::string(middleName.data()));
    editor->setValue("user_last_name", ConfigurationValueFamily::Editor, std::string(lastName.data()));
    editor->setValue("user_nickname", ConfigurationValueFamily::Editor, std::string(nickname.data()));
    editor->setValue("user_job", ConfigurationValueFamily::Editor, std::string(job.data()));
    editor->setValue("user_email", ConfigurationValueFamily::Editor, std::string(email.data()));
    editor->commit(false);
    config->serialize();
}

void EditorWelcomeState::updateCreationProgress(const std::string& progress) {
    std::lock_guard<std::mutex> guard(progressTextMutex);
    
    progressText.append("\n").append(progress);
}

EditorWelcomeState::EditorWelcomeState(Engine* engine) : iyf::GameState(engine), projectLoadRequested(false), pendingUserSetup(false), projectToLoad(0), nameBuffer{}, pathBuffer{} {}

void EditorWelcomeState::writeProjectList() {
    Configuration* config = engine->getConfiguration();
    auto editor = config->makeConfigurationEditor();
    
    assert(lastLoadedProjects.size() <= 10);
    for (std::size_t i = 0; i < lastLoadedProjects.size(); ++i) {
        const ProjectData& data = lastLoadedProjects[i];
        editor->setValue(PREVIOUS_PROJECT_NAMES[i], ConfigurationValueFamily::Editor, data.path);
        editor->setValue(PREVIOUS_PROJECT_OPEN_TIME_NAMES[i], ConfigurationValueFamily::Editor, std::to_string(data.lastOpen));
    }
    
    editor->commit(false);
    config->serialize();
}

void EditorWelcomeState::updateProjectOpenDate(std::size_t id) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now);
    std::uint64_t nowMillis = millis.count();
    
    // Extract the element and update the last open date
    ProjectData data = lastLoadedProjects[id];
    data.lastOpen = nowMillis;
    data.lastOpenText = timeSinceEpochToString(millis);
    
    if (id != 0) {
        // Move it to the top of the list
        lastLoadedProjects.erase(lastLoadedProjects.begin() + id);
        lastLoadedProjects.insert(lastLoadedProjects.begin(), data);
    } else {
        lastLoadedProjects[0] = data;
    }
    
    // and write it back out
    writeProjectList();
}

void EditorWelcomeState::initialize() {
    EntitySystemManagerCreateInfo ci(engine);
    ci.setEditorMode(true);
    world = std::make_unique<DefaultWorld>("defaultWorld", engine->getConfiguration(), ci);
    world->initialize();
    
    const Configuration* config = engine->getConfiguration();
    for (std::size_t i = 0; i < MAX_HISTORY_ELEMENTS; ++i) {
        const std::string path = config->getValue(PREVIOUS_PROJECT_NAMES[i], ConfigurationValueFamily::Editor);
        const std::string time = config->getValue(PREVIOUS_PROJECT_OPEN_TIME_NAMES[i], ConfigurationValueFamily::Editor);
        
        if (time.empty() || path.empty()) {
            break;
        }
        
        std::uint64_t timeInt;
        
        try {
            timeInt = std::stoull(time);
        } catch(std::logic_error& e) {
            LOG_W("Could not convert time string to uint64_t for project " << path);
            continue;
        }
        
        const std::string name = path.substr(path.find_last_of('/') + 1);
        
        ProjectData data;
        data.name = name;
        
        if (!fs::exists(path)) {
            LOG_V("Project " << name << " no longer exists in " << path << ". It will be removed from the project list.");
            data.path.clear();
        } else {
            data.path = path;
        }
        
        data.lastOpen = timeInt;
        data.lastOpenText = timeSinceEpochToString(timeInt);
        
        lastLoadedProjects.push_back(std::move(data));
    }
    
    lastLoadedProjects.erase(std::remove_if(lastLoadedProjects.begin(), lastLoadedProjects.end(), [](const ProjectData& data) {
//         LOG_D("PTH: " << data.path);
        return data.path.empty();
    }), lastLoadedProjects.end());
    
    LOG_V("Number of remembered loaded projects: " << lastLoadedProjects.size())
    writeProjectList();
    
    const std::string firstName = config->getValue("user_first_name", ConfigurationValueFamily::Editor);
    const std::string middleName = config->getValue("user_middle_name", ConfigurationValueFamily::Editor);
    const std::string lastName = config->getValue("user_last_name", ConfigurationValueFamily::Editor);
    const std::string nickname = config->getValue("user_nickname", ConfigurationValueFamily::Editor);
    const std::string job = config->getValue("user_job", ConfigurationValueFamily::Editor);
    const std::string email = config->getValue("user_email", ConfigurationValueFamily::Editor);
    
    std::memcpy(userData.firstName.data(), firstName.c_str(), firstName.length() + 1);
    std::memcpy(userData.middleName.data(), middleName.c_str(), middleName.length() + 1);
    std::memcpy(userData.lastName.data(), lastName.c_str(), lastName.length() + 1);
    std::memcpy(userData.nickname.data(), nickname.c_str(), nickname.length() + 1);
    std::memcpy(userData.job.data(), job.c_str(), job.length() + 1);
    std::memcpy(userData.email.data(), email.c_str(), email.length() + 1);
    
    bool userValid = userData.isValid() == UserDataValidationResult::Success;
    bool autoLoadLast = config->getValue(AUTO_LOAD_PROJECT_CONFIG_NAME, ConfigurationValueFamily::Editor);
    if (autoLoadLast && !userValid) {
        pendingUserSetup = true;
    } else if (autoLoadLast && userValid && !lastLoadedProjects.empty()) {
        projectLoadRequested = true;
        projectToLoad = 0;
    } else if (!userValid) {
        pendingUserSetup = true;
    }
}

void EditorWelcomeState::dispose() {
    world->dispose();
    world = nullptr;
}

void EditorWelcomeState::step() {
    if (engine->getInputState()->isKeyPressed(SDL_SCANCODE_Q)) {
        engine->quit();
    }
    
//     if (engine->getInputState()->isKeyPressed(SDL_SCANCODE_T)) {
//         engine->pushState(std::make_unique<test::TestState>(engine));
//     }
}

void EditorWelcomeState::frame(float delta) {
    GraphicsSystem* graphicsSystem = static_cast<GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    Camera& camera = graphicsSystem->getActiveCamera();
    //camera.get
    
    GraphicsAPI* api = engine->getGraphicsAPI();
    Configuration* config = engine->getConfiguration();
    
    // ImGui should always render to the final image
    const glm::uvec2 swapImageSize = api->getSwapchainImageSize();
    const std::uint32_t w = swapImageSize.x;
    const std::uint32_t h = swapImageSize.y;
    const std::uint32_t wMargin = w * 0.1f;
    const std::uint32_t hMargin = h * 0.1f;
    const std::uint32_t windowW = w - 2 * wMargin;
    const std::uint32_t windowH = h - 2 * hMargin;
    
    const float sectionWidthPercentage = 0.326f;
    
    ImGuiImplementation* impl = engine->getImGuiImplementation();
    impl->requestRenderThisFrame();
    
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    if (ImGui::Begin("Info Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
    
    ImGui::SetNextWindowPos(ImVec2(wMargin, hMargin), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowW, windowH), ImGuiCond_Always);
    ImGui::Begin("Create or open a project", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    
    if (std::strlen(userData.firstName.data()) > 0) {
        ImGui::Text("Welcome to IYFEngine, %s (%s)!", userData.firstName.data(), userData.email.data());
        ImGui::SameLine();
        if (ImGui::SmallButton("Edit user")) {
            pendingUserSetup = true;
        }
    } else {
        ImGui::Text("Welcome to IYFEngine!");
    }
    ImGui::TextWrapped("To begin, please start a new Project or choose an existing one.");
    
    ImGui::BeginChild("Project Section", ImVec2(windowW * sectionWidthPercentage, -1), true);
    ImGui::Text("Projects");
    ImGui::Separator();
    
    
    float buttonWidth = ImGui::GetContentRegionAvailWidth() * 0.5f;
    if (ImGui::Button("Create new project", ImVec2(buttonWidth, 0))) {
        ImGui::OpenPopup(NEW_PROJECT_POPUP_NAME.c_str());
    }
    
    if (ImGui::BeginPopupModal(NEW_PROJECT_POPUP_NAME.c_str(), nullptr, MODAL_POPUP_FLAGS)) {
        ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("This name is used as an internal identifier and as a name of a folder that will \n"
                              "be created in the directory specified below. You SHOULD NOT use spaces, punctuation \n"
                              "or any special characters in this name.\n\n"
                              "You'll be able to set, change and localize a human readable name later.");
        }
        ImGui::InputText("Directory", pathBuffer.data(), pathBuffer.size());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("A folder with a name matching the project's name will be automatically created \n"
                              "in this directory.");
        }
        ImGui::SameLine();
        if (ImGui::Button("Open Folder")) {
            // Immediately open a modal that will block any input. This is required because having two separate blocking actions
            // at the same time would cause BadThings™ to happen
            ImGui::OpenPopup(EDITOR_BUSY_POPUP_NAME.c_str());
            
            // Show the file open dialog in a separate thread because we don't want it to block rendering
            projectDirectoryPickFuture = std::async(std::launch::async, []() -> ResultPathPair {
                const char* folderPath = tinyfd_selectFolderDialog("Select the new project folder", nullptr);
                
                if (folderPath == nullptr) {
                    return {NewProjectResult::Cancelled, ""};
                }
                
                return {NewProjectResult::Created, folderPath};
            });
        }
        
        // The modal that was mentioned a couple comments ago
        if (ImGui::BeginPopupModal(EDITOR_BUSY_POPUP_NAME.c_str(), nullptr, MODAL_POPUP_FLAGS)) {
            ImGui::Text("The editor will resume once the file operation is complete.");
            
            assert(projectDirectoryPickFuture.valid());
            
            // Once again, if it's not ready yet, don't block
            auto status = projectDirectoryPickFuture.wait_for(0ms);
            
            if (status == std::future_status::ready) {
                ResultPathPair result = projectDirectoryPickFuture.get();
                
    //             if (result.first == NewProjectResult::FolderNotEmpty) {
    //                 errorSet = true;
    //                 errorText = "The selected folder was not empty.";
    //             } else 
                if (result.first == NewProjectResult::Created) {
                    const std::size_t byteCount = result.second.size() + 1;
                    if (byteCount > pathBuffer.size()) {
                        // TODO handle this properly
                        throw std::runtime_error("Path longer than buffer");
                    }
                    
                    std::memcpy(pathBuffer.data(), result.second.c_str(), byteCount);
                }
                
                projectDirectoryPickFuture = std::future<ResultPathPair>();
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        if (!messageText.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", messageText.c_str());
        }
        
        if (ImGui::Button("Create")) {
            ImGui::OpenPopup(PROJECT_CREATION_POPUP_NAME.c_str());
            
            std::function<void(const std::string&)> callbackFunc = std::bind(&EditorWelcomeState::updateCreationProgress, this, std::placeholders::_1);
            projectCreationFuture = std::async(std::launch::async, [this, callbackFunc]() {
                return Project::Create(std::string(pathBuffer.data()), std::string(nameBuffer.data()), "Your Company", callbackFunc, "en_US");
            });
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        
        bool closeThisWindow = false;
        if (ImGui::BeginPopupModal(PROJECT_CREATION_POPUP_NAME.c_str(), nullptr, MODAL_POPUP_FLAGS)) {
            // Introduce a scope to get the updated string in a safe way
            {
                std::lock_guard<std::mutex> guard(progressTextMutex);
                ImGui::Text("Creating a new project:\n%s", progressText.c_str());
            }
            
            if (projectCreationFuture.valid()) {
                auto status = projectCreationFuture.wait_for(0ms);
                if (status == std::future_status::ready) {
                    Project::CreationResult result = projectCreationFuture.get();
                    
                    // This is probably too much...
                    {
                        std::lock_guard<std::mutex> guard(progressTextMutex);
                        progressText.clear();
                    }
                    
                    if (result == Project::CreationResult::CreatedSuccessfully) {
                        messageText.clear();
                        closeThisWindow = true;
                        
                        auto now = std::chrono::system_clock::now().time_since_epoch();
                        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now);
                        std::uint64_t nowMillis = millis.count();
                        
                        ProjectData data;
                        data.name = std::string(nameBuffer.data());
                        data.path = std::string(pathBuffer.data()) + "/" + data.name;
                        data.lastOpen = nowMillis;
                        data.lastOpenText = timeSinceEpochToString(millis);
                        lastLoadedProjects.insert(lastLoadedProjects.begin(), data);
                        
                        if (lastLoadedProjects.size() > 10) {
                            lastLoadedProjects.pop_back();
                        }
                        
                        writeProjectList();
                    } else {
                        messageText = "Project creation failed:\n";
                        switch (result) {
                            case Project::CreationResult::EmptyPath:
                                messageText += "The project directory parameter cannot be empty.";
                                break;
                            case Project::CreationResult::EmptyName:
                                messageText += "The project name parameter cannot be empty.";
                                break;
                            case Project::CreationResult::NotADirectory:
                                messageText += "The project directory does not exist or it exists, but it is not a directory (e.g., you specified a file).";
                                break;
                            case Project::CreationResult::NonEmptyDirectory:
                                messageText += "The final project directory exists, but it is not empty.";
                                break;
                            case Project::CreationResult::FolderCreationFailed:
                                messageText += "Failed to create required project folders.";
                                break;
                            case Project::CreationResult::ProjectFileCreationFailed:
                                messageText += "Failed to create the Project file.";
                                break;
                            case Project::CreationResult::CreatedSuccessfully:
                                break;
                        }
                    }
                    
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::EndPopup();
        }
        
        if (closeThisWindow) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    bool bt = config->getValue(AUTO_LOAD_PROJECT_CONFIG_NAME, ConfigurationValueFamily::Editor);
    if (ImGui::Checkbox("Auto load last project", &bt)) {
        auto editor = config->makeConfigurationEditor();
        editor->setValue(AUTO_LOAD_PROJECT_CONFIG_NAME, ConfigurationValueFamily::Editor, bt);
        editor->commit(false);
        editor = nullptr;
        
        // TODO use a separate thread?
        config->serialize();
    }
    ImGui::Separator();
    
    ImGui::BeginChild("Project list");
    bool projectActivated = false;
    std::size_t activatedProjectID = 0;
    for (std::size_t i = 0; i < lastLoadedProjects.size(); ++i) {
        const auto& projectData = lastLoadedProjects[i];
        
        ImGui::PushID(i);
        ImGui::Text("%zu", i);
        ImGui::SameLine();
        if (ImGui::Button("Open", ImVec2(80.0f, 50.0f))) {
            projectActivated = true;
            activatedProjectID = i;
        }
        ImGui::SameLine();
        ImGui::Text("Name: %s\nPath: %s\nLast Opened: %s", projectData.name.c_str(), projectData.path.c_str(), projectData.lastOpenText.c_str());
        
        if (i < lastLoadedProjects.size() - 1) {
            ImGui::Separator();
        }
        
        ImGui::PopID();
    }
    
    if (projectActivated) {
        updateProjectOpenDate(activatedProjectID);
        projectLoadRequested = true;
        projectToLoad = 0;
    }
    
    ImGui::EndChild();
    ImGui::EndChild();
    
    if (projectLoadRequested) {
        projectLoadRequested = false;
        ImGui::OpenPopup(PROJECT_OPEN_POPUP_NAME.c_str());
        
        openProjectAsyncTask = std::async(std::launch::async, [this](){
            auto project = std::make_unique<Project>(lastLoadedProjects[projectToLoad].path);
            FileSystem* fileSystem = engine->getFileSystem();
            
            LOG_V("Write path before project change: " << fileSystem->getCurrentWriteDirectory());
            engine->setProject(std::move(project));
            LOG_V("Write path after project change: " << fileSystem->getCurrentWriteDirectory());
            
            // TODO should I initialize the editor state here?
            
//             Project::InitializationResult result = project->initialize();
//             
//             if (result != Project::InitializationResult::Success) {
//                 project = nullptr;
//             }
            
            return true;
        });
    }
    
    if (ImGui::BeginPopupModal(PROJECT_OPEN_POPUP_NAME.c_str(), nullptr, MODAL_POPUP_FLAGS)) {
        auto status = openProjectAsyncTask.wait_for(0ms);
        if (status == std::future_status::ready) {
            auto result = openProjectAsyncTask.get();
            
            if (result == true) {
                engine->pushState(std::make_unique<EditorState>(engine));
            }
            
            // TODO report project initialization errors
        }
        ImGui::Text("Please wait while the data is loaded.");
        ImGui::EndPopup();
    }
    
    ImGui::SameLine();
    
    ImGui::BeginChild("News Section", ImVec2(windowW * sectionWidthPercentage, -1), true);
    ImGui::Text("News and Updates");
    ImGui::Separator();
    ImGui::Text("TODO: Implement me");
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("Tutorial Section", ImVec2(windowW * sectionWidthPercentage, -1), true);
    ImGui::Text("Tutorials and Samples");
    ImGui::Separator();
    ImGui::Text("TODO: Implement me");
    ImGui::EndChild();
    
    ImGui::End();
    
    if (pendingUserSetup) {
        ImGui::OpenPopup(USER_SETUP_MODAL_NAME.c_str());
        userDataEdit = userData;
        pendingUserSetup = false;
    }
    
    if (ImGui::BeginPopupModal(USER_SETUP_MODAL_NAME.c_str(), nullptr, MODAL_POPUP_FLAGS)) {
        ImGui::Text("NOTE: the data you enter here will be stored locally and used to tag\nyour asset imports and file changes.");
        ImGui::Separator();
        ImGui::Text("The email is considered to be the user's primary identifier and should\nnever change.");
        ImGui::InputText("Email (*)", userDataEdit.email.data(), userDataEdit.email.size());
        ImGui::Separator();
        ImGui::InputText("First Name (*)", userDataEdit.firstName.data(), userDataEdit.firstName.size());
        ImGui::InputText("Middle Name", userDataEdit.middleName.data(), userDataEdit.middleName.size());
        ImGui::InputText("Last Name", userDataEdit.lastName.data(), userDataEdit.lastName.size());
        ImGui::InputText("Nickname", userDataEdit.nickname.data(), userDataEdit.nickname.size());
        ImGui::Separator();
        ImGui::InputText("Job", userDataEdit.job.data(), userDataEdit.job.size());
        ImGui::Separator();
        ImGui::Text("(*) - indicates required fields");
        
        if (!userDataValidationErrorDescription.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", userDataValidationErrorDescription.c_str());
        }
        
        if (ImGui::Button("Save")) {
            UserDataValidationResult result = userDataEdit.isValid();
            switch (result) {
                case UserDataValidationResult::Success:
                    userDataValidationErrorDescription.clear();
                    userData = userDataEdit;
                    userData.save(config);
                    
                    ImGui::CloseCurrentPopup();
                    break;
                case UserDataValidationResult::MissingEmail:
                    userDataValidationErrorDescription = "Email is missing";
                    break;
                case UserDataValidationResult::MissingFirstName:
                    userDataValidationErrorDescription = "First name is missing";
                    break;
                case UserDataValidationResult::InvalidEmail:
                    userDataValidationErrorDescription = "Email is invalid";
                    break;
            }
        }
        ImGui::EndPopup();
    }
    
    //ImGui::ShowTestWindow();
    
    ImGui::GetIO().MouseDrawCursor = true;
    
    world->setInputProcessingPaused(true);
    world->update(delta);
}

void EditorWelcomeState::pause() {
    
}

void EditorWelcomeState::resume() {
    
}
}
