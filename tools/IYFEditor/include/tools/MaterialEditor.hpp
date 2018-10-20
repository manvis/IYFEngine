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

#ifndef IYF_MATERIAL_EDITOR_HPP
#define IYF_MATERIAL_EDITOR_HPP

#include "graphics/MaterialFamilyDefinition.hpp"

#include "utilities/logicGraph/LogicGraph.hpp"
#include "utilities/logicGraph/LogicGraphEditor.hpp"

namespace iyf::editor {
enum class MaterialNodeConnectorType : std::uint8_t {
    Float = 0, /// < Single floating point value
    Vec2  = 1, /// < 2 component floating point vector
    Vec3  = 2, /// < 3 component floating point vector
    Vec4  = 3, /// < 4 component floating point vector
    COUNT
};

enum class MaterialNodeType : std::uint32_t {
    Output          = 0,
    TextureInput    = 1,
    NormalMapInput  = 2,
    PerFrameData    = 3,
    Radians         = 4,
    Degrees         = 5,
    Sin             = 6,
    Cos             = 7,
    Tan             = 8,
    Asin            = 9,
    Acos            = 10,
    Atan            = 11,
    Atan2           = 12,
    Sinh            = 13,
    Cosh            = 14,
    Tanh            = 15,
    Asinh           = 16,
    Acosh           = 17,
    Atanh           = 18,
    Pow             = 19,
    Exp             = 20,
    Log             = 21,
    Exp2            = 22,
    Log2            = 23,
    Sqrt            = 24,
    InverseSqrt     = 25,
    Abs             = 26,
    Sign            = 27,
    Floor           = 28,
    Trunc           = 29,
    Round           = 30,
    RoundEven       = 31,
    Ceil            = 32,
    Fract           = 33,
    Mod             = 34,
    ModFloat        = 35,
    ModF            = 36,
    Min             = 37,
    MinFloat        = 38,
    Max             = 39,
    MaxFloat        = 40,
    Clamp           = 41,
    ClampFloat      = 42,
    Mix             = 43,
    MixFloat        = 44,
    Step            = 45,
    StepFloat       = 46,
    SmoothStep      = 47,
    SmoothstepFloat = 48,
    Length          = 49,
    Distance        = 50,
    Dot             = 51,
    Cross           = 52,
    Normalize       = 53,
    FaceForward     = 54,
    Reflect         = 55,
    Refract         = 56,
    DfDx            = 57,
    DfDy            = 58,
    Splitter        = 59,
    Joiner          = 60,
    MaterialData    = 61,
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
    COUNT
};

using MaterialNodeKey = std::uint32_t;
using MaterialNodeConnector = LogicGraphConnector<MaterialNodeConnectorType>;
using MaterialNode = LogicGraphNode<MaterialNodeType, MaterialNodeConnector, MaterialNodeKey>;
using MaterialGraphNodeTypeInfo = LogicGraphNodeTypeInfo<MaterialNode, MaterialNodeGroup>;

class MaterialLogicGraph : public LogicGraph<MaterialNode, MaterialGraphNodeTypeInfo> {
public:
    MaterialLogicGraph(MaterialFamilyDefinition definition);
    virtual ~MaterialLogicGraph();
    
    virtual std::string getConnectorTypeName(MaterialNodeConnectorType type) const final override;
    virtual std::uint32_t getConnectorTypeColor(ConnectorType type, bool enabled) const final override;
    
    /*virtual NodeConnectionResult addConnection(MaterialNode* source, GraphNodeConnectorID outputID, MaterialNode* destination, GraphNodeConnectorID inputID) final override {
        return NodeConnectionResult::Success;
    }*/
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;
protected:
    virtual MaterialNode* addNodeImpl(NodeKey key, MaterialNodeType type, const Vec2& position, bool isDeserializing) final override;
private:
    MaterialFamilyDefinition definition;
};

class MaterialEditor : public LogicGraphEditor<MaterialLogicGraph> {
public:
    MaterialEditor(NodeEditorSettings settings = NodeEditorSettings());
    virtual std::string getWindowName() const final override;
    virtual ~MaterialEditor();
protected:
    virtual void onButtonClick(LogicGraphEditorButtonFlagBits button) final override;
    virtual void onDrawButtonRow() final override;
    void drawFilePopup(bool isLoadPopup);
    
    virtual std::unique_ptr<MaterialLogicGraph> makeNewGraph(const NewGraphSettings& settings) final override;
    
    std::vector<char> nameBuffer;
};

}

#endif // IYF_MATERIAL_EDITOR_HPP
