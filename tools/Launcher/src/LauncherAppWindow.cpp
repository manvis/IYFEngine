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

#include "LauncherAppWindow.hpp"
#include "LauncherApp.hpp"

#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/stackswitcher.h>
#include <gtkmm/image.h>
#include <gtkmm/listbox.h>
#include <gtkmm/listboxrow.h>
#include <gtkmm/label.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/filechooserdialog.h>

#include <sstream>
#include <iostream>
#include <glib/gi18n.h>

namespace iyf::launcher {
inline void CheckError(const char* name, void* object, const char* fileName = "launcher.glade") {
    if (!object) {
        std::stringstream ss;
        ss << "Failed to find a \"" << name << "\" object in " << fileName;
        throw std::runtime_error(ss.str());
    }
}

inline void SortVersionVector(std::vector<EngineVersionInfo>& vec) {
    std::sort(vec.begin(), vec.end(), [](const EngineVersionInfo& a, const EngineVersionInfo& b) {
        if ((a.major > b.major) ||
            (a.major == b.major && a.minor > b.minor) ||
            (a.major == b.major && a.minor == b.minor && a.patch > b.patch)) {
            return true;
            }
            
            return false;
    });
}

inline void SortProjectVector(std::vector<ProjectInfo>& vec) {
    std::sort(vec.begin(), vec.end(), [](const ProjectInfo& a, const ProjectInfo& b) {
        return a.path > b.path;
    });
}

LauncherAppWindow::LauncherAppWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
: Gtk::ApplicationWindow(cobject), builder(builder), stackSwitcher(nullptr), versionList(nullptr), projectList(nullptr) {
    builder->get_widget("stack_switcher", stackSwitcher);
    CheckError("stack_switcher", stackSwitcher);
    // addButton->signal_clicked().connect(sigc::mem_fun(*this, &LauncherAppWindow::onAddNew));
    
    builder->get_widget("version_list", versionList);
    CheckError("version_list", versionList);
    
    builder->get_widget("project_list", projectList);
    CheckError("project_list", projectList);
    
    builder->get_widget("main_menu", menuPopover);
    CheckError("main_menu", menuPopover);
    
    builder->get_widget("add_menu", addMenuPopover);
    CheckError("add_menu", addMenuPopover);
}

void LauncherAppWindow::bindMenuModel(Glib::RefPtr<Gio::MenuModel> model) {
    menuPopover->bind_model(model);
}


void LauncherAppWindow::bindAddMenuModel(Glib::RefPtr<Gio::MenuModel> model) {
    addMenuPopover->bind_model(model);
}

void LauncherAppWindow::rebuildLists() {
    clearLists();
    
    for (const auto& v : versions) {
        auto* hBox = Gtk::make_managed<Gtk::Box>();
        hBox->set_homogeneous(false);
        hBox->set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
        hBox->set_margin_start(8);
        hBox->set_margin_end(8);
        hBox->set_margin_top(8);
        hBox->set_margin_bottom(8);
        
        auto* box = Gtk::make_managed<Gtk::Box>();
        box->set_homogeneous(true);
        box->set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
        
        auto* path = Gtk::make_managed<Gtk::Label>(v.second.path);
        path->set_line_wrap(false);
        //path->set_line_wrap_mode(Pango::WRAP_WORD);
        path->set_alignment(Gtk::ALIGN_START);
        path->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
        
        std::stringstream ss;
        ss << _("Engine Version: ") << v.second.major << "." << v.second.minor << "." << v.second.patch;
        auto* version = Gtk::make_managed<Gtk::Label>(ss.str());
        //version->set_line_wrap_mode(Pango::WRAP_WORD);
        version->set_line_wrap(false);
        version->set_alignment(Gtk::ALIGN_START);
        version->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
        version->set_margin_bottom(4);
        
        auto* deleteButton = Gtk::make_managed<Gtk::Button>();
        deleteButton->set_image_from_icon_name("edit-delete-symbolic");
        deleteButton->signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &LauncherAppWindow::onVersionDeleteClicked), v.second.path));
        
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        
        box->pack_start(*version);
        box->pack_start(*path);
        hBox->pack_start(*box, true, true, 0);
        hBox->pack_end(*deleteButton, Gtk::PackOptions::PACK_SHRINK, false, 0);
        row->add(*hBox);
        versionList->insert(*row, -1);
    }
    
    //versionList->show();
    versionList->invalidate_sort();
    versionList->show_all();
    
    for (const auto& p : projects) {
        auto filePath = Gio::File::create_for_path(p.second.path);
        
        auto* hBox = Gtk::make_managed<Gtk::Box>();
        hBox->set_homogeneous(false);
        hBox->set_orientation(Gtk::Orientation::ORIENTATION_HORIZONTAL);
        hBox->set_margin_start(8);
        hBox->set_margin_end(8);
        hBox->set_margin_top(8);
        hBox->set_margin_bottom(8);
        
        auto* box = Gtk::make_managed<Gtk::Box>();
        box->set_homogeneous(true);
        box->set_orientation(Gtk::Orientation::ORIENTATION_VERTICAL);
        
        auto* deleteButton = Gtk::make_managed<Gtk::Button>();
        deleteButton->set_image_from_icon_name("edit-delete-symbolic");
        deleteButton->signal_clicked().connect(sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &LauncherAppWindow::onProjectDeleteClicked), p.second.path));
        
        std::string baseName = filePath->get_basename();
        const std::size_t dotLocation = baseName.find_last_of(".");
        if (dotLocation != std::string::npos) {
            baseName = baseName.substr(0, dotLocation);
        }
        auto* name = Gtk::make_managed<Gtk::Label>(baseName);
        name->set_line_wrap(false);
        name->set_alignment(Gtk::ALIGN_START);
        name->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
        name->set_margin_bottom(4);
        
        auto* engineVersion = Gtk::make_managed<Gtk::Label>("Engine Version 0.0.0");
        engineVersion->set_line_wrap(false);
        engineVersion->set_alignment(Gtk::ALIGN_START);
        engineVersion->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
        
        auto* path = Gtk::make_managed<Gtk::Label>(filePath->get_path());
        path->set_line_wrap(false);
        path->set_alignment(Gtk::ALIGN_START);
        path->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
        
        auto* row = Gtk::make_managed<Gtk::ListBoxRow>();
        
        box->pack_start(*name);
        box->pack_start(*engineVersion);
        box->pack_start(*path);

        hBox->pack_start(*box, true, true, 0);
        hBox->pack_end(*deleteButton, Gtk::PackOptions::PACK_SHRINK, false, 0);

        row->add(*hBox);
        projectList->insert(*row, -1);
    }
    
    projectList->invalidate_sort();
    projectList->show_all();
}

