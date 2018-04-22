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

#ifndef MESH_TYPE_MANAGER_HPP
#define MESH_TYPE_MANAGER_HPP

#include <vector>

#include "graphics/GraphicsAPI.hpp"
#include "physics/GraphicsToPhysicsDataMapping.hpp"
#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Mesh.hpp"
#include "utilities/BufferRangeSet.hpp"
#include "utilities/DataSizes.hpp"

namespace iyf {
class Engine;
/// \todo https://developer.nvidia.com/vulkan-memory-management Maybe I need to merge vertex and index buffers into
/// a single one and layout the memory of objects like this: obj1Vert,obj1Ind,obj2Vert,obj2Ind,...
class MeshTypeManager : public TypeManager<Mesh> {
public:
    struct BufferWithRanges {
        BufferWithRanges(Buffer buffer, Bytes size, void* data) : buffer(buffer), freeRanges(size), data(data) {}
        
        Buffer buffer;
        BufferRangeSet freeRanges;
        
        /// Pointer to a "mirror" buffer in system RAM that has the exact same data as the GPU buffer.
        /// The data in this buffer should typically be used to build various acceleration structures,
        /// such as those used by Steam Audio, or physics objects (such as terrains and convex meshes)
        void* data;
    };

    MeshTypeManager(AssetManager* manager, Bytes VBOSize, Bytes IBOSize);
    virtual ~MeshTypeManager();
    
    virtual AssetType getType() final {
        return AssetType::Mesh;
    }
    
    /// In order to minimize data duplication and reduce the size of Mesh objects, they store an 8 bit 
    /// index into the vertexDataBuffers vector. This method is typically used during rendering to retrieve
    /// the actual vertex buffer that can be bound to the GraphicsAPI.
    ///
    /// \param[in] id index of a vertex buffer to retrieve
    /// \return A vertex Buffer
    const Buffer& getVertexBuffer(std::uint8_t id) const {
        return vertexDataBuffers[id].buffer;
    }
    
    /// In order to minimize data duplication and reduce the size of Mesh objects, they store an 8 bit 
    /// index into the indexDataBuffers vector. This method is typically used during rendering to retrieve
    /// the actual index buffer that can be bound to the GraphicsAPI.
    ///
    /// \param[in] id index of an index buffer to retrieve
    /// \return An index Buffer
    const Buffer& getIndexBuffer(std::uint8_t id) const {
        return indexDataBuffers[id].buffer;
    }
    
    /// \return Is vertex and index data stored in one buffer or in several.
    /// \todo Change this after implementing the optimization mentioned in MeshTypeManager class documentation
    bool isSingleBufferMode() const {
        return false;
    }
    
    /// This method should be used for debugging and visualization purposes.
    ///
    /// \return All data about allocated vertex data buffers and free ranges inside them
    const std::vector<BufferWithRanges>& getVertexDataBuffersWithRanges() const {
        return vertexDataBuffers;
    }
    
    /// This method should be used for debugging and visualization purposes.
    ///
    /// \return All data about allocated index data buffers and free ranges inside them
    const std::vector<BufferWithRanges>& getIndexDataBuffersWithRanges() const {
        return indexDataBuffers;
    }

    /// \warning Make sure that the physics objects using these mappings get destroyed before the backing graphics data
    /// is cleared.
    std::pair<GraphicsToPhysicsDataMapping, GraphicsToPhysicsDataMapping> getGraphicsToPhysicsDataMapping(const Mesh& assetData) const;
protected:
    virtual void initMissingAssetHandle() final override;
    
    virtual void performLoad(hash32_t nameHash, const std::string& path, const Metadata& meta, Mesh& assetData) override;
    virtual void performFree(Mesh& assetData) override;
    
    struct RangeDataResult {
        BufferRange range;
        void* data;
        bool result;
        std::uint8_t bufferID;
    };
    
    RangeDataResult findRange(Bytes size, Bytes alignment, std::vector<BufferWithRanges>& buffers);
    
    const Bytes VBOSize;
    const Bytes IBOSize;
    
    std::vector<BufferWithRanges> vertexDataBuffers;
    std::vector<BufferWithRanges> indexDataBuffers;
    GraphicsAPI* api;
    Engine* engine;
};

}

#endif //MESH_TYPE_MANAGER_HPP
