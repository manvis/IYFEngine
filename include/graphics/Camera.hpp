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

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "core/Component.hpp"

namespace iyf {

class GraphicsSystem;
class TransformationComponent;
class Camera : public Component {
public:
    enum class Mode {
        Standard, ReverseZ, ReverseZInfiniteFar
    };
    
    static constexpr ComponentType Type = ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Camera);
    
    Camera();
    Camera(glm::uvec2 renderSurfaceSize);
    
    virtual void attach(System* system, std::uint32_t ownID) final override;
    virtual void detach(System* system, std::uint32_t ownID) final override;
    
    void update();
    
    inline void setExposure(float exposure) {
        this->exposure = exposure;
    }
    
    inline float getExposure() const {
        return exposure;
    }
    
    void setMode(Mode newMode) {
        mode = newMode;
        
        projectionNeedsUpdate = true;
    }
    
    Mode getMode() const {
        return mode;
    }

    inline void setRenderSurfaceSize(glm::uvec2 renderSurfaceSize) {
        this->renderSurfaceSize = renderSurfaceSize;

        aspect = (float) renderSurfaceSize.x / (float) renderSurfaceSize.y;
        
        projectionNeedsUpdate = true;
    }

    inline float getVerticalFOV() const {
        return fovY;
    }

    inline void setVerticalFOV(float fov) {
        fovY = fov;
        
        projectionNeedsUpdate = true;
    }

    inline void setHorizontalFOV(float fov) {
        float tanDivAspect = std::tan(fov * 0.5f) / aspect;
        fovY = 2 * std::atan(tanDivAspect);
        
        projectionNeedsUpdate = true;
    }

    inline float getHorizontalFOV() const {
        float tanMulAspect = std::tan(fovY * 0.5f) * aspect;
        return 2 * std::atan(tanMulAspect);
    }

    inline void setClippingDistance(float nearDistance, float farDistance) {
        this->nearDistance = nearDistance;
        this->farDistance = farDistance;

        projectionNeedsUpdate = true;
    }
    
    inline float getNearDistance() const {
        return nearDistance;
    }

    inline float getFarDistance() const {
        return farDistance;
    }

    inline float getAspectRatio() const {
        return aspect;
    }

    inline glm::uvec2 getRenderSurfaceSize() const {
        return renderSurfaceSize;
    }

    inline glm::mat4 getProjection() const {
        return projectionMatrix;
    }
    
    glm::mat4 getViewMatrix() const {
        return viewMatrix;
    }
    
    inline glm::mat4 getProjection(Mode requiredMode) const {
        if (mode == requiredMode && !projectionNeedsUpdate) {
            return projectionMatrix;
        } else {
            return computeProjection(requiredMode);
        }
    }
    
    inline glm::vec3 getForward() const {
        return forward;
    }
    
    inline glm::vec3 getRight() const {
        return right;
    }
    
    inline glm::vec3 getUp() const {
        return up;
    }
    
    glm::vec3 getPosition() const;
    
//     TransformationComponent& getTransformation() {
//         return *transformation;
//     }
//     
//     const TransformationComponent& getTransformation() const {
//         return *transformation;
//     }
private:
    friend class GraphicsSystem;
    TransformationComponent* transformation;
    glm::mat4 computeProjection(Mode requiredMode) const;

    float nearDistance;
    float farDistance;

    float aspect;
    glm::uvec2 renderSurfaceSize;
    
    std::uint32_t transformationUpdateCount;

    float fovY;
    
    float exposure;

    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 forward;
    
    Mode mode;
    bool projectionNeedsUpdate;
};

}
#endif //CAMERA_HPP
