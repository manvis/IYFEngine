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

#ifndef CAMERA_AND_LIGHT_BUFFER_LAYOUT_HPP
#define CAMERA_AND_LIGHT_BUFFER_LAYOUT_HPP

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "graphics/Lights.hpp"
#include "graphics/ShaderConstants.hpp"

namespace iyf {
struct CameraAndLightData {
    glm::mat4 V;
    glm::mat4 P;
    glm::mat4 VP;
    glm::vec3 cameraPosition;
    std::uint32_t directionalLightCount;
    float zNear;
    float zFar;
    glm::uvec2 framebufferDimensions;
    std::uint32_t pointLightCount;
    std::uint32_t spotLightCount;
    float padding1;
    float padding2;
    DirectionalLight directionalLights[con::MaxDirectionalLights];
    PointLight pointLights[con::MaxPointLights];
    SpotLight spotLights[con::MaxSpotLights];
};
}
#endif // CAMERA_AND_LIGHT_BUFFER_LAYOUT_HPP
