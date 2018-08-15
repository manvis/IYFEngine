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

#include "utilities/logicGraph/LogicGraph.hpp"
#include "utilities/logicGraph/LogicGraphEditor.hpp"

namespace iyf {
class MaterialPipelineDefinition;
}

namespace iyf::editor {
enum class MaterialNodeConnectorType : std::uint8_t {
    Float = 0, /// < Single floating point value
    Vec2  = 1, /// < 2 component floating point vector
    Vec3  = 2, /// < 3 component floating point vector
    Vec4  = 3, /// < 4 component floating point vector
    COUNT
};

enum class MaterialNodeType : std::uint32_t {
    Output         = 0,
    TextureInput   = 1,
    NormalMapInput = 2,
    PerFrameData   = 3,
    Radians        = 4,
    Degrees        = 5,
    Sine           = 6,
    Cosine         = 7,
    Tangent        = 8,
    Arcsine        = 9,
    Arccosine      = 10,
    Arctangent     = 11,
    Arctangent2    = 12,
    COUNT
};

using MaterialNodeKey = std::uint32_t;
using MaterialNodeConnector = LogicGraphConnector<MaterialNodeConnectorType>;
using MaterialNode = LogicGraphNode<MaterialNodeType, MaterialNodeConnector, MaterialNodeKey>;

class MaterialLogicGraph : public LogicGraph<MaterialNode> {
public:
    MaterialLogicGraph(const MaterialPipelineDefinition& definition);
    virtual ~MaterialLogicGraph();
    
    virtual MaterialNode* addNode(MaterialNodeType type, const Vec2& position) final override;
    
    virtual std::string getConnectorTypeName(MaterialNodeConnectorType type) const final override;
    virtual std::uint32_t getConnectorTypeColor(ConnectorType type) const final override;
    
    /*virtual NodeConnectionResult addConnection(MaterialNode* source, GraphNodeConnectorID outputID, MaterialNode* destination, GraphNodeConnectorID inputID) final override {
        return NodeConnectionResult::Success;
    }*/
};

class MaterialEditor : public LogicGraphEditor<MaterialLogicGraph> {
public:
    MaterialEditor(NodeEditorSettings settings = NodeEditorSettings());
    virtual std::string getWindowName() const final override;
    virtual ~MaterialEditor();
protected:
    virtual std::unique_ptr<MaterialLogicGraph> makeNewGraph(const NewGraphSettings& settings) final override;
};

}

#endif // IYF_MATERIAL_EDITOR_HPP
