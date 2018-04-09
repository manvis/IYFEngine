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

#ifndef TRANSFORMATION_COMPONENT_HPP
#define TRANSFORMATION_COMPONENT_HPP

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

namespace iyf {
class EntitySystemManager;
class Entity;

class TransformationComponent {
private:
    /// How often should the rotation quaternion be re-normalized?
    /// \todo This seems like a premature optimization. Why did I even do this? Should I re-normalize the rotation every time?
    /// If it's not a premature optimization, it can still cause problems. THIS MAY CAUSE EVERY 20th FRAME TO TAKE LONGER!!!
    /// Profile, check if this is really needed and, if it is + it affects performance of certain frames, add (subtract?) a random
    /// delay for each TransformationComponent. What's more, won't a bigger delay impact quality of transformation? Shit, I don't know...
    static const std::uint8_t RotationNormalizationFrequency = 20;
public:
    TransformationComponent() : transformDirty(true), staticObject(true), rotationUpdateCount(0), updateCount(0), position(0.0f, 0.0f, 0.0f), scaling(1.0f, 1.0f, 1.0f), rotation(1.0f, 0.0f, 0.0f, 0.0f) {}
    
    /// Updates the transformation matrix if position, rotation or scale have been changed.
    inline bool update() {
        if (!transformDirty) {
            return false;
        }
        
        performUpdate();
        return true;
    }
    
    /// Forces a transformation matrix update.
    inline void forcedUpdate() {
         performUpdate();
    }

    inline void setPosition(const glm::vec3 &newPosition) {
        position = newPosition;
        transformDirty = true;
    }
    
    inline void setPosition(float x, float y, float z) {
        position = glm::vec3(x, y, z);
        transformDirty = true;
    }
    
    inline void translate(const glm::vec3 &translation) {
        position += translation;
        transformDirty = true;
    }
    
    inline void translate(float x, float y, float z) {
        position += glm::vec3(x, y, z);
        transformDirty = true;
    }
    
    inline void translateRelative(const glm::vec3 &translation) {
        glm::vec3 temp = rotation * translation;
        position += temp;
        
        transformDirty = true;
    }
    
    inline void translateRelative(float x, float y, float z) {
        glm::vec3 temp = rotation * glm::vec3(x, y, z);
        position += temp;
        
        transformDirty = true;
    }

    inline void setRotation(const glm::quat &newRotation) {
        rotation = glm::normalize(newRotation);
        
        rotationUpdateCount = 0;
        transformDirty = true;
    }
    
    inline void rotate(const glm::quat &rotation) {
        this->rotation = rotation * this->rotation;
        
        rotationUpdateCount++;
        transformDirty = true;
    }
    
    inline void rotate(float angle, const glm::vec3 &axis) {
        rotate(glm::angleAxis(angle, glm::normalize(axis)));
    }
    
    inline void rotateRelative(const glm::quat &rotation) {
        this->rotation = this->rotation * rotation;
        
        rotationUpdateCount++;
        transformDirty = true;
    }
    
    inline void rotateRelative(float angle, const glm::vec3 &axis) {
        rotateRelative(glm::angleAxis(angle, glm::normalize(axis)));
    }
    
    inline void setScale(const glm::vec3 &newScale) {
        scaling = newScale;
        transformDirty = true;
    }
    
    inline void setScale(float x, float y, float z) {
        scaling = glm::vec3(x, y, z);
        transformDirty = true;
    }
    
    inline void scale(const glm::vec3 &newScale) {
        scaling *= newScale;
        transformDirty = true;
    }
    
    inline void scale(float x, float y, float z) {
        scaling *= glm::vec3(x, y, z);
        transformDirty = true;
    }

    inline const glm::vec3& getPosition() const {
        return position;
    }
    
    inline const glm::vec3& getScale() const {
        return scaling;
    }
    
    inline const glm::quat& getRotation() const {
        return rotation; 
    }
    
    inline const glm::mat4& getModelMatrix() const {
        return modelMatrix;
    }
    
    inline bool isDirty() const {
        return transformDirty;
    }
    
    inline void setStatic(bool value) {
        staticObject = value;
    }
    
    inline bool isStatic() const {
        return staticObject;
    }
    
    /// Resets the contents and prepares the TransformationComponent for reuse.
    ///
    /// \warning The entity field MUST NOT be cleared. It's only set once because it always corresponds to the
    /// same location in memory.
    inline void clear() {
        transformDirty = true;
        staticObject = true;
        rotationUpdateCount = 0;
        updateCount = 0;
        
        scaling = glm::vec3(1.0f, 1.0f, 1.0f);
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        
        parent = nullptr;
        children.clear();
    }
    
    /// Changes every time the transformation is updated. Helps to determine what to cache.
    std::uint32_t getUpdateCount() const {
        return updateCount;
    }
private:
    friend class EntitySystemManager;
    
    inline void performUpdate() {
        if (transformDirty) {
            modelMatrix = glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scaling);
            
            transformDirty = false;
            updateCount++;
        }

        if (rotationUpdateCount >= RotationNormalizationFrequency) {
            rotation = glm::normalize(rotation);
            rotationUpdateCount = 0;
        }
        
        transformDirty = false;
    }
    
    bool transformDirty;
    bool staticObject;
    std::uint16_t rotationUpdateCount;
    
    std::uint32_t updateCount;

    glm::vec3 position;
    glm::vec3 scaling;
    glm::quat rotation;
    glm::mat4 modelMatrix;
    Entity* entity;
    TransformationComponent* parent;
    std::vector<TransformationComponent*> children;
};
}

#endif // TRANSFORMATION_COMPONENT_HPP

