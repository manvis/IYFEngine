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

#include "graphics/MeshLoader.hpp"

#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "graphics/MeshFormats.hpp"
#include "core/Logger.hpp"
#include "graphics/culling/BoundingVolumes.hpp"

#include <vector>
#include <cstring>

#include <glm/gtx/string_cast.hpp>

namespace iyf {
using namespace iyf::literals;

bool MeshLoader::loadMesh(const std::string& path, LoadedMeshData& submeshes, void* vertexBuffer, void* indexBuffer, std::vector<Bone>* skeleton) const {
    FileSystem* fileSystem = engine->getFileSystem();
    bool exists = fileSystem->exists(path);
    
    if (exists) {
        File fr(path, File::OpenMode::Read);
        auto headerData = readHeader(fr);
        
        if (!headerData.first) {
            LOG_E("Can't load mesh from " << path << ". Magic number is not " << mf::MagicNumber)
            return false;
        }
        
        switch (headerData.second) {
        case 1:
            return loadMeshV1(fr, submeshes, vertexBuffer, indexBuffer, skeleton);
        default:
            LOG_E("Can't load mesh from " << path << ". Unknown version number: " << headerData.second)
            return false;
        }
    } else {
        LOG_E("Can't load mesh from " << path << ". File does not exist.")
        return false;
    }
}

bool MeshLoader::loadAnimation(const std::string& path, Animation& buffer) const {
    FileSystem* fileSystem = engine->getFileSystem();
    bool exists = fileSystem->exists(path);
    
    if (exists) {
        File fr(path, File::OpenMode::Read);
        auto headerData = readAnimationHeader(fr);
        
        if (!headerData.first) {
            LOG_E("Can't load animation from " << path << ". Magic number is not " << af::MagicNumber)
            return false;
        }
        
        switch (headerData.second) {
        case 1:
            return loadAnimationV1(fr, buffer);
        default:
            LOG_E("Can't load animation from " << path << ". Unknown version number: " << headerData.second)
            return false;
        }
    } else {
        LOG_E("Can't load animation from " << path << ". File does not exist.")
        return false;
    }
}

MeshLoader::MemoryRequirements MeshLoader::getMeshMemoryRequirements(const std::string& path) const {
    FileSystem* fileSystem = engine->getFileSystem();
    bool exists = fileSystem->exists(path);
    
    if (exists) {
        File fr(path, File::OpenMode::Read);
        auto headerData = readHeader(fr);
        
        if (!headerData.first) {
            LOG_E("Can't load mesh from " << path << ". Magic number is not " << mf::MagicNumber)
            throw std::runtime_error("Mesh file has an invalid magic number.");
        }
        
        switch (headerData.second) {
        case 1:
            return getMemoryRequirementsV1(fr);
        default:
            LOG_E("Can't load mesh from " << path << ". Unknown version number: " << headerData.second)
            throw std::runtime_error("Mesh file is of an unknown version.");
        }
    } else {
        LOG_E("Can't load mesh from " << path << ". File does not exist.")
        throw std::runtime_error("Mesh file does not exist.");
    }
}

MeshLoader::MemoryRequirements MeshLoader::getMemoryRequirementsV1(File& fr) const {
    fr.readUInt8();//read numSubMeshes
    std::uint32_t totalVertices = fr.readUInt32();
    std::uint32_t totalIndices = fr.readUInt32();
    std::uint8_t numBones = fr.readUInt8();
    std::uint8_t numVertexChannels = fr.readUInt8();
    
    MemoryRequirements mr;
    
    if (numBones == 0) {
        if (numVertexChannels == 0) {
            mr.vertexSize = Bytes(totalVertices * sizeof(MeshVertex));
            mr.vertexDataLayout = VertexDataLayout::MeshVertex;
        } else {
            mr.vertexSize = Bytes(totalVertices * sizeof(MeshVertexColored));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexColored;
        }
    } else {
        if (numVertexChannels == 0) {
            mr.vertexSize = Bytes(totalVertices * sizeof(MeshVertexWithBones));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexWithBones;
        } else {
            mr.vertexSize = Bytes(totalVertices * sizeof(MeshVertexColoredWithBones));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexColoredWithBones;
        }
    }

    mr.indexSize = Bytes(totalIndices * sizeof(std::uint16_t));
    mr.indices32Bit = false;
    mr.boneCount = numBones;
    
    return mr;
}

MeshLoader::MemoryRequirements MeshLoader::getMeshMemoryRequirements(const Metadata& metadata) const {
    assert(metadata.index() == static_cast<std::size_t>(AssetType::Mesh));
    
    const MeshMetadata& meshMetadata = std::get<MeshMetadata>(metadata);
    
    switch (meshMetadata.getMeshFormatVersion()) {
    case 1:
        return getMemoryRequirementsV1(meshMetadata);
    default:
        LOG_E("Unknown mesh version number in Metadata object: " << meshMetadata.getMeshFormatVersion())
        throw std::runtime_error("Unknown mesh version.");
    }
}

MeshLoader::MemoryRequirements MeshLoader::getMemoryRequirementsV1(const MeshMetadata& metadata) const {
    MemoryRequirements mr;
    
    if (metadata.getBoneCount() == 0) {
        if (metadata.getColorChannelCount() == 0) {
            mr.vertexSize = Bytes(metadata.getVertexCount() * sizeof(MeshVertex));
            mr.vertexDataLayout = VertexDataLayout::MeshVertex;
        } else {
            mr.vertexSize = Bytes(metadata.getVertexCount() * sizeof(MeshVertexColored));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexColored;
        }
    } else {
        if (metadata.getColorChannelCount() == 0) {
            mr.vertexSize = Bytes(metadata.getVertexCount() * sizeof(MeshVertexWithBones));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexWithBones;
        } else {
            mr.vertexSize = Bytes(metadata.getVertexCount() * sizeof(MeshVertexColoredWithBones));
            mr.vertexDataLayout = VertexDataLayout::MeshVertexColoredWithBones;
        }
    }
    
    mr.indexSize = Bytes(metadata.getIndexCount() * sizeof(std::uint16_t));
    mr.indices32Bit = false;
    mr.boneCount = metadata.getBoneCount();
    
    return mr;
}

std::pair<bool, std::uint16_t> MeshLoader::readHeader(File& fr) const {
    char magicNumber[4];
    fr.readBytes(magicNumber, sizeof(char) * 4);
    
    std::uint16_t version = fr.readUInt16();
    
    return std::make_pair(std::strncmp(mf::MagicNumber, magicNumber, 4) == 0, version);
}

std::pair<bool, std::uint16_t> MeshLoader::readAnimationHeader(File& fr) const {
    char magicNumber[4];
    fr.readBytes(magicNumber, sizeof(char) * 4);
    
    std::uint16_t version = fr.readUInt16();
    
    return std::make_pair(std::strncmp(af::MagicNumber, magicNumber, 4) == 0, version);
}

bool MeshLoader::loadMeshV1(File& fr, LoadedMeshData& meshData, void* vertexBuffer, void* indexBuffer, std::vector<Bone>* skeleton) const {
    if (vertexBuffer == nullptr || indexBuffer == nullptr) {
        LOG_E("Vertex and index buffer pointers passed to loadMesh function must never be nullptr.")
        return false;
    }
    
    std::uint8_t numSubMeshes = fr.readUInt8();
    std::uint32_t totalVerticesDeclared = fr.readUInt32();
    std::uint32_t totalIndicesDeclared = fr.readUInt32();
    std::uint8_t numBones = fr.readUInt8();
    std::uint8_t numVertexColors = fr.readUInt8();
    
    assert(numVertexColors == 0 || numVertexColors == 1);
    
    std::size_t totalVerticesActual = 0;
    std::size_t totalIndicesActual = 0;
    
    std::size_t vertexStructSize = 0;
    if (numBones == 0) {
        if (numVertexColors == 0) {
            vertexStructSize = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::MeshVertex)].getSize();
        } else {
            vertexStructSize = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::MeshVertexColored)].getSize();
        }
    } else {
        if (numVertexColors == 0) {
            vertexStructSize = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::MeshVertexWithBones)].getSize();
        } else {
            vertexStructSize = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::MeshVertexColoredWithBones)].getSize();
        }
    }
    
    // TODO support bones
    assert(vertexStructSize == 32 || vertexStructSize == 36);
    char* vbo = static_cast<char*>(vertexBuffer);
    char* ibo = static_cast<char*>(indexBuffer);
    
    if (numBones > 0 && skeleton == nullptr) {
        LOG_E("A pointer to the skeleton data vector must not be nullptr if the mesh has bones.")
        return false;
    }
    
    meshData.count = numSubMeshes;
    for (std::uint8_t s = 0; s < numSubMeshes; ++s) {
        std::uint16_t numVertices = fr.readUInt16();
        totalVerticesActual += numVertices;
        
        fr.readBytes(vbo, vertexStructSize * numVertices);
        vbo += vertexStructSize * numVertices;
        
        std::uint32_t numIndices = fr.readUInt32();
        totalIndicesActual += numIndices;
        
        fr.readBytes(ibo, sizeof(std::uint16_t) * numIndices);
        ibo += sizeof(std::uint16_t) * numIndices;
        
        auto& submeshData = meshData.submeshes[s];
        submeshData.numVertices = numVertices;
        submeshData.numIndices = numIndices;
        
//         submeshData.aabb.minCorner.x = fr.readFloat();
//         submeshData.aabb.minCorner.y = fr.readFloat();
//         submeshData.aabb.minCorner.z = fr.readFloat();
//         submeshData.aabb.maxCorner.x = fr.readFloat();
//         submeshData.aabb.maxCorner.y = fr.readFloat();
//         submeshData.aabb.maxCorner.z = fr.readFloat();
//         
//         submeshData.boundingSphere.center.x = fr.readFloat();
//         submeshData.boundingSphere.center.y = fr.readFloat();
//         submeshData.boundingSphere.center.z = fr.readFloat();
//         submeshData.boundingSphere.radius = fr.readFloat();
        
//         submeshData.defaultTexture = hash32_t(fr.readUInt32());
    }
        
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Minimum)].x = fr.readFloat();
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Minimum)].y = fr.readFloat();
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Minimum)].z = fr.readFloat();
    
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Maximum)].x = fr.readFloat();
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Maximum)].y = fr.readFloat();
    meshData.aabb.vertices[static_cast<int>(AABB::Vertex::Maximum)].z = fr.readFloat();

    meshData.boundingSphere.center.x = fr.readFloat();
    meshData.boundingSphere.center.y = fr.readFloat();
    meshData.boundingSphere.center.z = fr.readFloat();
    
    meshData.boundingSphere.radius = fr.readFloat();
    
    if (numBones > 0) {
        for (std::uint8_t b = 0; b < numBones; ++b) {
            Bone& bone = (*skeleton)[b];
            bone.parent = fr.readUInt8();
            
            float t[16];
            fr.readBytes(t, sizeof(float) * 16);
            
            bone.transform = glm::mat4(t[0],  t[1],  t[2],  t[3],
                                       t[4],  t[5],  t[6],  t[7],
                                       t[8],  t[9],  t[10], t[11],
                                       t[12], t[13], t[14], t[15]);
        }
    }
    
    std::size_t numAnimations = fr.readUInt32();
    meshData.animationCount = numAnimations;
    
    //LOG_D("ANIMATIONS: " << numAnimations << "; BONES: " << numBones << "; " << totalVerticesDeclared << " " << totalIndicesDeclared)
    for (std::size_t a = 0; a < numAnimations; ++a) {
        meshData.animations[a] = hash32_t(fr.readUInt32());
    }
    
    assert(fr.isEOF());
    assert(totalVerticesDeclared == totalVerticesActual);
    assert(totalIndicesDeclared == totalIndicesActual);
    
    return true;
}

