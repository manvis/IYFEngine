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

#include "graphics/materials/MaterialFamilyDefinition.hpp"
#include "core/filesystem/File.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "core/Logger.hpp"
#include "utilities/StringUtilities.hpp"
#include "utilities/Regexes.hpp"

#include "fmt/format.h"

#include <algorithm>
#include <unordered_set>

namespace std {
    template <>
    struct hash<iyf::MaterialComponent> {
        std::size_t operator()(const iyf::MaterialComponent& k) const { 
            return std::hash<std::string>{}(k.name);
        }
    };
}

namespace iyf {
static const std::array<char, 5> FamilyDefinitionMagicNumber = {'I', 'Y', 'F', 'P', 'R'};

void ShaderVariable::serialize(Serializer& fw) const {
    if (name.length() > con::MaxMaterialFamilyNameLength) {
        throw std::length_error("The length of the shader input variable name can't be longer than con::MaxMaterialFamilyNameLength");
    }
    
    fw.writeString(name, StringLengthIndicator::UInt8);
    fw.writeUInt8(static_cast<std::uint8_t>(type));
    fw.writeUInt8(static_cast<std::uint8_t>(format));
}

void ShaderVariable::deserialize(Serializer& fr) {
    name.clear();
    
    fr.readString(name, StringLengthIndicator::UInt8);
    type = static_cast<ShaderDataType>(fr.readUInt8());
    format = static_cast<ShaderDataFormat>(fr.readUInt8());
}

void MaterialComponent::serialize(Serializer& fw) const {
    if (name.length() > con::MaxMaterialFamilyNameLength) {
        throw std::length_error("The length of the material component name can't be longer than con::MaxMaterialFamilyNameLength");
    }
    
    fw.writeString(name, StringLengthIndicator::UInt8);
    fw.writeUInt16(componentCount);
    fw.writeUInt8(isSigned);
    fw.writeUInt32(offset);
}

void MaterialComponent::deserialize(Serializer& fr) {
    name.clear();
    
    fr.readString(name, StringLengthIndicator::UInt8);
    componentCount = fr.readUInt16();
    isSigned = fr.readUInt8();
    offset = fr.readUInt32();
}

ShaderVariable::ShaderVariable(std::string name, ShaderDataType type, ShaderDataFormat format) : type(type), format(format), name(name) {
    if (!validateName(name)) {
        throw std::invalid_argument("The name wasn't valid");
    }
}

bool ShaderVariable::validateName(const std::string& newName) const {
    if (newName.empty() || newName.length() > con::MaxShaderVariableNameLength) {
        return false;
    }
    
    return std::regex_match(newName, SystemRegexes().FunctionAndFileNameRegex);
}

void LightProcessingFunctionInput::serialize(Serializer& fw) const {
    variable.serialize(fw);
    
    fw.writeFloat(defaultValue.x);
    fw.writeFloat(defaultValue.y);
    fw.writeFloat(defaultValue.z);
    fw.writeFloat(defaultValue.w);
}

void LightProcessingFunctionInput::deserialize(Serializer& fr) {
    variable.deserialize(fr);
    
    defaultValue.x = fr.readFloat();
    defaultValue.y = fr.readFloat();
    defaultValue.z = fr.readFloat();
    defaultValue.w = fr.readFloat();
}

MaterialFamilyDefinition::MaterialFamilyDefinition() {
    const std::string defaultName = "EmptyFamily";
    assert(validateName(defaultName));
    setName(defaultName);
    
    setNormalDataRequired(false);
    setWorldSpacePositionRequired(true);
    setLightsSupported(true);
    setRequiredVertexColorChannelCount(0);
    setTextureCoordinatesRequired(true);
    setSupportedLanguages({ShaderLanguage::GLSLVulkan});
    
    setVertexShaderDataSet(PerFrameDataSet::ObjectTransformations, true);
    
    setFragmentShaderDataSet(PerFrameDataSet::LightsAndCamera, true);
    setFragmentShaderDataSet(PerFrameDataSet::RendererData, true);
    setFragmentShaderDataSet(PerFrameDataSet::MaterialData, true);
    setFragmentShaderDataSet(PerFrameDataSet::TextureData, true);
}

bool MaterialFamilyDefinition::setRequiredVertexColorChannelCount(std::uint8_t count) {
    if (count > 0) {
        throw std::invalid_argument("Count was greater than maximum");
    }
    
    requiredVertexColorChannelCount = count;
    return true;
}

void MaterialFamilyDefinition::setName(std::string name) {
    if (!validateName(name)) {
        throw std::invalid_argument("The name did not match the validation regex");
    }
    
    this->name = std::move(name);
}

void MaterialFamilyDefinition::setSupportedLanguages(std::vector<ShaderLanguage> supportedLanguages) {
    if (supportedLanguages.empty()) {
        throw std::invalid_argument("Must support at least one language");
    }
    
    std::unordered_set<ShaderLanguage> uniqueLanguages;
    for (ShaderLanguage l : supportedLanguages) {
        const auto result = uniqueLanguages.insert(l);
        if (!result.second) {
            throw std::invalid_argument("The elements of the supportedLanguages vector must be unique");
        }
    }
    
    const std::size_t newSize = supportedLanguages.size();
    this->supportedLanguages = std::move(supportedLanguages);
    
    additionalVertexProcessingCode.clear();
    additionalVertexProcessingCode.resize(newSize);
    
    lightProcessingCode.clear();
    lightProcessingCode.resize(newSize);
    
    globalLightProcessingCode.clear();
    globalLightProcessingCode.resize(newSize);
}

bool MaterialFamilyDefinition::validateName(const std::string& name) const {
    if (name.empty() || (name.length() > con::MaxMaterialFamilyNameLength)) {
        return false;
    }
    
    return std::regex_match(name, SystemRegexes().FunctionAndFileNameRegex);
}

bool MaterialFamilyDefinition::validateAdditionalVertexShaderOutputs(const std::vector<ShaderVariable>& additionalVertexOutputs) const {
    std::unordered_set<std::string> uniqueNames;
    
    for (const auto& avo : additionalVertexOutputs) {
        const auto result = uniqueNames.insert(avo.getName());
        if (!result.second) {
            return false;
        }
    }
    
    return true;
}

void MaterialFamilyDefinition::serialize(Serializer& fw) const {
    fw.writeBytes(FamilyDefinitionMagicNumber.data(), FamilyDefinitionMagicNumber.size());
    fw.writeUInt16(VERSION);
    
    fw.writeString(name, StringLengthIndicator::UInt8);
    
    fw.writeUInt8(static_cast<std::uint8_t>(supportedLanguages.size()));
    for (ShaderLanguage l : supportedLanguages) {
        fw.writeUInt8(static_cast<std::uint8_t>(l));
    }
    
    static_assert(sizeof(flagValues.to_ullong()) == 8, "A 64 bit value is needed");
    const std::uint64_t flagValueInt = flagValues.to_ullong();
    fw.writeUInt64(flagValueInt);
    
    assert(lightProcessingFunctionInputs.size() < 255);
    fw.writeUInt8(static_cast<std::uint8_t>(lightProcessingFunctionInputs.size()));
    for (const LightProcessingFunctionInput& lpfi : lightProcessingFunctionInputs) {
        lpfi.serialize(fw);
    }
    
    fw.writeUInt64(vertexShaderDataSets.to_ullong());
    
    fw.writeUInt8(static_cast<std::uint8_t>(additionalVertexOutputs.size()));
    for (const ShaderVariable& v : additionalVertexOutputs) {
        v.serialize(fw);
    }
    
    // The number of elements is known implicitly because the number must match languages.size()
    for (const std::string& code : additionalVertexProcessingCode) {
        fw.writeString(code, StringLengthIndicator::UInt32);
    }
    
    fw.writeUInt64(fragmentShaderDataSets.to_ullong());
    
    // The number of elements is known implicitly because the number must match languages.size()
    for (const std::string& code : lightProcessingCode) {
        fw.writeString(code, StringLengthIndicator::UInt32);
    }
    
    for (const std::string& code : globalLightProcessingCode) {
        fw.writeString(code, StringLengthIndicator::UInt32);
    }
    
    // TODO update version and start writing these when tessellation and geometry shaders are finally supported by the generator
//     fw.writeUInt8(usesTessellation);
//     
//     fw.writeUInt8(usesGeometryShaders);
//     fw.writeUInt8(vertexColorDataRequiredGS);
//     fw.writeUInt64(geometryShaderDataSets.to_ullong());
}

void MaterialFamilyDefinition::deserialize(Serializer& fr) {
    std::array<char, 5> magic;
    fr.readBytes(magic.data(), magic.size());
    
    if (magic != FamilyDefinitionMagicNumber) {
        throw std::runtime_error("An invalid MaterialFamilyDefinition file was provided (magic number mismatch)");
    }
    
    std::uint16_t version = fr.readUInt16();
    
    // At the moment we only support one version
    if (version != VERSION) {
        throw std::runtime_error(fmt::format("An invalid MaterialFamilyDefinition file was provided (unknown version {})", version));
    }
    
    name.clear();
    fr.readString(name, StringLengthIndicator::UInt8);
    
    std::size_t languageCount = fr.readUInt8();
    
    supportedLanguages.clear();
    supportedLanguages.reserve(languageCount);
    
    for (std::size_t i = 0; i < languageCount; ++i) {
        supportedLanguages.push_back(static_cast<ShaderLanguage>(fr.readUInt8()));
    }
    
    const std::uint64_t flagValueInt = fr.readUInt64();
    flagValues = std::bitset<64>(flagValueInt);
    
    const std::size_t lightProcessingFunctionInputCount = fr.readUInt8();
    
    lightProcessingFunctionInputs.clear();
    lightProcessingFunctionInputs.reserve(lightProcessingFunctionInputCount);
    
    for (std::size_t i = 0; i < lightProcessingFunctionInputCount; ++i) {
        lightProcessingFunctionInputs.emplace_back();
        lightProcessingFunctionInputs.back().deserialize(fr);
    }
    
    vertexShaderDataSets = ShaderDataSets(fr.readUInt64());
    
    std::size_t additionalVertexOutputCount = fr.readUInt8();
    
    additionalVertexOutputs.clear();
    additionalVertexOutputs.reserve(additionalVertexOutputCount);
    
    for (std::size_t i = 0; i < additionalVertexOutputCount; ++i) {
        additionalVertexOutputs.emplace_back("", ShaderDataType::Scalar, ShaderDataFormat::UnsignedInteger);
        additionalVertexOutputs.back().deserialize(fr);
    }
    
    additionalVertexProcessingCode.clear();
    additionalVertexProcessingCode.reserve(languageCount);
    
    for (std::size_t i = 0; i < languageCount; ++i) {
        additionalVertexProcessingCode.emplace_back();
        std::string& code = additionalVertexProcessingCode.back();
        fr.readString(code, StringLengthIndicator::UInt32);
    }
    
    fragmentShaderDataSets = ShaderDataSets(fr.readUInt64());
    
    lightProcessingCode.clear();
    lightProcessingCode.reserve(languageCount);
    
    for (std::size_t i = 0; i < languageCount; ++i) {
        lightProcessingCode.emplace_back();
        std::string& code = lightProcessingCode.back();
        fr.readString(code, StringLengthIndicator::UInt32);
    }
    
    globalLightProcessingCode.clear();
    globalLightProcessingCode.reserve(languageCount);
    
    for (std::size_t i = 0; i < languageCount; ++i) {
        globalLightProcessingCode.emplace_back();
        std::string& code = globalLightProcessingCode.back();
        fr.readString(code, StringLengthIndicator::UInt32);
    }
    
    // TODO update version and start writing these when tessellation and geometry shaders are finally supported by the generator
//     fw.writeUInt8(usesTessellation);
//     
//     fw.writeUInt8(usesGeometryShaders);
//     fw.writeUInt8(vertexColorDataRequiredGS);
//     fw.writeUInt64(geometryShaderDataSets.to_ullong());
}

FileHash MaterialFamilyDefinition::computeHash() const {
    MemorySerializer ms(1024);
    serialize(ms);
    
    return HF(ms.data(), ms.size());
}

static MaterialFamilyDefinition createToonFamilyDefinition() {
    MaterialFamilyDefinition definition;
    assert(definition.getSupportedLanguages().size() == 1);
    assert(definition.getSupportedLanguages()[0] == ShaderLanguage::GLSLVulkan);
    definition.setName("CellShaded");
    definition.setNormalDataRequired(true);
    // Allow loading meshes without real vertex color data and handle the case separately
    //definition.setRequiredVertexColorChannelCount(1);
    definition.setLightsSupported(true);
    
    std::stringstream ss;
    ss << "    vec3 normalizedNormal = normalize(normal);\n";
    ss << "    float NdotL = dot(normalizedNormal, lightDirection);\n";
    ss << "\n";
    ss << "    vec4 clampedAdjustments = clamp(adjustments, 0.0f, 1.0f);\n";
    ss << "    float toonStep = step(1.0f - adjustments.x, NdotL);\n"; // TODO Threshold?
    ss << "    vec3 litDiffuse = mix(diffuseColor.rgb, tint, toonStep) * lightColor * lightIntensity;\n"; // TODO tint mix or multiply?
    ss << "\n";
    ss << "    vec3 halfwayVec = normalize(lightDirection + viewDirection);\n";
    ss << "    float NdotH = dot(normalizedNormal, halfwayVec);\n";
    ss << "    float specularIntensity = pow(max(NdotH, 0.0f), specularLightness);\n";
    ss << "    float specularStep = step(specularCutoff, specularIntensity);\n";
    ss << "    vec3 specularColor = lightColor * lightIntensity * specularStep;\n";
    ss << "    // TODO ambient light and fog\n";
    ss << "    vec3 finalColor = litDiffuse + specularColor;\n";
    ss << "\n";
    ss << "    return vec4(finalColor, 0.0f);";// TODO handle alpha in global light processing code

    definition.setLightProcessingCode({ss.str()});
//     definition.additionalVertexOutputs.emplace_back("TestVar", ShaderDataType::Matrix4x2, ShaderDataFormat::Float);
    
    std::vector<LightProcessingFunctionInput> inputs;
    inputs.reserve(6);
    inputs.emplace_back("diffuseColor",      ShaderDataType::Vector4D, glm::vec4(1.0f, 0.0f, 1.0f, 0.0f));
    inputs.emplace_back("specularCutoff",    ShaderDataType::Scalar,   glm::vec4(0.3f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("tint",              ShaderDataType::Vector3D, glm::vec4(0.2f, 0.2f, 0.2f, 0.0f));
    inputs.emplace_back("specularLightness", ShaderDataType::Scalar,   glm::vec4(0.5f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("adjustments",       ShaderDataType::Vector4D, glm::vec4(0.5f, 0.0f, 0.0f, 0.0f));
    
    definition.setLightProcessingFunctionInputs(inputs);
    
    return definition;
}

static MaterialFamilyDefinition createPBRFamilyDefinition() {
    // Samples:
    // https://github.com/KhronosGroup/glTF-WebGL-PBR/blob/master/shaders/pbr-frag.glsl
    // https://github.com/SaschaWillems/Vulkan-glTF-PBR/blob/master/data/shaders/pbr_khr.frag
    // WARNING Especially need to look at Willems sample when implementing IBL. He inverts y axis for some reason.
    
    MaterialFamilyDefinition definition;
    assert(definition.getSupportedLanguages().size() == 1);
    assert(definition.getSupportedLanguages()[0] == ShaderLanguage::GLSLVulkan);
    definition.setName("PBR");
    definition.setNormalDataRequired(true);
    definition.setLightsSupported(true);
    
    // TODO some computed values are identical for all lights and can be reused. Implement a mechanism that would allow their reuse
    // TODO some equations may be optimized by moving certain variables around
    std::stringstream ss;
    ss <<
R"glsl(    
    // Based on the reference glTF PBR implementation
    // https://github.com/KhronosGroup/glTF-WebGL-PBR/blob/master/shaders/pbr-frag.glsl
    // which is under the MIT license
    float minRoughness = 0.04;
    vec3 f0 = vec3(0.04);

    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, minRoughness, 1.0);

    // Input is perceptual roughness, we need material roughness
    float alphaRoughness = roughness * roughness;

    vec4 baseColor = albedo * albedoFactor;

    vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= 1.0 - metallic;

    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    vec3 specularEnvReflectance0 = specularColor.rgb;
    vec3 specularEnvReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;

    vec3 normalizedNormal = normalize(normal);
    vec3 halfVec = normalize(lightDirection + normalizedNormal);

    float NdotL = clamp(dot(normalizedNormal, lightDirection), 0.001, 1.0);
    float NdotV = clamp(abs(dot(normalizedNormal, viewDirection)), 0.001, 1.0);
    float NdotH = clamp(dot(normalizedNormal, halfVec), 0.0, 1.0);
    float LdotH = clamp(dot(lightDirection, halfVec), 0.0, 1.0);
    float VdotH = clamp(dot(viewDirection, halfVec), 0.0, 1.0);

    // Specular reflection, a.k.a. F
    vec3 F = specularEnvReflectance0 + (specularEnvReflectance90 - specularEnvReflectance0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);

    // Geometric occlusion, a.k.a. G
    float r = alphaRoughness;
    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));

    float G = attenuationL * attenuationV;

    // Microfacet distribution, a.k.a. D
    float rSquare = r * r;
    float f = (NdotH * rSquare - NdotH) * NdotH + 1.0;
    float D = rSquare / (PI * f * f);

    vec3 diffuseContrib = (1.0 - F) * (diffuseColor / PI);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);

    vec3 radiance = lightColor * lightIntensity;

    vec3 color = NdotL * radiance * (diffuseContrib + specContrib);

    return vec4(color, 0.0);)glsl";

