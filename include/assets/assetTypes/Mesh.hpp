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

#ifndef MESH_HPP
#define MESH_HPP

#include <variant>

#include "assets/Asset.hpp"
#include "graphics/culling/BoundingVolumes.hpp"
#include "graphics/VertexDataLayouts.hpp"

namespace iyf {
struct PrimitiveData {
    std::uint32_t vertexCount;
    std::uint32_t indexCount;
    
    std::uint32_t vertexOffset;
    std::uint32_t indexOffset;
};

class SubmeshList {
public:
    SubmeshList(std::size_t submeshCount) {
        data = new PrimitiveData[submeshCount];
    }
    
    inline PrimitiveData& getSubmesh(std::size_t id) {
        return data[id];
    }
    
    inline const PrimitiveData& getSubmesh(std::size_t id) const {
        return data[id];
    }
    
    ~SubmeshList() {
        delete[] data;
    }
private:
    PrimitiveData* data;
};

class Mesh : public Asset {
public:
    std::uint8_t vboID;
    std::uint8_t iboID;
    std::uint8_t submeshCount;
    bool hasBones;
    
    std::variant<PrimitiveData, SubmeshList> meshData;
    
    inline bool hasSubmeshes() const {
        return meshData.index() == 1;
    }
    
    /// \warning Calling this when hasSubmeshes() == true will crash the Engine
    inline PrimitiveData& getMeshPrimitiveData() {
        return std::get<PrimitiveData>(meshData);
    }
    
    /// \warning Calling this when hasSubmeshes() == true will crash the Engine
    inline const PrimitiveData& getMeshPrimitiveData() const {
        return std::get<PrimitiveData>(meshData);
    }
    
    /// \warning Calling this when hasSubmeshes() == false will crash the Engine
    inline SubmeshList& getSubmeshPrimitiveData() {
        return std::get<SubmeshList>(meshData);
    }
    
    /// \warning Calling this when hasSubmeshes() == false will crash the Engine
    inline const SubmeshList& getSubmeshPrimitiveData() const {
        return std::get<SubmeshList>(meshData);
    }
    
    VertexDataLayout vertexDataLayout;
    bool indices32Bit;
    
    /// When storing vertices of different layouts into a single vertex buffer, some padding is often required.
    /// It is added before the data and is considered to be a part of the allocation range. This particular
    /// variable is used during the destruction of the mesh object. It is needed when computing the range that 
    /// has to be returned to the BufferRangeSet.
    std::uint8_t padding;
    
    // TODO maybe put something here? 5 whole Bytes are free because of alignment
    
    /// AABB before any world transformations.
    AABB aabb;
    /// BoundingSphere before any world transformations.
    BoundingSphere boundingSphere;
};
}

#endif //MESH_HPP
