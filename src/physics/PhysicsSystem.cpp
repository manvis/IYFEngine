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

#include "physics/PhysicsSystem.hpp"
#include "core/Engine.hpp"
#include "physics/BulletPhysicsDebugRenderer.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "graphics/Camera.hpp"
#include "graphics/GraphicsSystem.hpp"
#include "threading/ThreadProfiler.hpp"

namespace iyf {
PhysicsSystem::PhysicsSystem(EntitySystemManager* manager) : System(manager, ComponentBaseType::Physics, static_cast<std::size_t>(PhysicsComponent::COUNT)), drawDebug(false) { }

void PhysicsSystem::initialize() {
    //colliders.reserve();
    components[static_cast<std::size_t>(PhysicsComponent::Collider)] = std::make_unique<ChunkedColliderVector>(this, ComponentType(ComponentBaseType::Physics, PhysicsComponent::Collider));
    
    // TODO multithreaded Bullet? https://github.com/bulletphysics/bullet3/blob/master/examples/MultiThreadedDemo/CommonRigidBodyMTBase.cpp
    
    // Bullet has multiple broadphases available: http://bulletphysics.org/mediawiki-1.5.8/index.php/Broadphase
    //
    // - btDbvtBroadphase (http://bulletphysics.org/Bullet/BulletFull/structbtDbvtBroadphase.html#details). Docs say
    // this is the best general purpose broadphase. It automatically adapts to the size of the world and is very fast,
    // especially for very dynamic worlds with many moving objects. Moreover, it is mentioned that insertion and removal
    // to/from this broadphase is also very fast
    //
    // - According to the docs, bt32BitAxisSweep3 (http://bulletphysics.org/Bullet/BulletFull/classbt32BitAxisSweep3.html#details)
    // and btAxisSweep3 (http://bulletphysics.org/Bullet/BulletFull/classbtAxisSweep3.html#details) are best for mostly
    // static worlds and both require knowing the maximum size of the world in advance. The difference between them is that
    // btAxisSweep3 is faster, but only supports smaller worlds with up to 16384 objects, while bt32BitAxisSweep3 is 
    // slower and consumes more memory, but supports up to 1500000 objects.
    //
    // TODO for now, I'm using btDbvtBroadphase, but it would be nice to expose a way for the users of the engine to easily change it.
    broadphase = new btDbvtBroadphase();
    
    // Docs say that this object is used to fine tune collision algorithms. I don't know much about them and I'd rather not mess with them.
    collisionConfiguration = new btDefaultCollisionConfiguration();
    // TODO docs say that some collision algorithms need to be registered to the dispatcher. Do I need to register anything?
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    
    // There are more solvers, but I can't find much info on which ones to use, so I'm using the one that's used in most samples
    solver = new btSequentialImpulseConstraintSolver;
    
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    worldBottomShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    worldBottomState = btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -10, 0)));
    btRigidBody::btRigidBodyConstructionInfo rbci(0, &worldBottomState, worldBottomShape, btVector3(0, 0, 0));
    worldBottom = new btRigidBody(rbci);
    dynamicsWorld->addRigidBody(worldBottom);
}

void PhysicsSystem::rayPick(std::uint32_t x, std::uint32_t y, const Camera& camera) {
    glm::vec3 windowCoords(static_cast<float>(x), static_cast<float>(y), 0.0f);
    
    const glm::uvec2 renderSurfaceSize = camera.getRenderSurfaceSize();
    glm::vec4 viewport(0.0f, 0.0f, renderSurfaceSize.x, renderSurfaceSize.y);
    
    glm::vec3 nearPick = glm::unProject(windowCoords, camera.getViewMatrix(), camera.getProjection(), viewport);
    
    glm::vec3 cameraPosition = camera.getPosition();
    
    btVector3 fromWorld(cameraPosition.x, cameraPosition.y, cameraPosition.z);
    btVector3 toWorld(nearPick.x, nearPick.y, nearPick.z);
    
    btCollisionWorld::ClosestRayResultCallback rayCallback(fromWorld, toWorld);
    dynamicsWorld->rayTest(fromWorld, toWorld, rayCallback);
    
    if (rayCallback.hasHit()) {
        btRigidBody* rigidBody = (btRigidBody*)btRigidBody::upcast(rayCallback.m_collisionObject);
        
        if (rigidBody) {
            void* ptr = rigidBody->getUserPointer();
            
            // TODO notify about picks
            if (ptr != nullptr) {
                LOG_D("TODO NOTIFY ABOUT A PICK {}", ptr)
            }
        }
    }
}

