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

#ifndef COLLISION_SHAPE_TYPE_HPP
#define COLLISION_SHAPE_TYPE_HPP

#include <cstdint>

#include <glm/vec3.hpp>

#include "physics/GraphicsToPhysicsDataMapping.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/hashing/HashCombine.hpp"
#include "glm/gtx/hash.hpp"

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

/// A key value that uniquely identifies a collision shape. Used in unordered_maps when caching
class CollisionShapeKey {
public:
    constexpr inline CollisionShapeKey(std::size_t key) : key(key) { }
    
    std::size_t getKey() const {
        return key;
    }
    
    friend bool operator==(const CollisionShapeKey& left, const CollisionShapeKey& right) {
        return left.key == right.key;
    }
private:
    std::size_t key;
};

struct CollisionShapeCreateInfo {
    CollisionShapeCreateInfo(bool allowCachedShapeReuse = true) : allowCachedShapeReuse(allowCachedShapeReuse) { }
    
    /// The type of the collision shape that you want to create. Should be automatically
    /// set by the constructors of all structs deriving from CollisionShapeCreateInfo and
    /// not changed by the user.
    CollisionShapeType collisionShapeType;
    /// If this is true, the PhysicsSystem will try to find and reuse an already created/loaded CollisionShape.
    /// This saves memory, however, the lookup takes some time.
    ///
    /// \todo Allow the user to perform a cache lookup and store a temporary reference to a shape.
    /// The said reference should allow the user to prepare multiple RigidBodyComponent objects that
    /// use the same shape.
    bool allowCachedShapeReuse;
    
    CollisionShapeKey makeKey() const {
        throw std::logic_error("This should be hidden by deriving classes.");
    }
};

struct SphereCollisionShapeCreateInfo : public CollisionShapeCreateInfo {
    SphereCollisionShapeCreateInfo(float radius, bool allowCachedShapeReuse = true) : CollisionShapeCreateInfo(allowCachedShapeReuse), radius(radius) {
        collisionShapeType = CollisionShapeType::Sphere;
    }
    
    /// Radius of the sphere
    float radius;
    
    CollisionShapeKey makeKey() const {
        return std::hash<float>{}(radius);
    }
    
    friend bool operator==(const SphereCollisionShapeCreateInfo& left, const SphereCollisionShapeCreateInfo& right) {
        return left.radius == right.radius;
    }
};

struct BoxCollisionShapeCreateInfo : public CollisionShapeCreateInfo {
    BoxCollisionShapeCreateInfo(const glm::vec3& halfExtents, bool allowCachedShapeReuse = true) : CollisionShapeCreateInfo(allowCachedShapeReuse), halfExtents(halfExtents) {
        collisionShapeType = CollisionShapeType::Box;
    }
    
    /// Half extents of the box
    glm::vec3 halfExtents;
    
    CollisionShapeKey makeKey() const {
        return std::hash<glm::vec3>{}(halfExtents);
    }
    
    friend bool operator==(const BoxCollisionShapeCreateInfo& left, const BoxCollisionShapeCreateInfo& right) {
        return left.halfExtents == right.halfExtents;
    }
};

struct CapsuleCollisionShapeCreateInfo : public CollisionShapeCreateInfo {
    CapsuleCollisionShapeCreateInfo(float radius, float height, CapsuleShapeAxis capsuleShapeAxis, bool allowCachedShapeReuse = true) : CollisionShapeCreateInfo(allowCachedShapeReuse), radius(radius), height(height), capsuleShapeAxis(capsuleShapeAxis) {
        collisionShapeType = CollisionShapeType::Capsule;
    }
    
    float radius;
    float height;
    CapsuleShapeAxis capsuleShapeAxis;
    
    CollisionShapeKey makeKey() const {
        size_t seed = 0;

        util::HashCombine(seed, std::hash<float>{}(radius));
        util::HashCombine(seed, std::hash<float>{}(height));
        util::HashCombine(seed, std::hash<std::underlying_type<CapsuleShapeAxis>::type>{}(static_cast<std::underlying_type<CapsuleShapeAxis>::type>(capsuleShapeAxis)));
        
        return seed;
    }
    
    friend bool operator==(const CapsuleCollisionShapeCreateInfo& left, const CapsuleCollisionShapeCreateInfo& right) {
        return (left.radius == right.radius) && (left.height == right.height) && (left.capsuleShapeAxis == right.capsuleShapeAxis);
    }
};

