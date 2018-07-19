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

#include "tools/ShadingPipelineEditor.hpp"
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

ShadingPipelineEditor::ShadingPipelineEditor(const Engine* engine, const Renderer* renderer, EditorState* editorState) : pipelineNameBuf(""), newPipelineNameBuf(""), editorState(editorState), renderer(renderer), engine(engine), currentPipeline(0), chosenTemplate(0), complete(false), wasShownLastTime(false) {}

void ShadingPipelineEditor::show(bool* open) {
    // TODO expose all of the MaterialPipelineRequirements fields for editing
    bool showing = ImGui::Begin("Pipeline editor", open);
    if (showing && !wasShownLastTime) {
        wasShownLastTime = true;
        std::memset(newPipelineNameBuf, 0, sizeof(newPipelineNameBuf));
        chosenTemplate = 0;
    }
    
    if (!showing && wasShownLastTime) {
        wasShownLastTime = false;
    }
    
    if (showing) {
        ImGui::Text("Edit existing pipeline");
        ImGui::Combo("Pipeline", &currentPipeline, [](void* in, int idx, const char** out_text){
                   std::string* strs = static_cast<std::string*>(in);

                   // Checking if we're in bounds + silencing the comparison between signed and unsigned warning
                   if (idx < 0 || idx >= static_cast<int>(strs->size())) {
                       return false;
                   }

                   *out_text = strs[idx].c_str();
                   return true;
                }, reinterpret_cast<void*>(&pipelineNames), pipelineNames.size());
        ImGui::SameLine();
        if (ImGui::Button("Edit")) {
            // TODO implement editor
        }
        
        ImGui::Separator();
        ImGui::Text("Create a new pipeline");
        
        ImGui::InputText("Name", newPipelineNameBuf, sizeof(newPipelineNameBuf));
        ImGui::Combo("Template", &chosenTemplate, [](void* in, int idx, const char** out_text){
                DefaultMaterialPipelineDefinitionArray* defArray = static_cast<DefaultMaterialPipelineDefinitionArray*>(in);

                // Checking if we're in bounds + silencing the comparison between signed and unsigned warning
                if (idx < 0 || idx >= static_cast<int>(defArray->size())) {
                    return false;
                }

                *out_text = (*defArray)[idx].getName().c_str();
                return true;
            }, reinterpret_cast<void*>(const_cast<DefaultMaterialPipelineDefinitionArray*>(&DefaultMaterialPipelineDefinitions)), DefaultMaterialPipelineDefinitions.size());
        
        if (ImGui::Button("Generate Pipeline")) {
            if (validateNewName(newPipelineNameBuf, currentErrorText)) {
                ImGui::CloseCurrentPopup();
            } else {
                ImGui::OpenPopup("New Pipeline Error");
            }
        }
        
        ImGui::SetNextWindowSize(ImVec2(300, -1), ImGuiCond_Appearing);
        if (ImGui::BeginPopupModal("New Pipeline Error")) {
            ImGui::TextWrapped("%s", currentErrorText.c_str());
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::End();
    }
    
    //TODO IMPLEMENT ACTUAL EDITING, ADDING AND REMOVAL OF PIPELINES, USING THE FUNCTIONS OF THE PROJECT CLASS
    
//             if (ImGui::InputText("Name", pipelineNameBuf, sizeof(pipelineNameBuf))) {
//             LOG_D("Changed" << pipelineNameBuf);
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

void ShadingPipelineEditor::generate() {
    if (!complete) {
        complete = true;
        
//         VulkanGLSLShaderGenerator generator(engine);
//         generator.generateAllShaders(requirements, true);
    }
}

}
