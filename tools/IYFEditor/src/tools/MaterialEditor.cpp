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
#include "graphics/MaterialPipelineDefinition.hpp"
#include "localization/TextLocalization.hpp"
#include "utilities/logicGraph/LogicGraph.hpp"

#include "imgui.h"

namespace iyf::editor {
const char* MaterialNodeLocalizationNamespace = "material_nodes";

inline MaterialNodeConnectorType ShaderDataTypeToMaterialConnectorType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Scalar:
            return MaterialNodeConnectorType::Float;
        case ShaderDataType::Vector2D:
            return MaterialNodeConnectorType::Vec2;
        case ShaderDataType::Vector3D:
            return MaterialNodeConnectorType::Vec3;
        case ShaderDataType::Vector4D:
            return MaterialNodeConnectorType::Vec3;
        default:
            throw std::runtime_error("Invalid ShaderDataType");
    }
}

class PerFrameDataNode : public MaterialNode {
public:
    PerFrameDataNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNode(key, position, zIndex) {
        outputs.emplace_back(LH("uv", MaterialNodeLocalizationNamespace), 0, false, true, MaterialNodeConnectorType::Vec2);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::PerFrameData;
    }
};

class MaterialOutputNode : public MaterialNode {
public:
    /// The MaterialOutputNode must always have a key equal to 0. Moreover, exactly 1 
    /// MaterialOutputNode has to exist in the LogicGraph.
    MaterialOutputNode(const MaterialPipelineDefinition& definition, Vec2 position, std::uint32_t zIndex) : MaterialNode(0, position, zIndex) {
        assert(getKey() == 0);
        assert(definition.getLightProcessingFunctionInputs().size() <= std::numeric_limits<LogicGraphConnectorID>::max());
        
        const std::vector<LightProcessingFunctionInput>& functionInputs = definition.getLightProcessingFunctionInputs();
        inputs.reserve(functionInputs.size());
        
        for (std::size_t i = 0; i < functionInputs.size(); ++i) {
            const auto& functionInput = functionInputs[i];
            const auto& variableDefinition = functionInput.getVariableDefinition();
            
            assert(variableDefinition.format == ShaderDataFormat::Float);
            inputs.emplace_back(variableDefinition.getName(), i, false, true, ShaderDataTypeToMaterialConnectorType(variableDefinition.type));
//             LOG_D("MX: " << i << inputs.back().getName());
        }
        
//         LOG_V("MATS: " << inputs.size());
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Output;
    }
    
    /// MaterialOutputNode must never be deleted
    virtual bool isDeletable() const final override {
        return false;
    }
private:
};

