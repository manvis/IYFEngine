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
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "core/Constants.hpp"
#include "tools/AssetCreatorWindow.hpp"
#include "tools/MaterialEditor.hpp"
#include "imgui.h"
#include "utilities/Regexes.hpp"
#include "utilities/ImGuiUtils.hpp"

namespace iyf::editor {

AssetCreatorWindow::AssetCreatorWindow(Engine* engine, std::string basePath) : AssetCreatorWindow(engine, basePath, AssetType::COUNT) {}

AssetCreatorWindow::AssetCreatorWindow(Engine* engine, std::string basePath, AssetType type) : engine(engine), basePath(basePath), directory(type == AssetType::COUNT), showZeroLengthInfo(false), type(type), family(MaterialFamily::Toon) {
    if (type == AssetType::MaterialTemplate) {
        windowName = "New Material Template";
        extension = con::MaterialTemplateFormatExtension();
    } else if (type == AssetType::COUNT) {
        windowName = "New Directory";
    } else {
        throw std::logic_error("The AssetType cannot be created by the engine");
    }
}

void AssetCreatorWindow::openPopup() {
    name[0] = '\0';
    
    ImGui::OpenPopup(windowName.c_str());
}

void AssetCreatorWindow::executeOperation() {
    fs::path finalPath = basePath;
    finalPath /= name;
    
    if (directory) {
        FileSystem* fs = engine->getFileSystem();
        
        fs->createDirectory(finalPath);
    } else {
        const std::string json = MaterialEditor::CreateNewGraph(family);
        
        finalPath += con::MaterialTemplateFormatExtension();
        
        File out(finalPath, File::OpenMode::Write);
        out.writeBytes(json.data(), json.size());
    }
}

bool AssetCreatorWindow::show() {
    bool result = false;
    
    if (ImGui::BeginPopupModal(windowName.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::InputText("Name", name, MaxNameLength);
        ImGui::Text("Final path: %s/%s%s", basePath.c_str(), name, extension.c_str());
        
        bool validName = true;
        const std::size_t nameLength = std::strlen(name);
        if (nameLength > 0) {
            validName &= std::regex_match(name, SystemRegexes().FunctionAndFileNameRegex);
            if (!validName) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "The name of the file contains non-ASCII characters\nor starts with a number.");
            }
        } else {
            if (showZeroLengthInfo) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "The name of the file can't be empty.");
            }
            
            validName = false;
        }
        
        constexpr int MaterialFamilyCount = static_cast<int>(MaterialFamily::COUNT);
        if (type == AssetType::MaterialTemplate) {
            std::vector<std::string> names;
            names.reserve(MaterialFamilyCount);
            
            for (int i = 0; i < MaterialFamilyCount; ++i) {
                names.emplace_back(con::GetMaterialFamilyDefinition(static_cast<MaterialFamily>(i)).getName());
            }
            
            util::DisplayFlagPicker<MaterialFamily, MaterialFamilyCount>("Family", family, MaterialFamily::COUNT, names);
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("OK")) {
            if (validName) {
                ImGui::CloseCurrentPopup();
                result = true;
                
                executeOperation();
            } else if (nameLength == 0) {
                showZeroLengthInfo = true;
            }
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
            result = true;
        }
        
        ImGui::EndPopup();
    }
    
    return result;
}

}
