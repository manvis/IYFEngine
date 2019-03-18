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

#ifndef FRUSTUM_HPP
#define FRUSTUM_HPP

#include "graphics/culling/BoundingVolumes.hpp"

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace iyf {
class Camera;
class DebugRenderer;

/// A lot of code in this class is based on formulae and examples provided in http://www.lighthouse3d.com/tutorials/view-frustum-culling/
///
/// \todo glm has SIMD optimizations, however, unless I'm mistaken, they only work with vec4 types. Check if it's possible to make the math
/// in this class faster by switching to vec4 where possible.
class Frustum {
public:
    enum class Plane {
         Left = 0, Right = 1, Top = 2, Bottom = 3, Near = 4, Far = 5
    };
    
    enum class PlaneVertex {
        TopLeft = 0,
        TopRight = 1,
        BottomLeft = 2,
        BottomRight = 3
    };
    Frustum() { }

    void update(const Camera& camera);

    inline bool isBoundingVolumeInFrustum(const BoundingVolume& volume) const {
        #if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
            return isSphereInFrustum(volume);
        #elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
            return isAABBInFrustum(volume);
        #endif // IYF_BOUNDING_VOLUME
    }
    
    inline bool isSphereInFrustum(const BoundingSphere& sphere) const {
        for (const glm::vec4& plane : planes) {
            const glm::vec3 planeXYZ(plane.x, plane.y, plane.z);
            const float distance = plane.w + glm::dot(planeXYZ, sphere.center);

            if (distance > sphere.radius) {
                return false;
            }
        }
        
        return true;
    }

    inline bool isAABBInFrustum(const AABB& aabb) const {
        // http://www.txutxi.com/?p=584
        for (const glm::vec4& plane : planes) {
            const glm::vec3 planeNormal(plane.x, plane.y, plane.z);
            
            const int xID = static_cast<int>(planeNormal.x < 0.0f);
            const int yID = static_cast<int>(planeNormal.y < 0.0f);
            const int zID = static_cast<int>(planeNormal.z < 0.0f);
            
            const glm::vec3 testVec(aabb.vertices[xID].x,
                                    aabb.vertices[yID].y,
                                    aabb.vertices[zID].z);
            
            const float d = glm::dot(planeNormal, testVec);
            if (d > -plane.w) {
                return false;
            }
        }
        return true;
    }
    
    inline glm::vec4 getPlane(Plane planeID) const {
        return planes[static_cast<std::size_t>(planeID)];
    }
    
    /// \todo Create a dedicated debug renderer for graphics stuff
    void drawDebug(DebugRenderer* renderer);
    
    static glm::vec4 makePlaneFromPoints(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c);
    
    /// \param[in] plane either Frustum::Plane::Near or Frustum::Plane::Far
    /// \param[in] vertex one of Frustum::PlaneVertex
    inline glm::vec3 getVertex(Plane plane, PlaneVertex vertex) {
        if (plane != Plane::Near && plane != Plane::Far) {
            throw std::logic_error("Must use near plane or far plane.");
        }
        
        if (plane == Plane::Near) {
            return nearVertices[static_cast<std::size_t>(vertex)];
        } else {
            return farVertices[static_cast<std::size_t>(vertex)];
        }
    }
protected:
private:
    std::array<glm::vec3, 4> nearVertices;
    std::array<glm::vec3, 4> farVertices;
    std::array<glm::vec4, 6> planes;
    
    void setFrustumVertex(Plane plane, PlaneVertex vertex, const glm::vec3& vertexPos);
    inline glm::vec4 normalizePlane(const glm::vec4 &plane) {
        glm::vec3 normal(plane.x, plane.y, plane.z);
        
        float length = glm::length(normal);
        return plane / length;
    }
};
}

#endif /* FRUSTUM_HPP */

