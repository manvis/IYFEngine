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

#include "tools/MaterialEditor.hpp"

#include "core/Logger.hpp"
#include "core/filesystem/File.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "localization/TextLocalization.hpp"
#include "utilities/logicGraph/LogicGraph.hpp"
#include "utilities/ImGuiUtils.hpp"
#include "DragDropAssetPayload.hpp"

#include "imgui.h"

namespace iyf::editor {
const char* GraphicsLocalizationNamespace = "graphics";

inline NodeEditorSettings MakeNodeEditorSettings() {
    NodeEditorSettings s;
    s.shownButtons = LogicGraphEditorButtonFlagBits::Save;
    s.showNewAsClear = true;
    return s;
}

std::string MaterialEditor::CreateNewGraph(MaterialFamily family) {
    MaterialLogicGraph mlg(family);
    
    rj::StringBuffer sb;
    PrettyStringWriter pw(sb);
    
    assert(!mlg.makesJSONRoot());
    
    pw.StartObject();
    StoreEditorValues(1.0f, ImVec2(0.0f, 0.0f), pw);
    
    mlg.serializeJSON(pw);
    pw.EndObject();
    
    const char* jsonString = sb.GetString();
    const std::size_t jsonByteCount = sb.GetSize();
    
    return std::string(jsonString, jsonByteCount);
}

MaterialEditor::MaterialEditor() : LogicGraphEditor(MakeNodeEditorSettings()) {
    nameBuffer.resize(128, '\0');
    
    // TODO - dev only. REMOVE ONCE DONE
    graph = std::make_unique<MaterialLogicGraph>(MaterialFamily::Toon);
    TextureInputNode* nn = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 50.0f)));
    assert(nn != nullptr);
    TextureInputNode* nn2 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 150.0f)));
    assert(nn2 != nullptr);
    TextureInputNode* nn3 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 250.0f)));
    assert(nn3 != nullptr);
    TextureInputNode* nn4 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 350.0f)));
    assert(nn4 != nullptr);
    
    con::GetMaterialFamilyDefinition(MaterialFamily::Toon).computeHash();
    
    graph->addNode(MaterialNodeType::TextureCoordinates, Vec2(0.0f, 150.0f));
    
    const std::size_t materialFamilyCount = static_cast<std::size_t>(MaterialFamily::COUNT);
    materialFamilyNames.reserve(materialFamilyCount);
    
    for (std::size_t i = 0; i < materialFamilyCount; ++i) {
        materialFamilyNames.emplace_back(con::GetMaterialFamilyDefinition(static_cast<MaterialFamily>(i)).getName());
    }
    
//     LOG_V("LOCALIZED: " << nn->getLocalizationHandle().getHashValue());
}

std::string MaterialEditor::getWindowName() const {
    const std::string windowName = LOC_SYS(LH("material_editor", MaterialLogicGraph::GetMaterialNodeLocalizationNamespace()), filePath.generic_string());
//     LOG_D(windowName);
    return windowName;
}

void MaterialEditor::onButtonClick(LogicGraphEditorButtonFlagBits button) {
    switch (button) {
        case LogicGraphEditorButtonFlagBits::Load:
        case LogicGraphEditorButtonFlagBits::SaveAs:
            throw std::logic_error("This material editor button should have been disabled");
        case LogicGraphEditorButtonFlagBits::Save: {
            const std::string result = serializeJSON();
            
            File output(filePath, File::OpenMode::Write);
            output.writeBytes(result.data(), result.size());
            
            CodeGenerationResult cgr = graph->toCode(ShaderLanguage::GLSLVulkan);
            LOG_D(cgr.getCode());
            LOG_D(cgr.getMaterialStructCode());
            break;
        }
    }
    
    nameBuffer[0] = '\0';
}

