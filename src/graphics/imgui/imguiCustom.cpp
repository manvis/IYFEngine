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


#include <imgui.h>

// Needed by custom widgets
#include <imgui_internal.h>

#include <string>
#include <array>
#include <iostream>

#include "core/Constants.hpp"
#include "io/FileSystem.hpp"
#include "io/File.hpp"
#include "io/interfaces/Serializable.hpp"
#include "logging/Logger.hpp"
#include "utilities/IntegerPacking.hpp"

namespace ImGui {
static const float AssetKeyLockVerticalSlices = 3.5f;
static const float AssetKeyLockSizeMultiplier = 2.0f;
static const float AssetKeyLockBorderWidth = 1.0f;
static const ImVec2 AssetKeyLockWidgetSize(12.0f * AssetKeyLockSizeMultiplier + 2.0f * AssetKeyLockBorderWidth,
                                           12.0f * AssetKeyLockSizeMultiplier + 2.0f * AssetKeyLockBorderWidth);

// TODO adjust colors to make them stand out more
static const std::array<ImVec4, 1> Color2DUpperLine = {ImVec4(0.0f, 0.8f, 0.0f, 1.0f)};
static const std::array<ImVec4, 1> Texture2DUpperLine = {ImVec4(0.3f, 0.0f, 0.8f, 1.0f)};
static const std::array<ImVec4, 1> CubemapUpperLine = {ImVec4(0.0f, 0.8f, 0.8f, 1.0f)};
static const std::array<ImVec4, 1> ArrayUpperLine = {ImVec4(0.5f, 0.5f, 0.1f, 1.0f)};
static const std::array<ImVec4, 1> RLowerLine = {ImVec4(0.8f, 0.0f, 0.0f, 1.0f)};
static const std::array<ImVec4, 2> RGLowerLine = {ImVec4(0.8f, 0.0f, 0.0f, 1.0f),
                                                   ImVec4(0.0f, 0.8f, 0.0f, 1.0f)};
static const std::array<ImVec4, 3> RGBLowerLine = {ImVec4(0.8f, 0.0f, 0.0f, 1.0f),
                                                   ImVec4(0.0f, 0.8f, 0.0f, 1.0f),
                                                   ImVec4(0.0f, 0.0f, 0.8f, 1.0f)};
static const std::array<ImVec4, 4> RGBALowerLine = {ImVec4(0.8f, 0.0f, 0.0f, 1.0f),
                                                    ImVec4(0.0f, 0.8f, 0.0f, 1.0f),
                                                    ImVec4(0.0f, 0.0f, 0.8f, 1.0f),
                                                    ImVec4(0.3f, 0.3f, 0.3f, 1.0f)};

static const std::array<ImVec4, 1> MeshUpperLine = {ImVec4(0.8f, 0.8f, 0.0f, 1.0f)};
static const std::array<ImVec4, 1> MeshStaticLowerLine = {ImVec4(0.2f, 0.2f, 0.6f, 1.0f)};
static const std::array<ImVec4, 1> MeshBonedLowerLine = {ImVec4(0.5f, 0.1f, 0.1f, 1.0f)};

static const std::array<ImVec4, 1> FontLines = {ImVec4(0.5f, 0.5f, 0.5f, 1.0f)};
static const std::array<ImVec4, 1> AudioLines = {ImVec4(0.8f, 0.0f, 0.8f, 1.0f)};

enum class DrawMode {
    Key, Lock, Both
};

// Based on ImGui::ButtonEx
static bool AssetKeyLockImpl(const char* label, int upperLineElementCount, const ImVec4* upperLineColors, int lowerLineElementCount, const ImVec4* lowerLineColors, DrawMode mode, bool activeBorder) {
    ImGuiWindow* window = GetCurrentWindow();

    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    
//    const ImVec2 label_size = CalcTextSize(label, NULL, true);
//
    ImVec2 pos = window->DC.CursorPos;
//    if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
//        pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
    ImVec2 size = CalcItemSize(AssetKeyLockWidgetSize, style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f);
    
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ItemSize(bb, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    int flags = 0;
    // TODO if (window->DC.ButtonRepeat) flags |= ImGuiButtonFlags_Repeat;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    // Render
    ImVec4 increment = (hovered && held) ? ImVec4(0.2f, 0.2f, 0.2f, 0.0f) : hovered ? ImVec4(0.1f, 0.1f, 0.1f, 0.0f) : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    window->DrawList->AddRectFilled(bb.Min, bb.Max, ColorConvertFloat4ToU32({0.0f + increment.x, 0.0f + increment.y, 0.0f + increment.z, 1.0f + increment.w}), 0.0f);
    
    float upperLineYStart;
    float upperLineYEnd;
    if (mode == DrawMode::Key) {
        upperLineYStart = bb.Min.y + size.y / AssetKeyLockVerticalSlices;
        upperLineYEnd = bb.Min.y + size.y * 0.5f;
    } else if (mode == DrawMode::Lock) {
        upperLineYStart = bb.Min.y;
        upperLineYEnd = bb.Min.y + size.y / AssetKeyLockVerticalSlices;
    } else {
        upperLineYStart = bb.Min.y;
        upperLineYEnd = bb.Min.y + size.y * 0.5f;
    }
    
    const float upperLineWidth = (size.x - 2.0f * AssetKeyLockBorderWidth) / upperLineElementCount;
    
    for (int i = 0; i < upperLineElementCount; ++i) {
        float widthStart = bb.Min.x + upperLineWidth * i + AssetKeyLockBorderWidth;
        ImVec2 min(widthStart, upperLineYStart);
        ImVec2 max(widthStart + upperLineWidth, upperLineYEnd);

        window->DrawList->AddRectFilled(min, max, ColorConvertFloat4ToU32({upperLineColors[i].x + increment.x,
                                                                           upperLineColors[i].y + increment.y,
                                                                           upperLineColors[i].z + increment.z,
                                                                           upperLineColors[i].w + increment.w}), 0.0f);
    }
    
    float lowerLineYStart;
    float lowerLineYEnd;
    if (mode == DrawMode::Key) {
        lowerLineYStart = bb.Min.y + size.y * 0.5f;
        lowerLineYEnd = bb.Max.y - size.y / AssetKeyLockVerticalSlices;
    } else if (mode == DrawMode::Lock) {
        lowerLineYStart = bb.Max.y - size.y / AssetKeyLockVerticalSlices;
        lowerLineYEnd = bb.Max.y;
    } else {
        lowerLineYStart = bb.Min.y + size.y * 0.5f;
        lowerLineYEnd = bb.Max.y;
    }
    
    const float lowerLineWidth = (size.x - 2.0f * AssetKeyLockBorderWidth) / lowerLineElementCount;
    
    for (int i = 0; i < lowerLineElementCount; ++i) {
        float widthStart = bb.Min.x + lowerLineWidth * i + AssetKeyLockBorderWidth;
        ImVec2 min(widthStart, lowerLineYStart);
        ImVec2 max(widthStart + lowerLineWidth, lowerLineYEnd);
        
        window->DrawList->AddRectFilled(min, max, ColorConvertFloat4ToU32({lowerLineColors[i].x + increment.x,
                                                                           lowerLineColors[i].y + increment.y,
                                                                           lowerLineColors[i].z + increment.z,
                                                                           lowerLineColors[i].w + increment.w}), 0.0f);
    }

    if (activeBorder) {
        window->DrawList->AddRect(bb.Min, bb.Max, ColorConvertFloat4ToU32({0.0f + increment.x, 0.8f + increment.y, 0.8f + increment.z, 1.0f + increment.w}), 0.0f, 0, AssetKeyLockBorderWidth + 0.5f);
//        window->DrawList->AddRect(bb.Min, bb.Max, ColorConvertFloat4ToU32({0.8f + increment.x, 0.8f + increment.y, 0.8f + increment.z, 1.0f + increment.w}), 0.0f, 0, AssetKeyLockBorderWidth + 0.5f);
    } else {
        window->DrawList->AddRect(bb.Min, bb.Max, ColorConvertFloat4ToU32({0.8f + increment.x, 0.8f + increment.y, 0.8f + increment.z, 1.0f + increment.w}), 0.0f, 0, AssetKeyLockBorderWidth);
    }
    return pressed;
}


static bool AssetKeyLockColorSelector(const char* label, iyf::AssetType type, std::uint32_t data, DrawMode mode, bool activeBorder = false) {
    switch (static_cast<iyf::AssetType>(type)) {
        // TODO Make bone animations shareable and implement this.
//    case iyf::AssetType::Animation:
//        break;
    case iyf::AssetType::Font:
        return AssetKeyLockImpl(label, 1, FontLines.data(), 1, FontLines.data(), mode, activeBorder);
    case iyf::AssetType::Mesh: {
        // meshes can be static, have bone based or morph-target based animations
        std::array<std::uint8_t, 4> out;
        iyf::util::Int32ToBytes(data, out);
        
        const ImVec4* lowerLine;
        if (out[0] == 1) {
            lowerLine = MeshBonedLowerLine.data();
        } else {
            lowerLine = MeshStaticLowerLine.data();
        }
        
        return AssetKeyLockImpl(label, 1, MeshUpperLine.data(), 1, lowerLine, mode, activeBorder);
    }
    case iyf::AssetType::Audio:
        return AssetKeyLockImpl(label, 1, AudioLines.data(), 1, AudioLines.data(), mode, activeBorder);
    case iyf::AssetType::Texture: {
        // Textures can have 1, 2, 3 or 4 channels. They can be (for now) 2D or CubeMaps
        std::array<std::uint8_t, 4> out;
        iyf::util::Int32ToBytes(data, out);
        
        iyf::con::ColorChannelCountFlagBits channelCount = static_cast<iyf::con::ColorChannelCountFlagBits>(out[0]);
        iyf::con::ColorDataTypeFlagBits dataType = static_cast<iyf::con::ColorDataTypeFlagBits>(out[1]);
        
        const ImVec4* upperLine;
        const ImVec4* lowerLine;
        
        switch (dataType) {
        case iyf::con::ColorDataTypeFlagBits::Color:
            upperLine = Color2DUpperLine.data();
            break;
        case iyf::con::ColorDataTypeFlagBits::Texture2D:
            upperLine = Texture2DUpperLine.data();
            break;
        case iyf::con::ColorDataTypeFlagBits::TextureCube:
            upperLine = CubemapUpperLine.data();
            break;
        case iyf::con::ColorDataTypeFlagBits::TextureArray:
            upperLine = ArrayUpperLine.data();
            break;
        }
        
        std::uint8_t channelCountNumeric;
        
        switch (channelCount) {
        case iyf::con::ColorChannelCountFlagBits::One:
            lowerLine = RLowerLine.data();
            channelCountNumeric = 1;
            break;
        case iyf::con::ColorChannelCountFlagBits::Two:
            lowerLine = RGLowerLine.data();
            channelCountNumeric = 2;
            break;
        case iyf::con::ColorChannelCountFlagBits::Three:
            lowerLine = RGBLowerLine.data();
            channelCountNumeric = 3;
            break;
        case iyf::con::ColorChannelCountFlagBits::Four:
            lowerLine = RGBALowerLine.data();
            channelCountNumeric = 4;
            break;
        }
        
        return AssetKeyLockImpl(label, 1, upperLine, static_cast<int>(channelCountNumeric), lowerLine, mode, activeBorder);
    }
    default:
        // Not all asset types support keys and locks
        ImGui::InvisibleButton(label, AssetKeyLockWidgetSize);
        return false;
    }
}
    
bool AssetKey(const char* label, iyf::AssetType type, std::uint32_t data, bool activeBorder) {
    return AssetKeyLockColorSelector(label, type, data, DrawMode::Key, activeBorder);
}

bool AssetLock(const char* label, iyf::AssetType type, std::uint32_t data, bool activeBorder) {
    return AssetKeyLockColorSelector(label, type, data, DrawMode::Lock, activeBorder);
}

bool AssetKeyWithLock(const char* label, iyf::AssetType type, std::uint32_t data, bool activeBorder) {
    return AssetKeyLockColorSelector(label, type, data, DrawMode::Both, activeBorder);
}

// enum class FileDialogResult {
//     FileSaved, FileRead, AlreadyExists, DoesNotExist, CriticalError, EmptyPath, ValidationSuccessful, FileDeleted, AwaitingConfirmation
// };
// 
// static std::string makeFinalPath(const char* baseDir, const char* name, const char* extension) {
//     std::string finalPath;
//     
//     std::string fileName(name);
//     std::string fileExtension(extension);
//     
//     if (fileName.size() >= fileExtension.size() && fileName.compare(fileName.size() - fileExtension.size(), fileExtension.size(), fileExtension) == 0) {
//         finalPath = baseDir + std::string("/") + fileName;
//     } else {
//         finalPath = baseDir + std::string("/") + fileName + fileExtension;
//     }
//     
//     return finalPath;
// }
// 
// static FileDialogResult validateInputAndFileSystemState(const char* name, const char* baseDir) {
//     if (strlen(name) == 0) {
//         return FileDialogResult::EmptyPath;
//     }
//     
//     iyf::FileSystem* fileSystem = game->getFileSystem();
//     if (!fileSystem->exists(baseDir)) {
//         fileSystem->createDirectory(baseDir);
//     }
//     
//     if (!fileSystem->isDirectory(baseDir)) {
//         std::string error = baseDir + std::string(" is a file and not a folder");
//         LOG_E(error);
//         return FileDialogResult::CriticalError;
//     };
//     
//     return FileDialogResult::ValidationSuccessful;
// }
// 
// static FileDialogResult serialize(const char* name, bool overwrite, iyf::Serializable* serializable, const char* baseDir, const char* extension) {
//     FileDialogResult fdr = validateInputAndFileSystemState(name, baseDir);
//     if (fdr != FileDialogResult::ValidationSuccessful) {
//         return fdr;
//     }
//     
//     std::string finalPath = makeFinalPath(baseDir, name, extension);
//     if (iyf::File::exists(finalPath) && !overwrite) {
//         return FileDialogResult::AlreadyExists;
//     }
//     
//     iyf::FileWriter fw(finalPath);
//     serializable->serialize(fw);
//     
//     return FileDialogResult::FileSaved;
// }
// 
// static FileDialogResult deserialize(const char* name, iyf::Serializable* serializable, const char* baseDir, const char* extension) {
//     FileDialogResult fdr = validateInputAndFileSystemState(name, baseDir);
//     if (fdr != FileDialogResult::ValidationSuccessful) {
//         return fdr;
//     }
//     
//     std::string finalPath = makeFinalPath(baseDir, name, extension);
//     if (!iyf::File::exists(finalPath)) {
//         return FileDialogResult::DoesNotExist;
//     }
//     
//     iyf::FileReader fr(finalPath);
//     serializable->deserialize(fr);
//     
//     return FileDialogResult::FileRead;
// }
// 
// static FileDialogResult deleteFile(const char* name, bool confirmed, const char* baseDir, const char* extension) {
//     FileDialogResult fdr = validateInputAndFileSystemState(name, baseDir);
//     if (fdr != FileDialogResult::ValidationSuccessful) {
//         return fdr;
//     }
//     
//     std::string finalPath = makeFinalPath(baseDir, name, extension);
//     if (!iyf::File::exists(finalPath)) {
//         return FileDialogResult::DoesNotExist;
//     }
//     
//     if (!confirmed) {
//         return FileDialogResult::AwaitingConfirmation;
//     }
//     
//     iyf::File::deleteFile(finalPath);
//     return FileDialogResult::FileDeleted;
// }

static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
    const char** items = (const char**)data;
    if (out_text)
        *out_text = items[idx];
    return true;
}

bool ListBoxA(const char* label, int* current_item, const char** items, int items_count, int height_items)
{
    const bool value_changed = ListBoxA(label, current_item, Items_ArrayGetter, (void*)items, items_count, height_items);
    return value_changed;
}

bool ListBoxA(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data, int items_count, int height_in_items)
{
    if (!ListBoxHeader(label, items_count, height_in_items))
        return false;

    // Assume all items have even height (= 1 line of text). If you need items of different or variable sizes you can create a custom version of ListBox() in your code without using the clipper.
    bool value_changed = false;
    ImGuiListClipper clipper(items_count, GetTextLineHeightWithSpacing());
    while (clipper.Step())
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        {
            const bool item_selected = (i == *current_item);
            const char* item_text;
            if (!items_getter(data, i, &item_text))
                item_text = "*Unknown item*";

            PushID(i);
            if (Selectable(item_text, item_selected))
            {
                if (i == *current_item) {
                    *current_item = -1;
                } else {
                    *current_item = i;
                }
                
                value_changed = true;
            }
            PopID();
        }
    ListBoxFooter();
    return value_changed;
}

// void SingleDirFilePopupModal(const char* name, char** fileList, int& fileNameCount, int& selectedFile, char* fileNameBuffer, int fileNameBufferSize, iyf::Serializable* file, const char* baseDir, const char* extension) {
//     if (ImGui::BeginPopupModal(name, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//         if  (fileList ==  nullptr || fileNameCount == 0) {
//             fileList = iyf::File::getDirectoryContentsA(baseDir);
//             
//             fileNameCount = 0;
//             
//             char** iter;
//             for (iter = fileList; *iter != nullptr; iter++) {
//                 fileNameCount++;
//             } 
//         }
// 
//         ImGui::PushItemWidth(370);
//         
//         bool listBoxChanged = ListBoxA("Files", &selectedFile, const_cast<const char**>(fileList), fileNameCount, 7);
//         if (listBoxChanged && selectedFile != -1) {
//             strncpy(fileNameBuffer, fileList[selectedFile], fileNameBufferSize);
//         } else if (listBoxChanged && selectedFile == -1) {
//             strncpy(fileNameBuffer, "", fileNameBufferSize);
//         }
// 
//         ImGui::SameLine();
//         ImGui::TextDisabled("(?)");
//         if (ImGui::IsItemHovered()) {
//             ImGui::SetTooltip("Click selection a second time to clear it");
//         }
// 
//         if (ImGui::InputText("Name", fileNameBuffer, fileNameBufferSize)) {
//             selectedFile = -1;
//         }
//         ImGui::PopItemWidth();
// 
//         FileDialogResult result = FileDialogResult::FileSaved;
//         if (ImGui::Button("Save", ImVec2(120, 0))) {
//             result = serialize(fileNameBuffer, false, file, baseDir, extension);
//             
//             if (result == FileDialogResult::CriticalError) {
//                 ImGui::OpenPopup("Critical Error");
//             } else if (result == FileDialogResult::AlreadyExists) {
//                 ImGui::OpenPopup("Error");
//             } else if (result == FileDialogResult::EmptyPath) {
//                 ImGui::OpenPopup("Empty Path");
//             } else if (result == FileDialogResult::FileSaved) {
//                 iyf::File::freeDirectoryContentsA(fileList);
//                 fileList = nullptr;
//                 fileNameCount = 0;
//                 
//                 ImGui::CloseCurrentPopup();
//             }
//         }
// 
//         ImGui::SameLine();
// 
//         if (ImGui::Button("Load", ImVec2(120, 0))) {
//             result = deserialize(fileNameBuffer, file, baseDir, extension);
//             
//             if (result == FileDialogResult::CriticalError) {
//                 ImGui::OpenPopup("Critical Error");
//             } else if (result == FileDialogResult::DoesNotExist) {
//                 ImGui::OpenPopup("File Not Found");
//             } else if (result == FileDialogResult::EmptyPath) {
//                 ImGui::OpenPopup("Empty Path");
//             } else if (result == FileDialogResult::FileRead) {
//                 iyf::File::freeDirectoryContentsA(fileList);
//                 fileList = nullptr;
//                 fileNameCount = 0;
//                 
//                 ImGui::CloseCurrentPopup();
//             }
//         }
// 
//         ImGui::SameLine();
// 
//         if (ImGui::Button("Delete", ImVec2(120, 0))) {
//             result = deleteFile(fileNameBuffer, false, baseDir, extension);
//             
//             if (result == FileDialogResult::CriticalError) {
//                 ImGui::OpenPopup("Critical Error");
//             } else if (result == FileDialogResult::DoesNotExist) {
//                 ImGui::OpenPopup("File Not Found");
//             } else if (result == FileDialogResult::EmptyPath) {
//                 ImGui::OpenPopup("Empty Path");
//             } else if (result == FileDialogResult::AwaitingConfirmation) {
//                 ImGui::OpenPopup("Delete File");
//             }
//         }
// 
//         if (ImGui::BeginPopupModal("Critical Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//             ImGui::Text("Failed to save the file. Check log for more details");
//             
//             if (ImGui::Button("Close", ImVec2(120, 0))) {
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::EndPopup();
//         }
// 
//         bool doneOverwrite = false;
//         if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//             ImGui::Text("File already exists. Do you want to overwrite?");
//             
//             if (ImGui::Button("Yes", ImVec2(120, 0))) {
//                 result = serialize(fileNameBuffer, true, file, baseDir, extension);
//                 
//                 if (result != FileDialogResult::FileSaved) {
//                     LOG_E("Failed to overwrite file")
//                     throw std::runtime_error("Failed to overwrite file"); // TODO not crash and burn here
//                 }
//                 
//                 ImGui::CloseCurrentPopup();
//                 doneOverwrite = true;
//             }
//             
//             ImGui::SameLine();
//             
//             if (ImGui::Button("No", ImVec2(120, 0))) {
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::EndPopup();
//         }
//         
//         if (ImGui::BeginPopupModal("Delete File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//             ImGui::Text("Are you sure? This cannot be undone.");
//             
//             if (ImGui::Button("Yes", ImVec2(120, 0))) {
//                 result = deleteFile(fileNameBuffer, true, baseDir, extension);
//                 
//                 if (result != FileDialogResult::FileDeleted) {
//                     LOG_E("Failed to delete file")
//                     throw std::runtime_error("Failed to delete file"); // TODO not crash and burn here
//                 }
//                 
//                 iyf::File::freeDirectoryContentsA(fileList);
//                 
//                 fileList = iyf::File::getDirectoryContentsA(baseDir);
//                 fileNameCount = 0;
//             
//                 char** iter;
//                 for (iter = fileList; *iter != nullptr; iter++) {
//                     fileNameCount++;
//                 }
//                 
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::SameLine();
//             
//             if (ImGui::Button("No", ImVec2(120, 0))) {
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::EndPopup();
//         }
// 
//         if (ImGui::BeginPopupModal("Empty Path", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//             ImGui::Text("Name must not be empty");
//             
//             if (ImGui::Button("Close", ImVec2(120, 0))) {
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::EndPopup();
//         }
//         
//         if (ImGui::BeginPopupModal("File Not Found", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
//             ImGui::Text("Requested file does not exist");
//             
//             if (ImGui::Button("Close", ImVec2(120, 0))) {
//                 ImGui::CloseCurrentPopup();
//             }
//             
//             ImGui::EndPopup();
//         }
// 
//         ImGui::SameLine();
// 
//         if (ImGui::Button("Cancel", ImVec2(120, 0)) || doneOverwrite) {
//             ImGui::CloseCurrentPopup();
//             
//             iyf::File::freeDirectoryContentsA(fileList);
//             fileList = nullptr;
//             fileNameCount = 0;
//         }
// 
//         ImGui::EndPopup();
//     }
//     
// }

const ImWchar* ImGui_Impl_GetGlyphRangesLithuanian() {
    static const ImWchar ranges[] = {
        0x0020, 0x017F, // Basic Latin + Latin Supplement + Latin Extended-A
        
        0,
    };
    return &ranges[0];
}


}


