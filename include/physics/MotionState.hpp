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


#ifndef MOTION_STATE_HPP
#define MOTION_STATE_HPP

#include <stdexcept>
#include <btBulletDynamicsCommon.h>

#include "core/TransformationComponent.hpp"

namespace iyf {
class MotionState : public btMotionState {
public:
    MotionState() : transformation(nullptr) { }
    MotionState(TransformationComponent* transformation) : transformation(transformation) { }
    
    virtual ~MotionState() { }
    
    const TransformationComponent* getTransformation() const {
        return transformation;
    }
    
    virtual void getWorldTransform(btTransform& worldTransform) const final {
        if (transformation == nullptr) {
            throw std::logic_error("null transformation");
        }
        
        const glm::vec3& origin = transformation->getPosition();
        worldTransform.setOrigin(btVector3(origin.x, origin.y, origin.z));
        
        const glm::quat rotation = transformation->getRotation();
        worldTransform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
    }
    
    virtual void setWorldTransform(const btTransform& worldTransform) final {
        if (transformation == nullptr) {
            throw std::logic_error("null transformation");
        }
        
        const btQuaternion& rotation = worldTransform.getRotation();
        transformation->setRotation(glm::quat(rotation.w(), rotation.x(), rotation.y(), rotation.z()));
        
        const btVector3& origin = worldTransform.getOrigin();
        transformation->setPosition(origin.x(), origin.y(), origin.z());
    }
protected:
    TransformationComponent* transformation;
};
}

#endif // MOTION_STATE_HPP
