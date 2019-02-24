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

#ifndef COLLISION_SHAPE_CACHE_HPP
#define COLLISION_SHAPE_CACHE_HPP

#include <unordered_map>
#include <vector>
#include <atomic>

#include <btBulletDynamicsCommon.h>

#include "core/interfaces/GarbageCollecting.hpp"

#include "physics/CollisionShape.hpp"
#include "core/Logger.hpp"

#include "utilities/ChunkedVector.hpp"
#include "utilities/ReferenceCountedHandle.hpp"
#include "utilities/AlignedWrapper.hpp"

namespace iyf {
const std::size_t ShapeVectorChunkSize = 1024;

using CollisionShapeHandle = ReferenceCountedHandle<btCollisionShape, std::atomic<std::uint32_t>>;

template <typename T>
struct alignas(16) CollisionShape {
    using ShapeType = T;
    
    CollisionShape(CollisionShapeKey key, std::uint32_t uniqueID, std::uint32_t count) : key(key), uniqueID(uniqueID), count(count) {}
    
    /// Using optional gives us nice, in-place and delayed initialization
    std::optional<T> shape;
    /// Used for reverse lookup
    CollisionShapeKey key;
    std::uint32_t uniqueID;
    std::atomic<std::uint32_t> count;
};

template <typename ShapeType, typename CreateInfoType>
class CollisionShapeCache : public GarbageCollecting {
protected:
    static const std::uint32_t ClearedShape = std::numeric_limits<std::uint32_t>::max();
    
public:
    CollisionShapeCache() : uniqueCounter(0) {
        //shapeMap.reserve(1024);
        freeList.reserve(128);
    }
    
    CollisionShapeHandle getShapeHandle(const CreateInfoType& createInfo) {
        if (createInfo.allowCachedShapeReuse) {
            auto range = shapeMap.equal_range(createInfo.makeKey());
            
            if (range.first == shapeMap.end()) {
                // No shapes that we can reuse. Create a new one.
                Identifier identifier = createShape(createInfo);
                shapeMap.insert({createInfo.makeKey(), identifier});
                
                auto& shape = shapes[identifier.id];
                return CollisionShapeHandle(&(*shape.shape), &shape.count);
            } else {
                // Looking for a non-unique shape that we could reuse
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second.unique == 0) {
                        auto& shape = shapes[it->second.id];
                        return CollisionShapeHandle(&(*shape.shape), &shape.count);
                    }
                }
                
                // No such shape was found. Making a new one.
                Identifier identifier = createShape(createInfo);
                shapeMap.insert({createInfo.makeKey(), identifier});
                
                auto& shape = shapes[identifier.id];
                return CollisionShapeHandle(&(*shape.shape), &shape.count);
            }
        } else {
            Identifier identifier = createShape(createInfo);
            shapeMap.insert({createInfo.makeKey(), identifier});
            
            auto& shape = shapes[identifier.id];
            return CollisionShapeHandle(&(*shape.shape), &shape.count);
        }
    }
    
    std::size_t getShapeCount() const {
        return shapeMap.size();
    }
    
    std::size_t getFreeListSize() const {
        return freeList.size();
    }
    
    /// \todo React to the policy paremeter
    virtual void collectGarbage(GarbageCollectionRunPolicy) final {
        // Not using ids here because ChunkedVector iterator is MUCH MUCH MUCH faster due to some cached pointers.
        // Moreover, we don't even have to use direct chunk access because the ids are also stored in the map and can
        // be immediatelly copied over to the freeList
        for (auto& s : shapes) {
            if (s.count == 0) {
                auto range = shapeMap.equal_range(s.key);
                
                assert(range.first != shapeMap.end());
                
                for (auto it = range.first; it != range.second; ++it) {
                    if (it->second.unique == s.uniqueID) {
                        freeList.push_back(it->second.id);
                        shapeMap.erase(it);
                        break;
                    }
                }
                
                s.shape = std::nullopt;
                s.count = ClearedShape;
            }
        }
    }
