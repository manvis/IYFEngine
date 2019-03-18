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

#ifndef IYF_LIGHTS_HPP
#define IYF_LIGHTS_HPP

#include <cstdint>
#include <type_traits>
#include <glm/vec3.hpp>

namespace iyf {
enum class LightType : std::uint8_t {
    Directional = 0,
    Point = 1,
    Spot = 2
};

// WARNING keep these without constructors. We initialize in light components.

/// Point light data that's sent to shader.
///
/// \warning If you change this, you must also update the ShadingPipelineEditor
/// and then use it to regenerate the shaders.
struct PointLight {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    float intensity;
};

static_assert(sizeof(PointLight) == 32, "PointLight struct is not 32 bytes");
static_assert(std::is_trivially_copyable_v<PointLight>, "PointLight must be trivially copyable.");

/// Directional light data that's sent to shader.
///
/// \warning If you change this, you must also update the ShadingPipelineEditor
/// and then use it to regenerate the shaders.
struct DirectionalLight {
    glm::vec3 direction;
    float padding;
    glm::vec3 color;
    float intensity;
};

static_assert(sizeof(DirectionalLight) == 32, "DirectionalLight struct is not 32 bytes");
static_assert(std::is_trivially_copyable_v<DirectionalLight>, "DirectionalLight must be trivially copyable.");

/// Spot light data that's sent to the shader.
///
/// \warning If you change this, you must also update the ShadingPipelineEditor
/// and then use it to regenerate the shaders.
struct SpotLight {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    float intensity;
    glm::vec3 direction;
    float angle;
};

static_assert(sizeof(SpotLight) == 48, "SpotLight struct is not 48 bytes");
static_assert(std::is_trivially_copyable_v<SpotLight>, "SpotLight must be trivially copyable.");
}

#endif // IYF_LIGHTS_HPP

