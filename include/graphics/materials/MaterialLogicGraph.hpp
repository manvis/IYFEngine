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

#ifndef IYF_MATERIAL_LOGIC_GRAPH_HPP
#define IYF_MATERIAL_LOGIC_GRAPH_HPP

#include "graphics/materials/MaterialFamilyDefinition.hpp"

#include "utilities/logicGraph/LogicGraph.hpp"

namespace iyf {
class MaterialOutputNode;

enum class MaterialNodeConnectorType : std::uint8_t {
    Float = 0, /// < Single floating point value
    Vec2  = 1, /// < 2 component floating point vector
    Vec3  = 2, /// < 3 component floating point vector
    Vec4  = 3, /// < 4 component floating point vector
    COUNT
};

enum class MaterialNodeType : std::uint32_t {
    Output             = 0,
    TextureInput       = 1,
    ScreenDimensions   = 2,
    TextureCoordinates = 3,
    Radians            = 4,
    Degrees            = 5,
    Sin                = 6,
    Cos                = 7,
    Tan                = 8,
    Asin               = 9,
    Acos               = 10,
    Atan               = 11,
    Atan2              = 12,
    Sinh               = 13,
    Cosh               = 14,
    Tanh               = 15,
    Asinh              = 16,
    Acosh              = 17,
    Atanh              = 18,
    Pow                = 19,
    Exp                = 20,
    Log                = 21,
    Exp2               = 22,
    Log2               = 23,
    Sqrt               = 24,
    InverseSqrt        = 25,
    Abs                = 26,
    Sign               = 27,
    Floor              = 28,
    Trunc              = 29,
    Round              = 30,
    RoundEven          = 31,
    Ceil               = 32,
    Fract              = 33,
    Mod                = 34,
    ModFloat           = 35,
    ModF               = 36,
    Min                = 37,
    MinFloat           = 38,
    Max                = 39,
    MaxFloat           = 40,
    Clamp              = 41,
    ClampFloat         = 42,
    Mix                = 43,
    MixFloat           = 44,
    Step               = 45,
    StepFloat          = 46,
    SmoothStep         = 47,
    SmoothstepFloat    = 48,
    Length             = 49,
    Distance           = 50,
    Dot                = 51,
    Cross              = 52,
    Normalize          = 53,
    FaceForward        = 54,
    Reflect            = 55,
    Refract            = 56,
    DfDx               = 57,
    DfDy               = 58,
    Splitter           = 59,
    Joiner             = 60,
    Variable           = 61,
    Constant           = 62,
    Add                = 63,
    Subtract           = 64,
    Multiply           = 65,
    Divide             = 66,
    VertexColor        = 67,
    FragmentCoordinate = 68,
    Position           = 69,
    Normal             = 70,
    CameraPosition     = 71,
    CameraProperties   = 72,
    //Time               = ??, // I'm not sure about precision.
    COUNT
};

enum class MaterialNodeGroup : std::uint8_t {
    Input            = 0,
    VectorComponents = 1,
    Trigonometry     = 2,
    Exponential      = 3,
    CommonMath       = 4,
    Geometry         = 5,
    Derivatives      = 6,
    Output           = 7,
    Arithmetic       = 8,
    COUNT
};

using MaterialNodeKey = std::uint32_t;
using MaterialNodeConnector = LogicGraphConnector<MaterialNodeConnectorType>;
using MaterialNode = LogicGraphNode<MaterialNodeType, MaterialNodeConnector, MaterialNodeKey>;
using MaterialGraphNodeTypeInfo = LogicGraphNodeTypeInfo<MaterialNode, MaterialNodeGroup>;

class MaterialNodeBase : public MaterialNode {
public:
    MaterialNodeBase(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex, std::size_t selectedMode = 0);
};

class TextureInputNode : public MaterialNodeBase {
public:
    TextureInputNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex);
    
    virtual MaterialNodeType getType() const final override;
    virtual bool supportsMultipleModes() const final override;
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override;
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override;
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    inline StringHash getDefaultTexture() const {
        return defaultTexture;
    }
    
    inline void setDefaultTexture(StringHash defaultTexture) {
        this->defaultTexture = defaultTexture;
    }
protected:
    friend class MaterialLogicGraph;
    friend class MaterialEditor;
    
    StringHash defaultTexture;
};

class VariableNode : public MaterialNodeBase {
public:
    VariableNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex);
    
    virtual MaterialNodeType getType() const override;
    virtual bool supportsMultipleModes() const final override;
    
    virtual std::vector<ModeInfo>& getSupportedModes() const final override;
    
    virtual bool onModeChange(std::size_t, std::size_t requestedModeID, bool) final override;
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;

    inline glm::vec4 getDefaultValue() const {
        return value;
    }
    
    inline void setDefaultValue(glm::vec4 value) {
        this->value = value;
    }
    
    inline std::uint32_t getComponentCount() const {
        const Connector& ca = getOutputs()[0];
        
        return static_cast<std::uint32_t>(ca.getType()) + 1;
    }
    
    inline friend bool operator<(const VariableNode& a, const VariableNode& b) {
        const Connector& ca = a.getOutputs()[0];
        const Connector& cb = b.getOutputs()[0];
        
        return static_cast<std::uint8_t>(ca.getType()) < static_cast<std::uint8_t>(cb.getType());
    }
    
    inline friend bool operator>(const VariableNode& a, const VariableNode& b) {
        const Connector& ca = a.getOutputs()[0];
        const Connector& cb = b.getOutputs()[0];
        
        return static_cast<std::uint8_t>(ca.getType()) > static_cast<std::uint8_t>(cb.getType());
    }
