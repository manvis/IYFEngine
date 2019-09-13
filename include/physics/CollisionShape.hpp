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

#ifndef IYF_COLLISION_SHAPE_TYPE_HPP
#define IYF_COLLISION_SHAPE_TYPE_HPP

#include <cstdint>

#include <glm/vec3.hpp>

#include "physics/GraphicsToPhysicsDataMapping.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/hashing/HashCombine.hpp"

namespace iyf {
/// List of all collision types supported by the engine
enum class CollisionShapeType : std::uint8_t {
    Sphere,
    Box,
    Capsule,
    StaticPlane,
    ConvexHull,
    TriangleMesh,
    Heightfield
};

enum class CapsuleShapeAxis : std::uint8_t {
    X,
    Y,
    Z
};

class CollisionShapeCreateInfoBase {
public:
    CollisionShapeCreateInfoBase() : cacheKey(0) { }
    
    virtual CollisionShapeType getCollisionShapeType() const = 0;
    virtual ~CollisionShapeCreateInfoBase() {}
    
    /// Returns a cache key that allows the PhysicsSystem to reuse collision shapes
    inline StringHash getCacheKey() const {
        return cacheKey;
    }
    
    inline void setCacheKey(StringHash cacheKey) {
        this->cacheKey = cacheKey;
    }
    
    inline bool hasCacheKey() const {
        return cacheKey.value() != 0;
    }
    
    virtual bool compare(const CollisionShapeCreateInfoBase& other) const {
        return (getCollisionShapeType() == other.getCollisionShapeType()) && (cacheKey == other.cacheKey);
    }
private:
    StringHash cacheKey;
};

class SphereCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    SphereCollisionShapeCreateInfo(float radius) : CollisionShapeCreateInfoBase(), radius(radius) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::Sphere;
    }

    /// Radius of the sphere
    float radius;
    
    friend bool operator==(const SphereCollisionShapeCreateInfo& left, const SphereCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.radius == right.radius);
    }
};

class BoxCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    BoxCollisionShapeCreateInfo(glm::vec3 halfExtents) : CollisionShapeCreateInfoBase(), halfExtents(halfExtents) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::Box;
    }

    /// Half extents of the box
    glm::vec3 halfExtents;
    
    friend bool operator==(const BoxCollisionShapeCreateInfo& left, const BoxCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.halfExtents == right.halfExtents);
    }
};

class CapsuleCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    CapsuleCollisionShapeCreateInfo(float radius, float height, CapsuleShapeAxis capsuleShapeAxis) : CollisionShapeCreateInfoBase(), radius(radius), height(height), capsuleShapeAxis(capsuleShapeAxis) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::Capsule;
    }
    
    float radius;
    float height;
    CapsuleShapeAxis capsuleShapeAxis;
    
    friend bool operator==(const CapsuleCollisionShapeCreateInfo& left, const CapsuleCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.radius == right.radius) && (left.height == right.height) && (left.capsuleShapeAxis == right.capsuleShapeAxis);
    }
};

class StaticPlaceCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    StaticPlaceCollisionShapeCreateInfo(const glm::vec3 normal, float constant) : CollisionShapeCreateInfoBase(), normal(normal), constant(constant) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::StaticPlane;
    }

    glm::vec3 normal;
    float constant;
    
    friend bool operator==(const StaticPlaceCollisionShapeCreateInfo& left, const StaticPlaceCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.normal == right.normal) && (left.constant == right.constant);
    }
};

class ConvexHullCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    /// \warning Make sure that whatever the vertexData parameter is pointing to survives until the convex hull creation call returns.
    /// If the call fails to find cached data or allowCachedShapeReuse is set to false, it will make a copy of the vertices and then
    /// you'll be free to free (heh) the array.
    ///
    /// \todo Sane limit for the number of vertices
    ConvexHullCollisionShapeCreateInfo(const GraphicsToPhysicsDataMapping vertexData) : CollisionShapeCreateInfoBase(), vertexData(std::move(vertexData)) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::ConvexHull;
    }

    GraphicsToPhysicsDataMapping vertexData;
    
    friend bool operator==(const ConvexHullCollisionShapeCreateInfo& left, const ConvexHullCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.vertexData == right.vertexData);
    }
};

class TriangleMeshCollisionShapeCreateInfo : public CollisionShapeCreateInfoBase {
public:
    /// \warning Both vertexData and indexData must survive for as long as the collision shape is in use.
    ///
    /// \todo Does the data really need to survive?
    ///
    /// \todo Materials
    TriangleMeshCollisionShapeCreateInfo(GraphicsToPhysicsDataMapping vertexData, GraphicsToPhysicsDataMapping indexData) : CollisionShapeCreateInfoBase(), vertexData(std::move(vertexData)), indexData(std::move(indexData)) {}
    
    virtual CollisionShapeType getCollisionShapeType() const final override {
        return CollisionShapeType::TriangleMesh;
    }
    
    GraphicsToPhysicsDataMapping vertexData;
    GraphicsToPhysicsDataMapping indexData;
    
    friend bool operator==(const TriangleMeshCollisionShapeCreateInfo& left, const TriangleMeshCollisionShapeCreateInfo& right) {
        return left.compare(right) && (left.vertexData == right.vertexData) && (left.indexData == right.indexData);
    }
};
}

namespace std {
template<>
struct hash<iyf::SphereCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::SphereCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

template<>
struct hash<iyf::CapsuleCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::CapsuleCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

template<>
struct hash<iyf::BoxCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::BoxCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

template<>
struct hash<iyf::StaticPlaceCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::StaticPlaceCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

template<>
struct hash<iyf::ConvexHullCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::ConvexHullCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

template<>
struct hash<iyf::TriangleMeshCollisionShapeCreateInfo> {
    std::size_t operator()(const iyf::TriangleMeshCollisionShapeCreateInfo& ci) const noexcept {
        return ci.getCacheKey();
    }
};

}

#endif // IYF_COLLISION_SHAPE_TYPE_HPP