struct StaticPlaceCollisionShapeCreateInfo : public CollisionShapeCreateInfo {
    StaticPlaceCollisionShapeCreateInfo(const glm::vec3& normal, float constant, bool allowCachedShapeReuse = true) : CollisionShapeCreateInfo(allowCachedShapeReuse), normal(normal), constant(constant) {
        collisionShapeType = CollisionShapeType::StaticPlane;
    }
    
    glm::vec3 normal;
    float constant;
    
    CollisionShapeKey makeKey() const {
        size_t seed = 0;

        util::HashCombine(seed, std::hash<glm::vec3>{}(normal));
        util::HashCombine(seed, std::hash<float>{}(constant));
        
        return seed;
    }
    
    friend bool operator==(const StaticPlaceCollisionShapeCreateInfo& left, const StaticPlaceCollisionShapeCreateInfo& right) {
        return (left.normal == right.normal) && (left.constant == right.constant);
    }
};

/// Uses a tag (typically a hashed name of the 3D mesh) to implement cached data lookup in physical media
struct TaggedCollisionShapeCreateInfo : public CollisionShapeCreateInfo {
    TaggedCollisionShapeCreateInfo(StringHash tag, bool rebuildCachedData = false, bool allowCachedShapeReuse = true) : CollisionShapeCreateInfo(allowCachedShapeReuse), rebuildCachedData(rebuildCachedData), tag(tag) {}
    
    CollisionShapeKey makeKey() const {
        return CollisionShapeKey(tag);
    }
    
    /// Certain collision shapes (e.g. convex hulls and triangle meshes) require additional processing before they can be used.
    /// The processing can be lengthy, therefore, the results are cached and writen to physical media. This process happens automatically
    /// when a non-cached mesh (determined by the tag) is first loaded, regardless of the rebuildCachedData value. 
    ///
    /// If the cached data already exists, setting rebuildCachedData to true will rebuild it. This is typically done when a mesh
    /// is updated and reimported.
    bool rebuildCachedData;
    /// A tag that must uniquely identify the mesh that's going to be processed. It is also used to find loaded shapes for reuse when
    /// allowCachedShapeReuse is set to true
    StringHash tag;
};

struct ConvexHullCollisionShapeCreateInfo : public TaggedCollisionShapeCreateInfo {
    /// \warning Make sure that whatever the vertexData parameter is pointing to survives until the convex hull creation call returns.
    /// If the call fails to find cached data or allowCachedShapeReuse is set to false, it will make a copy of the vertices and then
    /// you'll be free to free (heh) the array.
    ///
    /// \todo Sane limit for the number of vertices
    ConvexHullCollisionShapeCreateInfo(const GraphicsToPhysicsDataMapping& vertexData, StringHash tag, bool rebuildCachedData = false, bool allowCachedShapeReuse = true) : TaggedCollisionShapeCreateInfo(tag, rebuildCachedData, allowCachedShapeReuse), vertexData(vertexData) {
        collisionShapeType = CollisionShapeType::ConvexHull;
    }

    GraphicsToPhysicsDataMapping vertexData;
    
    friend bool operator==(const ConvexHullCollisionShapeCreateInfo& left, const ConvexHullCollisionShapeCreateInfo& right) {
        return (left.tag == right.tag);
    }
};

struct TriangleMeshCollisionShapeCreateInfo : public TaggedCollisionShapeCreateInfo {
    /// \warning Both vertexData and indexData must survive for as long as the collision shape is in use.
    ///
    /// \todo Does the data really need to survive?
    ///
    /// \todo Materials
    TriangleMeshCollisionShapeCreateInfo(const GraphicsToPhysicsDataMapping& vertexData, const GraphicsToPhysicsDataMapping& indexData, StringHash tag, bool rebuildCachedData = false, bool allowCachedShapeReuse = true) : TaggedCollisionShapeCreateInfo(tag, rebuildCachedData, allowCachedShapeReuse), vertexData(vertexData), indexData(indexData) {
        collisionShapeType = CollisionShapeType::TriangleMesh;
    }
    
    GraphicsToPhysicsDataMapping vertexData;
    GraphicsToPhysicsDataMapping indexData;
    
    friend bool operator==(const TriangleMeshCollisionShapeCreateInfo& left, const TriangleMeshCollisionShapeCreateInfo& right) {
        return (left.tag == right.tag);
    }
};

}

namespace std {
    template <>
    struct hash<iyf::CollisionShapeKey> {
        std::size_t operator()(const iyf::CollisionShapeKey& k) const { 
            return k.getKey();
        }
    };
}

#endif //COLLISION_SHAPE_TYPE_HPP
