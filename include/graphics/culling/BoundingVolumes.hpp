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

#ifndef BOUNDING_VOLUMES_HPP
#define BOUNDING_VOLUMES_HPP

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <string>

#define IYF_SPHERE_BOUNDS 1
#define IYF_AABB_BOUNDS 2

/// Which bounding volume should be used for culling? Sphere (IYF_SPHERE_BOUNDS) or AABB (IYF_AABB_BOUNDS)
#define IYF_BOUNDING_VOLUME IYF_AABB_BOUNDS

namespace iyf {
struct AABB {
    enum class Vertex {
        Minimum = 0, Maximum = 1
    };
    
    AABB() : vertices{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)} {}
    AABB(const glm::vec3& minCorner, const glm::vec3& maxCorner) : vertices{minCorner, maxCorner} {}
    
    inline glm::vec3& getVertex(Vertex vertex) {
        return vertices[static_cast<int>(vertex)];
    }
    
    inline const glm::vec3& getVertex(Vertex vertex) const {
        return vertices[static_cast<int>(vertex)];
    }
    
    /// Creates a new AABB by transforming the current one.
    /// \remark Implementation based on https://github.com/erich666/GraphicsGems/blob/master/gems/TransBox.c
    inline AABB transform(const glm::mat4& transformation) const {
        AABB temp;
        
        const glm::vec3& srcMin = vertices[static_cast<int>(Vertex::Minimum)];
        const glm::vec3& srcMax = vertices[static_cast<int>(Vertex::Maximum)];
        
        glm::vec3& dstMin = temp.vertices[static_cast<int>(Vertex::Minimum)];
        glm::vec3& dstMax = temp.vertices[static_cast<int>(Vertex::Maximum)];
        
        dstMin = transformation[3];
        dstMax = transformation[3];
        
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                const float a = transformation[i][j] * srcMin[j];
                const float b = transformation[i][j] * srcMax[j];
                
                if (a < b) {
                    dstMin[i] += a;
                    dstMax[i] += b;
                } else {
                    dstMin[i] += b;
                    dstMax[i] += a;
                }
            }
        }
        
        return temp;
    }
    
    std::string print() const;
    
    glm::vec3 vertices[2];
};

struct BoundingSphere {
    BoundingSphere() : center(0.0f, 0.0f, 0.0f), radius (0.0f) {}
    BoundingSphere(const glm::vec3& center, float radius) : center(center), radius(radius) {}
    
    std::string print() const;
    
    inline BoundingSphere transform(const glm::mat4& M, const glm::vec3& scale) const {
        return BoundingSphere(M * glm::vec4(center, 1.0f), radius * std::max(std::max(scale.x, scale.y), scale.z));
//         currentBounds.center = modelMatrix * glm::vec4(preTransformBounds.center, 1.0f);
//         currentBounds.radius = preTransformBounds.radius * std::max(std::max(scale.x, scale.y), scale.z);
    }
    
    glm::vec3 center;
    float radius;
};

#if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
using BoundingVolume = BoundingSphere;
#elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
using BoundingVolume = AABB;
#endif // IYF_BOUNDING_VOLUME

}

#endif // BOUNDING_VOLUMES_HPP

