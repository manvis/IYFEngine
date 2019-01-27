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

#include "graphics/materials/MaterialLogicGraph.hpp"
#include "graphics/ShaderMacros.hpp"

#include "utilities/Regexes.hpp"
#include "utilities/StringUtilities.hpp"

#include "fmt/format.h"

#include "imgui.h"

namespace iyf {
const char* MaterialNodeLocalizationNamespace = "material_nodes";

const char* MaterialLogicGraph::GetMaterialNodeLocalizationNamespace() {
    return MaterialNodeLocalizationNamespace;
}

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

MaterialNodeBase::MaterialNodeBase(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t selectedMode)
    : MaterialNode(key, std::move(position), zIndex, selectedMode) {}

class GenTypeMaterialNode : public MaterialNodeBase {
public:
    GenTypeMaterialNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t inputsToChange, std::size_t floatInputs,
                        std::size_t outputsToChange, const std::vector<const char*>& ioNames, std::size_t selectedMode = 2)
        : MaterialNodeBase(key, position, zIndex, selectedMode), inputsToChange(inputsToChange), outputsToChange(outputsToChange) {
        
        std::size_t counter = 0;
        const auto connectorType = static_cast<MaterialNodeConnectorType>(selectedMode);
        for (std::size_t i = 0; i < inputsToChange; ++i) {
            addInput(getNextLocalizationHandle(ioNames, counter), connectorType, true, true);
        }
        
        for (std::size_t i = 0; i < floatInputs; ++i) {
            addInput(getNextLocalizationHandle(ioNames, counter), MaterialNodeConnectorType::Float, true, true);
        }
        
        for (std::size_t i = 0; i < outputsToChange; ++i) {
            addOutput(getNextLocalizationHandle(ioNames, counter), connectorType);
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
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override {
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
    }
    
    inline LocalizationHandle getNextLocalizationHandle(const std::vector<const char*>& ioNames, std::size_t& id) {
        if (id >= ioNames.size()) {
            throw std::logic_error("Not enough ioNames to name all inputs and outputs");
        }
        
        return LH(ioNames[id++], MaterialNodeLocalizationNamespace);
    }
private:
    std::size_t inputsToChange;
    std::size_t outputsToChange;
};

class TextureCoordinatesNode : public MaterialNodeBase {
public:
    TextureCoordinatesNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("uv", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::TextureCoordinates;
    }
};

// class TimeNode : public MaterialNodeBase {
// public:
//     TimeNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
//         addOutput(LH("time", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
//     }
//     
//     virtual MaterialNodeType getType() const final override {
//         return MaterialNodeType::Time;
//     }
// };

class VertexColorNode : public MaterialNodeBase {
public:
    VertexColorNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("rgba", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec4);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::VertexColor;
    }
};

class FragmentCoordinateNode : public MaterialNodeBase {
public:
    FragmentCoordinateNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("vec4", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec4);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::FragmentCoordinate;
    }
};

class PositionNode : public MaterialNodeBase {
public:
    PositionNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Position;
    }
};

class NormalNode : public MaterialNodeBase {
public:
    NormalNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Normal;
    }
};

class CameraPositionNode : public MaterialNodeBase {
public:
    CameraPositionNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::CameraPosition;
    }
};

