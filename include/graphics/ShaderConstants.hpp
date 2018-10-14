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

#ifndef SHADER_CONSTANTS_HPP
#define SHADER_CONSTANTS_HPP

#include <unordered_map>
#include <array>

#include <variant>

#include "graphics/GraphicsAPIConstants.hpp"

/// \file 
///
/// This file stores all Renderer independent constants that are used in shaders.
///
/// \warning All such constants MUST be stored in the DefaultSpecializationConstants array or
/// the renderers and the shader generator will miss them, causing hard to find bugs.

namespace iyf {
/// List of all shader languages that the generator supports.
///
/// \warning It's ok to add new languages, however, make sure that IDs are sequential and never removed
///  or rearranged because that may break your project.
enum class ShaderLanguage : std::uint8_t {
    GLSLVulkan = 0,
    // TODO add more, that would correspond to other backends
    COUNT
};

enum class ShaderDataFormat : std::uint8_t {
    UnsignedInteger = 0,
    Integer         = 1,
    Float           = 2,
    Double          = 3
};

enum class ShaderDataType : std::uint8_t {
    Scalar    = 0,
    Vector2D  = 1,
    Vector3D  = 2,
    Vector4D  = 3,
    Matrix2x2 = 4,
    Matrix2x3 = 5,
    Matrix2x4 = 6,
    Matrix3x2 = 7,
    Matrix3x3 = 8,
    Matrix3x4 = 9,
    Matrix4x2 = 10,
    Matrix4x3 = 11,
    Matrix4x4 = 12,
    COUNT
};

/// \warning Do not remove or reorder existing values. Moreover, make sure to never have more than 64 different
/// data sets specified in this enumerator. Disregarding this warning will break import of render family requirements.
enum class PerFrameDataSet {
    /// V, P and VP matrices; camera position; counts for all light types and per-light data
    LightsAndCamera = 0,
    /// M and MVP matrices for each visible object
    ObjectTransformations = 1,
    /// Depends on the renderer
    RendererData = 2,
    /// Depends on the family
    MaterialData = 3,
    /// Depends on the family
    TextureData = 4,
    COUNT
};

/// \todo Do I need vector or matrix constants?
using SpecializationConstantVariant = std::variant<std::int32_t, std::uint32_t, float, double>;

struct SpecializationConstant {
    SpecializationConstant(std::string name, Format format, SpecializationConstantVariant value) : name(name), format(format), value(value) { }
    
    std::string name;
    Format format;
    SpecializationConstantVariant value;
};

struct BindingAndSet {
    BindingAndSet(std::uint16_t binding, std::uint16_t set) : binding(binding), set(set) { }
    const std::uint16_t binding;
    const std::uint16_t set;
};

namespace con {

/// The set and the binding that will be used for the camera and light data buffer
const BindingAndSet CameraAndLightBuffer(0, 0);
/// The set and the binding that will be used for the transformation data buffer
const BindingAndSet TransformationDataBuffer(0, 1);
/// The set and the FIRST binding that will be used for renderer specific data.
const BindingAndSet RendererDataBuffer(0, 2);
/// The set and the binding that will be used for the material specific data buffer
const BindingAndSet MaterialDataBuffer(0, 3);
/// The set and the FIRST binding that will be used for material specific texture data.
const BindingAndSet TextureDataBuffer(0, 4);

/// Maximum number of directional lights that can exist in the scene at once
///
/// \warning Not all graphics backends support specialization constants. You may 
/// need to regenerate your shaders after changing this value or it won't apply
const std::uint32_t MaxDirectionalLights = 8;

/// Maximum number of point lights that can exist in the scene at once
///
/// \warning Not all graphics backends support specialization constants. You may 
/// need to regenerate your shaders after changing this value or it won't apply
const std::uint32_t MaxPointLights = 256;

/// Maximum number of spot lights that can exist in the scene at once
///
/// \warning Not all graphics backends support specialization constants. You may 
/// need to regenerate your shaders after changing this value or it won't apply
const std::uint32_t MaxSpotLights = 64;

/// Maximum number of materials that can exist in a single data buffer
///
/// \warning Not all graphics backends support specialization constants. You may 
/// need to regenerate your shaders after changing this value or it won't apply
const std::uint32_t MaxMaterials = 1024;

/// Maximum number of transformations that can exist in a single data buffer
///
/// \warning Not all graphics backends support specialization constants. You may 
/// need to regenerate your shaders after changing this value or it won't apply
const std::uint32_t MaxTransformations = 1024;

/// The name of the MaxDirectionalLights constant that's used in the shaders
///
/// \warning You will need your regenerate the shaders after changing this value
const std::string MaxDirectionalLightsConstName = "MAX_DIRECTIONAL_LIGHTS";

/// The name of the MaxPointLights constant that's used in the shaders
///
/// \warning You will need your regenerate the shaders after changing this value
const std::string MaxPointLightsConstName = "MAX_POINT_LIGHTS";

/// The name of the MaxSpotLights constant that's used in the shaders
///
/// \warning You will need your regenerate the shaders after changing this value
const std::string MaxSpotLightsConstName = "MAX_SPOT_LIGHTS";

/// The name of the MaxMaterials constant that's used in the shaders
///
/// \warning You will need your regenerate the shaders after changing this value
const std::string MaxMaterialsConstName = "MAX_MATERIALS";

/// The name of the MaxTransformations constant that's used in the shaders
///
/// \warning You will need your regenerate the shaders after changing this value
const std::string MaxTransformationsConstName = "MAX_TRANSFORMATIONS";

const std::size_t MaxMaterialFamilyNameLength = 64;
const std::size_t MaxShaderVariableNameLength = 64;

/// \warning This array should not be used directly. Each renderer should make a copy
/// and append constants that are specific to it.
extern const std::array<SpecializationConstant, 5> DefaultSpecializationConstants;

/// Used by the shader generator to turn a Format used by a VertexAttribute into an appropriate in-shader data type
extern const std::unordered_map<Format, std::pair<ShaderDataFormat, ShaderDataType>> FormatToShaderDataType;

}
}

#endif // SHADER_CONSTANTS_HPP