void MaterialEditor::onDrawButtonRow() {
    ImGui::SameLine();
    
    if (graph != nullptr) {
        int currentMaterialFamily = static_cast<int>(graph->getMaterialFamily());
        if (ImGui::Combo("Material Family", &currentMaterialFamily, util::StringVectorGetter, &materialFamilyNames, materialFamilyNames.size())) {
            if (currentMaterialFamily != static_cast<int>(graph->getMaterialFamily())) {
                graph->setMaterialFamily(MaterialFamily::Toon);
            }
        }
    }
}

void ShowOutputNodeUI(MaterialNode& node) {
    MaterialOutputNode& n = static_cast<MaterialOutputNode&>(node);
    
    // CULLING
    ImGui::Spacing();
    
    std::vector<std::string> cullModes = {
        LOC_SYS(LH("cull_mode_none", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("cull_mode_front", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("cull_mode_back", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("cull_mode_front_and_back", GraphicsLocalizationNamespace))
    };
    
    int currentCullMode = unsigned(n.cullMode);
    if (currentCullMode >= static_cast<int>(CullModeFlagBits::FrontAndBack)) {
        currentCullMode = static_cast<int>(CullModeFlagBits::Back);
    }
    
    if (ImGui::Combo(LOC_SYS(LH("cull_mode", GraphicsLocalizationNamespace)).c_str(), &currentCullMode, util::StringVectorGetter, &cullModes, cullModes.size())) {
        n.cullMode = static_cast<CullModeFlagBits>(currentCullMode);
    }
    
    // DEPTH
    ImGui::Spacing();
    ImGui::Checkbox(LOC_SYS(LH("depth_write_enabled", GraphicsLocalizationNamespace)).c_str(), &(n.depthWriteEnabled));
    ImGui::Checkbox(LOC_SYS(LH("depth_test_enabled", GraphicsLocalizationNamespace)).c_str(), &(n.depthTestEnabled));
    
    std::vector<std::string> depthCompareOps = {
        LOC_SYS(LH("compare_op_never", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_less", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_less_equal", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_equal", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_greater", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_greater_equal", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_always", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("compare_op_not_equal", GraphicsLocalizationNamespace)),
    };
    util::DisplayFlagPicker<CompareOp, static_cast<int>(CompareOp::COUNT)>(LOC_SYS(LH("depth_compare_op", GraphicsLocalizationNamespace)), n.depthCompareOp, CompareOp::Less, depthCompareOps);
    
    // BLENDING
    ImGui::Spacing();
    ImGui::Checkbox(LOC_SYS(LH("blending_enabled", GraphicsLocalizationNamespace)).c_str(), &(n.blendEnabled));
    
    std::vector<std::string> blendFactors = {
        LOC_SYS(LH("blend_fac_zero", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_one", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_src_color", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_one_minus_src_color", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_dst_color", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_one_minus_dst_color", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_src_alpha", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_one_minus_src_alpha", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_dst_alpha", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_fac_one_minus_dst_alpha", GraphicsLocalizationNamespace)),
    };

    util::DisplayFlagPicker<BlendFactor, static_cast<int>(BlendFactor::ConstantColor)>(LOC_SYS(LH("src_color_blend_fac", GraphicsLocalizationNamespace)), n.srcColorBlendFactor, BlendFactor::One, blendFactors);
    util::DisplayFlagPicker<BlendFactor, static_cast<int>(BlendFactor::ConstantColor)>(LOC_SYS(LH("dst_color_blend_fac", GraphicsLocalizationNamespace)), n.dstColorBlendFactor, BlendFactor::Zero, blendFactors);
    util::DisplayFlagPicker<BlendFactor, static_cast<int>(BlendFactor::ConstantColor)>(LOC_SYS(LH("src_alpha_blend_fac", GraphicsLocalizationNamespace)), n.srcAlphaBlendFactor, BlendFactor::One, blendFactors);
    util::DisplayFlagPicker<BlendFactor, static_cast<int>(BlendFactor::ConstantColor)>(LOC_SYS(LH("dst_alpha_blend_fac", GraphicsLocalizationNamespace)), n.dstAlphaBlendFactor, BlendFactor::Zero, blendFactors);

    std::vector<std::string> blendOps = {
        LOC_SYS(LH("blend_op_add", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_op_subtract", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_op_reverse_subtract", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_op_min", GraphicsLocalizationNamespace)),
        LOC_SYS(LH("blend_op_max", GraphicsLocalizationNamespace)),
    };
    
    util::DisplayFlagPicker<BlendOp, static_cast<int>(BlendOp::COUNT)>(LOC_SYS(LH("color_blend_op", GraphicsLocalizationNamespace)), n.colorBlendOp, BlendOp::Add, blendOps);
    util::DisplayFlagPicker<BlendOp, static_cast<int>(BlendOp::COUNT)>(LOC_SYS(LH("alpha_blend_op", GraphicsLocalizationNamespace)), n.alphaBlendOp, BlendOp::Add, blendOps);
}

void ShowConstantNodeUI(MaterialNode& node) {
    ConstantNode& n = static_cast<ConstantNode&>(node);
    
    ImGui::Spacing();
    
    ImGui::Text("Value");
    
    glm::vec4 value = n.getDefaultValue();
    ImGui::DragFloat("X", &(value.x));
    ImGui::DragFloat("Y", &(value.y));
    ImGui::DragFloat("Z", &(value.z));
    ImGui::DragFloat("W", &(value.w));
    n.setDefaultValue(value);
}

void ShowVariableNodeUI(MaterialNode& node) {
    VariableNode& n = static_cast<VariableNode&>(node);
    
    ImGui::Spacing();
    
    ImGui::Text("Initial value");
    
    glm::vec4 value = n.getDefaultValue();
    ImGui::DragFloat("X", &(value.x));
    ImGui::DragFloat("Y", &(value.y));
    ImGui::DragFloat("Z", &(value.z));
    ImGui::DragFloat("W", &(value.w));
    n.setDefaultValue(value);
}

void ShowTextureInputNodeUI(MaterialNode& node) {
    TextureInputNode& n = static_cast<TextureInputNode&>(node);
    
    ImGui::Spacing();
    
    ImGui::Text("Default texture");
    
    const float contentWidth = ImGui::GetContentRegionAvailWidth();
    ImGui::Image(reinterpret_cast<void*>(n.getDefaultTexture().value()), ImVec2(contentWidth, contentWidth));
    
    if (ImGui::BeginDragDropTarget()) {
        const char* payloadName = util::GetPayloadNameForAssetType(AssetType::Texture);
        
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadName)) {
            assert(payload->DataSize == sizeof(DragDropAssetPayload));
            
            DragDropAssetPayload payloadDestination;
            std::memcpy(&payloadDestination, payload->Data, sizeof(DragDropAssetPayload));
            
            n.setDefaultTexture(payloadDestination.getNameHash());
        }
        
        ImGui::EndDragDropTarget();
    }
    ImGui::TextDisabled("(drop texture above)");
    //ImGui::Image();
}

void MaterialEditor::drawNodeExtraProperties(MaterialNode& node) {
    switch (node.getType()) {
    case MaterialNodeType::Output:
        ShowOutputNodeUI(node);
        break;
    case MaterialNodeType::Constant:
        ShowConstantNodeUI(node);
        break;
    case MaterialNodeType::Variable:
        ShowVariableNodeUI(node);
        break;
    case MaterialNodeType::TextureInput:
        ShowTextureInputNodeUI(node);
        break;
    default:
        break;
    }
}

void MaterialEditor::setPath(fs::path path) {
    filePath = std::move(path);
    File in(filePath, File::OpenMode::Read);
    auto wholeFile = in.readWholeFile();
    
    deserializeJSON(wholeFile.first.get(), wholeFile.second);
}

MaterialEditor::~MaterialEditor() {}

std::unique_ptr<MaterialLogicGraph> MaterialEditor::makeNewGraph(const NewGraphSettings&) {
    return std::make_unique<MaterialLogicGraph>(MaterialFamily::Toon);
}

}
