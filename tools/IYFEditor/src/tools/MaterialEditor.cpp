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

class MaterialNodeBase : public MaterialNode {
public:
    MaterialNodeBase(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t selectedMode = 0)
        : MaterialNode(key, std::move(position), zIndex, selectedMode) {}
};

class GenTypeMaterialNode : public MaterialNodeBase {
public:
    GenTypeMaterialNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t inputsToChange, std::size_t outputsToChange, std::size_t selectedMode = 2)
        : MaterialNodeBase(key, position, zIndex, selectedMode), inputsToChange(inputsToChange), outputsToChange(outputsToChange) {
        const auto connectorType = static_cast<MaterialNodeConnectorType>(selectedMode);
        
        for (std::size_t i = 0; i < inputsToChange; ++i) {
            addInput(getLocalizationHandle(connectorType), connectorType);
        }
        
        for (std::size_t i = 0; i < outputsToChange; ++i) {
            addOutput(getLocalizationHandle(connectorType), connectorType);
        }
    }
    
    virtual bool supportsMultipleModes() const final override {
        return true;
    }
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override {
        static std::vector<ModeInfo> modes = {
            ModeInfo(LH("float", MaterialNodeLocalizationNamespace), LH("float_mode_doc", MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec2",  MaterialNodeLocalizationNamespace), LH("vec2_mode_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec3",  MaterialNodeLocalizationNamespace), LH("vec3_mode_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec4",  MaterialNodeLocalizationNamespace), LH("vec4_mode_doc",  MaterialNodeLocalizationNamespace)),
        };
        
        return modes;
    }
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID) final override {
        if (requestedModeID >= getSupportedModes().size()) {
            return false;
        }
        
        const auto newType = static_cast<MaterialNodeConnectorType>(requestedModeID);
        
        for (std::size_t i = 0; i < inputsToChange; ++i) {
            auto& input = getInput(i);
            changeConnector(input, newType);
        }
        
        for (std::size_t i = 0; i < outputsToChange; ++i) {
            auto& output = getOutput(i);
            changeConnector(output, newType);
        }
        
        return true;
    }
protected:
    inline void changeConnector(Connector& connector, MaterialNodeConnectorType newType) {
        connector.setType(newType);
        connector.setLocalizationHandle(getLocalizationHandle(newType));
    }
    
    inline LocalizationHandle getLocalizationHandle(MaterialNodeConnectorType type) {
        switch (type) {
            case MaterialNodeConnectorType::Float:
                return LH("float", MaterialNodeLocalizationNamespace);
            case MaterialNodeConnectorType::Vec2:
                return LH("vec2", MaterialNodeLocalizationNamespace);
            case MaterialNodeConnectorType::Vec3:
                return LH("vec3", MaterialNodeLocalizationNamespace);
            case MaterialNodeConnectorType::Vec4:
                return LH("vec4", MaterialNodeLocalizationNamespace);
            case MaterialNodeConnectorType::COUNT:
                throw std::logic_error("COUNT is not a valid ConnectorType");
        }
        
        throw std::runtime_error("Unhandled node connector type");
    }
private:
    std::size_t inputsToChange;
    std::size_t outputsToChange;
};

class RadiansNode : public GenTypeMaterialNode {
public:
    RadiansNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Radians;
    }
};

class DegreesNode : public GenTypeMaterialNode {
public:
    DegreesNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Degrees;
    }
};

class SineNode : public GenTypeMaterialNode {
public:
    SineNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Sine;
    }
};

class CosineNode : public GenTypeMaterialNode {
public:
    CosineNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Cosine;
    }
};

class TangentNode : public GenTypeMaterialNode {
public:
    TangentNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Tangent;
    }
};

class ArcsineNode : public GenTypeMaterialNode {
public:
    ArcsineNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Arcsine;
    }
};

class ArccosineNode : public GenTypeMaterialNode {
public:
    ArccosineNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Arccosine;
    }
};

class ArctangentNode : public GenTypeMaterialNode {
public:
    ArctangentNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 1, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Arctangent;
    }
};