protected:
    glm::vec4 value;
};

class VariableNodeStructField {
public:
    inline VariableNodeStructField() : offset(0), node(nullptr) {}
    inline VariableNodeStructField(VariableNode* node) : offset(0), node(node) {}
    
    inline std::size_t getComponentCount() const {
        if (isPadding()) {
            return 1;
        } else {
            return node->getComponentCount();
        }
    }
    
    inline bool isPadding() const {
        return node == nullptr;
    }
    
    inline const VariableNode* getNode() {
        return node;
    }
    
    inline std::size_t getOffset() const {
        return offset;
    }
    
    inline void setOffset(std::size_t offset) {
        this->offset = offset;
    }
    
    inline const VariableNode* getNode() const {
        return node;
    }
private:
    std::size_t offset;
    VariableNode* node;
};

class ConstantNode : public VariableNode {
public:
    ConstantNode(MaterialNodeKey key, Vec2 position, std::uint32_t zIndex) : VariableNode(key, position, zIndex) {}
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Constant;
    }
};

class MaterialOutputNode : public MaterialNodeBase {
public:
    /// The MaterialOutputNode must always have a key equal to 0. Moreover, exactly 1 
    /// MaterialOutputNode has to exist in the LogicGraph.
    MaterialOutputNode(const MaterialFamilyDefinition& definition, Vec2 position, std::uint32_t zIndex);
    
    virtual MaterialNodeType getType() const final override {
        return MaterialNodeType::Output;
    }
    
    void changeMaterialFamily(const MaterialFamilyDefinition& definition);
    LogicGraphConnectorID getNormalConnectorID() const;
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    CullModeFlags cullMode;
    CompareOp depthCompareOp;
    bool depthWriteEnabled;
    bool depthTestEnabled;
    
    bool blendEnabled;
    BlendFactor srcColorBlendFactor;
    BlendFactor dstColorBlendFactor;
    BlendFactor srcAlphaBlendFactor;
    BlendFactor dstAlphaBlendFactor;
    BlendOp colorBlendOp;
    BlendOp alphaBlendOp;
private:
    void buildConnectors(const MaterialFamilyDefinition& definition);
    
    LogicGraphConnectorID normalID;
};

class CodeGenerationResult {
public:
    CodeGenerationResult() : success(false) {}
    CodeGenerationResult(std::string code, std::string structCode, std::vector<VariableNodeStructField> fields, bool success) 
        : code(std::move(code)), structCode(std::move(structCode)), fields(std::move(fields)), success(success) {}
    
    inline const std::string& getCode() const {
        return code;
    }
    
    inline const std::string& getMaterialStructCode() const {
        return structCode;
    }
    
    inline bool isSuccessful() const {
        return success;
    }
    
    inline const std::vector<VariableNodeStructField>& getFields() const {
        return fields;
    }
private:
    std::string code;
    std::string structCode;
    std::vector<VariableNodeStructField> fields;
    bool success;
};

class MaterialLogicGraph : public LogicGraph<MaterialNode, MaterialGraphNodeTypeInfo> {
public:
    MaterialLogicGraph(MaterialFamily materialFamily);
    virtual ~MaterialLogicGraph();
    
    static const char* GetMaterialNodeLocalizationNamespace();
    
    virtual std::string getConnectorTypeName(MaterialNodeConnectorType type) const final override;
    virtual std::uint32_t getConnectorTypeColor(ConnectorType type, bool enabled) const final override;
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;
    virtual bool validate(std::stringstream* ss) const final override;
    
    std::vector<const TextureInputNode*> getTextureNodes() const;
    std::vector<const VariableNode*> getVariableNodes() const;
    
    void setMaterialFamily(MaterialFamily materialFamily);
    MaterialFamily getMaterialFamily() const;
    
    CodeGenerationResult toCode(ShaderLanguage language) const;
protected:
    virtual MaterialNode* addNodeImpl(NodeKey key, MaterialNodeType type, const Vec2& position, bool isDeserializing) final override;
    
    void singleOutputFunctionNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn,
                                        const char* functionName,
                                        const std::vector<MaterialNode::Connector>& inputs,
                                        const std::vector<MaterialNode::Connector>& outputs) const;
                                        
    void arithmeticNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn,
                              const char* operatorValue,
                              const std::vector<MaterialNode::Connector>& inputs,
                              const std::vector<MaterialNode::Connector>& outputs) const;
    
    void constantNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn, const std::vector<MaterialNode::Connector>& outputs) const;
    void splitterNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn) const;
    void joinerNodeToCode(ShaderLanguage language, std::stringstream& ss, const MaterialNode& mn) const;
    
    bool validateInternal(std::stringstream* ss, std::vector<VariableNodeStructField>& fields) const;
private:
    MaterialOutputNode* output;
    MaterialFamily materialFamily;
};
}

#endif // IYF_MATERIAL_LOGIC_GRAPH_HPP
