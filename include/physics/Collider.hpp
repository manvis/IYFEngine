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

#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <btBulletDynamicsCommon.h>

// TODO replace with std::optional when support is mature enough
#include <boost/optional.hpp>

#include "core/Component.hpp"
#include "physics/CollisionShape.hpp"
#include "physics/CollisionShapeCache.hpp"
#include "physics/MotionState.hpp"

namespace iyf {
class Collider : public Component {
public:
    static constexpr ComponentType Type = ComponentType(ComponentBaseType::Physics, PhysicsComponent::Collider);
    Collider() : Component(ComponentType(ComponentBaseType::Physics, PhysicsComponent::Collider)) { }
    
    /// btRigidBody does not have an empty "invalid" constructor, but I really
    /// want to have all rigid bodies inside Collider objects, hence 
    /// the use of optional
    boost::optional<btRigidBody> rigidBody;
    
    /// MotionState wraps a pointer to a TransformationComponent. This is possible
    /// because TransformationComponents are stored in a ChunkedVector and their
    /// position in memory is consistent throughout the existence of an Entity.
    ///
    /// \todo is this really enough or do I need aligned storage?
    alignas(16) MotionState motionState;
    
    /// Required here because we delay the creation of the rigidBody component
    float mass;
    
    CollisionShapeHandle collisionShape;
    
    virtual void attach(System* system, std::uint32_t ownID) final;
    virtual void detach(System* system, std::uint32_t ownID) final;
    
    virtual ~Collider() { }
};
}

#endif // COLLIDER_HPP
