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

#include "graphics/Camera.hpp"

#include "logging/Logger.hpp"
#include "core/EntitySystemManager.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

namespace iyf {

Camera::Camera() : Component(Camera::Type), transformation(nullptr), nearDistance(0.1f), farDistance(100.0f), aspect(1.0f), renderSurfaceSize(1.0f, 1.0f), fovY(glm::radians(45.0f)), exposure(1.0f), mode(Mode::ReverseZ), projectionNeedsUpdate(true) { }

Camera::Camera(glm::uvec2 renderSurfaceSize) : Camera() {
    this->renderSurfaceSize = renderSurfaceSize;
    
    aspect = (float) renderSurfaceSize.x / (float) renderSurfaceSize.y;
}

void Camera::attach(System* system, std::uint32_t ownID) {
    EntitySystemManager* manager = system->getManager();
    transformation = &manager->getEntityTransformation(ownID);
}

void Camera::detach(System*, std::uint32_t) {
    transformation = nullptr;
}

void Camera::onTransformationChanged(TransformationComponent* /*transformation*/) {}

void Camera::update() {
    // Update projection
    if (projectionNeedsUpdate) {
        projectionMatrix = computeProjection(mode);
        projectionNeedsUpdate = false;
    }
    
    // update view
    // TODO use onTransformationChanged instead of this
    std::uint32_t currentUpdateCount = transformation->getUpdateCount();
    if (currentUpdateCount != transformationUpdateCount) {
        transformationUpdateCount = currentUpdateCount;
    
        glm::vec3 position = transformation->getPosition();
        position.y *= 1.0f;
        glm::quat rotation = transformation->getRotation();//glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f)) * transformation->getRotation();//
//         rotation.w *= -1.0f;
        
        viewMatrix = glm::inverse(glm::translate(position) * glm::mat4_cast(rotation));// * glm::scale(glm::vec3(-1.0f, 1.0f, 1.0f))
//         viewMatrix = glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
//         viewMatrix = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f));
        right = glm::row(viewMatrix, 0);
        up = glm::row(viewMatrix, 1);
        forward = glm::row(viewMatrix, 2);
        
//         rotation = glm::normalize(rotation);
//         
//         viewMatrix = glm::mat4_cast(rotation);
//         viewMatrix = glm::translate(viewMatrix, position);
//         
//         glm::mat4 modView = glm::scale(viewMatrix, glm::vec3(-1.0f, 1.0f, 1.0f));
//         glm::mat4 glmLook = glm::mat4_cast(glm::quatLookAtLH(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
//         LOG_D("PRE: " << glm::to_string(viewMatrix) << "\n\tPOST: " << glm::to_string(modView) << "\n\tCORR: " << glm::to_string(glmLook) 
//             << "\n\tCR" << glm::to_string(glm::column(glmLook, 0))
//             << "\n\tCU" << glm::to_string(glm::column(glmLook, 1))
//             << "\n\tCF" << glm::to_string(glm::column(glmLook, 2))
//         );
//         
//         viewMatrix = glm::translate(glmLook, position);//modView;//
//         //viewMatrix = modView;
//         // column 0 is actually left, since we're using a left handed coordinate system
//         right = glm::column(viewMatrix, 0);//glm::conjugate(rotation) * glm::vec3(1.0f, 0.0f, 0.0f);
//         up = glm::column(viewMatrix, 1);//glm::conjugate(rotation) * glm::vec3(0.0f, 1.0f, 0.0f);
//         forward = glm::column(viewMatrix, 2);//glm::conjugate(rotation) * glm::vec3(0.0f, 0.0f, 1.0f);
//         //viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Building a view matrix by hand
//         forward = position - glm::rotate(rotation, glm::vec3(0.0f, 0.0f, 1.0f));
//         up = glm::rotate(rotation, glm::vec3(0.0f, 1.0f, 0.0f));
//         right = glm::rotate(rotation, glm::vec3(1.0f, 0.0f, 0.0f));
//         
//         forward = glm::normalize(forward);
//         up = glm::normalize(up);
//         right = glm::normalize(right);
//         
//         viewMatrix = glm::mat4(1.0f);
//         viewMatrix[0][0] = right.x;
//         viewMatrix[1][0] = right.y;
//         viewMatrix[2][0] = right.z;
//         viewMatrix[0][1] = up.x;
//         viewMatrix[1][1] = up.y;
//         viewMatrix[2][1] = up.z;
//         viewMatrix[0][2] = forward.x;
//         viewMatrix[1][2] = forward.y;
//         viewMatrix[2][2] = forward.z;
//         viewMatrix[3][0] = -glm::dot(right, position);
//         viewMatrix[3][1] = -glm::dot(up, position);
//         viewMatrix[3][2] = -glm::dot(forward, position);
//         
//         //viewMatrix = glm::scale(viewMatrix, glm::vec3(1.0f, -1.0f, 1.0f));
    }
}

glm::vec3 Camera::getPosition() const {
    return transformation->getPosition();
}

glm::mat4 Camera::computeProjection(Mode requiredMode) const {
    // http://dev.theomader.com/depth-precision/
    if (requiredMode == Mode::Standard) {
        glm::mat4 projection = glm::perspectiveLH_ZO(fovY, aspect, nearDistance, farDistance);
        projection = glm::scale(projection, glm::vec3(1.0f, -1.0f, 1.0f));
        return projection;
    } else if (requiredMode == Mode::ReverseZInfiniteFar) {
        // TODO is this correct?
        const float f = (glm::cos(0.5f * fovY) / glm::sin(0.5f * fovY));

        glm::mat4 projection = glm::mat4{
          f/aspect, 0.0f,         0.0f, 0.0f,
              0.0f,    f,         0.0f, 0.0f,
              0.0f, 0.0f,         0.0f, 1.0f,
              0.0f, 0.0f, nearDistance, 0.0f
        };
        
        projection = glm::scale(projection, glm::vec3(1.0f, -1.0f, 1.0f));
        return projection;
    } else {// Reverse Z
        glm::mat4 projection(0.0f);
        
        float halfFovTangent = glm::tan(fovY / 2.0f);
        projection[0][0] =  1.0f / (aspect * halfFovTangent);
        projection[1][1] =  1.0f / halfFovTangent;
        projection[2][3] =  1.0f;
        
        float FminusN = farDistance - nearDistance;
        float FmultN = farDistance * nearDistance;
        
        float C1 = -nearDistance / FminusN;
        float C2 = FmultN / FminusN;
        
        projection[2][2] = C1;
        projection[3][2] = C2;
        
        projection = glm::scale(projection, glm::vec3(1.0f, -1.0f, 1.0f));
        
        return projection;
    }
}

}