Collider PhysicsSystem::prepareSphereRigidBody(const SphereCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::Sphere);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.sphereCache.getShapeHandle(createInfo);
    component.mass = mass;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

Collider PhysicsSystem::prepareBoxRigidBody(const BoxCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::Box);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.boxCache.getShapeHandle(createInfo);
    component.mass = mass;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

Collider PhysicsSystem::prepareCapsuleRigidBody(const CapsuleCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::Capsule);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.capsuleCache.getShapeHandle(createInfo);
    component.mass = mass;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

Collider PhysicsSystem::prepareStaticPlaneRigidBody(const StaticPlaceCollisionShapeCreateInfo& createInfo, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::StaticPlane);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.staticPlaneCache.getShapeHandle(createInfo);
    component.mass = 0;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

Collider PhysicsSystem::prepareConvexHullRigidBody(const ConvexHullCollisionShapeCreateInfo& createInfo, float mass, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::ConvexHull);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.convexHullCache.getShapeHandle(createInfo);
    component.mass = mass;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

Collider PhysicsSystem::prepareTriangleMeshRigidBody(const TriangleMeshCollisionShapeCreateInfo& createInfo, TransformationComponent* transformation) {
    assert(createInfo.collisionShapeType == CollisionShapeType::TriangleMesh);
    
    Collider component;
    
    component.collisionShape = collisionShapeCacheManager.triangleMeshCache.getShapeHandle(createInfo);
    component.mass = 0;
    
    component.motionState = MotionState(transformation);
    
    return component;
}

void PhysicsSystem::dispose() {
    if (debugRenderer != nullptr) {
        debugRenderer->dispose();
        debugRenderer = nullptr;
    }
    drawDebug = false;
    
    dynamicsWorld->removeRigidBody(worldBottom);
    delete worldBottom;
    
    delete worldBottomShape;
    
    collisionShapeCacheManager.collectGarbage(GarbageCollectionRunPolicy::FullCollectionDuringDestruction);
    
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
}

void PhysicsSystem::update(float delta, const EntityStateVector& entityStates) {
    IYFT_PROFILE(PhysicsUpdate, iyft::ProfilerTag::Physics);
    
    // http://bulletphysics.org/mediawiki-1.5.8/index.php/Stepping_The_World
    dynamicsWorld->stepSimulation(delta, 8);
    
    if (drawDebug) {
        debugRenderer->update(delta);
        dynamicsWorld->debugDrawWorld();
    }
}

void PhysicsSystem::setDrawDebug(bool value) {
    if (value == drawDebug) {
        return;
    }
    
    drawDebug = value;
    
    if (drawDebug) {
        assert(debugRenderer == nullptr);
        
        debugRenderer = std::make_unique<BulletPhysicsDebugRenderer>(manager->getEngine()->getAssetManager(), manager->getEngine()->getRenderer());
        debugRenderer->initialize();
        
        btIDebugDraw* temp = dynamic_cast<BulletPhysicsDebugRenderer*>(debugRenderer.get());
        temp->setDebugMode(BulletPhysicsDebugRenderer::DebugDrawModes::DBG_DrawWireframe |
                           BulletPhysicsDebugRenderer::DebugDrawModes::DBG_DrawAabb |
                           BulletPhysicsDebugRenderer::DebugDrawModes::DBG_DrawConstraints);
        dynamicsWorld->setDebugDrawer(temp);
    } else {
        assert(debugRenderer != nullptr);
        debugRenderer->dispose();
        debugRenderer = nullptr;
        
        dynamicsWorld->setDebugDrawer(nullptr);
    }
}

void PhysicsSystem::createAndAttachComponent(const EntityKey& key, const ComponentType& type) {
    assert(!hasComponent(key.getID(), type));
    
    PhysicsComponent component = static_cast<PhysicsComponent>(type.getSubType());
    switch (component) {
        case PhysicsComponent::Collider: {
            /// TODO determine size (and shape?) from a Mesh component if one exists
            /// TODO Is 1.0f a good default mass for non-static Entities?
            TransformationComponent& transformation = manager->getEntityTransformation(key.getID());
            
            SphereCollisionShapeCreateInfo ci(1.0f);
            Collider collider = prepareSphereRigidBody(ci, transformation.isStatic() ? 0.0f : 1.0f, &transformation);
            
            setComponent(key.getID(), std::move(collider));
            
            break;
        }
        case PhysicsComponent::COUNT:
            throw std::runtime_error("COUNT is an invalid value");
    }
}

}
