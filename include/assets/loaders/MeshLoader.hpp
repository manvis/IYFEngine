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

#ifndef MESHLOADER_HPP
#define MESHLOADER_HPP

#include "utilities/DataSizes.hpp"
#include "assets/metadata/Metadata.hpp"
#include "core/filesystem/File.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "graphics/AnimationDataStructures.hpp"
#include "graphics/culling/BoundingVolumes.hpp"
#include "core/Constants.hpp"

namespace iyf {
class Engine;
/// This class reads mesh and animation files and writes their contents to provided memory buffers.
class MeshLoader {
public:
    MeshLoader(Engine* engine) : engine(engine) {}
    
    struct MemoryRequirements {
        Bytes vertexSize;
        Bytes indexSize;
        VertexDataLayout vertexDataLayout;
        bool indices32Bit;
        std::size_t boneCount;
    };
    
    struct LoadedSubMeshData {
        std::size_t numVertices;
        std::size_t numIndices;
        
//         StringHash defaultTexture;
//         
//         AABB aabb;
//         BoundingSphere boundingSphere;
    };
    
    struct LoadedMeshData {
        std::size_t count;
        LoadedSubMeshData submeshes[con::MaxSubMeshes];
        
        AABB aabb;
        BoundingSphere boundingSphere;
        
        std::size_t animationCount;
        StringHash animations[con::MaxAnimations];
    };
    
    /// Load mesh data and write it to appropriate buffers. Make sure to call getMeshMemoryRequirements before this to know how much space will be needed in each buffer.
    /// 
    /// \param[in] path path to a mesh file that needs to be loaded
    /// \param[out] meshData a reference to a MeshLoader::LoadedMeshData object that will be used to store the number of loaded meshes and per-submesh data (number of vertices, indices, etc.).
    /// \param[out] vertexBuffer pointer to a memory location which will be used to store main vertex data (positions, normals, tangents, bitangens, uv coordinates). Must NEVER be nullptr.
    /// \param[out] indexBuffer pointer to a memory location which will be used to store vertex indices. Must NEVER be nullptr.
    /// \param[out] skeleton pointer to an std::vector of bone transformation and hierarchy data. Must NOT be nullptr if the mesh has bones (check MemoryRequirements.boneCount). Must be resized to 
    /// MemoryRequirements.boneCount (or more) before being passed to this function.
    /// \return if the mesh loading succeeded or not.
    bool loadMesh(const fs::path& path, LoadedMeshData& meshData, void* vertexBuffer, void* indexBuffer, std::vector<Bone>* skeleton = nullptr) const;
    bool loadAnimation(const fs::path& path, Animation& buffer) const;
    
    /// Get the amount of memory that the data of this mesh will require on the GPU directly from the file.
    /// To avoid unnecessary reads, use the getMeshMemoryRequirements(const Metadata&) const.
    /// \param[in] path Path to a mesh file.
    /// \return How much vertex, index and bone data the mesh has.
    MemoryRequirements getMeshMemoryRequirements(const fs::path& path) const;
    
    /// Get the amount of memory that the data of this mesh will require on the GPU from a Metadata object.
    /// \param[in] metadata A metadata variant that must be convertable to MeshMetadata.
    /// \return How much vertex, index and bone data the mesh has.
    MemoryRequirements getMeshMemoryRequirements(const Metadata& metadata) const;
    
protected:
    MemoryRequirements getMemoryRequirementsV1(File& fr) const;
    MemoryRequirements getMemoryRequirementsV1(const MeshMetadata& metadata) const;
    bool loadMeshV1(File& fr, LoadedMeshData& meshData, void* vertexBuffer, void* indexBuffer, std::vector<Bone>* skeleton = nullptr) const;
    bool loadAnimationV1(File& fr, Animation& buffer) const;
    std::pair<bool, std::uint16_t> readHeader(File& fr) const;
    std::pair<bool, std::uint16_t> readAnimationHeader(File& fr) const;
    
    Engine* engine;
};
}

#endif /* MESHLOADER_HPP */

