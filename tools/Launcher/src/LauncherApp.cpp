// The IYFEngine
//
// Copyright (C) 2015-2019, Manvydas Šliamka
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
#include "LauncherApp.hpp"
#include "LauncherAppWindow.hpp"
#include "ProgressDialog.hpp"

#include "../../VERSION.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/comboboxtext.h>
#include <glibmm/i18n.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <iostream>
#include <regex>

namespace iyf::launcher {
const char* css =
"progressbar trough, progressbar progress { min-height: 15px; }\n"
".errorborder { border-color: Red; }";
inline void CheckError(const char* name, void* object, const char* fileName = "new_project_dialog.glade") {
    if (!object) {
        std::stringstream ss;
        ss << "Failed to find a \"" << name << "\" object in " << fileName;
        throw std::runtime_error(ss.str());
    }
}

inline bool ValidateString(const Glib::ustring& str, Glib::RefPtr<Glib::Regex>& regex) {
    if (!str.validate())
    {
        return false;
    }

    return regex->match_all(str);
}

inline bool ParseInt(const std::string& segment, int& val) {
    char* end;
    long long temp = std::strtoll(segment.c_str(), &end, 10);
    
    if (segment == end || errno == ERANGE) {
        errno = 0;
        
        return false;
    }
    
    val = static_cast<int>(temp);
    return true;
}

inline void ToggleCreate(bool nameOK, bool companyOK, bool pathOK, bool localeOK, bool versionOK, Gtk::Button* button) {
    if (nameOK && companyOK && pathOK && localeOK && versionOK) {
        button->set_sensitive(true);
    } else {
        button->set_sensitive(false);
    }
}

inline std::string BuildDataFilePath() {
    char* temp = g_build_filename(g_get_user_data_dir(), "IYFEditor", "data.json", nullptr);
    std::string path = temp;
    g_free(temp);
    
    return path;
}

LauncherApp::LauncherApp() : Gtk::Application("com.iyfengine.iyflauncher") {
    validNameRegex = Glib::Regex::create("^[a-zA-Z0-9]\\w{0,127}$");
    validLocaleRegex = Glib::Regex::create("^[a-z]{2,3}_[A-Z]{2}$");
}

Glib::RefPtr<LauncherApp> LauncherApp::Create() {
    return Glib::RefPtr<LauncherApp>(new LauncherApp());
}

void LauncherApp::on_activate() {
    try {
        LauncherAppWindow* window = createMainWindow();

        Glib::RefPtr<Gdk::Screen> screen = Gdk::Screen::get_default();
        if (screen) {
            Glib::RefPtr<Gtk::StyleContext> context = window->get_style_context();
            Glib::RefPtr<Gtk::CssProvider> provider = Gtk::CssProvider::create();
            if (provider->load_from_data(css)) {
                context->add_provider_for_screen(screen, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            }
        }

        window->present();
        
        mainWindow = window;
        
        dataFile = Gio::File::create_for_path(BuildDataFilePath());
        int r = g_mkdir_with_parents(dataFile->get_parent()->get_path().c_str(), 0775);
        if (r == -1) {
            throw std::runtime_error("Failed to create data file path");
        }
        
        char* data;
        gsize length;
        dataFile->load_contents(data, length);
        
        mainWindow->deserializeData(data, length);
        
        g_free(data);
        
        mainWindow->rebuildLists();
        //dataFile->read()->read_all();
    } catch (const Glib::Error& e) {
        std::cerr << "Glib::Error in LauncherApp::on_activate: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "std::exception in LauncherApp::on_activate: " << e.what() << std::endl;
    }
}

void LauncherApp::on_startup() {
    Gtk::Application::on_startup();
    
    add_action("about", sigc::mem_fun(*this, &LauncherApp::onAbout));
    add_action("quit", sigc::mem_fun(*this, &LauncherApp::onQuit));
    add_action("add_version", sigc::mem_fun(*this, &LauncherApp::onAddVersion));
    add_action("add_project", sigc::mem_fun(*this, &LauncherApp::onAddProject));
    add_action("new_project", sigc::mem_fun(*this, &LauncherApp::onNewProject));
    
    set_accel_for_action("app.quit", "<Ctrl>Q");
}

LauncherAppWindow* LauncherApp::createMainWindow() {
    LauncherAppWindow* window = LauncherAppWindow::Create();
    
    add_window(*window);
    window->signal_hide().connect(sigc::bind<Gtk::Window*>(sigc::mem_fun(*this, &LauncherApp::onWindowHide), window));
    
    return window;
}

void LauncherApp::onWindowHide(Gtk::Window* window) {
    delete window;
}

std::string LauncherApp::openAddProjectDialog() const {
    Gtk::FileChooserDialog dialog(*mainWindow, _("Add engine install"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    
    dialog.add_button(_("_Cancel"), GTK_RESPONSE_CANCEL);
    dialog.add_button(_("_Open"), GTK_RESPONSE_ACCEPT);
    
    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("IYFEditor executable"));
    filter->add_pattern("IYFEditor*");
    dialog.add_filter(filter);
    
    std::string path;
    const auto res = dialog.run();
    if (res == Gtk::RESPONSE_ACCEPT) {
        path = dialog.get_filename();
    }

    return path;
}

void LauncherApp::onAddVersion() {
    const std::string openResult = openAddProjectDialog();
    if (!openResult.empty()) {
        EngineVersionInfo enVer;
        auto command = [this, openResult, &enVer]() {
            std::string command = openResult;
            command.append(" --version");
            
            std::stringstream output;
            const int res = RunInPipe(command, output);
            if (res != 0) {
                showError(_("Failed to read version data from the executable. Did you choose a valid IYFEngine install?"));
                return;
            }
            
            std::regex regex("(\\d+)\\.(\\d+)\\.(\\d+)");
            std::smatch match;
            const std::string outputStr = output.str();
            if (!std::regex_search(outputStr, match, regex) || match.size() != 4) {
                showError(_("Version data retrieved from the executable is formatted incorrectly. Did you choose a valid IYFEngine install?"));
                return;
            }
            
            int major = 0;
            int minor = 0;
            int patch = 0;
            if (!ParseInt(match[1], major) || !ParseInt(match[2], minor) || !ParseInt(match[3], patch)) {
                showError(_("Version data retrieved from the executable is impossible to parse. Did you choose a valid IYFEngine install?"));
                return;
            }
            
            enVer = EngineVersionInfo(openResult, major, minor, patch, false);
        };

        ProgressDialog progDialog(*mainWindow, std::move(command));
        progDialog.run();
        
        // NOT THREAD SAFE - must be done here and not in ProgressDialog thread
        const std::string newData = mainWindow->addVersion(enVer);
        saveDataFile(newData);
    }
}

void LauncherApp::saveDataFile(const std::string& data) {
    std::string tag;
    dataFile->replace_contents(data, std::string(), tag, true, Gio::FILE_CREATE_REPLACE_DESTINATION);
    
    std::cout << "Saved changes to " << dataFile->get_path() << "\n";
}

void LauncherApp::onAddProject() {
    Gtk::FileChooserDialog dialog(*mainWindow, _("Add project"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    
    dialog.add_button(_("_Cancel"), GTK_RESPONSE_CANCEL);
    dialog.add_button(_("_Open"), GTK_RESPONSE_ACCEPT);
    
    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("IYFEngine project"));
    filter->add_pattern("*.iyfp");
    dialog.add_filter(filter);
    
    const auto res = dialog.run();
    if (res == Gtk::RESPONSE_ACCEPT) {
        ProjectInfo info(dialog.get_filename());

        const std::string newData = mainWindow->addProject(info);
        saveDataFile(newData);
    }
}

void LauncherApp::onNewProject() {
    auto builder = Gtk::Builder::create_from_resource("/com/iyfengine/iyflauncher/new_project_dialog.glade");
    
    Gtk::Dialog* createProjectDialog = nullptr;

    bool projectNameOK = false;
    bool companyNameOK = false;
    bool pathOK = false;
    bool localeOK = false;
    bool versionOK = false;

    builder->get_widget("create_project_dialog", createProjectDialog);
    CheckError("create_project_dialog", createProjectDialog);

    createProjectDialog->signal_hide().connect([createProjectDialog](){
        createProjectDialog->response(0);
        std::cout << "Hide\n";
    });

    createProjectDialog->set_transient_for(*mainWindow);
    createProjectDialog->set_attached_to(*mainWindow);

    Gtk::Button* cancel = nullptr;
    builder->get_widget("cancel_new_project", cancel);
    CheckError("cancel_new_project", cancel);
    cancel->signal_clicked().connect([createProjectDialog](){
        createProjectDialog->response(Gtk::RESPONSE_CANCEL);
    });

    Gtk::Button* create = nullptr;
    builder->get_widget("create_new_project", create);
    CheckError("create_new_project", create);
    create->signal_clicked().connect([createProjectDialog](){
        createProjectDialog->response(Gtk::RESPONSE_ACCEPT);
    });
    ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);

    Gtk::Entry* projectName = nullptr;
    builder->get_widget("project_name_input", projectName);
    CheckError("project_name_input", projectName);
    projectName->signal_changed().connect([this, projectName, &projectNameOK, &companyNameOK, &pathOK, &localeOK, &versionOK, &create]() {
        const Glib::ustring str = projectName->get_text();
        projectNameOK = ValidateString(str, validNameRegex);
        if (projectNameOK) {
            projectName->get_style_context()->remove_class("errorborder");
        } else {
            projectName->get_style_context()->add_class("errorborder");
        }

        ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);
    });

    Gtk::Entry* companyName = nullptr;
    builder->get_widget("company_name_input", companyName);
    CheckError("company_name_input", companyName);
    companyName->signal_changed().connect([this, companyName, &projectNameOK, &companyNameOK, &pathOK, &localeOK, &versionOK, &create]() {
        const Glib::ustring str = companyName->get_text();
        companyNameOK = ValidateString(str, validNameRegex);

        if (companyNameOK) {
            companyName->get_style_context()->remove_class("errorborder");
        } else {
            companyName->get_style_context()->add_class("errorborder");
        }

        ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);
    });

    Gtk::Entry* projectPath = nullptr;
    builder->get_widget("project_path_input", projectPath);
    CheckError("project_path_input", projectPath);

    Gtk::Button* chooseProjectPath = nullptr;
    builder->get_widget("choose_project_path", chooseProjectPath);
    CheckError("choose_project_path", chooseProjectPath);
    chooseProjectPath->signal_clicked().connect([createProjectDialog, projectPath, &projectNameOK, &companyNameOK, &pathOK, &localeOK, &versionOK, &create](){
        Gtk::FileChooserDialog pathPicker(*createProjectDialog, _("Choose New Project Path"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_CREATE_FOLDER);
    
        pathPicker.add_button(_("_Cancel"), GTK_RESPONSE_CANCEL);
        pathPicker.add_button(_("_Choose"), GTK_RESPONSE_ACCEPT);
        
        int response = pathPicker.run();
        if (response == Gtk::RESPONSE_ACCEPT) {
            projectPath->set_text(pathPicker.get_filename());
            pathOK = true;
        } else {
            projectPath->set_text("");
            pathOK = false;
        }

        if (pathOK) {
            projectPath->get_style_context()->remove_class("errorborder");
        } else {
            projectPath->get_style_context()->add_class("errorborder");
        }

        ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);
    });

    Gtk::Entry* baseLocale = nullptr;
    builder->get_widget("base_locale_input", baseLocale);
    CheckError("base_locale_input", baseLocale);
    baseLocale->signal_changed().connect([this, baseLocale, &projectNameOK, &companyNameOK, &pathOK, &localeOK, &versionOK, &create]() {
        const Glib::ustring str = baseLocale->get_text();
        localeOK = ValidateString(str, validLocaleRegex);

        if (localeOK) {
            baseLocale->get_style_context()->remove_class("errorborder");
        } else {
            baseLocale->get_style_context()->add_class("errorborder");
        }

        ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);
    });

