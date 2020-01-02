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

#include "../../VERSION.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/aboutdialog.h>
#include <glibmm/i18n.h>
#include <gtkmm/filechooserdialog.h>
#include <iostream>
#include <regex>

namespace iyf::launcher {

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

inline std::string BuildDataFilePath() {
    char* temp = g_build_filename(g_get_user_data_dir(), "IYFEditor", "data.json", nullptr);
    std::string path = temp;
    g_free(temp);
    
    return path;
}

LauncherApp::LauncherApp() : Gtk::Application("com.iyfengine.iyflauncher") {}

Glib::RefPtr<LauncherApp> LauncherApp::Create() {
    return Glib::RefPtr<LauncherApp>(new LauncherApp());
}

void LauncherApp::on_activate() {
    try {
        LauncherAppWindow* window = createMainWindow();
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

void LauncherApp::onAddVersion() {
    Gtk::FileChooserDialog dialog(*mainWindow, _("Add engine install"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_OPEN);
    
    dialog.add_button(_("_Cancel"), GTK_RESPONSE_CANCEL);
    dialog.add_button(_("_Open"), GTK_RESPONSE_ACCEPT);
    
    auto filter = Gtk::FileFilter::create();
    filter->set_name(_("IYFEditor executable"));
    filter->add_pattern("IYFEditor*");
    dialog.add_filter(filter);
    
    const auto res = dialog.run();
    if (res == Gtk::RESPONSE_ACCEPT) {
        std::stringstream command;
        command << dialog.get_filename() << " --version";
        
        std::stringstream output;
        const int res = RunInPipe(command.str(), output);
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
        
        const EngineVersionInfo enVer(dialog.get_filename(), major, minor, patch, false);
        
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
        std::cout << dialog.get_filename() << "\n";
    }
}

void LauncherApp::onNewProject() {
    Gtk::FileChooserDialog dialog(*mainWindow, _("Create project"), Gtk::FileChooserAction::FILE_CHOOSER_ACTION_CREATE_FOLDER);
    
    dialog.add_button(_("_Cancel"), GTK_RESPONSE_CANCEL);
    dialog.add_button(_("_New"), GTK_RESPONSE_ACCEPT);

    const auto res = dialog.run();
    if (res == Gtk::RESPONSE_ACCEPT) {
        std::cout << dialog.get_filename() << "\n";
    }
}

void LauncherApp::showError(const std::string& text) {
    std::cerr << text << "\n";
}

void LauncherApp::onAbout() {
    Gtk::AboutDialog dialog;
    dialog.set_program_name(_("IYFEngine Launcher"));
    
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