protected:
    struct Identifier {
        std::uint32_t id;
        std::uint32_t unique;
    };
    
    Identifier createShape(const CreateInfoType& createInfo) {
        if (freeList.empty()) {
            std::uint32_t id = shapes.size();
            
            if (!createInfo.allowCachedShapeReuse) {
                uniqueCounter++;
                
                emplaceShape(createInfo, uniqueCounter);
            
                return {id, uniqueCounter};
            } else {
                emplaceShape(createInfo, 0);
                return {id, 0};
            }
        } else {
            std::uint32_t id = freeList.back();
            freeList.pop_back();
            
            overwriteShape(id, createInfo);
            
            shapes[id].key = createInfo.makeKey();
            assert(shapes[id].count == ClearedShape);
            shapes[id].count = 0;
            
            if (!createInfo.allowCachedShapeReuse) {
                uniqueCounter++;
                shapes[id].uniqueID = uniqueCounter;
                
                return {id, uniqueCounter};
            } else {
                shapes[id].uniqueID = 0;
                
                return {id, 0};
            }
            
            
        }
    }
    
    virtual void emplaceShape(const CreateInfoType& createInfo, std::uint32_t uniqueCounter) = 0;
    virtual void overwriteShape(std::uint32_t id, const CreateInfoType& createInfo) = 0;
    
    std::unordered_multimap<CollisionShapeKey, Identifier> shapeMap;
    std::vector<std::uint32_t> freeList;
    ChunkedVector<ShapeType, ShapeVectorChunkSize> shapes;
    std::uint32_t uniqueCounter;
};

class SphereCollisionShapeCache : public CollisionShapeCache<CollisionShape<btSphereShape>, SphereCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const SphereCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(createInfo.radius);
    }
    
    virtual void overwriteShape(std::uint32_t id, const SphereCollisionShapeCreateInfo& createInfo) final {
        shapes[id].shape.emplace(createInfo.radius);
    }
};

class BoxCollisionShapeCache : public CollisionShapeCache<CollisionShape<btBoxShape>, BoxCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const BoxCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(btVector3(createInfo.halfExtents.x, createInfo.halfExtents.y, createInfo.halfExtents.z));
    }
    
    virtual void overwriteShape(std::uint32_t id, const BoxCollisionShapeCreateInfo& createInfo) final {
        shapes[id].shape.emplace(btVector3(createInfo.halfExtents.x, createInfo.halfExtents.y, createInfo.halfExtents.z));
    }
};

class CapsuleCollisionShapeCache : public CollisionShapeCache<CollisionShape<btCapsuleShape>, CapsuleCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const CapsuleCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(createInfo.radius, createInfo.height);
    }
    
    virtual void overwriteShape(std::uint32_t id, const CapsuleCollisionShapeCreateInfo& createInfo) final {
        shapes[id].shape.emplace(createInfo.radius, createInfo.height);
    }
};

class StaticPlaneCollisionShapeCache : public CollisionShapeCache<CollisionShape<btStaticPlaneShape>, StaticPlaceCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const StaticPlaceCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(btVector3(createInfo.normal.x, createInfo.normal.y, createInfo.normal.z), createInfo.constant);
    }
    
    virtual void overwriteShape(std::uint32_t id, const StaticPlaceCollisionShapeCreateInfo& createInfo) final {
        shapes[id].shape.emplace(btVector3(createInfo.normal.x, createInfo.normal.y, createInfo.normal.z), createInfo.constant);
    }
};

class ConvexHullCollisionShapeCache : public CollisionShapeCache<CollisionShape<btConvexHullShape>, ConvexHullCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const ConvexHullCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(reinterpret_cast<const float*>(createInfo.vertexData.data), createInfo.vertexData.count, createInfo.vertexData.stride);
    }
    
    virtual void overwriteShape(std::uint32_t id, const ConvexHullCollisionShapeCreateInfo& createInfo) final {
        shapes[id].shape.emplace(reinterpret_cast<const float*>(createInfo.vertexData.data), createInfo.vertexData.count, createInfo.vertexData.stride);
    }
};

