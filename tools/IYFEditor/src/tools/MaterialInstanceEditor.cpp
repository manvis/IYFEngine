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


#include "tools/MaterialInstanceEditor.hpp"

#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "assets/AssetManager.hpp"
#include "assets/metadata/MaterialInstanceMetadata.hpp"
#include "assets/metadata/MaterialTemplateMetadata.hpp"
#include "graphics/materials/MaterialInstanceDefinition.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "utilities/ImGuiUtils.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

namespace iyf::editor {
inline bool PairVectorGetter(void* in, int idx, const char** out_text) {
    std::vector<std::pair<MaterialRenderMode, std::string>>* names = static_cast<std::vector<std::pair<MaterialRenderMode, std::string>>*>(in);
    
     // Checking if we're in bounds
    if (idx < 0 || idx >= static_cast<int>(names->size())) {
        return false;
    }

    *out_text = (*names)[idx].second.c_str();
    return true;
}

const char* MaterialInstanceEditorLocNamespace = "material_instances";

MaterialInstanceEditor::MaterialInstanceEditor(Engine* engine) : engine(engine) {
    if (engine == nullptr) {
        throw std::runtime_error("Engine can't be nullptr");
    }
    
    assetManager = engine->getAssetManager();
    
    constexpr std::size_t renderModeCount = static_cast<std::size_t>(MaterialRenderMode::COUNT);
    materialRenderModeNames.reserve(renderModeCount);
    
    for (std::size_t i = 0; i < renderModeCount; ++i) {
        const MaterialRenderMode renderMode = static_cast<MaterialRenderMode>(i);
        materialRenderModeNames.emplace_back(std::make_pair(renderMode, LOC_SYS(con::GetMaterialRenderModeLocalizationHandle(renderMode))));
    }
}

void MaterialInstanceEditor::show(bool* showing) {
    if (instanceDefinition == nullptr) {
        *showing = false;
    }
//     std::optional<Metadata> metadata = assetManager->getMetadataCopy(fileNameHash);
//     if (!metadata) {
//         LOG_V("Failed to fetch metadata. Material Instance Definition probably got erased.");
//         (*showing) = false;
//         return;
//     }
//     
//     if (metadata->getAssetType() != AssetType::MaterialInstance) {
//         LOG_V("The asset is not a Material Instance Definition. It probably got replaced.");
//     }
/*    
    MaterialInstanceMetadata instanceMetadata = (*metadata).get<MaterialInstanceMetadata>();*/
    const std::string windowName = LOC_SYS(LH("material_instance_editor", MaterialInstanceEditorLocNamespace), filePath.generic_string());
    if (ImGui::Begin(windowName.c_str(), showing)) {
        
        if (ImGui::Button("Save")) {
            std::vector<std::pair<StringHash, glm::vec4>> variableVector;
            variableVector.reserve(instanceVariables.size());
            for (const auto& v : instanceVariables) {
                variableVector.emplace_back(v);
            }
            
            std::vector<std::pair<StringHash, StringHash>> textureVector;
            variableVector.reserve(instanceTextures.size());
            for (const auto& t : instanceTextures) {
                textureVector.emplace_back(t);
            }
            
            instanceDefinition->setMaterialTemplateDefinition(materialTemplateAsset, variableVector, textureVector);
            instanceDefinition->setRenderMode(materialRenderMode);
            
            File output(filePath, File::OpenMode::Write);
            const std::string result = instanceDefinition->getJSONString();
            output.writeBytes(result.data(), result.size());
        }
        
        ImGui::SameLine();
        
        const auto templateMetadata = assetManager->getMetadataCopy(materialTemplateAsset);
        const float dropTargetWidth = ImGui::GetContentRegionAvailWidth();
        if (!templateMetadata) {
            util::AssetDragDropTarget("Undefined Material Template", AssetType::MaterialTemplate, std::bind(&MaterialInstanceEditor::changeAsset, this, std::placeholders::_1), ImVec2(dropTargetWidth, 0));
        } else {
            const auto& value = templateMetadata.value();
            const MaterialTemplateMetadata& metadata = value.get<MaterialTemplateMetadata>();
            const std::string text = fmt::format("Material template: {}", metadata.getSourceAssetPath().generic_string());
            util::AssetDragDropTarget(text.c_str(), AssetType::MaterialTemplate, std::bind(&MaterialInstanceEditor::changeAsset, this, std::placeholders::_1), ImVec2(dropTargetWidth, 0));
            
            int currentItem = static_cast<int>(materialRenderMode);
            
            if (ImGui::Combo("Render Mode", &currentItem, PairVectorGetter, &materialRenderModeNames, materialRenderModeNames.size())) {
                materialRenderMode = static_cast<MaterialRenderMode>(currentItem);
            }
            
            // TODO implement and listen to callbacks instead of doing all this work every frame.
            std::vector<MaterialInputTexture> textures = metadata.getRequiredTextures();
            std::sort(textures.begin(), textures.end(), [](const MaterialInputTexture& a, const MaterialInputTexture& b) {
                return a.getName() < b.getName();
            });
            
            ImGui::Text("Textures");
            for (const MaterialInputTexture& t : textures) {
                const float imageHeight = 64;
                const float midpoint = (imageHeight - ImGui::GetFont()->FontSize) / 2;
                
                const std::string& name = t.getName();
                const StringHash nameHash = t.getNameHash();
                auto result = instanceTextures.find(nameHash);
                
                const bool instanceOverrideExists = (result != instanceTextures.end());
                const StringHash textureToUse = instanceOverrideExists ? result->second : t.getDefaultTexture();
                
                util::AssetDragDropImageTarget(textureToUse, ImVec2(imageHeight, imageHeight), [this, &nameHash, &instanceOverrideExists, &result](const util::DragDropAssetPayload& payload) {
                    if (instanceOverrideExists) {
                        result->second = payload.getNameHash();
                    } else {
                        instanceTextures.emplace(nameHash, payload.getNameHash());
                    }
                });
                
                ImGui::SameLine();
                
                const ImVec2 cursorPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + midpoint));
                ImGui::TextUnformatted(name.data(), name.data() + name.length());
                
                ImGui::SameLine();
                
                if (instanceOverrideExists) {
                    ImGui::Text("(o)");
                    util::ShowTooltip("Overridden by the instance definition");
                } else {
                    ImGui::Text("(d)");
                    util::ShowTooltip("Default value from the template definition");
                }
                
                const ImVec2 newLinePos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(newLinePos.x, newLinePos.y - midpoint));
            }
            
