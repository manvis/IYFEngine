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

#include "tools/MaterialFamilyEditor.hpp"
#include "imgui.h"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/shaderGeneration/VulkanGLSLShaderGenerator.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/File.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "utilities/Regexes.hpp"

#include <glm/vec4.hpp>

namespace iyf::editor {
static bool validateNewName(const char* name, std::string& currentErrorText) {
    if (std::strlen(name) == 0) {
        currentErrorText = "Can't use an empty name";
        return false;
    }
    
    if (!std::regex_match(name, SystemRegexes().FunctionAndFileNameRegex)) {
        currentErrorText = "You must use a name that can serve both as a function name and a file name.\n\nThis name failed to match the required regex \"[a-zA-Z][a-zA-Z0-9]*\"";
        return false;
    }
    
    return true;
}

MaterialFamilyEditor::MaterialFamilyEditor(const Engine* engine, const Renderer* renderer, EditorState* editorState) : familyNameBuf(""), newFamilyNameBuf(""), editorState(editorState), renderer(renderer), engine(engine), currentFamily(0), chosenTemplate(0), complete(false), wasShownLastTime(false) {}

void MaterialFamilyEditor::show(bool* open) {
    bool showing = ImGui::Begin("Material Family Editor", open);
    if (showing && !wasShownLastTime) {
        wasShownLastTime = true;
        std::memset(newFamilyNameBuf, 0, sizeof(newFamilyNameBuf));
        chosenTemplate = 0;
    }
    
    if (!showing && wasShownLastTime) {
        wasShownLastTime = false;
    }
    
    if (showing) {
        ImGui::Text("Edit existing material family");
        ImGui::Combo("Family", &currentFamily, [](void* in, int idx, const char** out_text){
                   std::string* strs = static_cast<std::string*>(in);

                   // Checking if we're in bounds + silencing the comparison between signed and unsigned warning
                   if (idx < 0 || idx >= static_cast<int>(strs->size())) {
                       return false;
                   }

                   *out_text = strs[idx].c_str();
                   return true;
                }, reinterpret_cast<void*>(&familyNames), familyNames.size());
        ImGui::SameLine();
        if (ImGui::Button("Edit")) {
            // TODO implement editor
        }
        
        ImGui::Separator();
        ImGui::Text("Create a new family");
        
        ImGui::InputText("Name", newFamilyNameBuf, sizeof(newFamilyNameBuf));
        ImGui::Combo("Template", &chosenTemplate, [](void*, int idx, const char** out_text){
                // Checking if we're in bounds
                if (idx < 0 || idx >= static_cast<int>(MaterialFamily::COUNT)) {
                    return false;
                }

                *out_text = con::GetMaterialFamilyDefinition(static_cast<MaterialFamily>(idx)).getName().c_str();
                return true;
            }, nullptr, static_cast<int>(MaterialFamily::COUNT));
        
        if (ImGui::Button("Generate Material Family")) {
            if (validateNewName(newFamilyNameBuf, currentErrorText)) {
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::OpenPopup("New Material Family Error");
            }
        }
        
        ImGui::SetNextWindowSize(ImVec2(300, -1), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("New Material Family Error")) {
            ImGui::TextWrapped("%s", currentErrorText.c_str());
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::End();
    }
    
    //TODO IMPLEMENT ACTUAL EDITING, ADDING AND REMOVAL OF FAMILIES, USING THE FUNCTIONS OF THE PROJECT CLASS
    
//             if (ImGui::InputText("Name", familyNameBuf, sizeof(familyNameBuf))) {
//             LOG_D("Changed" << familyNameBuf);
//         }
//     if (ImGui::Begin("Vertex Shader Editor")) {
//         ImGui::InputText("Name", shaderNameBuf, sizeof(shaderNameBuf));
//         
//         std::size_t bindingCount = Renderer::DefaultMeshVertexBufferLayout.bindingDescriptions.size();
//         ImGui::Columns(bindingCount);
//         
//         bool t = false;
//         const auto& attributes = Renderer::DefaultMeshVertexBufferLayout.attributeDescriptions;
//         const auto& attributeNames = Renderer::DefaultMeshVertexBufferLayout.attributeNames;
//         
//         humanReadableBindingNames.resize(Renderer::DefaultMeshVertexBufferLayout.bindingNames.size());
//         humanReadableAttributeNames.resize(attributeNames.size());
//         
//         humanReadableBindingNames[0] = LOC(Renderer::DefaultMeshVertexBufferLayout.bindingNames[0]);
//         ImGui::Text(humanReadableBindingNames[0].c_str());
// 
//         std::uint32_t previousBinding = attributes[0].binding;
//         for (std::size_t i = 0; i < attributes.size(); ++i) {
//             const auto& a = attributes[i];
//             humanReadableAttributeNames[i] = LOC(attributeNames[i]);
//             
//             if (a.binding != previousBinding) {
//                 humanReadableBindingNames[a.binding] = LOC(Renderer::DefaultMeshVertexBufferLayout.bindingNames[a.binding]);
//                 previousBinding = a.binding;
//                 ImGui::NextColumn();
//                 ImGui::Text(humanReadableBindingNames[a.binding].c_str());
//             }
//             
//             ImGui::Checkbox(humanReadableAttributeNames[i].c_str(), &t);
//         }
//         
//         ImGui::Columns();
// //        for ()
// //        static bool pos = false, nor = false, tanAndBi = false, bitan = false;
// //        ImGui::Columns(2);
// //        ImGui::Text("Main mesh data buffer");
// //        ImGui::Checkbox("Position", &pos);
// //        ImGui::Checkbox("Normals", &nor);
// //        ImGui::Checkbox("Tangents and Bitangents", &tanAndBi);
// //        
// //        ImGui::NextColumn();
// //        
// //        ImGui::Text("tEST");
// //        
// //        ImGui::Columns();
//         
//         ImGui::Button("Close");
//         
//         ImGui::End();
//     }
}

void MaterialFamilyEditor::generate() {
    if (!complete) {
        complete = true;
        
//         VulkanGLSLShaderGenerator generator(engine);
//         generator.generateAllShaders(requirements, true);
    }
}

}
