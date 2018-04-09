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

#ifndef RENDERDATAKEY_HPP
#define RENDERDATAKEY_HPP

namespace iyf {
template <typename T>
class IDType {
public:
    IDType(T id) : id (id) {}
    
    inline T getID() const {
        return id;
    }
private:
    T id;
};

/// Quite a few Pipelines may exist to accomodate vrious materials and even permutations of same materials,
/// hence the 16 bit value.
class PipelineID : public IDType<std::uint16_t> {
public:
    PipelineID(std::uint16_t id) : IDType(id) {}
};

/// The engine always tries to keep the number of vertex and index buffers low because having a couple
/// large buffers is better for performance than having many small ones. That's why having an 8 bit value
/// to represent all vertex buffers should always be enough.
class VertexBufferID : public IDType<std::uint8_t> {
public:
    VertexBufferID(std::uint8_t id) : IDType(id) {}
};

/// The engine always tries to keep the number of vertex and index buffers low because having a couple
/// large buffers is better for performance than having many small ones. That's why having an 8 bit value
/// to represent all index buffers should always be enough.
class IndexBufferID : public IDType<std::uint8_t> {
public:
    IndexBufferID(std::uint8_t id) : IDType(id) {}
};

/// Same logic applies here as with VertexBufferID and IndexBufferID. However, while VertexIndexBufferID
/// always references a combintion of a vertex buffer and an index buffer, a single UniformBufferID references 
/// combinations of a variable number of uniform buffers. Exact specifics depend on the Pipeline.
///
/// \todo update this piece of documentation to reflect reality
class UniformBufferID : public IDType<std::uint8_t> {
public:
    UniformBufferID(std::uint8_t id) : IDType(id) {}
};

/// Identifies a unique combination of textures and other material data (constants, colors, etc.).
/// Just like with UniformBufferID, the specifics depend on the Pipeline.
class MaterialID : public IDType<std::uint16_t> {
public:
    MaterialID(std::uint16_t id) : IDType(id) {}
};
    
/// RenderDataKey is used when sorting RenderDataComponent objects that survived the culling.
/// This allows us to minimize GPU state changes and signifficantly increase rendering performance.
/// 
/// The key is created by packing various integer identifiers into a single std::uint64_t, which turns
/// a multiple variable comparison into a single integer comparison and signifficantly speeds up sorting.
/// More complicated and rarer state changes (e.g. pipeline changes) are placed in more signifficant bits than
/// less complitated and more frequent changes.
///
/// \todo figure out a good use for the last 8 bytes
class RenderDataKey {
public:
    inline RenderDataKey() : key(0) {}
    
    inline RenderDataKey(const PipelineID& pipeline, const VertexBufferID& vboID, const IndexBufferID& iboID, const UniformBufferID& uboID, const MaterialID& material) {
        key = (std::uint64_t(pipeline.getID()) << 48 | std::uint64_t(vboID.getID()) << 40 | std::uint64_t(iboID.getID()) << 32 | std::uint64_t(uboID.getID()) << 24 | material.getID() << 8);
    }
    
    inline std::uint64_t getKey() const {
        return key;
    }
    
    inline PipelineID getPipelineID() const {
        return PipelineID(key >> 48);
    }
    
    inline VertexBufferID getVertexBufferID() const {
        return VertexBufferID(key >> 40);
    }
    
    inline IndexBufferID getIndexBufferID() const {
        return IndexBufferID(key >> 32);
    }
    
    inline UniformBufferID getUniformBufferID() const {
        return UniformBufferID(key >> 24);
    }
    
    inline MaterialID getMaterialID() const {
        return MaterialID(key >> 8);
    }
    
    inline bool operator<(const RenderDataKey& other) const {
        return key < other.key;
    }
    
    inline bool isValid() const {
        return key != 0;
    }
private:
    std::uint64_t key;
};
}

#endif /* RENDERDATAKEY_HPP */