void LauncherAppWindow::onVersionDeleteClicked(Glib::ustring data) {
    versions.erase(data);
    rebuildLists();
    
    auto launcherApp = Glib::RefPtr<LauncherApp>::cast_dynamic(get_application());
    launcherApp->saveDataFile(serializeData());
}

void LauncherAppWindow::onProjectDeleteClicked(Glib::ustring data) {
    projects.erase(data);
    rebuildLists();
    
    auto launcherApp = Glib::RefPtr<LauncherApp>::cast_dynamic(get_application());
    launcherApp->saveDataFile(serializeData());
}

void LauncherAppWindow::clearLists() {
    for (Gtk::Widget* child : versionList->get_children()) {
        versionList->remove(*child);
    }
    
    for (Gtk::Widget* child : projectList->get_children()) {
        projectList->remove(*child);
    }
}

LauncherAppWindow* LauncherAppWindow::Create() {
    auto builder = Gtk::Builder::create_from_resource("/com/iyfengine/iyflauncher/launcher.glade");
    
    LauncherAppWindow* window;
    
    builder->get_widget_derived("main_window", window);
    CheckError("main_window", window);
    
    auto menuBuilder = Gtk::Builder::create_from_resource("/com/iyfengine/iyflauncher/menu.ui");
    auto menuObject = menuBuilder->get_object("launchermenu");
    auto launcherMenu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(menuObject);
    
    if (!launcherMenu) {
        throw std::runtime_error("Failed to find \"launchermenu\" in menu.ui");
    }
    
    window->bindMenuModel(launcherMenu);
    
    auto addMenuBuilder = Gtk::Builder::create_from_resource("/com/iyfengine/iyflauncher/add_menu.ui");
    auto addMenuObject = addMenuBuilder->get_object("addmenu");
    auto addMenu = Glib::RefPtr<Gio::MenuModel>::cast_dynamic(addMenuObject);
    
    if (!addMenu) {
        throw std::runtime_error("Failed to find \"addmenu\" in add_menu.ui");
    }
    
    window->bindAddMenuModel(addMenu);
    
    return window;
}

