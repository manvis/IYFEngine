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

#ifndef IYF_LAUNCHER_APP_HPP
#define IYF_LAUNCHER_APP_HPP

#include <gtkmm/application.h>
#include <glibmm/regex.h>

namespace iyf::launcher {
class LauncherAppWindow;

class LauncherApp : public Gtk::Application {
public:
    static Glib::RefPtr<LauncherApp> Create();
    static int RunInPipe(const std::string& command, std::stringstream& ss);
    
    void saveDataFile(const std::string& data);
protected:
    LauncherApp();
    
    void on_activate() override;
    void on_startup() override;
private:
    LauncherAppWindow* createMainWindow();
    
    void onWindowHide(Gtk::Window* window);
    void onAbout();
    void onQuit();
    void onAddVersion();
    std::string openAddProjectDialog() const;
    void onAddProject();
    void onNewProject();
    void showError(const std::string& text);
    
    LauncherAppWindow* mainWindow;
    Glib::RefPtr<Gio::File> dataFile;
    Glib::RefPtr<Glib::Regex> validNameRegex;
    Glib::RefPtr<Glib::Regex> validLocaleRegex;
};
}

#endif // IYF_LAUNCHER_APP_HPP
