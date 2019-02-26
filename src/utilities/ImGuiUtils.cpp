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

#include "utilities/ImGuiUtils.hpp"

namespace iyf {
namespace util {
const char* GetPayloadNameForAssetType(AssetType type) {
    switch (type) {
        case AssetType::Animation:
            return "pldAnimAss";
        case AssetType::Mesh:
            return "pldMshAss";
        case AssetType::Texture:
            return "pldTexAss";
        case AssetType::Font:
            return "pldFontAss";
        case AssetType::Audio:
            return "pldAudAss";
        case AssetType::Video:
            return "pldVidAss";
        case AssetType::Script:
            return "pldScrAss";
        case AssetType::Shader:
            return "pldShaAss";
        case AssetType::Strings:
            return "pldStrAss";
        case AssetType::MaterialTemplate:
            return "pldMatTem";
        case AssetType::MaterialInstance:
            return "pldMatIns";
        case AssetType::Custom:
            return "pldCustAss";
        case AssetType::ANY:
            throw std::runtime_error("ANY/COUNT is not a valid asset type");
    }
    
    throw std::runtime_error("Unknown AssetType value");
}

void AssetDragDropTarget(const char* text, AssetType type, std::function<void(DragDropAssetPayload)> callback, const ImVec2& dimensions, const ImVec2& textAlignment) {
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, textAlignment);
    ImGui::Button(text, dimensions);
    ImGui::PopStyleVar();
    
    if (ImGui::BeginDragDropTarget()) {
        const char* payloadName = util::GetPayloadNameForAssetType(type);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName)) {
            assert(payload->DataSize == sizeof(DragDropAssetPayload));
            
            DragDropAssetPayload payloadDestination;
            std::memcpy(&payloadDestination, payload->Data, sizeof(DragDropAssetPayload));
            
            if (callback) {
                callback(payloadDestination);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void AssetDragDropImageTarget(StringHash currentImageID, ImVec2 dimensions, std::function<void(DragDropAssetPayload)> callback) {
    ImGui::Image(reinterpret_cast<void*>(currentImageID.value()), dimensions);
    
    if (ImGui::BeginDragDropTarget()) {
        const char* payloadName = util::GetPayloadNameForAssetType(AssetType::Texture);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName)) {
            assert(payload->DataSize == sizeof(DragDropAssetPayload));
            
            DragDropAssetPayload payloadDestination;
            std::memcpy(&payloadDestination, payload->Data, sizeof(DragDropAssetPayload));
            
            if (callback) {
                callback(payloadDestination);
            }
        }
        
        ImGui::EndDragDropTarget();
    }
}

void ShowTooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void ShowTooltip(const std::string& text) {
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(text.c_str(), text.c_str() + text.length());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}
}