            std::vector<MaterialInputVariable> variables = metadata.getRequiredVariables();
            std::sort(variables.begin(), variables.end(), [](const MaterialInputVariable& a, const MaterialInputVariable& b) {
                return a.getName() < b.getName();
            });
            
            ImGui::Text("Variables");
            for (const MaterialInputVariable& v : variables) {
                const std::string& name = v.getName();
                const StringHash nameHash = v.getNameHash();
                auto result = instanceVariables.find(nameHash);
                
                const bool instanceOverrideExists = (result != instanceVariables.end());
                glm::vec4 values = instanceOverrideExists ? result->second : v.getDefaultValue();
                
                if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(values))) {
                    if (instanceOverrideExists) {
                        result->second = values;
                    } else {
                        instanceVariables.emplace(nameHash, values);
                    }
                }
                ImGui::SameLine();
                
                if (instanceOverrideExists) {
                    ImGui::Text("(o)");
                    util::ShowTooltip("Overridden by the instance definition");
                } else {
                    ImGui::Text("(d)");
                    util::ShowTooltip("Default value from the template definition");
                }
            }
        }
        
        util::ShowTooltip("Drop Material Template asset here");
        
        ImGui::End();
    }
}
void MaterialInstanceEditor::setFilePath(const fs::path& path) {
    filePath = path;
    
    File in(filePath, File::OpenMode::Read);
    auto wholeFile = in.readWholeFile();
    rj::Document document;
    document.Parse(wholeFile.first.get(), static_cast<std::size_t>(wholeFile.second));
    
    instanceDefinition = std::make_unique<MaterialInstanceDefinition>();
    instanceDefinition->deserializeJSON(document);
    
    materialRenderMode = instanceDefinition->getRenderMode();
    materialTemplateAsset = instanceDefinition->getMaterialTemplateDefinition();
    
    instanceTextures.clear();
    for (const auto& t : instanceDefinition->getTextures()) {
        instanceTextures.emplace(t.first, t.second);
    }
    
    instanceVariables.clear();
    for (const auto& t : instanceDefinition->getVariables()) {
        instanceVariables.emplace(t.first, t.second);
    }
}

std::string MaterialInstanceEditor::CreateNew() {
    MaterialInstanceDefinition mid;
    return mid.getJSONString();
}

void MaterialInstanceEditor::changeAsset(const util::DragDropAssetPayload& payload) {
    assert(payload.type == AssetType::MaterialTemplate);
    
    materialTemplateAsset = payload.getNameHash();
}

}