    Gtk::ComboBoxText* engineVersion = nullptr;
    builder->get_widget("engine_version_combo", engineVersion);
    CheckError("engine_version_combo", engineVersion);

    for (const auto& version : mainWindow->getVersions()) {
        const auto& info = version.second;

        std::stringstream ss;
        ss << info.major << "." << info.minor << "." << info.patch << "; " << info.path;
        std::string line = ss.str();
        engineVersion->append(line);
    }
    engineVersion->signal_changed().connect([this, engineVersion, &projectNameOK, &companyNameOK, &pathOK, &localeOK, &versionOK, &create](){
        const int activeID = engineVersion->get_active_row_number();
        versionOK = (activeID >= 0);


        std::string enginePath = engineVersion->get_active_text();
        enginePath = enginePath.substr(enginePath.find("; ") + 2);
        //std::cout << enginePath << "\n";

        ToggleCreate(projectNameOK, companyNameOK, pathOK, localeOK, versionOK, create);
    });

    int result = createProjectDialog->run();

    if (result == Gtk::RESPONSE_ACCEPT)
    {
        std::string enginePath = engineVersion->get_active_text();
        enginePath = enginePath.substr(enginePath.find("; ") + 2);

        const std::string path = std::string(projectPath->get_text());
        const std::string name = projectName->get_text();
        const std::string company = companyName->get_text();
        const std::string locale = baseLocale->get_text();

        delete createProjectDialog;

        std::string errorText;
        ProjectInfo info;
        auto command = [this, &info, &errorText, &enginePath, &path, &name, &company, &locale]() {
            std::stringstream ss;
            ss << enginePath <<  " --new-project \"" << path << "\" \"" << name << "\" \"" << company << "\" \"" << locale << "\"";

            const std::string pipeCommand = ss.str();
            std::stringstream output;
            const int res = RunInPipe(pipeCommand, output);
            if (res != 0) {
                errorText = output.str();
                if (errorText.empty()) {
                    errorText = "Failed to create the project. Did you pick invalid settings?";
                }
                showError(_("Failed to create the project. Did you pick invalid settings?"));
                return;
            }

            info.path = path;
        };

        auto progDialog = std::make_unique<ProgressDialog>(*mainWindow, std::move(command));
        progDialog->run();
        progDialog = nullptr;

        if (errorText.empty()) {
            // NOT THREAD SAFE - must be done here and not in ProgressDialog thread
            const std::string newData = mainWindow->addProject(info);
            saveDataFile(newData);
        } else {
            errorText.append("\n\nYou may wish to check the engine logs for more info");

            Gtk::MessageDialog errorDialog(errorText, true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
            errorDialog.run();
        }
    } else {
        delete createProjectDialog;
    }
}

void LauncherApp::showError(const std::string& text) {
    std::cerr << text << "\n";
}

void LauncherApp::onAbout() {
    Gtk::AboutDialog dialog;
    dialog.set_program_name(_("IYFEngine Launcher"));
    dialog.set_modal(true);

    dialog.set_transient_for(*mainWindow);
    dialog.set_attached_to(*mainWindow);
    
    const auto version = iyf::con::LauncherVersion;
    std::stringstream ss;
    ss << version.getMajor() << "." << version.getMinor() << "." << version.getPatch();
    dialog.set_version(ss.str());
    
    dialog.set_copyright("Manvydas Šliamka");
    
    dialog.set_wrap_license(true);
    dialog.set_license("BSD-3-Clause\n\nPlease visit https://github.com/manvis/IYFEngine/blob/master/LICENSE.md to obtain the complete license text");
    
    // TODO set website and icon. Docs: https://developer.gnome.org/gtkmm/stable/classGtk_1_1AboutDialog.html
    //dialog.set_website("");
    //dialog.set_website_label(_(""));
    
    dialog.run();
}

void LauncherApp::onQuit() {
    auto windows = get_windows();
    for (auto window : windows)
        window->hide();
    
    quit();
}

int LauncherApp::RunInPipe(const std::string& command, std::stringstream& ss) {
#ifdef WIN32
#error "Not yet implemented"
#else
    constexpr const int bufferSize = 512;
    char buffer[bufferSize];
    
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        std::stringstream ssErr;
        ssErr << "Command " << command << " failed: " << strerror(errno);
        std::cerr << ssErr.str() << "\n";
        
        return -1;
    }
    
    while (fgets(buffer, bufferSize, pipe) != nullptr) {
        ss << buffer;
    }
    
    int returnResult = pclose(pipe);
    if (returnResult == -1) {
        std::stringstream ssErr;
        ssErr << "Failed to clean up pipe after command " << command << strerror(errno);
        std::cerr << ssErr.str() << "\n";
        
        return -1;
    } else {
        const int finalResult = WEXITSTATUS(returnResult);
        return finalResult;
    }
#endif
}

}
