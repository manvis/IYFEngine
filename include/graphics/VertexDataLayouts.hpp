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

#ifndef VERTEX_DATA_LAYOUTS_HPP
#define VERTEX_DATA_LAYOUTS_HPP

#include "utilities/hashing/Hashing.hpp"
#include "utilities/DataSizes.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/GraphicsAPI.hpp"
#include "graphics/VertexDataTypes.hpp"

#include <string>
#include <stdexcept>
#include <bitset>
#include <initializer_list>

namespace iyf {
/// Each different Mesh vertex layout struct from VertexDataTypes.hpp must have a corresponding VertexDataLayout
/// entry and an entry in the VertexDataLayoutDefinitions array.
///
/// \warning Feel free to add custom layouts, but do NOT remove or re-arrange them mid-project. These IDs are 
/// written to some files and such changes may make them invalid
///
/// \warning DO NOT skip IDs. They must be contiguous
///
/// \warning The underlying type must be std::uint8_t. Do not change it.
enum class VertexDataLayout : std::uint8_t {
    /// Corresponds to MeshVertex
    MeshVertex = 0,
    /// Corresponds to MeshVertexWithBones
    MeshVertexWithBones = 1,
    /// Corresponds to SimpleMeshVertex
    SimpleMeshVertex = 2,
    /// Corresponds to MinimalMeshVertex
    MinimalMeshVertex = 3,
    /// Corresponds to ColoredDebugVertex
    ColoredDebugVertex = 4,
    /// Corresponds to MeshVertexColored
    MeshVertexColored = 5,
    /// Corresponds to MeshVertexColoredWithBones
    MeshVertexColoredWithBones = 6,
    /// Corresponds to SimpleMeshVertexColored
    SimpleMeshVertexColored = 7,
    /// Corresponds to MeshVertex. However, generated VertexInputStateCreateInfo objects only use the position component.
    MeshVertexPositionOnly = 8,
    COUNT
};

enum class NormalMappingMode {
    NotSupported        = 0, /// < Impossible to do normal mapping with this VertexDataLayout
    Regular             = 1, /// < This VertexDataLayout has Normal, Tangent and Bitangent data and can be used for normal mapping
    BitangentRecovering = 2  /// < This VertexDataLayout has Normal and TangentAndBias data
};

/// These are used when creating new shaders and pipelines. Each attribute type must also have a corresponding
/// entry in VertexAttributeNames and in VertexShaderAttributeNames
enum class VertexAttributeType {
    Position3D = 0,
    Normal = 1,
    Tangent = 2,
    Bitangent = 3,
    UV = 4,
    BoneID = 5,
    BoneWeight = 6,
    /// A potential VRAM space optimization is to store a bias in A component of A2B10G10R10 and recover
    /// the bitangent from normal, tangent and the said bias in the vertex shader. Currently, no vertex 
    ///layouts in the engine use this attribute and this is kept as a reminder of the possibility
    TangentAndBias = 7,
    Color = 8,
    // Other potential, but not yet used, attributes are: Position2D, multiple UV sets, multiple vertex colors...
    // Make sure to only insert them before COUNT!!!
    COUNT
};

namespace con {
/// Translation strings for vertex attribute names
StringHash GetVertexAttributeNameHash(VertexAttributeType type);

/// Obtain vertex input variable names. The shader generator uses these when creating the vertex shader.
///
/// \warning feel free to add new ones that correspond to your attributes, but DO NOT edit existing names
const std::string& GetVertexAttributeName(VertexAttributeType type);
}

struct VertexAttribute {
    VertexAttribute(VertexAttributeType type, Format format, std::uint32_t offset) : type(type), format(format), offset(offset) {}
    
    VertexAttributeType type;
    Format format;
    std::uint32_t offset;
};

class VertexDataLayoutDefinition {
public:
    VertexDataLayoutDefinition(std::string name, std::uint32_t size, std::initializer_list<VertexAttribute> attributeList);
    
    inline const std::vector<VertexAttribute>& getAttributes() const {
        return attributes;
    }
    
    inline Bytes getSize() const {
        return Bytes(size);
    }
    
    inline bool hasAttribute(VertexAttributeType type) const {
        return attributeTypes[static_cast<std::size_t>(type)];
    }
    
    inline const std::string& getName() const {
        return name;
    }
    
    inline NormalMappingMode getSupportedNormalMappingMode() const {
        if (!hasAttribute(VertexAttributeType::Normal)) {
            return NormalMappingMode::NotSupported;
        }
        
        if (hasAttribute(VertexAttributeType::Tangent) && hasAttribute(VertexAttributeType::Bitangent)) {
            return NormalMappingMode::Regular;
        }
        
        if (hasAttribute(VertexAttributeType::TangentAndBias)) {
            return NormalMappingMode::BitangentRecovering;
        }
        
        
        return NormalMappingMode::NotSupported;
    }
    
    VertexInputStateCreateInfo createVertexInputStateCreateInfo(std::uint32_t binding) const {
        VertexInputStateCreateInfo vertexInputState;
        vertexInputState.vertexBindingDescriptions = 
            {{0, static_cast<std::uint32_t>(size), iyf::VertexInputRate::Vertex}};
        
        vertexInputState.vertexAttributeDescriptions.reserve(attributes.size());
        for (std::uint32_t i = 0; i < attributes.size(); ++i) {
            const VertexAttribute& attribute = attributes[i];
            vertexInputState.vertexAttributeDescriptions.emplace_back(i, binding, attribute.format, attribute.offset);
        }
        
        return vertexInputState;
    }
private:
    std::string name;
    std::uint32_t size;
    std::bitset<static_cast<std::size_t>(VertexAttributeType::COUNT)> attributeTypes;
    std::vector<VertexAttribute> attributes;
};

namespace con {
const VertexDataLayoutDefinition& GetVertexDataLayoutDefinition(VertexDataLayout layout);
}

}

#endif /* VERTEX_DATA_LAYOUTS_HPP */

