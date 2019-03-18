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


#ifndef IYF_BULLET_PHYSICS_SYSTEM_HPP
#define IYF_BULLET_PHYSICS_SYSTEM_HPP

#include <btBulletDynamicsCommon.h>
#include <vector>

#include "physics/PhysicsSystem.hpp"
#include "core/ChunkedComponentVector.hpp"

namespace iyf {
class BulletPhysicsShapeCache;

using ChunkedRigidBodyVector = ChunkedComponentVector<RigidBody>;

class BulletPhysicsSystem : public PhysicsSystem {
public:
    BulletPhysicsSystem(EntitySystemManager* manager);
    virtual ~BulletPhysicsSystem();
    
    virtual void initialize() final override;
    virtual void dispose() final override;
    
    virtual void preAttach(Component& component, std::uint32_t id) final override;
    virtual void postDetach(Component& component, std::uint32_t id) final override;
    
    virtual void rayPick(std::uint32_t x, std::uint32_t y, const Camera& camera) final override;
    
    virtual void update(float delta, const EntityStateVector& entityStates) final override;
    
    virtual Component& createAndAttachComponent(const EntityKey& key, const ComponentType& type) final override;
    
    virtual void collectGarbage(GarbageCollectionRunPolicy policy) final override;
    
    virtual bool isDrawingDebug() const  final override {
        return drawDebug;
    }
    
    virtual void setDrawDebug(bool value) final override;
protected:
    friend class Collider;
    // For drawing physics debug. This should be removed, though
    friend class GraphicsSystem;
    
    btDynamicsWorld* getPhysicsWorld() {
        return dynamicsWorld;
    }
    
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btConstraintSolver* solver;
    btDynamicsWorld* dynamicsWorld;
    
    // Bottom of world
    btCollisionShape* worldBottomShape;
    btDefaultMotionState* worldBottomState;
    btRigidBody* worldBottom;
    
    std::unique_ptr<BulletPhysicsShapeCache> shapeCache;
    
    bool drawDebug;
};
}


#endif // IYF_BULLET_PHYSICS_SYSTEM_HPP

