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


#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include <btBulletDynamicsCommon.h>
#include <vector>

#include "core/EntitySystemManager.hpp"
#include "physics/Collider.hpp"
#include "physics/MotionState.hpp"
#include "physics/CollisionShapeCache.hpp"
#include "graphics/DebugRenderer.hpp"
#include "core/ChunkedComponentVector.hpp"

namespace iyf {
class Camera;

using ChunkedColliderVector = ChunkedComponentVector<Collider>;

/// \todo This uses Bullet physics exclusively. It would be nice to move the physics engine behind an abstraction
/// so that engines could be changed freely
///
/// \todo Implement a cache for collision shapes. It should probably include reference counting as well. At the moment, create???RigidBody methods create new shapes regardle
class PhysicsSystem : public System {
public:
    PhysicsSystem(EntitySystemManager* manager);
    
    virtual void initialize() final override;
    virtual void dispose() final override;
    
    void rayPick(std::uint32_t x, std::uint32_t y, const Camera& camera);
    
    /// Prepares all data for the creation of a sphere shaped Collider. The actual RigidBody is created and added to the
    /// simulation world when EntitySystemManager::attachComponent() is called with the returned Collider as a parameter.
    ///
    /// \param[in] createInfo Information required to construct the collision shape
    /// \param[in] mass Mass of the object (0 implies a static object)
    /// \param[in] transformation Pointer to the TransformationComponent of the corresponding Entity that will be used to initialize
    /// the motion state.
    Collider prepareSphereRigidBody(const SphereCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation);
    
    Collider prepareBoxRigidBody(const BoxCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation);
    
    Collider prepareCapsuleRigidBody(const CapsuleCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation);
    
    Collider prepareStaticPlaneRigidBody(const StaticPlaceCollisionShapeCreateInfo& createInfo, TransformationComponent* transformation);
    
    /// \todo React to the rebuild parameter
    /// \todo To optimize or not to optimize
    Collider prepareConvexHullRigidBody(const ConvexHullCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation);
    
    /// \todo React to the rebuild parameter
    Collider prepareTriangleMeshRigidBody(const TriangleMeshCollisionShapeCreateInfo& createInfo, TransformationComponent* transformation);
    
    virtual void update(float delta, const EntityStateVector& entityStates) final override;
    
    virtual void createAndAttachComponent(const EntityKey& key, const ComponentType& type) final override;
    
    virtual void collectGarbage(GarbageCollectionRunPolicy policy) final override {
        collisionShapeCacheManager.collectGarbage(policy);
    }
    
    inline bool isDrawingDebug() const {
        return drawDebug;
    }
    
    virtual std::size_t getSubTypeCount() const final override {
        return static_cast<std::size_t>(PhysicsComponent::COUNT);
    }
    
    void setDrawDebug(bool value);
    
    const DebugRenderer* getDebugRenderer() const {
        return debugRenderer.get();
    }
protected:
    friend class Collider;
    // For drawing physics debug. This should be removed, though
    friend class GraphicsSystem;
    
    btDynamicsWorld* getPhysicsWorld() {
        return dynamicsWorld;
    }
    
    CollisionShapeCacheManager& getCollisionShapeCahce() {
        return collisionShapeCacheManager;
    }
    
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btConstraintSolver* solver;
    btDynamicsWorld* dynamicsWorld;
    
    CollisionShapeCacheManager collisionShapeCacheManager;
    
    // Bottom of world
    btCollisionShape* worldBottomShape;
    btDefaultMotionState worldBottomState;
    btRigidBody* worldBottom;
    
    std::unique_ptr<DebugRenderer> debugRenderer;
    
    bool drawDebug;
};
}


#endif // PHYSICS_SYSTEM_HPP
