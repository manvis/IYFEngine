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

#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/loaders/MeshLoader.hpp"
#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"

namespace iyf {
using namespace iyf::literals;

// Transfer destination is needed because we'll be copying data there
const BufferUsageFlags VBOUsageFlags = BufferUsageFlagBits::VertexBuffer | BufferUsageFlagBits::TransferDestination;
const BufferUsageFlags IBOUsageFlags = BufferUsageFlagBits::IndexBuffer  | BufferUsageFlagBits::TransferDestination;
const BufferUsageFlags CombinedUsageFlags = BufferUsageFlagBits::VertexBuffer | BufferUsageFlagBits::IndexBuffer | BufferUsageFlagBits::TransferDestination;

MeshTypeManager::MeshTypeManager(AssetManager* manager, Bytes VBOSize, Bytes IBOSize) : TypeManager(manager), VBOSize(VBOSize), IBOSize(IBOSize) {
    engine = manager->getEngine();
    api = engine->getGraphicsAPI();
    
    std::vector<BufferCreateInfo> bci = {
        {VBOUsageFlags, VBOSize},
        {IBOUsageFlags, IBOSize}
    };
    
    std::vector<Buffer> output;
    output.reserve(2);
    
    api->createBuffers(bci, MemoryType::DeviceLocal, output);
    
    // The actual size of a buffer may be different (bigger) because of alignment requirements.
    char* vboData = new char[VBOSize.count()];
    vertexDataBuffers.emplace_back(output[0], output[0].size(), vboData);
    
    char* iboData = new char[IBOSize.count()];
    indexDataBuffers.emplace_back(output[1], output[1].size(), iboData);
}

void MeshTypeManager::initMissingAssetHandle() {
    // TODO this is old - update
//     /// Initialize the missing asset data
//     std::uint32_t idOut = 0;
//     VirtualFileSystemSerializer fr((con::MeshPath / (con::MissingMesh + con::MetadataExtension)).generic_string(), File::OpenMode::Read);
//     MeshMetadata meshMeta;
//     meshMeta.deserialize(fr);
//     Metadata metadata = meshMeta;
//     missingAssetHandle = load(HS(con::MissingMesh.c_str()), con::MissingMesh, metadata, idOut);
}

MeshTypeManager::~MeshTypeManager() {
    std::vector<Buffer> toClear;
    toClear.reserve(vertexDataBuffers.size() + indexDataBuffers.size());
    
    for (auto& ib : indexDataBuffers) {
        toClear.push_back(ib.buffer);
        
        char* data = static_cast<char*>(ib.data);
        delete[] data;
    }
    
    for (auto& vb : vertexDataBuffers) {
        toClear.push_back(vb.buffer);
        
        char* data = static_cast<char*>(vb.data);
        delete[] data;
    }
    
    api->destroyBuffers(toClear);
}

void MeshTypeManager::performLoad(hash32_t nameHash, const fs::path& path, const Metadata& meta, Mesh& assetData) {
    const MeshLoader loader(engine);
    MeshLoader::MemoryRequirements requirements = loader.getMeshMemoryRequirements(meta);
    
    // TODO implement bones
    //LOG_D(requirements.boneCount << " " << requirements.vertexBoneDataSize)
    assert(requirements.boneCount == 0);
    // Required to store multiple vertex types in a single VBO
    Bytes vertexAlignment(con::VertexDataLayoutDefinitions[static_cast<std::size_t>(requirements.vertexDataLayout)].getSize());
    Bytes indexAlignment(requirements.indices32Bit ? 4 : 2);
    
    // TODO make this buffer reuse code generic and move it somewhere else
    // We call findRange even if indexSize and vertexSize are bigger than IBOSize and VBOSize because
    // we may be able to reuse an older big buffer.
    auto iboRangeResult = findRange(requirements.indexSize, indexAlignment, indexDataBuffers);
    auto vboRangeResult = findRange(requirements.vertexSize, vertexAlignment, vertexDataBuffers);
    
    if (!iboRangeResult.result && !vboRangeResult.result) {
        Bytes newVBOSize = std::max(VBOSize, requirements.vertexSize);
        Bytes newIBOSize = std::max(IBOSize, requirements.indexSize);
        
        std::vector<BufferCreateInfo> bci = {
            {VBOUsageFlags, newVBOSize},
            {IBOUsageFlags, newIBOSize}
        };
        
        std::vector<Buffer> output;
        output.reserve(2);
        
        api->createBuffers(bci, MemoryType::DeviceLocal, output);
        
        // The actual size of a buffer may be different (bigger) because of alignment requirements.
        char* vboData = new char[newVBOSize.count()];
        vertexDataBuffers.emplace_back(output[0], output[0].size(), vboData);
        
        char* iboData = new char[newIBOSize.count()];
        indexDataBuffers.emplace_back(output[1], output[1].size(), iboData);
        
        auto vbo = vertexDataBuffers.back().freeRanges.getFreeRange(requirements.vertexSize, vertexAlignment);
        auto ibo = indexDataBuffers.back().freeRanges.getFreeRange(requirements.indexSize, indexAlignment);
        
        assert(vbo.status);
        assert(ibo.status);
        assert(vertexDataBuffers.size() <= 255);
        assert(indexDataBuffers.size() <= 255);
        
        char* vboLocation = vboData + (vbo.completeRange.offset.count() + vbo.startPadding);
        char* iboLocation = iboData + (ibo.completeRange.offset.count() + ibo.startPadding);
        
        vboRangeResult = {vbo.completeRange, vboLocation, vbo.status, static_cast<uint8_t>(vertexDataBuffers.size() - 1)};
        iboRangeResult = {ibo.completeRange, iboLocation, ibo.status, static_cast<uint8_t>(indexDataBuffers.size() - 1)};
    } else if (!iboRangeResult.result) {
        Bytes newIBOSize = std::max(IBOSize, requirements.indexSize);
        
        std::vector<BufferCreateInfo> bci = {
            {IBOUsageFlags, newIBOSize}
        };
        
        std::vector<Buffer> output;
        output.reserve(1);
        
        api->createBuffers(bci, MemoryType::DeviceLocal, output);
        
        char* iboData = new char[newIBOSize.count()];
        indexDataBuffers.emplace_back(output[0], output[0].size(), iboData);
        
        auto ibo = indexDataBuffers.back().freeRanges.getFreeRange(requirements.indexSize, indexAlignment);
        
        assert(ibo.status);
        assert(indexDataBuffers.size() <= 255);
        
        char* iboLocation = iboData + (ibo.completeRange.offset.count() + ibo.startPadding);
        
        iboRangeResult = {ibo.completeRange, iboLocation, ibo.status, static_cast<uint8_t>(indexDataBuffers.size() - 1)};
    } else if (!vboRangeResult.result) {
        Bytes newVBOSize = std::max(VBOSize, requirements.vertexSize);
        
        std::vector<BufferCreateInfo> bci = {
            {VBOUsageFlags, newVBOSize}
        };
        
        std::vector<Buffer> output;
        output.reserve(1);
        
        api->createBuffers(bci, MemoryType::DeviceLocal, output);
        
        char* vboData = new char[newVBOSize.count()];
        vertexDataBuffers.emplace_back(output[0], output[0].size(), vboData);
        
        auto vbo = vertexDataBuffers.back().freeRanges.getFreeRange(requirements.vertexSize, vertexAlignment);
        
        assert(vbo.status);
        assert(vertexDataBuffers.size() <= 255);
        
        char* vboLocation = vboData + (vbo.completeRange.offset.count() + vbo.startPadding);
        
        vboRangeResult = {vbo.completeRange, vboLocation, vbo.status, static_cast<uint8_t>(vertexDataBuffers.size() - 1)};
    }
    
    MeshLoader::LoadedMeshData lmd;
    if (!loader.loadMesh((con::MeshPath / path).generic_string(), lmd, vboRangeResult.data, iboRangeResult.data)) {
        throw std::runtime_error("Failed to find a mesh file");
    }
    
    assetData.vboID = vboRangeResult.bufferID;
    assetData.iboID = iboRangeResult.bufferID;
    assetData.submeshCount = lmd.count;
    assetData.hasBones = false; // TODO change
    assetData.vertexDataLayout = requirements.vertexDataLayout;
    assetData.indices32Bit = requirements.indices32Bit;
    assetData.aabb  = lmd.aabb;
    assetData.boundingSphere = lmd.boundingSphere;
    
    if (lmd.count > 1) {
        assetData.meshData = SubmeshList(lmd.count);
        //TODO implement
        throw std::runtime_error("IMPLEMENT ME");
//         for (std::size_t i = 0; i < assetData
    } else {
        PrimitiveData data;
        assert(iboRangeResult.range.offset % indexAlignment == 0);
        assert(vboRangeResult.range.offset % vertexAlignment == 0);
        
        // range.offset is in bytes, we need a number of indices. Since one index is 2 bytes, we divide
        data.indexOffset = iboRangeResult.range.offset / indexAlignment;
        data.indexCount = lmd.submeshes[0].numIndices;
        // range offset is in bytes, we need a number of vertices.
        data.vertexOffset = vboRangeResult.range.offset / vertexAlignment;
        data.vertexCount = lmd.submeshes[0].numVertices;
        
        assetData.meshData = data;
    }
    
    api->updateDeviceVisibleBuffer(vertexDataBuffers[vboRangeResult.bufferID].buffer, {{0, vboRangeResult.range.offset, vboRangeResult.range.size}}, vboRangeResult.data);
    api->updateDeviceVisibleBuffer(indexDataBuffers[iboRangeResult.bufferID].buffer, {{0, iboRangeResult.range.offset, iboRangeResult.range.size}}, iboRangeResult.data);
}

void MeshTypeManager::performFree(Mesh& assetData) {
    // TODO remove bones
    if (assetData.submeshCount > 1) {
        throw std::runtime_error("IMPLEMENT ME");
    } else {
        const PrimitiveData& data = assetData.getMeshPrimitiveData();
        std::size_t vertexAlignment = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(assetData.vertexDataLayout)].getSize();
        std::size_t indexAlignment = assetData.indices32Bit ? 4 : 2;

        BufferRange vboRange(Bytes(data.vertexOffset * vertexAlignment), Bytes(data.vertexCount * vertexAlignment));
        BufferRange iboRange(Bytes(data.indexOffset * indexAlignment), Bytes(data.indexCount * indexAlignment));
        
        vertexDataBuffers[assetData.vboID].freeRanges.insert(vboRange);
        indexDataBuffers[assetData.iboID].freeRanges.insert(iboRange);
    }
}

