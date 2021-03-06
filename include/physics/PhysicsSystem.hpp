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


#ifndef IYF_PHYSICS_SYSTEM_HPP
#define IYF_PHYSICS_SYSTEM_HPP
#include <vector>

#include "core/EntitySystemManager.hpp"
#include "physics/RigidBody.hpp"
#include "physics/CollisionShape.hpp"

namespace iyf {
class Camera;
class DebugRenderer;

class PhysicsEngineData {
public:
    virtual ~PhysicsEngineData() {}
};

/// \todo This uses Bullet physics exclusively. It would be nice to move the physics engine behind an abstraction
/// so that engines could be changed freely
///
/// \todo Implement a cache for collision shapes. It should probably include reference counting as well. At the moment, create???RigidBody methods create new shapes regardle
class PhysicsSystem : public System {
public:
    PhysicsSystem(EntitySystemManager* manager);
    
    virtual void rayPick(std::uint32_t x, std::uint32_t y, const Camera& camera) = 0;
    
    virtual bool isDrawingDebug() const = 0;
    virtual void setDrawDebug(bool value) = 0;
    
    virtual std::size_t getSubTypeCount() const final override {
        return static_cast<std::size_t>(PhysicsComponent::COUNT);
    }
    
    inline const DebugRenderer* getDebugRenderer() const {
        return debugRenderer.get();
    }
    
    void setPhysicsEngineData(RigidBody& rigidBody, PhysicsEngineData* data);
protected:
    friend class GraphicsSystem;
    
    std::unique_ptr<DebugRenderer> debugRenderer;
};
}


#endif // IYF_PHYSICS_SYSTEM_HPP
