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

#include "physics/RigidBody.hpp"
#include "physics/PhysicsSystem.hpp"
#include "logging/Logger.hpp"

namespace iyf {
RigidBody::RigidBody() : Component(ComponentType(ComponentBaseType::Physics, PhysicsComponent::RigidBody)), 
    engineData(nullptr), physicsSystem(nullptr), createInfo(SphereCollisionShapeCreateInfo(1.0f)), mass(0.0f) { }

void RigidBody::setMass(float newMass) {
    throwIfAttached();
    
    mass = newMass;
}

float RigidBody::getMass() const {
    return mass;
}

void RigidBody::setCollisionShapeCreateInfo(CollisionShapeCreateInfo info) {
    throwIfAttached();
    
    createInfo = info;
}

void RigidBody::attach(System* system, std::uint32_t) {
    physicsSystem = static_cast<PhysicsSystem*>(system);
}

void RigidBody::detach(System* system, std::uint32_t) {
    assert(system == physicsSystem);
    physicsSystem = nullptr;
}

void RigidBody::onTransformationChanged(TransformationComponent*) {
    // Simply ignore transformation changes.
}

void RigidBody::throwIfAttached() {
    if (physicsSystem != nullptr) {
        throw std::runtime_error("Can't edit an attached RigidBody");
    }
}

}