std::pair<GraphicsToPhysicsDataMapping, GraphicsToPhysicsDataMapping> MeshTypeManager::getGraphicsToPhysicsDataMapping(const Mesh& assetData) const {
    const char* vboData = reinterpret_cast<const char*>(vertexDataBuffers[assetData.vboID].data);
    const char* iboData = reinterpret_cast<const char*>(indexDataBuffers[assetData.iboID].data);
    
    std::uint32_t vboStride = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(assetData.vertexDataLayout)].getSize();
    std::uint32_t iboStride = assetData.indices32Bit ? 4 : 2;
    
    if (assetData.submeshCount > 1) {
        throw std::runtime_error("IMPLEMENT ME");
    } else {
        const PrimitiveData& data = assetData.getMeshPrimitiveData();
        vboData += vboStride * data.vertexOffset;
        iboData += iboStride * data.indexOffset;
        
        return {{vboData, data.vertexCount, vboStride}, {iboData, data.indexCount, iboStride}};
    }
}

MeshTypeManager::RangeDataResult MeshTypeManager::findRange(Bytes size, Bytes alignment, std::vector<BufferWithRanges>& buffers) {
    // Look for a buffer that would be able to fit the required amount of data
    for (std::size_t i = 0; i < buffers.size(); ++i) {
        auto& b = buffers[i];
        
        if (b.freeRanges.getFreeSpace() >= size) {
            // Try obtaining a free range
            auto rangeAndResult = b.freeRanges.getFreeRange(size, alignment);
            
            // Even if there's enough free space in a buffer, retrieval may fail due to fragmentation.
            // In that case, we'll simply continue looking for an another buffer.
            if (rangeAndResult.status) {
                char* location = static_cast<char*>(b.data);
                location += (rangeAndResult.completeRange.offset.count() + rangeAndResult.startPadding);
                
                return {rangeAndResult.completeRange, location, rangeAndResult.status, static_cast<std::uint8_t>(i)};
            }
        }
    }
    
    return {BufferRange(0_B, size), nullptr, false, 0};
}

}
