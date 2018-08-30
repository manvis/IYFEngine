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
    GenTypeMaterialNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t inputsToChange, std::size_t floatInputs,
                        std::size_t outputsToChange, const std::vector<const char*>& ioNames, std::size_t selectedMode = 2)
        : MaterialNodeBase(key, position, zIndex, selectedMode), inputsToChange(inputsToChange), outputsToChange(outputsToChange) {
        
        std::size_t counter = 0;
        const auto connectorType = static_cast<MaterialNodeConnectorType>(selectedMode);
        for (std::size_t i = 0; i < inputsToChange; ++i) {
            addInput(getNextLocalizationHandle(ioNames, counter), connectorType);
        }
        
        for (std::size_t i = 0; i < floatInputs; ++i) {
            addInput(getNextLocalizationHandle(ioNames, counter), MaterialNodeConnectorType::Float);
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
        addOutput(LH("rgb", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::TextureInput;
    }
    
    virtual bool supportsMultipleModes() const final override {
        return true;
    }
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override {
        static std::vector<ModeInfo> modes = {
            ModeInfo(LH("r", MaterialNodeLocalizationNamespace), LH("float_texture_doc", MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("rg",  MaterialNodeLocalizationNamespace), LH("vec2_texture_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("rgb",  MaterialNodeLocalizationNamespace), LH("vec3_texture_doc",  MaterialNodeLocalizationNamespace)),
            ModeInfo(LH("rgba",  MaterialNodeLocalizationNamespace), LH("vec4_texture_doc",  MaterialNodeLocalizationNamespace)),
        };
        
        return modes;
    }
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override {
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

class CrossNode : public MaterialNodeBase {
public:
    CrossNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : MaterialNodeBase(key, position, zIndex) {
        addInput(LH("x", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
        addInput(LH("y", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec3);
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
        addInput(LH("vector", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec2);
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
        addInput(LH("x", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addInput(LH("y", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addInput(LH("z", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
        addInput(LH("w", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Float);
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

class MaterialDataNode : public MaterialNodeBase {
public:
    MaterialDataNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) 
        : MaterialNodeBase(key, position, zIndex) {
        for (std::size_t i = 0; i < (con::MaxMaterialComponents / 4); ++i) {
            addOutput(LH("vec4", MaterialNodeLocalizationNamespace), MaterialNodeConnectorType::Vec4);
        }
    }
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::MaterialData;
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

MaterialLogicGraph::MaterialLogicGraph(MaterialPipelineDefinition definition) : definition(std::move(definition)) {
    const char* ns = MaterialNodeLocalizationNamespace;
    
    addNodeTypeInfo(MaterialNodeType::Output, "material_output_node", "material_output_node_doc", ns, MaterialNodeGroup::Output, false, false);
    
    const MaterialNodeGroup input = MaterialNodeGroup::Input;
    addNodeTypeInfo(MaterialNodeType::TextureInput, "texture_input_node", "texture_input_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::NormalMapInput, "normal_map_input_node", "normal_map_input_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::PerFrameData, "per_frame_data_node", "per_frame_data_node_doc", ns, input);
    addNodeTypeInfo(MaterialNodeType::MaterialData, "material_data_node", "material_data_node_doc", ns, input);
    
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
    
    validateNodeTypeInfo();
    assert(getNextKey() == 0);
    
    MaterialOutputNode* outputNode = new MaterialOutputNode(this->definition, Vec2(0.0f, 0.0f), getNextZIndex());
    insertNode(outputNode, getNextKey(), false);
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
                node = new MaterialOutputNode(this->definition, position, getNextZIndex());
            }
            
            break;
        IYF_CREATE_NODE_CASE(TextureInput);
        IYF_CREATE_NODE_CASE(NormalMapInput);
        IYF_CREATE_NODE_CASE(PerFrameData);
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
        IYF_CREATE_NODE_CASE(MaterialData);
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

MaterialEditor::MaterialEditor(NodeEditorSettings settings) : LogicGraphEditor(settings) {
    nameBuffer.resize(128, '\0');
    
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
    
    con::GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline::Toon).computeHash();
    
    graph->addNode(MaterialNodeType::PerFrameData, Vec2(0.0f, 150.0f));
//     LOG_V("LOCALIZED: " << nn->getLocalizationHandle().getHashValue());
}

std::string MaterialEditor::getWindowName() const {
    return LOC_SYS(LH("material_editor", MaterialNodeLocalizationNamespace));
}

void MaterialEditor::onButtonClick(LogicGraphEditorButton button) {
    switch (button) {
        case LogicGraphEditorButton::Load:
            ImGui::OpenPopup(LOC_SYS(LH("load_material_editor", MaterialNodeLocalizationNamespace)).c_str());
            break;
        case LogicGraphEditorButton::Save:
            ImGui::OpenPopup(LOC_SYS(LH("save_material_editor", MaterialNodeLocalizationNamespace)).c_str());
            break;
    }
    
    nameBuffer[0] = '\0';
}

void MaterialEditor::onDrawButtonRow() {
    drawFilePopup(true);
    drawFilePopup(false);
}

void MaterialEditor::drawFilePopup(bool isLoadPopup) {
    const std::string popupName = isLoadPopup ? 
        LOC_SYS(LH("load_material_editor", MaterialNodeLocalizationNamespace)) :
        LOC_SYS(LH("save_material_editor", MaterialNodeLocalizationNamespace));
    
    if (ImGui::BeginPopupModal(popupName.c_str())) {
        const std::string actionName = isLoadPopup ?
        LOC_SYS(LH("load", MaterialNodeLocalizationNamespace)) :
        LOC_SYS(LH("save", MaterialNodeLocalizationNamespace));
    
        const std::string nameFieldName = LOC_SYS(LH("name", MaterialNodeLocalizationNamespace));
        if (ImGui::InputText(nameFieldName.c_str(), nameBuffer.data(), nameBuffer.size())) {
            // TODO validate here or on save?
        }
        
        if (ImGui::BeginChild("##material_instance_def_list", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()))) {
            ImGui::Columns(3);
            ImGui::Separator();
            
            ImGui::Text("%s", nameFieldName.c_str());
            ImGui::NextColumn();
            
            std::string pipelineName = LOC_SYS(LH("pipeline", MaterialNodeLocalizationNamespace));
            ImGui::Text("%s", pipelineName.c_str());
            ImGui::NextColumn();
            
            const std::string hashName = LOC_SYS(LH("hash", MaterialNodeLocalizationNamespace));
            ImGui::Text("%s", hashName.c_str());
            ImGui::NextColumn();
            ImGui::Separator();
            
            // TODO list all definitions
            
            ImGui::Columns();
        }
        ImGui::Separator();
        
        ImGui::EndChild();
        
        if (ImGui::Button(actionName.c_str())) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button(LOC_SYS(LH("cancel", MaterialNodeLocalizationNamespace)).c_str())) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

MaterialEditor::~MaterialEditor() {}

std::unique_ptr<MaterialLogicGraph> MaterialEditor::makeNewGraph(const NewGraphSettings&) {
    return std::make_unique<MaterialLogicGraph>(con::GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline::Toon));
}

}