class Arctangent2Node : public GenTypeMaterialNode {
public:
    Arctangent2Node(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : GenTypeMaterialNode(key, position, zIndex, 2, 1) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Arctangent2;
    }
};

class PerFrameDataNode : public MaterialNodeBase {
public:
    PerFrameDataNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("uv", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::PerFrameData;
    }
};

class MaterialOutputNode : public MaterialNodeBase {
public:
    /// The MaterialOutputNode must always have a key equal to 0. Moreover, exactly 1 
    /// MaterialOutputNode has to exist in the LogicGraph.
    MaterialOutputNode(const MaterialPipelineDefinition& definition, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(0, position, zIndex) {
        assert(getKey() == 0);
        assert(definition.getLightProcessingFunctionInputs().size() <= std::numeric_limits<LogicGraphConnectorID>::max());
        
        const std::vector<LightProcessingFunctionInput>& functionInputs = definition.getLightProcessingFunctionInputs();
        for (std::size_t i = 0; i < functionInputs.size(); ++i) {
            const auto& functionInput = functionInputs[i];
            const auto& variableDefinition = functionInput.getVariableDefinition();
            
            assert(variableDefinition.format == ShaderDataFormat::Float);
            addInput(variableDefinition.getName(), ShaderDataTypeToMaterialConnectorType(variableDefinition.type));
//             LOG_D("MX: " << i << inputs.back().getName());
        }
        
//         LOG_V("MATS: " << inputs.size());
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Output;
    }
private:
};

class TextureInputNode : public MaterialNodeBase {
public:
    TextureInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex, 2) {
        addInput(LH("uv", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
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
        
        auto& output = getOutput(0);
        output.setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
//         inputs[0].setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
        
        switch (requestedModeID) {
            case 0:
                output.setLocalizationHandle(LH("float", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("float", MaterialNodeLocalizationNamespace));
                break;
            case 1:
                output.setLocalizationHandle(LH("vec2", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec2", MaterialNodeLocalizationNamespace));
                break;
            case 2:
                output.setLocalizationHandle(LH("vec3", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec3", MaterialNodeLocalizationNamespace));
                break;
            case 3:
                output.setLocalizationHandle(LH("vec4", MaterialNodeLocalizationNamespace));
//                 inputs[0].setLocalizationHandle(LH("vec4", MaterialNodeLocalizationNamespace));
                break;
        }
        
        return true;
    }
};

/// Depending on a platform, normal maps may have either 2 or 3 channels.
/// Unlike the TextureInputNode
class NormalMapInputNode : public MaterialNodeBase {
public:
    NormalMapInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addInput(LH("uv", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::NormalMapInput;
    }
};

// class VectorSplitter : public MaterialNode {
//     
// };

MaterialLogicGraph::MaterialLogicGraph(const MaterialPipelineDefinition& definition) {
    nodeTypeInfo.reserve(static_cast<std::uint32_t>(MaterialNodeType::COUNT));
    
    // MaterialNodeType::Output
    nodeTypeInfo.emplace_back(LH("material_output_node", MaterialNodeLocalizationNamespace),
                              LH("material_output_node_doc", MaterialNodeLocalizationNamespace),
                              false, false);
    
    // MaterialNodeType::TextureInput
    nodeTypeInfo.emplace_back(LH("texture_input_node", MaterialNodeLocalizationNamespace),
                              LH("texture_input_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    // MaterialNodeType::NormalMapInput
    nodeTypeInfo.emplace_back(LH("normal_map_input_node", MaterialNodeLocalizationNamespace),
                              LH("normal_map_input_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::PerFrameData
    nodeTypeInfo.emplace_back(LH("per_frame_data_node", MaterialNodeLocalizationNamespace),
                              LH("per_frame_data_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Radians
    nodeTypeInfo.emplace_back(LH("radians_node", MaterialNodeLocalizationNamespace),
                              LH("radians_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Degrees
    nodeTypeInfo.emplace_back(LH("degrees_node", MaterialNodeLocalizationNamespace),
                              LH("degrees_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Sine
    nodeTypeInfo.emplace_back(LH("sine_node", MaterialNodeLocalizationNamespace),
                              LH("sine_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Cosine
    nodeTypeInfo.emplace_back(LH("cosine_node", MaterialNodeLocalizationNamespace),
                              LH("cosine_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Tangent
    nodeTypeInfo.emplace_back(LH("tangent_node", MaterialNodeLocalizationNamespace),
                              LH("tangent_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Arcsine
    nodeTypeInfo.emplace_back(LH("arcsine_node", MaterialNodeLocalizationNamespace),
                              LH("arcsine_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Arccosine
    nodeTypeInfo.emplace_back(LH("arccosine_node", MaterialNodeLocalizationNamespace),
                              LH("arccosine_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Arctangent
    nodeTypeInfo.emplace_back(LH("arctangent_node", MaterialNodeLocalizationNamespace),
                              LH("arctangent_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    //MaterialNodeType::Arctangent2
    nodeTypeInfo.emplace_back(LH("arctangent2_node", MaterialNodeLocalizationNamespace),
                              LH("arctangent2_node_doc", MaterialNodeLocalizationNamespace),
                              true, true);
    
    assert(nodeTypeInfo.size() == static_cast<std::uint32_t>(MaterialNodeType::COUNT));
    assert(getNextKey() == 0);
    
    MaterialOutputNode* outputNode = new MaterialOutputNode(definition, Vec2(0.0f, 0.0f), getNextZIndex());
    insertNode(outputNode);
}

MaterialLogicGraph::~MaterialLogicGraph() {}

MaterialNode* MaterialLogicGraph::addNode(MaterialNodeType type, const Vec2& position) {
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
        case MaterialNodeType::Radians:
            node = newNode<RadiansNode>(position, type);
            break;
        case MaterialNodeType::Degrees:
            node = newNode<DegreesNode>(position, type);
            break;
        case MaterialNodeType::Sine:
            node = newNode<SineNode>(position, type);
            break;
        case MaterialNodeType::Cosine:
            node = newNode<CosineNode>(position, type);
            break;
        case MaterialNodeType::Tangent:
            node = newNode<TangentNode>(position, type);
            break;
        case MaterialNodeType::Arcsine:
            node = newNode<ArcsineNode>(position, type);
            break;
        case MaterialNodeType::Arccosine:
            node = newNode<ArccosineNode>(position, type);
            break;
        case MaterialNodeType::Arctangent:
            node = newNode<ArctangentNode>(position, type);
            break;
        case MaterialNodeType::Arctangent2:
            node = newNode<Arctangent2Node>(position, type);
            break;
        case MaterialNodeType::COUNT:
            throw std::logic_error("COUNT is not a valid NodeType");
    }
    
    assert(node != nullptr);
    
    if (!insertNode(node)) {
        throw std::runtime_error("Failed to insert a material node.");
    }
    
    return node;
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
        case MaterialNodeConnectorType::COUNT:
            throw std::logic_error("COUNT is not a valid ConnectorType");
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
        case MaterialNodeConnectorType::COUNT:
            throw std::logic_error("COUNT is not a valid ConnectorType");
    }
    
    throw std::runtime_error("Unhandled node connector type");
}

MaterialEditor::MaterialEditor(NodeEditorSettings settings) : LogicGraphEditor(settings) {
    // TODO - dev only. REMOVE ONCE DONE
    graph = std::make_unique<MaterialLogicGraph>(con::GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline::Toon));
    TextureInputNode* nn = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 50.0f)));
    assert(nn != nullptr);
    TextureInputNode* nn2 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 150.0f)));
    assert(nn2 != nullptr);
    TextureInputNode* nn3 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 250.0f)));
    assert(nn3 != nullptr);
    TextureInputNode* nn4 = dynamic_cast<TextureInputNode*>(graph->addNode(MaterialNodeType::TextureInput, Vec2(200.0f, 350.0f)));
    assert(nn4 != nullptr);
    
    graph->addNode(MaterialNodeType::PerFrameData, Vec2(0.0f, 150.0f));
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
