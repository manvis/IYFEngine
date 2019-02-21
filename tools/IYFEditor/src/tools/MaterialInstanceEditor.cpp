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

#include "imgui.h"

namespace iyf::editor {

MaterialInstanceEditor::MaterialInstanceEditor(Engine* engine) : engine(engine) {
    if (engine == nullptr) {
        throw std::runtime_error("Engine can't be nullptr");
    }
    
    assetManager = engine->getAssetManager();
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
    if (ImGui::Begin("Material instance editor", showing)) {
        
        ImGui::Text("MID");
        
        const StringHash sh = instanceDefinition->getMaterialTemplateDefinition();
        const auto templateMetadata = assetManager->getMetadataCopy(sh);
        
        if (!templateMetadata) {
            ImGui::Text("Undefined material template");
        } else {
            const auto& value = templateMetadata.value();
            const MaterialTemplateMetadata& metadata = value.get<MaterialTemplateMetadata>();
            ImGui::Text("Material template: %s", metadata.getSourceAssetPath().c_str());
        }
        
//         if (sh == 0) {
//             ImGui::Text("Undefined material template");
//         } else {
//             ImGui::Text("Material template: %lu", sh.value());
//         }
        
        ImGui::End();
    }
}
void MaterialInstanceEditor::setFilePath(const fs::path& path) {
    filePath = path;
//     
//     VirtualFileSystemSerializer serializer(path, File::OpenMode::Read);
//     instanceDefinition = std::make_unique<MaterialInstanceDefinition>();
//     instanceDefinition->deserialize(serializer);
    
    File in(filePath, File::OpenMode::Read);
    auto wholeFile = in.readWholeFile();
    rj::Document document;
    document.Parse(wholeFile.first.get(), static_cast<std::size_t>(wholeFile.second));
    
    instanceDefinition = std::make_unique<MaterialInstanceDefinition>();
    instanceDefinition->deserializeJSON(document);
}

std::string MaterialInstanceEditor::CreateNew() {
    MaterialInstanceDefinition mid;
    return mid.getJSONString();
}
}