class CameraPropertiesNode : public MaterialNodeBase {
public:
    CameraPropertiesNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("znear", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addOutput(LH("zfar", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addOutput(LH("fov", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::CameraProperties;
    }
};

class ScreenDimensionsNode : public MaterialNodeBase {
public:
    ScreenDimensionsNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addOutput(LH("vec2", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::ScreenDimensions;
    }
};


const char* CULL_MODE_FIELD_NAME = "cullMode";
const char* DEPTH_COMPARE_OP_FIELD_NAME = "depthCompareOp";
const char* DEPTH_WRITE_ENABLED_FIELD_NAME = "depthWriteEnabled";
const char* DEPTH_TEST_ENABLED_FIELD_NAME = "depthTestEnabled";
const char* BLEND_ENABLED_FIELD_NAME = "blendEnabled";
const char* SRC_COLOR_BLEND_FACTOR_FIELD_NAME = "srcColorBlendFac";
const char* DST_COLOR_BLEND_FACTOR_FIELD_NAME = "dstColorBlendFac";
const char* SRC_ALPHA_BLEND_FACTOR_FIELD_NAME = "srcAlphaBlendFac";
const char* DST_ALPHA_BLEND_FACTOR_FIELD_NAME = "dstAlphaBlendFac";
const char* COLOR_BLEND_OP_FIELD_NAME = "colorBlendOp";
const char* ALPHA_BLEND_OP_FIELD_NAME = "alphaBlendOp";

MaterialOutputNode::MaterialOutputNode(const MaterialFamilyDefinition& definition, Vec2 position, std::uint32_t zIndex) 
    : MaterialNodeBase(0, position, zIndex), cullMode(CullModeFlagBits::Back), depthCompareOp(CompareOp::Less), depthWriteEnabled(true), depthTestEnabled(true),
    blendEnabled(false), srcColorBlendFactor(BlendFactor::One), dstColorBlendFactor(BlendFactor::Zero), srcAlphaBlendFactor(BlendFactor::One), dstAlphaBlendFactor(BlendFactor::Zero),
    colorBlendOp(BlendOp::Add), alphaBlendOp(BlendOp::Add), normalID(Connector::InvalidID)
{
    assert(getKey() == 0);
    
    buildConnectors(definition);
}

void MaterialOutputNode::changeMaterialFamily(const MaterialFamilyDefinition& definition) {
    removeAllConnectors();
    buildConnectors(definition);
}

LogicGraphConnectorID MaterialOutputNode::getNormalConnectorID() const {
    return normalID;
}

void MaterialOutputNode::buildConnectors(const MaterialFamilyDefinition& definition) {
    assert(definition.getLightProcessingFunctionInputs().size() <= std::numeric_limits<LogicGraphConnectorID>::max());
    
    const std::vector<LightProcessingFunctionInput>& functionInputs = definition.getLightProcessingFunctionInputs();
    for (std::size_t i = 0; i < functionInputs.size(); ++i) {
        const auto& functionInput = functionInputs[i];
        const auto& variableDefinition = functionInput.getVariableDefinition();
        
        assert(variableDefinition.format == ShaderDataFormat::Float);
        addInput(variableDefinition.getName(), ShaderDataTypeToMaterialConnectorType(variableDefinition.type), true, false);
//             LOG_D("MX: " << i << inputs.back().getName());
    }
    
    // Normals should be adjustable. We wouldn't be able to use a normal map otherwise.
    if (definition.isNormalDataRequired()) {
        addInput("normal", ShaderDataTypeToMaterialConnectorType(ShaderDataType::Vector3D), true, false);
        
        normalID = static_cast<LogicGraphConnectorID>(getInputs().size()) - 1;
    }
}

void MaterialOutputNode::deserializeJSON(JSONObject& jo) {
    LogicGraphNode::deserializeJSON(jo);
    
    cullMode = CullModeFlags(static_cast<CullModeFlagBits>(jo[CULL_MODE_FIELD_NAME].GetUint()));
    depthCompareOp = static_cast<CompareOp>(jo[DEPTH_COMPARE_OP_FIELD_NAME].GetUint());
    depthWriteEnabled = jo[DEPTH_WRITE_ENABLED_FIELD_NAME].GetBool();
    depthTestEnabled = jo[DEPTH_TEST_ENABLED_FIELD_NAME].GetBool();
    
    blendEnabled = jo[BLEND_ENABLED_FIELD_NAME].GetBool();
    srcColorBlendFactor = static_cast<BlendFactor>(jo[SRC_COLOR_BLEND_FACTOR_FIELD_NAME].GetUint());
    dstColorBlendFactor = static_cast<BlendFactor>(jo[DST_COLOR_BLEND_FACTOR_FIELD_NAME].GetUint());
    srcAlphaBlendFactor = static_cast<BlendFactor>(jo[SRC_ALPHA_BLEND_FACTOR_FIELD_NAME].GetUint());
    dstAlphaBlendFactor = static_cast<BlendFactor>(jo[DST_ALPHA_BLEND_FACTOR_FIELD_NAME].GetUint());
    colorBlendOp = static_cast<BlendOp>(jo[COLOR_BLEND_OP_FIELD_NAME].GetUint());
    alphaBlendOp = static_cast<BlendOp>(jo[ALPHA_BLEND_OP_FIELD_NAME].GetUint());
}

void MaterialOutputNode::serializeJSON(PrettyStringWriter& pw) const {
    LogicGraphNode::serializeJSON(pw);
    
    pw.String(CULL_MODE_FIELD_NAME);
    pw.Uint(std::uint32_t(cullMode));
    
    pw.String(DEPTH_COMPARE_OP_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(depthCompareOp));
    
    pw.String(DEPTH_WRITE_ENABLED_FIELD_NAME);
    pw.Bool(depthWriteEnabled);
    
    pw.String(DEPTH_TEST_ENABLED_FIELD_NAME);
    pw.Bool(depthTestEnabled);
    
    pw.String(BLEND_ENABLED_FIELD_NAME);
    pw.Bool(blendEnabled);
    
    pw.String(SRC_COLOR_BLEND_FACTOR_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(srcColorBlendFactor));
    
    pw.String(DST_COLOR_BLEND_FACTOR_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(dstColorBlendFactor));
    
    pw.String(SRC_ALPHA_BLEND_FACTOR_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(srcAlphaBlendFactor));
    
    pw.String(DST_ALPHA_BLEND_FACTOR_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(dstAlphaBlendFactor));
    
    pw.String(COLOR_BLEND_OP_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(colorBlendOp));
    
    pw.String(ALPHA_BLEND_OP_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(alphaBlendOp));
}

const char* DEFAULT_TEXTURE_FIELD_NAME = "defaultTexture";

TextureInputNode::TextureInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex, 2), defaultTexture(HS(con::MissingTexture())) {
    addInput(LH("uv", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2, true, true);
    addOutput(LH("rgb", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
}

MaterialNodeType TextureInputNode::getType() const {
        return MaterialNodeType::TextureInput;
}

bool TextureInputNode::supportsMultipleModes() const {
    return true;
}

std::vector<ModeInfo>& TextureInputNode::getSupportedModes() const {
    static std::vector<ModeInfo> modes = {
        ModeInfo(LH("r", MaterialNodeLocalizationNamespace), LH("float_texture_doc", MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("rg",  MaterialNodeLocalizationNamespace), LH("vec2_texture_doc",  MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("rgb",  MaterialNodeLocalizationNamespace), LH("vec3_texture_doc",  MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("rgba",  MaterialNodeLocalizationNamespace), LH("vec4_texture_doc",  MaterialNodeLocalizationNamespace)),
    };
    
    return modes;
}

bool TextureInputNode::onModeChange(std::size_t, std::size_t requestedModeID, bool) {
    if (requestedModeID >= getSupportedModes().size()) {
        return false;
    }
    
    auto& output = getOutput(0);
    output.setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
    
    switch (requestedModeID) {
        case 0:
            output.setLocalizationHandle(LH("r", MaterialNodeLocalizationNamespace));
            break;
        case 1:
            output.setLocalizationHandle(LH("rg", MaterialNodeLocalizationNamespace));
            break;
        case 2:
            output.setLocalizationHandle(LH("rgb", MaterialNodeLocalizationNamespace));
            break;
        case 3:
            output.setLocalizationHandle(LH("rgba", MaterialNodeLocalizationNamespace));
            break;
    }
    
    return true;
}

void TextureInputNode::serializeJSON(PrettyStringWriter& pw) const {
    LogicGraphNode::serializeJSON(pw);
    
    pw.String(DEFAULT_TEXTURE_FIELD_NAME);
    pw.Uint64(defaultTexture.value());
}

void TextureInputNode::deserializeJSON(JSONObject& jo) {
    LogicGraphNode::deserializeJSON(jo);
    
    defaultTexture = StringHash(jo[DEFAULT_TEXTURE_FIELD_NAME].GetUint64());
}

class CrossNode : public MaterialNodeBase {
public:
    CrossNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addInput(LH("x", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3, true, true);
        addInput(LH("y", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3, true, true);
        addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Cross;
    }
};

class LengthNode : public GenTypeMaterialNode {
public:
    LengthNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex)
        : GenTypeMaterialNode(key, position, zIndex, 1, 0, 0, {"x"}) {
        addOutput(LH("length_node", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Length;
    }
};

class DistanceNode : public GenTypeMaterialNode {
public:
    DistanceNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex)
        : GenTypeMaterialNode(key, position, zIndex, 2, 0, 0, {"x", "y"}) {
        addOutput(LH("length_node", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Distance;
    }
};

class DotNode : public GenTypeMaterialNode {
public:
    DotNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex)
        : GenTypeMaterialNode(key, position, zIndex, 2, 0, 0, {"x", "y"}) {
        addOutput(LH("length_node", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Dot;
    }
};

const char* VALUE_FIELD_NAME = "value";
const char* CONSTANT_X_FIELD_NAME = "x";
const char* CONSTANT_Y_FIELD_NAME = "y";
const char* CONSTANT_Z_FIELD_NAME = "z";
const char* CONSTANT_W_FIELD_NAME = "w";

VariableNode::VariableNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
    : MaterialNodeBase(key, position, zIndex, 2), value(0.0f, 0.0f, 0.0f, 0.0f) {
    addOutput(LH("vec3", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
//     for (std::size_t i = 0; i < (con::MaxMaterialComponents / 4); ++i) {
//         addOutput(LH("vec4", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec4);
//     }
}

MaterialNodeType VariableNode::getType() const {
    return MaterialNodeType::Variable;
}

bool VariableNode::supportsMultipleModes() const {
    return true;
}

std::vector<ModeInfo>& VariableNode::getSupportedModes() const {
    static std::vector<ModeInfo> modes = {
        ModeInfo(LH("float", MaterialNodeLocalizationNamespace), LH("float_constant_doc", MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("vec2",  MaterialNodeLocalizationNamespace), LH("vec2_constant_doc",  MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("vec3",  MaterialNodeLocalizationNamespace), LH("vec3_constant_doc",  MaterialNodeLocalizationNamespace)),
        ModeInfo(LH("vec4",  MaterialNodeLocalizationNamespace), LH("vec4_constant_doc",  MaterialNodeLocalizationNamespace)),
    };
    
    return modes;
}

bool VariableNode::onModeChange(std::size_t, std::size_t requestedModeID, bool) {
    if (requestedModeID >= getSupportedModes().size()) {
        return false;
    }
    
    auto& output = getOutput(0);
    output.setType(static_cast<MaterialNodeConnectorType>(requestedModeID));
    
    switch (requestedModeID) {
        case 0:
            output.setLocalizationHandle(LH("float", MaterialNodeLocalizationNamespace));
            break;
        case 1:
            output.setLocalizationHandle(LH("vec2", MaterialNodeLocalizationNamespace));
            break;
        case 2:
            output.setLocalizationHandle(LH("vec3", MaterialNodeLocalizationNamespace));
            break;
        case 3:
            output.setLocalizationHandle(LH("vec4", MaterialNodeLocalizationNamespace));
            break;
    }
    
    return true;
}

void VariableNode::serializeJSON(PrettyStringWriter& pw) const {
    LogicGraphNode::serializeJSON(pw);
    
    pw.String(VALUE_FIELD_NAME);
    pw.StartObject();
    
    pw.String(CONSTANT_X_FIELD_NAME);
    pw.Double(value.x);
    
    pw.String(CONSTANT_Y_FIELD_NAME);
    pw.Double(value.y);
    
    pw.String(CONSTANT_Z_FIELD_NAME);
    pw.Double(value.z);
    
    pw.String(CONSTANT_W_FIELD_NAME);
    pw.Double(value.w);
    
    pw.EndObject();
}

void VariableNode::deserializeJSON(JSONObject& jo) {
    LogicGraphNode::deserializeJSON(jo);
    
    JSONObject val = jo[VALUE_FIELD_NAME].GetObject();
    
    value.x = val[CONSTANT_X_FIELD_NAME].GetFloat();
    value.y = val[CONSTANT_Y_FIELD_NAME].GetFloat();
    value.z = val[CONSTANT_Z_FIELD_NAME].GetFloat();
    value.w = val[CONSTANT_W_FIELD_NAME].GetFloat();
}

class SplitterJoinerBase : public MaterialNodeBase {
public:
    SplitterJoinerBase(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t selectedMode = 2) 
        : MaterialNodeBase(key, position, zIndex, selectedMode) {}
    
    virtual bool supportsMultipleModes() const final override {
        return true;
    }
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override {
        static std::vector<ModeInfo> modes = {
            ModeInfo(LH("vec2",  MaterialNodeLocalizationNamespace), LH("vec2_mode_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec3",  MaterialNodeLocalizationNamespace), LH("vec3_mode_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("vec4",  MaterialNodeLocalizationNamespace), LH("vec4_mode_doc",  MaterialNodeLocalizationNamespace)),
        };
        
        return modes;
    }
};

class SplitterNode : public SplitterJoinerBase {
public:
    SplitterNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : SplitterJoinerBase(key, position, zIndex, 0) {
        addInput(LH("vector", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2, true, true);
        addOutput(LH("x", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addOutput(LH("y", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addOutput(LH("z", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addOutput(LH("w", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        
        setSelectedModeID(1);
        onModeChange(0, 1, false);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Splitter;
    }
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override {
        if (requestedModeID >= getSupportedModes().size()) {
            return false;
        }
        
        auto& output = getInput(0);
        output.setType(static_cast<MaterialNodeConnectorType>(requestedModeID + 1));
        
        switch (requestedModeID) {
            case 0:
                getOutput(0).setEnabled(true);
                getOutput(1).setEnabled(true);
                getOutput(2).setEnabled(false);
                getOutput(3).setEnabled(false);
                break;
            case 1:
                getOutput(0).setEnabled(true);
                getOutput(1).setEnabled(true);
                getOutput(2).setEnabled(true);
                getOutput(3).setEnabled(false);
                break;
            case 2:
                getOutput(0).setEnabled(true);
                getOutput(1).setEnabled(true);
                getOutput(2).setEnabled(true);
                getOutput(3).setEnabled(true);
                break;
        }
        
        return true;
    }
};

class JoinerNode : public SplitterJoinerBase {
public:
    JoinerNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : SplitterJoinerBase(key, position, zIndex, 0) {
        addInput(LH("x", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float, true, true);
        addInput(LH("y", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float, true, true);
        addInput(LH("z", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float, true, true);
        addInput(LH("w", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float, true, true);
        addOutput(LH("vector", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
        
        setSelectedModeID(1);
        onModeChange(0, 1, false);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Joiner;
    }
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override {
        if (requestedModeID >= getSupportedModes().size()) {
            return false;
        }
        
        auto& output = getOutput(0);
        output.setType(static_cast<MaterialNodeConnectorType>(requestedModeID + 1));
        
        switch (requestedModeID) {
            case 0:
                getInput(0).setEnabled(true);
                getInput(1).setEnabled(true);
                getInput(2).setEnabled(false);
                getInput(3).setEnabled(false);
                break;
            case 1:
                getInput(0).setEnabled(true);
                getInput(1).setEnabled(true);
                getInput(2).setEnabled(true);
                getInput(3).setEnabled(false);
                break;
            case 2:
                getInput(0).setEnabled(true);
                getInput(1).setEnabled(true);
                getInput(2).setEnabled(true);
                getInput(3).setEnabled(true);
                break;
        }
        
        return true;
    }
};

#define IYF_MAKE_GEN_TYPE_NODE(name, inputs, outputs, ...) \
class name##Node : public GenTypeMaterialNode { \
public: \
    name##Node(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) \
        : GenTypeMaterialNode(key, position, zIndex, inputs, 0, outputs, __VA_ARGS__) {} \
    \
    virtual MaterialNodeType getType() const final override { \
        return MaterialNodeType::name; \
    } \
}

#define IYF_MAKE_GEN_TYPE_FLOAT_NODE(name, inputs, floatInputs, outputs, ...) \
class name##Node : public GenTypeMaterialNode { \
public: \
    name##Node(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) \
        : GenTypeMaterialNode(key, position, zIndex, inputs, floatInputs, outputs, __VA_ARGS__) {} \
    \
    virtual MaterialNodeType getType() const final override { \
        return MaterialNodeType::name; \
    } \
}

#define IYF_MAKE_GEN_TYPE_FLOAT_OUT_NODE(name, inputs, outputs, ...) \
class name##Node : public GenTypeMaterialNode { \
public: \
    name##Node(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) \
        : GenTypeMaterialNode(key, position, zIndex, inputs, 0, outputs, __VA_ARGS__) { \
            addOutput(LH(name##"_node", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float); \
        } \
    \
    virtual MaterialNodeType getType() const final override { \
        return MaterialNodeType::name; \
    } \
}

IYF_MAKE_GEN_TYPE_NODE(Radians, 1, 1, {"degrees", "radians"});
IYF_MAKE_GEN_TYPE_NODE(Degrees, 1, 1, {"radians", "degrees"});
IYF_MAKE_GEN_TYPE_NODE(Sin, 1, 1, {"x", "sin_node"});
IYF_MAKE_GEN_TYPE_NODE(Cos, 1, 1, {"x", "cos_node"});
IYF_MAKE_GEN_TYPE_NODE(Tan, 1, 1, {"x", "tan_node"});
IYF_MAKE_GEN_TYPE_NODE(Asin, 1, 1, {"x", "asin_node"});
IYF_MAKE_GEN_TYPE_NODE(Acos, 1, 1, {"x", "acos_node"});
IYF_MAKE_GEN_TYPE_NODE(Atan, 1, 1, {"x", "atan_node"});
IYF_MAKE_GEN_TYPE_NODE(Atan2, 2, 1, {"x", "y", "atan2_node"});
IYF_MAKE_GEN_TYPE_NODE(Sinh, 1, 1, {"x", "sinh_node"});
IYF_MAKE_GEN_TYPE_NODE(Cosh, 1, 1, {"x", "cosh_node"});
IYF_MAKE_GEN_TYPE_NODE(Tanh, 1, 1, {"x", "tanh_node"});
IYF_MAKE_GEN_TYPE_NODE(Asinh, 1, 1, {"x", "asinh_node"});
IYF_MAKE_GEN_TYPE_NODE(Acosh, 1, 1, {"x", "acosh_node"});
IYF_MAKE_GEN_TYPE_NODE(Atanh, 1, 1, {"x", "atanh_node"});
IYF_MAKE_GEN_TYPE_NODE(Pow, 2, 1, {"x", "y", "pow_node"});
IYF_MAKE_GEN_TYPE_NODE(Exp, 1, 1, {"x", "exp_node"});
IYF_MAKE_GEN_TYPE_NODE(Log, 1, 1, {"x", "log_node"});
IYF_MAKE_GEN_TYPE_NODE(Exp2, 1, 1, {"x", "exp2_node"});
IYF_MAKE_GEN_TYPE_NODE(Log2, 1, 1, {"x", "log2_node"});
IYF_MAKE_GEN_TYPE_NODE(Sqrt, 1, 1, {"x", "sqrt_node"});
IYF_MAKE_GEN_TYPE_NODE(InverseSqrt, 1, 1, {"x", "inverse_sqrt_node"});
IYF_MAKE_GEN_TYPE_NODE(Abs, 1, 1, {"x", "abs_node"});
IYF_MAKE_GEN_TYPE_NODE(Sign, 1, 1, {"x", "sign_node"});
IYF_MAKE_GEN_TYPE_NODE(Floor, 1, 1, {"x", "floor_node"});
IYF_MAKE_GEN_TYPE_NODE(Trunc, 1, 1, {"x", "trunc_node"});
IYF_MAKE_GEN_TYPE_NODE(Round, 1, 1, {"x", "round_node"});
IYF_MAKE_GEN_TYPE_NODE(RoundEven, 1, 1, {"x", "round_even_node"});
IYF_MAKE_GEN_TYPE_NODE(Ceil, 1, 1, {"x", "ceil_node"});
IYF_MAKE_GEN_TYPE_NODE(Fract, 1, 1, {"x", "fract_node"});
IYF_MAKE_GEN_TYPE_NODE(Mod, 2, 1, {"x", "y", "mod_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(ModFloat, 1, 1, 1, {"x", "y", "mod_node"});
IYF_MAKE_GEN_TYPE_NODE(ModF, 1, 2, {"x", "F", "I"});
IYF_MAKE_GEN_TYPE_NODE(Min, 2, 1, {"x", "y", "min_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(MinFloat, 1, 1, 1, {"x", "y", "min_node"});
IYF_MAKE_GEN_TYPE_NODE(Max, 2, 1, {"x", "y", "max_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(MaxFloat, 1, 1, 1, {"x", "y", "max_node"});
IYF_MAKE_GEN_TYPE_NODE(Clamp, 3, 1, {"x", "min_value", "max_value", "clamp_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(ClampFloat, 1, 2, 1, {"x", "min_value", "max_value", "clamp_node"});
IYF_MAKE_GEN_TYPE_NODE(Mix, 3, 1, {"x", "y", "a", "mix_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(MixFloat, 2, 1, 1, {"x", "y", "a", "mix_node"});
IYF_MAKE_GEN_TYPE_NODE(Step, 2, 1, {"x", "edge", "step_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(StepFloat, 1, 1, 1, {"x", "edge", "step_node"});
IYF_MAKE_GEN_TYPE_NODE(SmoothStep, 3, 1, {"x", "edge", "edge2", "smooth_step_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(SmoothstepFloat, 1, 2, 1, {"x", "edge", "edge2", "smooth_step_node"});
IYF_MAKE_GEN_TYPE_NODE(Normalize, 1, 1, {"x", "normalize_node"});
IYF_MAKE_GEN_TYPE_NODE(FaceForward, 3, 1, {"N", "I", "Nref" "face_forward_node"});
IYF_MAKE_GEN_TYPE_NODE(Reflect, 2, 1, {"I", "N", "reflect_node"});
IYF_MAKE_GEN_TYPE_FLOAT_NODE(Refract, 2, 1, 1, {"I", "N", "x", "refract_node"});
IYF_MAKE_GEN_TYPE_NODE(DfDx, 1, 1, {"p", "dfdx_node"});
IYF_MAKE_GEN_TYPE_NODE(DfDy, 1, 1, {"p", "dfdy_node"});
IYF_MAKE_GEN_TYPE_NODE(Add, 2, 1, {"x", "y", "result"});
IYF_MAKE_GEN_TYPE_NODE(Subtract, 2, 1, {"x", "y", "result"});
IYF_MAKE_GEN_TYPE_NODE(Multiply, 2, 1, {"x", "y", "result"});
IYF_MAKE_GEN_TYPE_NODE(Divide, 2, 1, {"x", "y", "result"});

MaterialLogicGraph::MaterialLogicGraph(MaterialFamily materialFamily) : materialFamily(materialFamily) {
    const char* ns = MaterialNodeLocalizationNamespace;
    
    addNodeTypeInfo(MaterialNodeType::Output, "material_output_node", "material_output_node_doc", ns, MaterialNodeGroup::Output, false, false);
    
    const MaterialNodeGroup input = MaterialNodeGroup::Input;
    addNodeTypeInfo(MaterialNodeType::TextureInput, "texture_input_node", "texture_input_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::TextureCoordinates, "texture_coordinates_node", "texture_coordinates_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::Variable, "variable_node", "variable_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::Constant, "constant_node", "constant_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::ScreenDimensions, "screen_dimensions_node", "screen_dimensions_node_doc", ns, input);
    //addNodeTypeInfo(MaterialNodeType::Time, "time_node", "time_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::VertexColor, "vertex_color_node", "vertex_color_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::FragmentCoordinate, "fragment_coordinate_node", "fragment_coordinate_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::Position, "position_node", "position_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::Normal, "normal_node", "normal_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::CameraPosition, "camera_position_node", "camera_position_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::CameraProperties, "camera_properties_node", "camera_properties_node_doc", ns, input);
    
    const MaterialNodeGroup arithmetic = MaterialNodeGroup::Arithmetic;
    addNodeTypeInfo(MaterialNodeType::Add, "add_node", "add_node_doc", ns, arithmetic);
    addNodeTypeInfo(MaterialNodeType::Subtract, "subtract_node", "subtract_node_doc", ns, arithmetic);
    addNodeTypeInfo(MaterialNodeType::Multiply, "multiply_node", "multiply_node_doc", ns, arithmetic);
    addNodeTypeInfo(MaterialNodeType::Divide, "divide_node", "divide_node_doc", ns, arithmetic);
    
    const MaterialNodeGroup vecs = MaterialNodeGroup::VectorComponents;
    addNodeTypeInfo(MaterialNodeType::Splitter, "splitter_node", "splitter_node_doc", ns, vecs);
    addNodeTypeInfo(MaterialNodeType::Joiner, "joiner_node", "joiner_node_doc", ns, vecs);
    
    const MaterialNodeGroup trig = MaterialNodeGroup::Trigonometry;
    addNodeTypeInfo(MaterialNodeType::Radians, "radians_node", "radians_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Degrees, "degrees_node", "degrees_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Sin, "sin_node", "sin_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Cos, "cos_node", "cos_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Tan, "tan_node", "tan_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Asin, "asin_node", "asin_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Acos, "acos_node", "acos_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Atan, "atan_node", "atan_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Atan2, "atan2_node", "atan2_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Sinh, "sinh_node", "sinh_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Cosh, "cosh_node", "cosh_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Tanh, "tanh_node", "tanh_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Asinh, "asinh_node", "asinh_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Acosh, "acosh_node", "acosh_node_doc", ns, trig);
    addNodeTypeInfo(MaterialNodeType::Atanh, "atanh_node", "atanh_node_doc", ns, trig);
    
    const MaterialNodeGroup exp = MaterialNodeGroup::Exponential;
    addNodeTypeInfo(MaterialNodeType::Pow, "pow_node", "pow_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::Exp, "exp_node", "exp_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::Log, "log_node", "log_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::Exp2, "exp2_node", "exp2_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::Log2, "log2_node", "log2_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::Sqrt, "sqrt_node", "sqrt_node_doc", ns, exp);
    addNodeTypeInfo(MaterialNodeType::InverseSqrt, "inverse_sqrt_node", "inverse_sqrt_node_doc", ns, exp);

    const MaterialNodeGroup com = MaterialNodeGroup::CommonMath;
    addNodeTypeInfo(MaterialNodeType::Abs, "abs_node", "abs_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Sign, "sign_node", "sign_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Floor, "floor_node", "floor_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Trunc, "trunc_node", "trunc_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Round, "round_node", "round_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::RoundEven, "round_even_node", "round_even_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Ceil, "ceil_node", "ceil_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Fract, "fract_node", "fract_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Mod, "mod_node", "mod_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::ModFloat, "mod_float_node", "mod_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::ModF, "modf_node", "modf_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Min, "min_node", "min_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::MinFloat, "min_float_node", "min_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Max, "max_node", "max_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::MaxFloat, "max_float_node", "max_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Clamp, "clamp_node", "clamp_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::ClampFloat, "clamp_float_node", "clamp_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Mix, "mix_node", "mix_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::MixFloat, "mix_float_node", "mix_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::Step, "step_node", "step_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::StepFloat, "step_float_node", "step_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::SmoothStep, "smooth_step_node", "smooth_step_node_doc", ns, com);
    addNodeTypeInfo(MaterialNodeType::SmoothstepFloat, "smooth_step_float_node", "smooth_step_node_doc", ns, com);
    
    const MaterialNodeGroup geo = MaterialNodeGroup::Geometry;
    addNodeTypeInfo(MaterialNodeType::Length, "length_node", "length_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Distance, "distance_node", "distance_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Dot, "dot_node", "dot_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Cross, "cross_node", "cross_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Normalize, "normalize_node", "normalize_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::FaceForward, "face_forward_node", "face_forward_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Reflect, "reflect_node", "reflect_node_doc", ns, geo);
    addNodeTypeInfo(MaterialNodeType::Refract, "refract_node", "refract_node_doc", ns, geo);
    
    
    const MaterialNodeGroup der = MaterialNodeGroup::Derivatives;
    addNodeTypeInfo(MaterialNodeType::DfDx, "dfdx_node", "dfdx_node_doc", ns, der);
    addNodeTypeInfo(MaterialNodeType::DfDy, "dfdy_node", "dfdy_node_doc", ns, der);
    
    setNodeGroupName(MaterialNodeGroup::Input, LH("input", ns));
    setNodeGroupName(MaterialNodeGroup::VectorComponents, LH("vector_components", ns));
    setNodeGroupName(MaterialNodeGroup::Trigonometry, LH("trigonometry", ns));
    setNodeGroupName(MaterialNodeGroup::Exponential, LH("exponential", ns));
    setNodeGroupName(MaterialNodeGroup::CommonMath, LH("common_math", ns));
    setNodeGroupName(MaterialNodeGroup::Geometry, LH("geometry", ns));
    setNodeGroupName(MaterialNodeGroup::Derivatives, LH("derivatives", ns));
    setNodeGroupName(MaterialNodeGroup::Output, LH("output", ns));
    setNodeGroupName(MaterialNodeGroup::Arithmetic, LH("arithmetic", ns));
    
    validateNodeTypeInfo();
    assert(getNextKey() == 0);
    
    output = new MaterialOutputNode(con::GetMaterialFamilyDefinition(materialFamily), Vec2(0.0f, 0.0f), getNextZIndex());
    insertNode(output, getNextKey(), false);
}

MaterialLogicGraph::~MaterialLogicGraph() {}

#define IYF_CREATE_NODE_CASE(name) \
        case MaterialNodeType::name:\
            node = newNode<name##Node>(key, position, type, isDeserializing);\
            break

MaterialNode* MaterialLogicGraph::addNodeImpl(NodeKey key, MaterialNodeType type, const Vec2& position, bool isDeserializing) {
    MaterialNode* node = nullptr;
    
    switch (type) {
        case MaterialNodeType::Output:
            if (!isDeserializing) {
                throw std::logic_error("An Output node can only be created automatically by the LogicGraph during its initialization");
            } else {
                node = new MaterialOutputNode(con::GetMaterialFamilyDefinition(materialFamily), position, getNextZIndex());
            }
            
            break;
        IYF_CREATE_NODE_CASE(TextureInput);
        IYF_CREATE_NODE_CASE(TextureCoordinates);
        IYF_CREATE_NODE_CASE(Radians);
        IYF_CREATE_NODE_CASE(Degrees);
        IYF_CREATE_NODE_CASE(Sin);
        IYF_CREATE_NODE_CASE(Cos);
        IYF_CREATE_NODE_CASE(Tan);
        IYF_CREATE_NODE_CASE(Asin);
        IYF_CREATE_NODE_CASE(Acos);
        IYF_CREATE_NODE_CASE(Atan);
        IYF_CREATE_NODE_CASE(Atan2);
        IYF_CREATE_NODE_CASE(Sinh);
        IYF_CREATE_NODE_CASE(Cosh);
        IYF_CREATE_NODE_CASE(Tanh);
        IYF_CREATE_NODE_CASE(Asinh);
        IYF_CREATE_NODE_CASE(Acosh);
        IYF_CREATE_NODE_CASE(Atanh);
        IYF_CREATE_NODE_CASE(Pow);
        IYF_CREATE_NODE_CASE(Exp);
        IYF_CREATE_NODE_CASE(Log);
        IYF_CREATE_NODE_CASE(Exp2);
        IYF_CREATE_NODE_CASE(Log2);
        IYF_CREATE_NODE_CASE(Sqrt);
        IYF_CREATE_NODE_CASE(InverseSqrt);
        IYF_CREATE_NODE_CASE(Abs);
        IYF_CREATE_NODE_CASE(Sign);
        IYF_CREATE_NODE_CASE(Floor);
        IYF_CREATE_NODE_CASE(Trunc);
        IYF_CREATE_NODE_CASE(Round);
        IYF_CREATE_NODE_CASE(RoundEven);
        IYF_CREATE_NODE_CASE(Ceil);
        IYF_CREATE_NODE_CASE(Fract);
        IYF_CREATE_NODE_CASE(Mod);
        IYF_CREATE_NODE_CASE(ModFloat);
        IYF_CREATE_NODE_CASE(ModF);
        IYF_CREATE_NODE_CASE(Min);
        IYF_CREATE_NODE_CASE(MinFloat);
        IYF_CREATE_NODE_CASE(Max);
        IYF_CREATE_NODE_CASE(MaxFloat);
        IYF_CREATE_NODE_CASE(Clamp);
        IYF_CREATE_NODE_CASE(ClampFloat);
        IYF_CREATE_NODE_CASE(Mix);
        IYF_CREATE_NODE_CASE(MixFloat);
        IYF_CREATE_NODE_CASE(Step);
        IYF_CREATE_NODE_CASE(StepFloat);
        IYF_CREATE_NODE_CASE(SmoothStep);
        IYF_CREATE_NODE_CASE(SmoothstepFloat);
        IYF_CREATE_NODE_CASE(Length);
        IYF_CREATE_NODE_CASE(Distance);
        IYF_CREATE_NODE_CASE(Dot);
        IYF_CREATE_NODE_CASE(Cross);
        IYF_CREATE_NODE_CASE(Normalize);
        IYF_CREATE_NODE_CASE(FaceForward);
        IYF_CREATE_NODE_CASE(Reflect);
        IYF_CREATE_NODE_CASE(Refract);
        IYF_CREATE_NODE_CASE(DfDx);
        IYF_CREATE_NODE_CASE(DfDy);
        IYF_CREATE_NODE_CASE(Splitter);
        IYF_CREATE_NODE_CASE(Joiner);
        IYF_CREATE_NODE_CASE(Variable);
        IYF_CREATE_NODE_CASE(Constant);
        IYF_CREATE_NODE_CASE(Add);
        IYF_CREATE_NODE_CASE(Subtract);
        IYF_CREATE_NODE_CASE(Multiply);
        IYF_CREATE_NODE_CASE(Divide);
//         IYF_CREATE_NODE_CASE(Time);
        IYF_CREATE_NODE_CASE(VertexColor);
        IYF_CREATE_NODE_CASE(FragmentCoordinate);
        IYF_CREATE_NODE_CASE(Position);
        IYF_CREATE_NODE_CASE(Normal);
        IYF_CREATE_NODE_CASE(CameraPosition);
        IYF_CREATE_NODE_CASE(CameraProperties);
        IYF_CREATE_NODE_CASE(ScreenDimensions);
        case MaterialNodeType::COUNT:
            throw std::logic_error("COUNT is not a valid NodeType");
    }
    
    assert(node != nullptr);
    
    if (!insertNode(node, key, isDeserializing)) {
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

std::uint32_t MaterialLogicGraph::getConnectorTypeColor(ConnectorType type, bool enabled) const {
    // TODO something more friendly towards colorblind individuals
    if (!enabled) {
        return IM_COL32(64, 64, 64, 255);
    }
    
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

const char* MATERIAL_INFO_FIELD_NAME = "materialInfo";
const char* MATERIAL_INFO_VERSION_FIELD_NAME = "version";
const char* MATERIAL_FAMILY_ID_FIELD_NAME = "familyID";
const char* MATERIAL_FAMILY_HASH_FIELD_NAME = "familyHash";
const int MATERIAL_INFO_VERSION = 1;

void MaterialLogicGraph::serializeJSON(PrettyStringWriter& pw) const {
    LogicGraph::serializeJSON(pw);
    
    pw.String(MATERIAL_INFO_FIELD_NAME);
    
    pw.StartObject();
    
    pw.String(MATERIAL_INFO_VERSION_FIELD_NAME);
    pw.Uint(MATERIAL_INFO_VERSION);
    
    pw.String(MATERIAL_FAMILY_ID_FIELD_NAME);
    pw.Uint64(static_cast<std::uint64_t>(materialFamily));
    
    pw.String(MATERIAL_FAMILY_HASH_FIELD_NAME);
    pw.Uint64(con::GetMaterialFamilyDefinition(materialFamily).computeHash());
    
    pw.EndObject();
}

void MaterialLogicGraph::deserializeJSON(JSONObject& jo) {
    JSONObject& materialInfo = jo[MATERIAL_INFO_FIELD_NAME];
    
    assert(materialInfo[MATERIAL_INFO_VERSION_FIELD_NAME].GetUint() == 1);
    materialFamily = static_cast<MaterialFamily>(materialInfo[MATERIAL_FAMILY_ID_FIELD_NAME].GetUint64());
    
    if (materialInfo[MATERIAL_FAMILY_HASH_FIELD_NAME].GetUint64() != con::GetMaterialFamilyDefinition(materialFamily).computeHash()) {
        LOG_W("A Material Family Definition hash mismatch has been detected. It's possible some connections will no longer be valid.");
    }
    
    LogicGraph::deserializeJSON(jo);
}

std::vector<const TextureInputNode*> MaterialLogicGraph::getTextureNodes() const {
    std::vector<const TextureInputNode*> nodes;
    nodes.reserve(10);
    
    for (const auto& node : getNodes()) {
        if (node.second->getType() == MaterialNodeType::TextureInput) {
            nodes.emplace_back(static_cast<TextureInputNode*>(node.second));
        }
    }
    
    std::sort(nodes.begin(), nodes.end(), [](const TextureInputNode* a, const TextureInputNode* b) {
        return a->getKey() < b->getKey();
    });
    
    return nodes;
}

std::vector<const VariableNode*> MaterialLogicGraph::getVariableNodes() const {
    std::vector<const VariableNode*> nodes;
    nodes.reserve(10);
    
    for (const auto& node : getNodes()) {
        if (node.second->getType() == MaterialNodeType::Variable) {
            nodes.emplace_back(static_cast<VariableNode*>(node.second));
        }
    }
    
    std::sort(nodes.begin(), nodes.end(), [](const VariableNode* a, const VariableNode* b) {
        return a->getKey() < b->getKey();
    });
    
    return nodes;
}

inline bool ProcessDynamicDataNodeValidation(std::stringstream* ss, MaterialNode* node, std::unordered_map<std::string, MaterialNode*>& map, const char* name, bool isTextureNode) {
    if (!node->hasName()) {
        if (ss != nullptr) {
            (*ss) << "Node " << node->getKey() << " is a " << name << " node. " << name << " nodes must be named.\n";
        }
        
        return false;
    } else {
        if (!std::regex_match(node->getName(), SystemRegexes().FunctionAndFileNameRegex)) {
            if (ss != nullptr) {
                (*ss) << "Names of " << name << " nodes must be usable as variable names. \"" << node->getName() << "\" is not.\n";
            }
            
            return false;
        }
        
        if (!isTextureNode && util::startsWith(node->getName(), "padding")) {
            if (ss != nullptr) {
                (*ss) << "Names of " << name << " nodes must not start with the word \"padding\" because it is a reserved word used internally.";
            }
            
            return false;
        }
        
        auto result = map.try_emplace(node->getName(), node);
        if (!result.second) {
            if (ss != nullptr) {
                if (!isTextureNode) {
                    (*ss) << "There's more than one " << name << " node called \"" << node->getName() << "\".\n" << name << " nodes must have unique names.\n";
                } else {
                    (*ss) << "There's more than one texture backed node called \"" << node->getName() << ".\"\nEven if their types differ, all texture backed nodes must have unique names.\n";
                }
            }
            
            return false;
        }
    }
    
    return true;
}

// std::pair<bool, std::string> MaterialFamilyDefinition::setMaterialComponents(const std::vector<MaterialComponent>& componentList) {
//     std::unordered_set<MaterialComponent> uniqueNames;
//     
//     for (const auto& c : componentList) {
//         
//         const auto result = uniqueNames.insert(c);
//         if (!result.second) {
//             std::string error = "All names of material components must be unique. \"";
//             error.append(c.name);
//             error.append("\" was not.");
//             LOG_E("Material component list error: " << error);
//             
//             return {false, error};
//         }
//     }
//     
//     std::vector<MaterialComponent> materialComponentsSorted = componentList;
//     if (packMaterialData(materialComponentsSorted).first != true) {
//         std::stringstream ss;
//         ss <<"The material has more than " << con::MaxMaterialComponents << " components." << "\n\tPlease note possible padding:";
//         for (const auto& c : materialComponentsSorted) {
//             ss << "\n\t\t" << c.name << "; Size: " << c.componentCount << "; Offset: " << c.offset;
//         }
//         
//         const std::string error = ss.str();
//         LOG_E("Material component packing failed: " << error);
//         
//         return {false, error};
//     }
//     
//     materialComponents = materialComponentsSorted;
//     
//     return {true, ""};
// }
// 

bool PackMaterialData(std::stringstream* ss, std::vector<VariableNodeStructField>& fields) {
    for (const VariableNodeStructField& f : fields) {
        if (f.isPadding()) {
            throw std::logic_error("PackMaterialData can't be passed a padding field");
        }
    }
    
    std::sort(fields.begin(), fields.end(), [](const VariableNodeStructField& a, const VariableNodeStructField& b) {
        return (*a.getNode()) > (*b.getNode());
    });

    std::size_t paddingCount = 0;
    std::size_t packedCount = 0;
    
    // 4 component data is already aligned and stored the way we want.
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (fields[i].getComponentCount() == 4) {
            packedCount++;
        } else {
            break;
        }
    }
    
    // When storing 3 component data, if possible, fill the remaining last gap with a 1 byte value. If not, create a padding component
    for (std::size_t i = packedCount; i < fields.size(); ++i) {
        if (fields[i].getComponentCount() == 3 && fields.back().getComponentCount() == 1) {
            VariableNodeStructField last = fields.back();
            fields.pop_back();
            fields.insert(fields.begin() + i + 1, last);
            
            i++;
        } else if (fields[i].getComponentCount() == 3) {
            fields.insert(fields.begin() + i + 1, VariableNodeStructField());
            paddingCount++;
            
            i++;
        } else {
            packedCount = i;
            break;
        }
    }
    
    // Nothing to do with 2 or 1 component values
//     // We can store two 2 component elements one after another
//     for (std::size_t i = packedCount; i < fields.size(); ++i) {
//         if (fields[i].getComponentCount() == 2 && i + 1 < fields.size() && fields[i + 1].getComponentCount() == 2) {
//             i++;
//         }
//     }
    
    // Store the offsets and compute the final size. It may have increased due to padding.
    std::size_t totalSize = 0;
    for (auto& f : fields) {
        f.setOffset(totalSize);
        totalSize += f.getComponentCount();
    }
    
    // See if we still fit into memory.
    if (totalSize <= con::MaxMaterialComponents) {
        while (totalSize < con::MaxMaterialComponents) {
            fields.emplace_back(VariableNodeStructField());
            totalSize++;
        }
        
        return true;
    } else {
        if (ss != nullptr) {
            (*ss) << "The Variable nodes won't fit into " << con::MaxMaterialComponents << " floats. Note possible padding.";
        }
        
        return false;
    }
}

bool MaterialLogicGraph::validate(std::stringstream* ss) const {
    std::vector<VariableNodeStructField> fields;
    return validateInternal(ss, fields);
}

bool MaterialLogicGraph::validateInternal(std::stringstream* ss, std::vector<VariableNodeStructField>& fields) const {
    if (!LogicGraph::validate(ss)) {
        return false;
    }
    
    std::unordered_map<std::string, MaterialNode*> variables;
    std::unordered_map<std::string, MaterialNode*> textures;

    bool result = true;
    
    for (const auto& node : getNodes()) {
        const MaterialNodeType type = node.second->getType();
        
        switch (type) {
            case MaterialNodeType::Variable:
                result &= ProcessDynamicDataNodeValidation(ss, node.second, variables, "Variable", false);
                break;
            case MaterialNodeType::TextureInput:
                result &= ProcessDynamicDataNodeValidation(ss, node.second, textures, "Texture Input", true);
                break;
            default:
                break;
        }
    }
    
    if (!result) {
        return false;
    }
    
    fields.reserve(variables.size());
    
    for (const auto& v : variables) {
        fields.emplace_back(static_cast<VariableNode*>(v.second));
    }
    
    result = PackMaterialData(ss, fields);
    return result;
}

void MaterialLogicGraph::setMaterialFamily(MaterialFamily materialFamily) {
    this->materialFamily = materialFamily;
    
    const bool disconnected = disconnectNode(output->getKey());
    assert(disconnected);
    
    output->changeMaterialFamily(con::GetMaterialFamilyDefinition(this->materialFamily));
}

MaterialFamily MaterialLogicGraph::getMaterialFamily() const {
    return materialFamily;
}

inline std::string MakeOutputVariableName(ShaderLanguage language, MaterialNodeKey key, LogicGraphConnectorID outputID) {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Can't generate a variable for an unsupported language.");
    }
    
    return fmt::format("n{}o{}", key, outputID);
}

inline std::string MakeOutputVariableType(ShaderLanguage language, MaterialNodeConnectorType type) {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Can't generate a variable type for an unsupported language.");
    }
    
    switch (type) {
        case MaterialNodeConnectorType::Float:
            return "float";
        case MaterialNodeConnectorType::Vec2:
            return "vec2";
        case MaterialNodeConnectorType::Vec3:
            return "vec3";
        case MaterialNodeConnectorType::Vec4:
            return "vec4";
        default:
            throw std::runtime_error("Unsupported MaterialNodeConnectorType");
    }
}

void MaterialLogicGraph::arithmeticNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn,
                                              const char* operatorValue,
                                              const std::vector<MaterialNode::Connector>& inputs,
                                              const std::vector<MaterialNode::Connector>& outputs) const {
    if (inputs.size() != 2) {
        throw std::logic_error("An arithmetic node can only have 2 inputs.");
    }
    
    const MaterialNodeConnectorType type = outputs[0].getType();
    ss << MakeOutputVariableType(language, type) << " ";
    ss << MakeOutputVariableName(language, mn.getKey(), 0) << " = ";
    
    KeyConnectorPair pairA = getInputSource(mn.getKey(), 0);
    assert(pairA.isValid());
    ss << MakeOutputVariableName(language, pairA.getNodeKey(), pairA.getConnectorID());
    
    ss << " " << operatorValue << " ";
    
    KeyConnectorPair pairB = getInputSource(mn.getKey(), 0);
    assert(pairB.isValid());
    ss << MakeOutputVariableName(language, pairB.getNodeKey(), pairB.getConnectorID());
    
    ss << ";\n";
}

void MaterialLogicGraph::singleOutputFunctionNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn,
                                                        const char* functionName,
                                                        const std::vector<MaterialNode::Connector>& inputs,
                                                        const std::vector<MaterialNode::Connector>& outputs) const {
    const MaterialNodeConnectorType type = outputs[0].getType();
    ss << MakeOutputVariableType(language, type) << " ";
    ss << MakeOutputVariableName(language, mn.getKey(), 0);
    ss << " = ";
    ss << functionName << "(";
    
    const std::size_t lastVal = inputs.size() - 1;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        KeyConnectorPair pair = getInputSource(mn.getKey(), i);
        assert(pair.isValid());
        
        ss << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID());
        
        if (i != lastVal) {
            ss << ", ";
        }
    }
    
    ss << ");\n";
}

void MaterialLogicGraph::constantNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn, const std::vector<MaterialNode::Connector>& outputs) const {
    const MaterialNodeConnectorType type = outputs[0].getType();
    const std::string typeStr = MakeOutputVariableType(language, type);
    
    ss << typeStr << " ";
    ss << MakeOutputVariableName(language, mn.getKey(), 0);
    ss << " = ";
    
    const ConstantNode& cn = static_cast<const ConstantNode&>(mn);
    
    glm::vec4 value = cn.getDefaultValue();
    switch (type) {
        case MaterialNodeConnectorType::Float:
            ss << value.x << ";\n";
            break;
        case MaterialNodeConnectorType::Vec2:
            ss << typeStr << "(" << value.x << ", " << value.y << ");\n";
            break;
        case MaterialNodeConnectorType::Vec3:
            ss << typeStr << "(" << value.x << ", " << value.y << ", " << value.z << ");\n";
            break;
        case MaterialNodeConnectorType::Vec4:
            ss << typeStr << "(" << value.x << ", " << value.y << ", " << value.z << ", " << value.w << ");\n";
            break;
        default:
            throw std::runtime_error("Invalid MaterialNodeConnectorType");
    }
}

void MaterialLogicGraph::splitterNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn) const {
    const SplitterNode& splitter = static_cast<const SplitterNode&>(mn);
    const std::string varType = MakeOutputVariableType(language, MaterialNodeConnectorType::Float);
    
    KeyConnectorPair pair = getInputSource(mn.getKey(), 0);
    assert(pair.isValid());
    
    const std::size_t modeID = splitter.getSelectedModeID();
    ss << varType << " "
        << MakeOutputVariableName(language, mn.getKey(), 0) << " = " 
        << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID())
        << ".x;\n";
    
    ss << varType << " "
        << MakeOutputVariableName(language, mn.getKey(), 1) << " = " 
        << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID())
        << ".y;\n";
    
    if (modeID >= 1) {
        ss << varType << " "
            << MakeOutputVariableName(language, mn.getKey(), 2) << " = " 
            << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID())
            << ".z;\n";
    }
    
    if (modeID >= 2) {
        ss << varType << " "
            << MakeOutputVariableName(language, mn.getKey(), 3) << " = " 
            << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID())
            << ".w;\n";
    }
    
}

void MaterialLogicGraph::joinerNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn) const {
    const JoinerNode& joiner = static_cast<const JoinerNode&>(mn);
                
    if (joiner.getSelectedModeID() == 0) {
        KeyConnectorPair p1 = getInputSource(mn.getKey(), 0);
        KeyConnectorPair p2 = getInputSource(mn.getKey(), 1);
        
        const std::string varType = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec2);
        ss << varType << " ";
        ss << MakeOutputVariableName(language, mn.getKey(), 0) << " = " ;
        ss << varType << "(";
        ss << MakeOutputVariableName(language, p1.getNodeKey(), p1.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p2.getNodeKey(), p2.getConnectorID());
        ss << ")";
    } else if (joiner.getSelectedModeID() == 1) {
        KeyConnectorPair p1 = getInputSource(mn.getKey(), 0);
        KeyConnectorPair p2 = getInputSource(mn.getKey(), 1);
        KeyConnectorPair p3 = getInputSource(mn.getKey(), 2);
        
        const std::string varType = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec3);
        ss << varType << " ";
        ss << MakeOutputVariableName(language, mn.getKey(), 0) << " = " ;
        ss << varType << "(";
        ss << MakeOutputVariableName(language, p1.getNodeKey(), p1.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p2.getNodeKey(), p2.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p3.getNodeKey(), p3.getConnectorID());
        ss << ")";
    } else if (joiner.getSelectedModeID() == 2) {
        KeyConnectorPair p1 = getInputSource(mn.getKey(), 0);
        KeyConnectorPair p2 = getInputSource(mn.getKey(), 1);
        KeyConnectorPair p3 = getInputSource(mn.getKey(), 2);
        KeyConnectorPair p4 = getInputSource(mn.getKey(), 3);
        
        const std::string varType = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec4);
        ss << varType << " ";
        ss << MakeOutputVariableName(language, mn.getKey(), 0) << " = " ;
        ss << varType << "(";
        ss << MakeOutputVariableName(language, p1.getNodeKey(), p1.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p2.getNodeKey(), p2.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p3.getNodeKey(), p3.getConnectorID()) << ", ";
        ss << MakeOutputVariableName(language, p4.getNodeKey(), p4.getConnectorID());
        ss << ")";
    }
}

#define SINGLE_OUTPUT_FUNC(type, funcName) \
    case MaterialNodeType::type: \
        singleOutputFunctionNodeToCode(language, ss, mn, funcName, inputs, outputs); \
        break;

#define ARITHMETIC_FUNC(type, operatorValue) \
    case MaterialNodeType::type: \
            arithmeticNodeToCode(language, ss, mn, operatorValue, inputs, outputs); \
            break;

CodeGenerationResult MaterialLogicGraph::toCode(ShaderLanguage language) const {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Tried to generate shader code for an unsupported language");
    }
    
    std::stringstream ssErr;
    ssErr << "Cannot generate code because validation of the MaterialLogicGraph failed with error(s):";
    
    std::vector<VariableNodeStructField> fields;
    if (!validateInternal(&ssErr, fields)) {
        return CodeGenerationResult(ssErr.str(), "", std::move(fields), false);
    }
    
    std::stringstream ss;
    const std::vector<MaterialNodeKey> nodes = getTopologicalSort();
    for (auto iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
        const MaterialNodeKey n = *iter;
        const MaterialNode& mn = (*getNode(n));
        
        const std::vector<MaterialNode::Connector>& inputs = mn.getInputs();
        const std::vector<MaterialNode::Connector>& outputs = mn.getOutputs();
        
        const MaterialNodeType type = mn.getType();

        switch(type) {
            SINGLE_OUTPUT_FUNC(Radians, "radians");
            SINGLE_OUTPUT_FUNC(Degrees, "degrees");
            SINGLE_OUTPUT_FUNC(Sin, "sin");
            SINGLE_OUTPUT_FUNC(Cos, "cos");
            SINGLE_OUTPUT_FUNC(Tan, "tan");
            SINGLE_OUTPUT_FUNC(Asin, "asin");
            SINGLE_OUTPUT_FUNC(Acos, "acos");
            SINGLE_OUTPUT_FUNC(Atan, "atan");
            SINGLE_OUTPUT_FUNC(Atan2, "atan2");
            SINGLE_OUTPUT_FUNC(Sinh, "sinh");
            SINGLE_OUTPUT_FUNC(Cosh, "cosh");
            SINGLE_OUTPUT_FUNC(Tanh, "tanh");
            SINGLE_OUTPUT_FUNC(Asinh, "asinh");
            SINGLE_OUTPUT_FUNC(Acosh, "acosh");
            SINGLE_OUTPUT_FUNC(Atanh, "atanh");
            SINGLE_OUTPUT_FUNC(Pow, "pow");
            SINGLE_OUTPUT_FUNC(Exp, "exp");
            SINGLE_OUTPUT_FUNC(Log, "log");
            SINGLE_OUTPUT_FUNC(Exp2, "exp2");
            SINGLE_OUTPUT_FUNC(Log2, "log2");
            SINGLE_OUTPUT_FUNC(Sqrt, "sqrt");
            SINGLE_OUTPUT_FUNC(InverseSqrt, "inversesqrt");
            SINGLE_OUTPUT_FUNC(Abs, "abs");
            SINGLE_OUTPUT_FUNC(Sign, "sign");
            SINGLE_OUTPUT_FUNC(Floor, "floor");
            SINGLE_OUTPUT_FUNC(Trunc, "trunc");
            SINGLE_OUTPUT_FUNC(Round, "round");
            SINGLE_OUTPUT_FUNC(RoundEven, "roundEven");
            SINGLE_OUTPUT_FUNC(Ceil, "ceil");
            SINGLE_OUTPUT_FUNC(Fract, "fract");
            SINGLE_OUTPUT_FUNC(Mod, "mod");
            SINGLE_OUTPUT_FUNC(ModFloat, "mod");
            SINGLE_OUTPUT_FUNC(Min, "min");
            SINGLE_OUTPUT_FUNC(MinFloat, "min");
            SINGLE_OUTPUT_FUNC(Max, "max");
            SINGLE_OUTPUT_FUNC(MaxFloat, "max");
            SINGLE_OUTPUT_FUNC(Clamp, "clamp");
            SINGLE_OUTPUT_FUNC(ClampFloat, "clamp");
            SINGLE_OUTPUT_FUNC(Mix, "mix");
            SINGLE_OUTPUT_FUNC(MixFloat, "mix");
            SINGLE_OUTPUT_FUNC(Step, "step");
            SINGLE_OUTPUT_FUNC(StepFloat, "step");
            SINGLE_OUTPUT_FUNC(SmoothStep, "smoothstep");
            SINGLE_OUTPUT_FUNC(SmoothstepFloat, "smoothstep");
            SINGLE_OUTPUT_FUNC(Length, "length");
            SINGLE_OUTPUT_FUNC(Distance, "distance");
            SINGLE_OUTPUT_FUNC(Dot, "dot");
            SINGLE_OUTPUT_FUNC(Cross, "cross");
            SINGLE_OUTPUT_FUNC(Normalize, "normalize");
            SINGLE_OUTPUT_FUNC(FaceForward, "faceforward");
            SINGLE_OUTPUT_FUNC(Reflect, "reflect");
            SINGLE_OUTPUT_FUNC(Refract, "refract");
            SINGLE_OUTPUT_FUNC(DfDx, "dFdx");
            SINGLE_OUTPUT_FUNC(DfDy, "dFdy");
            ARITHMETIC_FUNC(Add, "+");
            ARITHMETIC_FUNC(Subtract, "-");
            ARITHMETIC_FUNC(Divide, "/");
            ARITHMETIC_FUNC(Multiply, "*");
            case MaterialNodeType::Constant:
                constantNodeToCode(language, ss, mn, outputs);
                break;
            case MaterialNodeType::Splitter:
                splitterNodeToCode(language, ss, mn);
                break;
            case MaterialNodeType::Joiner:
                joinerNodeToCode(language, ss, mn);
                break;
            case MaterialNodeType::ModF: {
                const MaterialNodeConnectorType connectorType = outputs[0].getType(); // All inputs and outputs use the same type in this node
                const std::string varType = MakeOutputVariableType(language, connectorType);
                const std::string integralPart = MakeOutputVariableName(language, mn.getKey(), 0);
                
                const char* funcName = nullptr;
                switch (language) {
                    case ShaderLanguage::GLSLVulkan:
                        funcName = "modf";
                        break;
                    default:
                        throw std::runtime_error("modf not supported in this language");
                }
                
                KeyConnectorPair pair = getInputSource(mn.getKey(), 0);
                
                ss << varType << " " << integralPart << ";\n";
                ss << varType << " " << MakeOutputVariableName(language, mn.getKey(), 1) << " = "
                   << funcName << "(" << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID()) << ", " << integralPart << ");\n";
                break;
            }
            case MaterialNodeType::Output: {
                const MaterialOutputNode& mnOut = static_cast<const MaterialOutputNode&>(mn);
                const LogicGraphConnectorID normalConnectorID = mnOut.getNormalConnectorID();
                const bool normalConnectorInUse = (normalConnectorID != MaterialNodeBase::Connector::InvalidID) && getInputSource(mn.getKey(), normalConnectorID).isValid();
                
                for (std::size_t c = 0; c < inputs.size(); ++c) {
                    const KeyConnectorPair pair = getInputSource(mn.getKey(), c);
                    const MaterialNodeKey key = pair.getNodeKey();
                    const LogicGraphConnectorID connectorID = pair.getConnectorID();
                    
                    if (pair.isValid()) {
                        if (normalConnectorInUse && (inputs[c].getID() == normalConnectorID)) {
                            const std::string varType = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec3);
                            ss << varType << " " << inputs[c].getName() << " = " << MakeOutputVariableName(language, key, connectorID) << ";\n";
                        } else {
                            ss << inputs[c].getName() << " = " << MakeOutputVariableName(language, key, connectorID) << ";\n";
                        }
                    }
                }
                
                if (normalConnectorInUse) {
                    ss << "#define " << GetShaderMacroName(ShaderMacro::NormalSetByMaterialGraph) <<"\n";
                }
                
                break;
            }
            case MaterialNodeType::Variable: {
                assert(language == ShaderLanguage::GLSLVulkan);
                assert(mn.hasName());
                
                const MaterialNodeConnectorType connectorType = outputs[0].getType();
                
                ss << MakeOutputVariableType(language, connectorType) << " " << MakeOutputVariableName(language, mn.getKey(), 0);
                ss << " = ";
                ss << "materials.materials[ID.material]." << mn.getName() << ";\n";
                
                break;
            }
            case MaterialNodeType::TextureInput: {
                assert(language == ShaderLanguage::GLSLVulkan);
                assert(mn.hasName());
                
                KeyConnectorPair pair = getInputSource(mn.getKey(), 0);
                
                const MaterialNodeConnectorType connectorType = outputs[0].getType();
                const char* swizzle = nullptr;
                
                switch (connectorType) {
                    case MaterialNodeConnectorType::Float:
                        swizzle = ".x";
                        break;
                    case MaterialNodeConnectorType::Vec2:
                        swizzle = ".xy";
                        break;
                    case MaterialNodeConnectorType::Vec3:
                        swizzle = ".xyz";
                        break;
                    case MaterialNodeConnectorType::Vec4:
                        swizzle = ".xyzw";
                        break;
                    default:
                        throw std::logic_error("Unsupported MaterialNodeConnectorType");
                }
                
                ss << MakeOutputVariableType(language, connectorType) << " " << MakeOutputVariableName(language, mn.getKey(), 0);
                ss << " = ";
                ss << "texture(" << mn.getName() << ", " << MakeOutputVariableName(language, pair.getNodeKey(), pair.getConnectorID()) << ")" << swizzle << ";\n";
                
                break;
            }
            case MaterialNodeType::ScreenDimensions: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec2);
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 0);
                ss << " = " << type << "(cameraAndLights.framebufferDimensions.xy);\n";
                break;
            }
            case MaterialNodeType::CameraPosition: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec3);
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 0);
                ss << " = cameraAndLights.cameraPosition;\n";
                break;
            }
            case MaterialNodeType::FragmentCoordinate: {
                assert(language == ShaderLanguage::GLSLVulkan);

                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec4);
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 0);
                ss << " = gl_FragCoord;\n";
                break;
            }
            case MaterialNodeType::CameraProperties: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Float);
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 0) << " = cameraAndLights.zNear;\n";
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 1) << " = cameraAndLights.zFar;\n";
                ss << type << " " << MakeOutputVariableName(language, mn.getKey(), 2) << " = cameraAndLights.fieldOfView;\n";
                break;
            }
            case MaterialNodeType::TextureCoordinates: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec2);
                const std::string varName = MakeOutputVariableName(language, mn.getKey(), 0);
                
                ss << "#ifdef " << GetShaderMacroName(ShaderMacro::TextureCoordinatesAvailable) << "\n";
                ss << type << " " << varName << " = " << "fragmentInput.UV;\n";
                ss << "#else\n";
                ss << type << " " << varName << " = " << type << "(0.0, 0.0);\n";
                ss << "#endif\n";
                break;
            }
            case MaterialNodeType::VertexColor: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec4);
                const std::string varName = MakeOutputVariableName(language, mn.getKey(), 0);
                
                ss << "#ifdef " << GetShaderMacroName(ShaderMacro::VertexColorAvailable) << "\n";
                ss << type << " " << varName << " = " << "fragmentInput.vertColor;\n";
                ss << "#else\n";
                ss << type << " " << varName << " = " << type << "(1.0, 1.0, 1.0, 1.0);\n";
                ss << "#endif\n";
                break;
            }
            case MaterialNodeType::Position: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec3);
                const std::string varName = MakeOutputVariableName(language, mn.getKey(), 0);
                
                ss << "#ifdef " << GetShaderMacroName(ShaderMacro::WorldSpacePositionAvailable) << "\n";
                ss << type << " " << varName << " = " << "fragmentInput.positionWS;\n";
                ss << "#else\n";
                ss << type << " " << varName << " = " << type << "(0.0, 0.0, 0.0);\n";
                ss << "#endif\n";
                break;
            }
            case MaterialNodeType::Normal: {
                const std::string type = MakeOutputVariableType(language, MaterialNodeConnectorType::Vec3);
                const std::string varName = MakeOutputVariableName(language, mn.getKey(), 0);
                
                ss << "#ifdef " << GetShaderMacroName(ShaderMacro::NormalAvailable) << "\n";
                ss << "#if (" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0)\n";
                ss << type << " " << varName << " = " << "fragmentInput.TBN[2];\n";
                ss << "#else //" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << "\n";
                ss << type << " " << varName << " = " << "fragmentInput.normalWS;\n";
                ss << "#endif //" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << "\n";
                ss << "#else //" << GetShaderMacroName(ShaderMacro::NormalAvailable) << "\n";
                ss << type << " " << varName << " = " << type << "(0.0, 0.0, 0.0);\n";
                ss << "#endif //"<< GetShaderMacroName(ShaderMacro::NormalAvailable) << "\n";
                break;
            }
            case MaterialNodeType::COUNT:
                break;
        }// MODF
    }
    
    std::stringstream ssStr;
    ssStr << "struct Material {\n";
    std::size_t paddingCount = 0;
    for (const auto& f : fields) {
        ssStr << "    ";
        
        if (f.isPadding()) {
            ssStr << "float padding" << paddingCount << ";\n";
            paddingCount++;
        } else {
            const std::size_t componentCount = f.getComponentCount();
            
            switch (componentCount) {
                case 1:
                    ssStr << "float";
                    break;
                case 2:
                    ssStr << "vec2";
                    break;
                case 3:
                    ssStr << "vec3";
                    break;
                case 4:
                    ssStr << "vec4";
                    break;
            }
            
            ssStr << " " << f.getNode()->getName();
            ssStr << "; // Key: " << f.getNode()->getKey() << "\n";
        };
    }
    ssStr << "};\n";
    
    return CodeGenerationResult(ss.str(), ssStr.str(), std::move(fields), true);
}

}