    definition.setLightProcessingCode({ss.str()});
    
    // TODO Implement IBL and evaluate IBL contribution here.
    definition.setGlobalLightProcessingCode({R"glsl(//TODO IBL
    
    lightSum *= occlusion;
    lightSum += vec4(emission, 0.0);
    
    float finalAlpha = albedo.a * albedoFactor.a;
    return vec4(lightSum.rgb, finalAlpha);
)glsl"});
    
    std::vector<LightProcessingFunctionInput> inputs;
    inputs.reserve(4);
    inputs.emplace_back("albedo",      ShaderDataType::Vector4D, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    inputs.emplace_back("albedoFactor",      ShaderDataType::Vector4D, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    inputs.emplace_back("normalScale",    ShaderDataType::Scalar,   glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("metallic",    ShaderDataType::Scalar,   glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("roughness",    ShaderDataType::Scalar,   glm::vec4(0.5f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("occlusion",    ShaderDataType::Scalar,   glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("emission",    ShaderDataType::Vector3D,   glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    definition.setLightProcessingFunctionInputs(inputs);
    
    return definition;
}

namespace con {

const MaterialFamilyDefinition& GetMaterialFamilyDefinition(MaterialFamily family) {
    using DefaultMaterialFamilyDefinitionArray = std::array<MaterialFamilyDefinition, static_cast<std::size_t>(MaterialFamily::COUNT)>;
    static const DefaultMaterialFamilyDefinitionArray DefaultMaterialFamilyDefinitions = {
        createToonFamilyDefinition(),
        createPBRFamilyDefinition(),
    };
    
    return DefaultMaterialFamilyDefinitions[static_cast<std::size_t>(family)];
}

}
}
