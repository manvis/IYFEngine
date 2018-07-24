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

#include "graphics/VertexDataLayouts.hpp"

#include "core/Constants.hpp"

namespace iyf {
VertexDataLayoutDefinition::VertexDataLayoutDefinition(std::string name, std::uint32_t size, std::initializer_list<VertexAttribute> attributeList) : name(name), size(size), attributes(attributeList) {
    if (size >= con::MaxVertexSize) {
        throw std::logic_error("A single vertex can't have more than con::MaxVertexSize bytes of data.");
    }
    
    if (attributes.size() == 0) {
        throw std::logic_error("Vertices with no components are invalid.");
    }
    
    if (attributes[0].offset != 0) {
        throw std::logic_error("The offset of the first vertex attribute must be 0.");
    }
    
    if (attributes.size() == 1) {
        return;
    }
    
    attributeTypes[static_cast<std::size_t>(attributes[0].type)] = true;
    
    std::uint32_t lastOffset = 0;

    for (std::size_t i = 1; i < attributes.size(); ++i) {
        const VertexAttribute& attribute = attributes[i];
        
        if (attributeTypes[static_cast<std::size_t>(attribute.type)]) {
            throw std::logic_error("Vertex attributes may not repeat.");
        } else {
            attributeTypes[static_cast<std::size_t>(attribute.type)] = true;
        }
        
        if (lastOffset >= attribute.offset) {
            throw std::logic_error("Offsets must be constantly increasing.");
        } else {
            lastOffset = attribute.offset;
        }
        
        // TODO compare offset with format and check if it's of the right size
    }
    
    if (!attributeTypes[static_cast<std::size_t>(VertexAttributeType::Position3D)]) {
        throw std::logic_error("The vertex must have a position component.");
    }
    
    bool hasNormals = attributeTypes[static_cast<std::size_t>(VertexAttributeType::Normal)];
    bool hasTangents = attributeTypes[static_cast<std::size_t>(VertexAttributeType::Tangent)];
    bool hasBitangents = attributeTypes[static_cast<std::size_t>(VertexAttributeType::Bitangent)];
    bool hasTangentWithBias = attributeTypes[static_cast<std::size_t>(VertexAttributeType::TangentAndBias)];
    
    if (hasTangents && (!hasBitangents || !hasNormals)) {
        throw std::logic_error("If the vertex has a tangent component, it must have normals and bitangents as well.");
    }
    
    if (hasBitangents && (!hasTangents || !hasNormals)) {
        throw std::logic_error("If the vertex has a bitangent component, it must have normals and tangents as well.");
    }
    
    if (hasTangentWithBias && (hasTangents || hasBitangents)) {
        throw std::logic_error("If the vertex has a tangent with bias component, it must not have tangents or bitangents.");
    }
    
    if (hasTangentWithBias && !hasNormals) {
        throw std::logic_error("If the vertex has a tangent with bias component, it must have normals.");
    }
    
    if (attributeTypes[static_cast<std::size_t>(VertexAttributeType::BoneID)] != attributeTypes[static_cast<std::size_t>(VertexAttributeType::BoneWeight)]) {
        throw std::logic_error("The vertex must either have both bone IDs and weights or neither.");
    }
}

namespace con {

hash32_t GetVertexAttributeNameHash(VertexAttributeType type) {
    static const std::array<hash32_t, static_cast<std::size_t>(VertexAttributeType::COUNT)> VertexAttributeNameHashes = {
        hash32_t(HS("position")),
        hash32_t(HS("normal")),
        hash32_t(HS("tangent")),
        hash32_t(HS("bitangent")),
        hash32_t(HS("uv")),
        hash32_t(HS("bone_id")),
        hash32_t(HS("bone_weight")),
        hash32_t(HS("tangent_and_bias")),
        hash32_t(HS("color")),// TODO add translations
    };
    
    return VertexAttributeNameHashes[static_cast<std::size_t>(type)];
}

const std::string& GetVertexAttributeName(VertexAttributeType type) {
    static const std::array<std::string, static_cast<std::size_t>(VertexAttributeType::COUNT)> VertexAttributeNames = {
        "position",
        "normal",
        "tangent",
        "bitangent",
        "uv",
        "boneId",
        "boneWeight",
        "tangentAndBias",
        "color"
    };
    
    return VertexAttributeNames[static_cast<std::size_t>(type)];
}

const VertexDataLayoutDefinition& GetVertexDataLayoutDefinition(VertexDataLayout layout) {
    static const std::array<VertexDataLayoutDefinition, static_cast<std::uint32_t>(VertexDataLayout::COUNT)> VertexDataLayoutDefinitions = {
        // 0 - MeshVertex
        VertexDataLayoutDefinition("MeshVertex", sizeof(MeshVertex),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(MeshVertex, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertex, normal)},
        {VertexAttributeType::Tangent,    Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertex, tangent)},
        {VertexAttributeType::Bitangent,  Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertex, bitangent)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(MeshVertex, uv)}}
        ),
        // 1 - MeshVertexWithBones
        VertexDataLayoutDefinition("MeshVertexWithBones", sizeof(MeshVertexWithBones),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(MeshVertexWithBones, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexWithBones, normal)},
        {VertexAttributeType::Tangent,    Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexWithBones, tangent)},
        {VertexAttributeType::Bitangent,  Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexWithBones, bitangent)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(MeshVertexWithBones, uv)},
        {VertexAttributeType::BoneID,     Format::R8_G8_B8_A8_uInt,            offsetof(MeshVertexWithBones, boneIDs)},
        {VertexAttributeType::BoneWeight, Format::R8_G8_B8_A8_uNorm,           offsetof(MeshVertexWithBones, boneWeights)}}
        ),
        // 2 - SimpleMeshVertex
        VertexDataLayoutDefinition("SimpleMeshVertex", sizeof(SimpleMeshVertex),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(SimpleMeshVertex, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(SimpleMeshVertex, normal)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(SimpleMeshVertex, uv)}}
        ),
        // 3 - MinimalMeshVertex
        VertexDataLayoutDefinition("MinimalMeshVertex", sizeof(MinimalMeshVertex),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(MinimalMeshVertex, position)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(MinimalMeshVertex, uv)}}
        ),
        // 4 - ColoredDebugVertex
        VertexDataLayoutDefinition("ColoredDebugVertex", sizeof(ColoredDebugVertex),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(ColoredDebugVertex, position)},
        {VertexAttributeType::Color,      Format::R8_G8_B8_A8_uNorm,           offsetof(ColoredDebugVertex, color)}}
        ),
        /// 5 - MeshVertexColored
        VertexDataLayoutDefinition("MeshVertexColored", sizeof(MeshVertexColored),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(MeshVertexColored, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColored, normal)},
        {VertexAttributeType::Tangent,    Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColored, tangent)},
        {VertexAttributeType::Bitangent,  Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColored, bitangent)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(MeshVertexColored, uv)},
        {VertexAttributeType::Color,      Format::R8_G8_B8_A8_uNorm,           offsetof(MeshVertexColored, color)}}
        ),
        /// 6 - MeshVertexColoredWithBones
        VertexDataLayoutDefinition("MeshVertexColoredWithBones", sizeof(MeshVertexColoredWithBones),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(MeshVertexColoredWithBones, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColoredWithBones, normal)},
        {VertexAttributeType::Tangent,    Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColoredWithBones, tangent)},
        {VertexAttributeType::Bitangent,  Format::A2_B10_G10_R10_sNorm_pack32, offsetof(MeshVertexColoredWithBones, bitangent)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(MeshVertexColoredWithBones, uv)},
        {VertexAttributeType::BoneID,     Format::R8_G8_B8_A8_uInt,            offsetof(MeshVertexColoredWithBones, boneIDs)},
        {VertexAttributeType::BoneWeight, Format::R8_G8_B8_A8_uNorm,           offsetof(MeshVertexColoredWithBones, boneWeights)},
        {VertexAttributeType::Color,      Format::R8_G8_B8_A8_uNorm,           offsetof(MeshVertexColoredWithBones, color)}}
        ),
        /// 7 - SimpleMeshVertexColored
        VertexDataLayoutDefinition("SimpleMeshVertexColored", sizeof(SimpleMeshVertexColored),
        {{VertexAttributeType::Position3D, Format::R32_G32_B32_sFloat,          offsetof(SimpleMeshVertexColored, position)},
        {VertexAttributeType::Normal,     Format::A2_B10_G10_R10_sNorm_pack32, offsetof(SimpleMeshVertexColored, normal)},
        {VertexAttributeType::UV,         Format::R32_G32_sFloat,              offsetof(SimpleMeshVertexColored, uv)},
        {VertexAttributeType::Color,      Format::R8_G8_B8_A8_uNorm,           offsetof(SimpleMeshVertexColored, color)}}
        ),
    };
    
    return VertexDataLayoutDefinitions[static_cast<std::size_t>(layout)];
}

}
}