class TriangleMeshCollisionShapeCache : public CollisionShapeCache<CollisionShape<btBvhTriangleMeshShape>, TriangleMeshCollisionShapeCreateInfo> {
protected:
    virtual void emplaceShape(const TriangleMeshCollisionShapeCreateInfo& createInfo, std::uint32_t uniqueCounter) final {
        auto& wrapper = stridingMeshInterfaces.emplace_back();
        btTriangleIndexVertexArray& tiva = wrapper.data;
        
        const GraphicsToPhysicsDataMapping& indexMapping = createInfo.indexData;
        const GraphicsToPhysicsDataMapping& vertexMapping = createInfo.vertexData;
        
        btIndexedMesh im;
        im.m_numTriangles = indexMapping.count / 3;
        im.m_triangleIndexBase = reinterpret_cast<const unsigned char*>(indexMapping.data);
        im.m_triangleIndexStride = indexMapping.stride * 3;
        
        im.m_numVertices = vertexMapping.count;
        im.m_vertexBase = reinterpret_cast<const unsigned char*>(vertexMapping.data);
        im.m_vertexStride = vertexMapping.stride;
        
//         im.m_indexType = PHY_INTEGER;
//         im.m_vertexType = PHY_FLOAT;
        
        assert(indexMapping.stride == 4 || indexMapping.stride == 2);
        
        tiva.addIndexedMesh(im, indexMapping.stride == 4 ? PHY_INTEGER : PHY_SHORT);
        auto& shape = shapes.emplace_back(createInfo.makeKey(), uniqueCounter, 0);
        shape.shape.emplace(&wrapper.data, true, true);
    }
    
    virtual void overwriteShape(std::uint32_t id, const TriangleMeshCollisionShapeCreateInfo& createInfo) final {
        stridingMeshInterfaces[id].data = btTriangleIndexVertexArray();
        btTriangleIndexVertexArray& tiva = stridingMeshInterfaces[id].data;
        
        const GraphicsToPhysicsDataMapping& indexMapping = createInfo.indexData;
        const GraphicsToPhysicsDataMapping& vertexMapping = createInfo.vertexData;
        
        btIndexedMesh im;
        im.m_numTriangles = indexMapping.count / 3;
        im.m_triangleIndexBase = reinterpret_cast<const unsigned char*>(indexMapping.data);
        im.m_triangleIndexStride = indexMapping.stride * 3;
        
        im.m_numVertices = vertexMapping.count;
        im.m_vertexBase = reinterpret_cast<const unsigned char*>(vertexMapping.data);
        im.m_vertexStride = vertexMapping.stride;
        
//         im.m_indexType = PHY_INTEGER;
//         im.m_vertexType = PHY_FLOAT;
        
        assert(indexMapping.stride == 4 || indexMapping.stride == 2);
        
        tiva.addIndexedMesh(im, indexMapping.stride == 4 ? PHY_INTEGER : PHY_SHORT);
        shapes[id].shape.emplace(&stridingMeshInterfaces[id].data, true, true);
    }
    
    ChunkedVector<AlignedWrapper<btTriangleIndexVertexArray>, ShapeVectorChunkSize> stridingMeshInterfaces;
};

class CollisionShapeCacheManager : public GarbageCollecting {
public:
    
    virtual void collectGarbage(GarbageCollectionRunPolicy policy = GarbageCollectionRunPolicy::FullCollection) final {
        sphereCache.collectGarbage(policy);
        boxCache.collectGarbage(policy);
        capsuleCache.collectGarbage(policy);
        staticPlaneCache.collectGarbage(policy);
        convexHullCache.collectGarbage(policy);
        triangleMeshCache.collectGarbage(policy);
    }
    
    SphereCollisionShapeCache sphereCache;
    BoxCollisionShapeCache boxCache;
    CapsuleCollisionShapeCache capsuleCache;
    StaticPlaneCollisionShapeCache staticPlaneCache;
    ConvexHullCollisionShapeCache convexHullCache;
    TriangleMeshCollisionShapeCache triangleMeshCache;
    // TODO heightfield
};
}

#endif // COLLISION_SHAPE_CACHE_HPP
