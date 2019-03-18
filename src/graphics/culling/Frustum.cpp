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

#include "graphics/culling/Frustum.hpp"
#include "graphics/Camera.hpp"
#include "core/Logger.hpp"
#include "graphics/DebugRenderer.hpp"
#include "core/TransformationComponent.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/string_cast.hpp"

//#define IYF_LOG_FRUSTUM_PLANES
//#define IYF_LOG_FRUSTUM_VERTS

namespace iyf {
void Frustum::update(const Camera& camera) {
    const float halfFovTan = glm::tan(camera.getVerticalFOV() * 0.5f);
    
    const float nearHeight = camera.getNearDistance() * halfFovTan;
    const float nearWidth = camera.getAspectRatio() * nearHeight;
    
    const float farHeight = camera.getFarDistance() * halfFovTan;
    const float farWidth = camera.getAspectRatio() * farHeight;
    
    glm::vec3 Z = -camera.getForward();
    glm::vec3 X =  camera.getRight();
    glm::vec3 Y =  camera.getUp();
    
    // Near vertices
    const glm::vec3 nearCenter = camera.getPosition() - Z * camera.getNearDistance();
    
    const glm::vec3 nA = nearCenter + Y * nearHeight;
    const glm::vec3 nB = nearCenter - Y * nearHeight;
    const glm::vec3 nC = X * nearWidth;
    
    const glm::vec3 tln = nA - nC;
    setFrustumVertex(Plane::Near, PlaneVertex::TopLeft,     tln);
    const glm::vec3 trn = nA + nC;
    setFrustumVertex(Plane::Near, PlaneVertex::TopRight,    trn);
    const glm::vec3 bln = nB - nC;
    setFrustumVertex(Plane::Near, PlaneVertex::BottomLeft,  bln);
    const glm::vec3 brn = nB + nC;
    setFrustumVertex(Plane::Near, PlaneVertex::BottomRight, brn);
    
    
    // Far vertices
    const glm::vec3 farCenter  = camera.getPosition() - Z * camera.getFarDistance();
    
    const glm::vec3 fA = farCenter + Y * farHeight;
    const glm::vec3 fB = farCenter - Y * farHeight;
    const glm::vec3 fC = X * farWidth;
    
    const glm::vec3 tlf = fA - fC;
    setFrustumVertex(Plane::Far, PlaneVertex::TopLeft,     tlf);
    const glm::vec3 trf = fA + fC;
    setFrustumVertex(Plane::Far, PlaneVertex::TopRight,    trf);
    const glm::vec3 blf = fB - fC;
    setFrustumVertex(Plane::Far, PlaneVertex::BottomLeft,  blf);
    const glm::vec3 brf = fB + fC;
    setFrustumVertex(Plane::Far, PlaneVertex::BottomRight, brf);

    planes[static_cast<std::size_t>(Plane::Left)] = makePlaneFromPoints(tln, bln, blf);
    planes[static_cast<std::size_t>(Plane::Right)] = makePlaneFromPoints(brn, trn, brf);
    planes[static_cast<std::size_t>(Plane::Top)] = makePlaneFromPoints(trn, tln, tlf);
    planes[static_cast<std::size_t>(Plane::Bottom)] = makePlaneFromPoints(bln, brn, brf);
    planes[static_cast<std::size_t>(Plane::Near)] = makePlaneFromPoints(tln, trn, brn);
    planes[static_cast<std::size_t>(Plane::Far)] = makePlaneFromPoints(trf, tlf, blf);
    
#ifdef IYF_LOG_FRUSTUM_PLANES
    LOG_D("LEFT: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Left)]) << "\n\t" <<
          "RIGHT: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Right)]) << "\n\t" <<
          "TOP: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Top)]) << "\n\t" <<
          "BOTTOM: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Bottom)]) << "\n\t" <<
          "NEAR: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Near)]) << "\n\t" <<
          "FAR: " << glm::to_string(planes[static_cast<std::size_t>(Frustum::Plane::Far)])
    );
#endif // IYF_LOG_FRUSTUM_PLANES
    
#ifdef IYF_LOG_FRUSTUM_VERTS
    LOG_D("POINTS: \n\tTLN" << glm::to_string(tln) << 
                  "\n\tBLN" << glm::to_string(bln) <<
                  "\n\tTRN" << glm::to_string(trn) <<
                  "\n\tBRN" << glm::to_string(brn) <<
                  "\n\tTLF" << glm::to_string(tlf) <<
                  "\n\tBLF" << glm::to_string(blf) <<
                  "\n\tTRF" << glm::to_string(trf) <<
                  "\n\tBRF" << glm::to_string(brf));
#endif // IYF_LOG_FRUSTUM_VERTS
}

void Frustum::setFrustumVertex(Plane plane, PlaneVertex vertex, const glm::vec3& vertexPos) {
    if (plane != Plane::Near && plane != Plane::Far) {
        throw std::runtime_error("Can't set this plane");
    }
    
    if (plane == Plane::Near) {
        nearVertices[static_cast<std::size_t>(vertex)] = vertexPos;
    } else {
        farVertices[static_cast<std::size_t>(vertex)] = vertexPos;
    }
}

glm::vec4 Frustum::makePlaneFromPoints(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
    const glm::vec3 temp1 = a - b;
    const glm::vec3 temp2 = c - b;
    
    const glm::vec3 normal = glm::normalize(glm::cross(temp2, temp1));
    const float d = -(glm::dot(b, normal));

    return glm::vec4(normal, d);
}

void Frustum::drawDebug(DebugRenderer* renderer) {
    const glm::vec3 color(1.0f, 0.0f, 0.0f);

    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::BottomLeft), getVertex(Plane::Near, PlaneVertex::BottomRight), color);
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::BottomLeft), getVertex(Plane::Far, PlaneVertex::BottomLeft), color);
    renderer->drawLine(getVertex(Plane::Far, PlaneVertex::BottomLeft), getVertex(Plane::Far, PlaneVertex::BottomRight), color);
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::BottomRight), getVertex(Plane::Far, PlaneVertex::BottomRight), color);
    
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::TopLeft), getVertex(Plane::Near, PlaneVertex::TopRight), color);
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::TopLeft), getVertex(Plane::Far, PlaneVertex::TopLeft), color);
    renderer->drawLine(getVertex(Plane::Far, PlaneVertex::TopLeft), getVertex(Plane::Far, PlaneVertex::TopRight), color);
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::TopRight), getVertex(Plane::Far, PlaneVertex::TopRight), color);
    
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::TopRight), getVertex(Plane::Near, PlaneVertex::BottomRight), color);
    renderer->drawLine(getVertex(Plane::Far, PlaneVertex::TopRight), getVertex(Plane::Far, PlaneVertex::BottomRight), color);
    renderer->drawLine(getVertex(Plane::Near, PlaneVertex::TopLeft), getVertex(Plane::Near, PlaneVertex::BottomLeft), color);
    renderer->drawLine(getVertex(Plane::Far, PlaneVertex::TopLeft), getVertex(Plane::Far, PlaneVertex::BottomLeft), color);
}

}