void LauncherAppWindow::deserializeData(char* data, std::size_t length) {
    rapidjson::Document doc;
    doc.Parse(data, length);
    
    if (doc.HasParseError()) {
        std::cerr << "Failed to parse the data file. Error code: " << doc.GetParseError() <<"\n";
        return;
    }
    
    std::map<std::string, EngineVersionInfo> tempVersions;
    std::map<std::string, ProjectInfo> tempProjects;
    
    if (!doc.HasMember("versions") || !doc.HasMember("projects")) {
        std::cerr << "Failed to parse the data file. At least one required data array is missing\n";
        return;
    }
    
    if (!doc["versions"].IsArray() || !doc["projects"].IsArray()) {
        std::cerr << "Failed to parse the data file. At least one data array is of an invalid type\n";
        return;
    }
    
    for (const auto& v : doc["versions"].GetArray()) {
        EngineVersionInfo info;
        if (!info.deserialize(v)) {
            std::cerr << "Failed to deserialize EngineVersionInfo\n";
            return;
        }
        
        tempVersions.emplace(std::make_pair(info.path, info));
    }
    
    for (const auto& v : doc["projects"].GetArray()) {
        ProjectInfo info;
        if (!info.deserialize(v)) {
            std::cerr << "Failed to deserialize ProjectInfo\n";
            return;
        }
        
        tempProjects.emplace(std::make_pair(info.path, info));
    }
    
    versions = tempVersions;
    projects = tempProjects;
}

std::string LauncherAppWindow::serializeData() {
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> pw(sb);
    
    pw.SetIndent('\t', 1);
    
    pw.StartObject();
    
    pw.Key("versions");
    pw.StartArray();
    for (const auto& version : versions) {
        version.second.serialize(pw);
    }
    pw.EndArray();
    
    pw.Key("projects");
    pw.StartArray();
    for (const auto& project : projects) {
        project.second.serialize(pw);
    }
    pw.EndArray();
    
    pw.EndObject();
    
    const char* jsonString = sb.GetString();
    const std::size_t jsonByteCount = sb.GetSize();
    
    const std::string data(jsonString, jsonByteCount);
    return data;
}

std::string LauncherAppWindow::addVersion(EngineVersionInfo version) {
    versions.insert_or_assign(version.path, version);
    
    rebuildLists();
    
    return serializeData();
}

std::string LauncherAppWindow::addProject(ProjectInfo projectInfo) {
    projects.insert_or_assign(projectInfo.path, projectInfo);
    
    rebuildLists();
    
    return serializeData();
}

const std::map<std::string, EngineVersionInfo>& LauncherAppWindow::getVersions() const {
    return versions;
}

const std::map<std::string, ProjectInfo>& LauncherAppWindow::getProjects() const {
    return projects;
}

bool operator==(const EngineVersionInfo& a, const EngineVersionInfo& b) {
    return (a.path == b.path) && (a.major == b.major) && (a.minor == b.minor) && (a.patch == b.patch);
}

bool operator==(const ProjectInfo& a, const ProjectInfo& b) {
    return (a.path == b.path);
}

void EngineVersionInfo::serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& wr) const {
    wr.StartObject();
    
    wr.Key("path");
    wr.String(path);
    
    wr.Key("major");
    wr.Uint(major);
    
    wr.Key("minor");
    wr.Uint(minor);
    
    wr.Key("patch");
    wr.Uint(patch);
    
    wr.Key("managedByLanucher");
    wr.Bool(managedByLanucher);
    
    wr.EndObject();
}

bool EngineVersionInfo::deserialize(const rapidjson::Value& doc) {
    if (!doc.HasMember("path") || !doc.HasMember("major") || !doc.HasMember("minor") || !doc.HasMember("patch") || !doc.HasMember("managedByLanucher")) {
        std::cerr << "Required EngineVersionInfo member(s) missing\n";
        return false;
    }
    
    if (!doc["path"].IsString() || !doc["major"].IsUint() || !doc["minor"].IsUint() || !doc["patch"].IsUint() || !doc["managedByLanucher"].IsBool()) {
        std::cerr << "EngineVersionInfo member(s) are of incorrect type missing\n";
        return false;
    }
    
    path = doc["path"].GetString();
    major = doc["major"].GetUint();
    minor = doc["minor"].GetUint();
    patch = doc["patch"].GetUint();
    managedByLanucher = doc["managedByLanucher"].GetBool();
    
    return true;
}

void ProjectInfo::serialize(rapidjson::PrettyWriter<rapidjson::StringBuffer>& wr) const {
    wr.StartObject();
    
    wr.Key("path");
    wr.String(path);
    
    wr.EndObject();
}

bool ProjectInfo::deserialize(const rapidjson::Value& doc) {
    if (!doc.HasMember("path")) {
        std::cerr << "Required ProjectInfo member(s) missing\n";
        return false;
    }
    
    if (!doc["path"].IsString()) {
        std::cerr << "ProjectInfo member(s) are of incorrect type missing\n";
        return false;
    }
    
    path = doc["path"].GetString();
    
    return true;
}

}
