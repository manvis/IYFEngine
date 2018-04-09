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

#ifndef VERTEX_DATA_TYPES_HPP
#define VERTEX_DATA_TYPES_HPP

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace iyf {
// -------- DEFINITIONS OF STRUCTS REPRESENTING ALL VERTEX TYPES THAT ARE USED IN THE ENGINE -------------
// WARNING: DO NOT EDIT existing sturcts, exiting mesh formats may rely on them. Create new vertex layouts
// and extend loaders/converters to process them instead. Moreover, don't forget to add the new structs to
// the VertexDataLayout enum and the VertexDataLayoutDefinitions array that are located in the
// VertexDataLayouts.hpp file.

/// Default data structure for 3D mesh data. Used for almost everything. Contains
/// enough data for normal mapping.
struct MeshVertex {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// Tangent vector, packed as A2B10G10R10
    std::uint32_t tangent;
    /// Bitangent vector, packed as A2B10G10R10
    std::uint32_t bitangent;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
};

static_assert(sizeof(MeshVertex) == 32, "Unexpected padding in MeshVertex");

/// Default data structure for 3D mesh data. Used for almost everything when vertex 
/// colors are needed. Contains enough data for normal mapping.
struct MeshVertexColored {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// Tangent vector, packed as A2B10G10R10
    std::uint32_t tangent;
    /// Bitangent vector, packed as A2B10G10R10
    std::uint32_t bitangent;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
    /// RGBA color array, packed as 4 unorm integers
    std::uint32_t color;
};

static_assert(sizeof(MeshVertexColored) == 36, "Unexpected padding in MeshVertexColored");

/// Same as MeshVertex, plus everything you will need to render a skinned mesh
struct MeshVertexWithBones {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// Tangent vector, packed as A2B10G10R10
    std::uint32_t tangent;
    /// Bitangent vector, packed as A2B10G10R10
    std::uint32_t bitangent;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
    /// A mesh can't have more than 255 bones
    std::uint8_t boneIDs[4];
    /// Weights in [0; 1] interval, packed as 8 bit unorm
    std::uint32_t boneWeights;
};

static_assert(sizeof(MeshVertexWithBones) == 40, "Unexpected padding in MeshVertexWithBones");

/// Same as MeshVertexColored, plus everything you will need to render a skinned mesh
struct MeshVertexColoredWithBones {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// Tangent vector, packed as A2B10G10R10
    std::uint32_t tangent;
    /// Bitangent vector, packed as A2B10G10R10
    std::uint32_t bitangent;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
    /// A mesh can't have more than 255 bones
    std::uint8_t boneIDs[4];
    /// Weights in [0; 1] interval, packed as 8 bit unorm
    std::uint32_t boneWeights;
    /// RGBA color array, packed as 4 unorm integers
    std::uint32_t color;
};

static_assert(sizeof(MeshVertexColoredWithBones) == 44, "Unexpected padding in MeshVertexColoredWithBones");

/// Simplified 3D mesh data that skips tangents and bitangents
struct SimpleMeshVertex {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
};

static_assert(sizeof(SimpleMeshVertex) == 24, "Unexpected padding in SimpleMeshVertex");

/// Simplified 3D mesh data that skips tangents and bitangents, but still retains color
struct SimpleMeshVertexColored {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// Normal vector, packed as A2B10G10R10
    std::uint32_t normal;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
    /// RGBA color array, packed as 4 unorm integers
    std::uint32_t color;
};

static_assert(sizeof(SimpleMeshVertexColored) == 28, "Unexpected padding in SimpleMeshVertexColored");

/// Position + UV only.
struct MinimalMeshVertex {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// UV coordinates, 2 floats
    glm::vec2 uv;
};

static_assert(sizeof(MinimalMeshVertex) == 20, "Unexpected padding in MinimalMeshVertex");

struct ColoredDebugVertex {
    /// Position vector. Regular float values
    glm::vec3 position;
    /// RGBA color array, packed as 4 unorm integers
    std::uint32_t color;
};

static_assert(sizeof(ColoredDebugVertex) == 16, "Unexpected padding in ColoredDebugVertex");
}

#endif // VERTEX_DATA_TYPES_HPP
