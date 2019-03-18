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

#ifndef IYF_RIGID_BODY_HPP
#define IYF_RIGID_BODY_HPP

#include "core/Component.hpp"
#include "physics/CollisionShape.hpp"

#include <variant>

namespace iyf {
class PhysicsEngineData;
class PhysicsSystem;

using CollisionShapeCreateInfo = std::variant<SphereCollisionShapeCreateInfo, BoxCollisionShapeCreateInfo, CapsuleCollisionShapeCreateInfo, StaticPlaceCollisionShapeCreateInfo, ConvexHullCollisionShapeCreateInfo, TriangleMeshCollisionShapeCreateInfo>;

class RigidBody : public Component {
public:
    static constexpr ComponentType Type = ComponentType(ComponentBaseType::Physics, PhysicsComponent::RigidBody);
    RigidBody();
    
    virtual void attach(System* system, std::uint32_t ownID) final override;
    virtual void detach(System* system, std::uint32_t ownID) final override;
    virtual void onTransformationChanged(TransformationComponent* transformation) final override;
    
    virtual ~RigidBody() { }
    
    void setCollisionShapeCreateInfo(CollisionShapeCreateInfo info);
    
    void setMass(float newMass);
    float getMass() const;
    
    const CollisionShapeCreateInfo& getCollisionShapeCreateInfo() const {
        return createInfo;
    }
    
    PhysicsEngineData* getPhysicsEngineData() {
        return engineData;
    }
private:
    friend class PhysicsSystem;
    
    void throwIfAttached();
    
    PhysicsEngineData* engineData;
    PhysicsSystem* physicsSystem;
    CollisionShapeCreateInfo createInfo;
    float mass;
};
}

#endif // IYF_RIGID_BODY_HPP
