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

#ifndef IYF_LAUNCHER_APP_WINDOW_HPP
#define IYF_LAUNCHER_APP_WINDOW_HPP

#include <gtkmm/applicationwindow.h>
#include <map>
#include <string>
#include <mutex>

#include "LauncherData.hpp"

namespace Gtk {
class Builder;
class Button;
class StackSwitcher;
class ListBox;
class PopoverMenu;
}

namespace iyf::launcher {

class LauncherAppWindow : public Gtk::ApplicationWindow {
public:
    LauncherAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
    
    void bindMenuModel(Glib::RefPtr<Gio::MenuModel> model);
    void bindAddMenuModel(Glib::RefPtr<Gio::MenuModel> model);
    
    static LauncherAppWindow* Create();
    void rebuildLists();
    
    /// \remark NOT thread safe
    std::string addVersion(EngineVersionInfo version);

    /// \remark NOT thread safe
    std::string addProject(ProjectInfo projectInfo);

    /// \remark NOT thread safe
    const std::map<std::string, EngineVersionInfo>& getVersions() const;

    /// \remark NOT thread safe
    const std::map<std::string, ProjectInfo>& getProjects() const;
    
    /// \remark NOT thread safe
    void deserializeData(char* data, std::size_t length);

    /// \remark NOT thread safe
    std::string serializeData();
private:
    void onAddVersion();
    void onAddProject();
    void onNewProject();
    void onVersionDeleteClicked(Glib::ustring data);
    void onProjectDeleteClicked(Glib::ustring data);
    void onProjectOpened(Glib::ustring data, std::uint32_t major, std::uint32_t minor, std::uint32_t patch);
    void clearLists();
    
    Glib::RefPtr<Gtk::Builder> builder;
    Gtk::StackSwitcher* stackSwitcher;
    Gtk::ListBox* versionList;
    Gtk::ListBox* projectList;
    Gtk::PopoverMenu* menuPopover;
    Gtk::PopoverMenu* addMenuPopover;
    
    std::map<std::string, EngineVersionInfo> versions;
    std::map<std::string, ProjectInfo> projects;

    static constexpr const char* Filename = "launcher.glade";
};

}


#endif // IYF_LAUNCHER_APP_WINDOW_HPP
