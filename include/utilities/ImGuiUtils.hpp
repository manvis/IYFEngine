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

#ifndef IYF_IMGUI_UTILS_HPP
#define IYF_IMGUI_UTILS_HPP

#include "assets/AssetConstants.hpp"
#include "utilities/DragDropAssetPayload.hpp"
#include "imgui.h"
#include <string>
#include <functional>

namespace iyf {
namespace util {

inline bool StringVectorGetter(void* in, int idx, const char** out_text) {
    std::vector<std::string>* names = static_cast<std::vector<std::string>*>(in);
    
     // Checking if we're in bounds
    if (idx < 0 || idx >= static_cast<int>(names->size())) {
        return false;
    }

    *out_text = (*names)[idx].c_str();
    return true;
}

template <typename T, int MaxValue>
inline void DisplayFlagPicker(const std::string& comboName, T& inOut, T defaultValue, std::vector<std::string>& names) {
    int currentValue = static_cast<int>(inOut);
    if (currentValue >= MaxValue) {
        currentValue = static_cast<int>(defaultValue);
    }
    
    if (ImGui::Combo(comboName.c_str(), &currentValue, util::StringVectorGetter, &names, names.size())) {
        inOut = static_cast<T>(currentValue);
    }
}

const char* GetPayloadNameForAssetType(AssetType type);

void AssetDragDropTarget(const char* text, AssetType type, std::function<void(DragDropAssetPayload)> callback, const ImVec2& dimensions = ImVec2(0.0f, 0.0f), const ImVec2& textAlignment = ImVec2(0.0f, 0.5f));
void AssetDragDropImageTarget(StringHash currentImageID, ImVec2 dimensions, std::function<void(DragDropAssetPayload)> callback);

inline void SquareConstraint(ImGuiSizeCallbackData* data) {
    data->DesiredSize = ImVec2(std::max(data->DesiredSize.x, data->DesiredSize.y), std::max(data->DesiredSize.x, data->DesiredSize.y));
}

void ShowTooltip(const char* text);
void ShowTooltip(const std::string& text);

}
}

#endif // IYF_IMGUI_UTILS_HPP