bool MeshLoader::loadAnimationV1(File& fr, Animation& buffer) const {
    buffer.duration = fr.readFloat();
    buffer.ticksPerSecond = fr.readFloat();
    
    std::size_t channelCount = fr.readUInt8();
    buffer.animationChannels.resize(channelCount);
    
//    LOG_D(channelCount)
    
    for (std::size_t c = 0; c < channelCount; ++c) {
        std::size_t numLocKeys = fr.readUInt32();
        buffer.animationChannels[c].locations.resize(numLocKeys);
        
//        LOG_D(channelCount)
        
        for (std::size_t l = 0; l < numLocKeys; ++l) {
            LocationKey& lk = buffer.animationChannels[c].locations[l];
            lk.location.x = fr.readFloat();
            lk.location.y = fr.readFloat();
            lk.location.z = fr.readFloat();
        }
        
        std::size_t numRotKeys = fr.readUInt32();
        buffer.animationChannels[c].rotations.resize(numRotKeys);
        
        for (std::size_t r = 0; r < numRotKeys; ++r) {
            RotationKey& rk = buffer.animationChannels[c].rotations[r];
            rk.rotation.w = fr.readFloat();
            rk.rotation.x = fr.readFloat();
            rk.rotation.y = fr.readFloat();
            rk.rotation.z = fr.readFloat();
        }
        
        std::size_t numSclKeys = fr.readUInt32();
        buffer.animationChannels[c].scales.resize(numSclKeys);
        
        for (std::size_t s = 0; s < numSclKeys; ++s) {
            ScaleKey& sk = buffer.animationChannels[c].scales[s];
            sk.scale.x = fr.readFloat();
            sk.scale.y = fr.readFloat();
            sk.scale.z = fr.readFloat();
        }
    }
    
    assert(fr.isEOF());
    return true;
}
}
