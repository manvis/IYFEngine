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

#include "physics/Collider.hpp"
#include "physics/PhysicsSystem.hpp"
#include "core/Logger.hpp"

namespace iyf {
void Collider::attach(System* system, std::uint32_t) {
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(system);
    
    btVector3 localInertia(0, 0, 0);
    if (mass != 0) {
        collisionShape->calculateLocalInertia(mass, localInertia);
    }

    //LOG_D("AMM {}", *collisionShape);
    btRigidBody::btRigidBodyConstructionInfo rbci(mass, &motionState, *collisionShape, localInertia);
    rigidBody.emplace(rbci);
    rigidBody->setUserPointer(reinterpret_cast<void*>(42));
    
    // This is only safe because we use ChunkedVectors and their contents, unlike those of an std::vector, don't move in memory
    physicsSystem->getPhysicsWorld()->addRigidBody(&*(rigidBody));
}

void Collider::detach(System* system, std::uint32_t) {
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(system);
    
    physicsSystem->getPhysicsWorld()->removeRigidBody(&*(rigidBody));
    
    //LOG_D("BMM {}", *collisionShape);
    collisionShape = CollisionShapeHandle();
    rigidBody = std::nullopt;
    // TODO clean motion state
    //motionState = std::nullopt;
}

}