class TextureInputNode : public MaterialNode {
public:
    TextureInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNode(key, position, zIndex, 2) {
        inputs.emplace_back(LH("uv", MaterialNodeLocalizationNamespace), 0, false, true, MaterialNodeConnectorType::Vec2);
        outputs.emplace_back(LH("vec3", MaterialNodeLocalizationNamespace), 0, false, true, MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::TextureInput;
    }
    
    virtual bool supportsMultipleModes() const final override {
        return true;
    }
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override {
        static std::vector<ModeInfo> modes = {
            ModeInfo(LH("float_texture", MaterialNodeLocalizationNamespace), LH("float_texture_doc", MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec2_texture",  MaterialNodeLocalizationNamespace), LH("vec2_texture_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec3_texture",  MaterialNodeLocalizationNamespace), LH("vec3_texture_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec4_texture",  MaterialNodeLocalizationNamespace), LH("vec4_texture_doc",  MaterialNodeLocalizationNamespace)),
        };
        
        return modes;
    }
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID) final override {
        if (requestedModeID >= getSupportedModes().size()) {
            return false;
        }
        
        outputs[0].setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
//         inputs[0].setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
        
        switch (requestedModeID) {
            case 0:
                outputs[0].setLocalizationHandle(LH("float", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("float", MaterialNodeLocalizationNamespace));
                break;
            case 1:
                outputs[0].setLocalizationHandle(LH("vec2", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec2", MaterialNodeLocalizationNamespace));
                break;
            case 2:
                outputs[0].setLocalizationHandle(LH("vec3", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec3", MaterialNodeLocalizationNamespace));
                break;
            case 3:
                outputs[0].setLocalizationHandle(LH("vec4", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec4", MaterialNodeLocalizationNamespace));
                break;
        }
        
        return true;
    }
};

/// Depending on a platform, normal maps may have either 2 or 3 channels.
/// Unlike the TextureInputNode
class NormalMapInputNode : public MaterialNode {
public:
    NormalMapInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNode(key, position, zIndex) {
        inputs.emplace_back(LH("uv", MaterialNodeLocalizationNamespace), 0, false, true, MaterialNodeConnectorType::Vec2);
        outputs.emplace_back(LH("vec3", MaterialNodeLocalizationNamespace), 0, false, true, MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::NormalMapInput;
    }
};

// class VectorSplitter : public MaterialNode {
//     
// };

MaterialLogicGraph::MaterialLogicGraph(const MaterialPipelineDefinition& definition) {
    assert(getNextKey() == 0);
    
    MaterialOutputNode* outputNode = new MaterialOutputNode(definition, Vec2(0.0f, 0.0f), getNextZIndex());
    insertNode(outputNode);
}

MaterialLogicGraph::~MaterialLogicGraph() {}

MaterialNode* MaterialLogicGraph::makeNode(MaterialNodeType type, const Vec2& position) {
    MaterialNode* node = nullptr;
    
    switch (type) {
        case MaterialNodeType::Output:
            throw std::logic_error("An Output node can only be created automatically by the LogicGraph during its initialization");
        case MaterialNodeType::TextureInput:
            node = newNode<TextureInputNode>(position, type);
            break;
        case MaterialNodeType::NormalMapInput:
            node = newNode<NormalMapInputNode>(position, type);
            break;
        case MaterialNodeType::PerFrameData:
            node = newNode<PerFrameDataNode>(position, type);
            break;
    }
    
    assert(node != nullptr);
    
    if (!insertNode(node)) {
        throw std::runtime_error("Failed to insert a material node.");
    }
    
    return node;
}

LocalizationHandle MaterialLogicGraph::getNodeNameLocalizationHandle(NodeTypeEnum type) const {
    switch (type) {
        case MaterialNodeType::Output:
            return LH("material_output_node", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::TextureInput:
            return LH("texture_input_node", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::NormalMapInput:
            return LH("normal_map_input_node", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::PerFrameData:
            return LH("per_frame_data_node", MaterialNodeLocalizationNamespace);
    }
    
    throw std::runtime_error("Unknown NodeTypeEnum");
}

LocalizationHandle MaterialLogicGraph::getNodeDocumentationLocalizationHandle(NodeTypeEnum type) const {
    switch (type) {
        case MaterialNodeType::Output:
            return LH("material_output_node_doc", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::TextureInput:
            return LH("texture_input_node_doc", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::NormalMapInput:
            return LH("normal_map_input_node_doc", MaterialNodeLocalizationNamespace);
        case MaterialNodeType::PerFrameData:
            return LH("per_frame_data_node_doc", MaterialNodeLocalizationNamespace);
    }
    
    throw std::runtime_error("Unknown NodeTypeEnum");
}

std::string MaterialLogicGraph::getConnectorTypeName(MaterialNodeConnectorType type) const {
    switch (type) {
        case MaterialNodeConnectorType::Float:
            return "Float";
        case MaterialNodeConnectorType::Vec2:
            return "Vec2";
        case MaterialNodeConnectorType::Vec3:
            return "Vec3";
        case MaterialNodeConnectorType::Vec4:
            return "Vec4";
    }
    
    throw std::runtime_error("Unhandled node connector type");
}

std::uint32_t MaterialLogicGraph::getConnectorTypeColor(ConnectorType type) const {
    // TODO something more friendly towards colorblind individuals
    switch (type) {
        case MaterialNodeConnectorType::Float:
            return IM_COL32(200, 64, 64, 255);
        case MaterialNodeConnectorType::Vec2:
            return IM_COL32(64, 200, 64, 255);
        case MaterialNodeConnectorType::Vec3:
            return IM_COL32(64, 64, 200, 255);
        case MaterialNodeConnectorType::Vec4:
            return IM_COL32(200, 200, 200, 255);
    }
    
    throw std::runtime_error("Unhandled node connector type");
}

MaterialEditor::MaterialEditor(NodeEditorSettings settings) : LogicGraphEditor(settings) {
    // TODO - dev only. REMOVE ONCE DONE
    graph = std::make_unique<MaterialLogicGraph>(con::GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline::Toon));
    TextureInputNode* nn = dynamic_cast<TextureInputNode*>(graph->makeNode(MaterialNodeType::TextureInput, Vec2(200.0f, 50.0f)));
    assert(nn != nullptr);
    TextureInputNode* nn2 = dynamic_cast<TextureInputNode*>(graph->makeNode(MaterialNodeType::TextureInput, Vec2(200.0f, 150.0f)));
    assert(nn2 != nullptr);
    TextureInputNode* nn3 = dynamic_cast<TextureInputNode*>(graph->makeNode(MaterialNodeType::TextureInput, Vec2(200.0f, 250.0f)));
    assert(nn3 != nullptr);
    TextureInputNode* nn4 = dynamic_cast<TextureInputNode*>(graph->makeNode(MaterialNodeType::TextureInput, Vec2(200.0f, 350.0f)));
    assert(nn4 != nullptr);
    
    graph->makeNode(MaterialNodeType::PerFrameData, Vec2(0.0f, 150.0f));
//     LOG_V("LOCALIZED: " << nn->getLocalizationHandle().getHashValue());
}

std::string MaterialEditor::getWindowName() const {
    return LOC_SYS(LH("material_editor", MaterialNodeLocalizationNamespace));
}

MaterialEditor::~MaterialEditor() {}

std::unique_ptr<MaterialLogicGraph> MaterialEditor::makeNewGraph(const NewGraphSettings&) {
    return std::make_unique<MaterialLogicGraph>(con::GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline::Toon));
}

}
