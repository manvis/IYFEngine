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

#include "graphics/MaterialPipelineDefinition.hpp"
#include "core/filesystem/File.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "core/Logger.hpp"
#include "utilities/StringUtilities.hpp"
#include "utilities/Regexes.hpp"

#include "format/format.h"

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
static const std::array<char, 5> PipelineDefinitionMagicNumber = {'I', 'Y', 'F', 'P', 'R'};

void ShaderVariable::serialize(Serializer& fw) const {
    if (name.length() > con::MaxPipelineNameLength) {
        throw std::length_error("The length of the shader input variable name can't be longer than con::MaxPipelineNameLength");
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
    if (name.length() > con::MaxPipelineNameLength) {
        throw std::length_error("The length of the material component name can't be longer than con::MaxPipelineNameLength");
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

MaterialPipelineDefinition::MaterialPipelineDefinition() {
    const std::string defaultName = "EmptyPipeline";
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

void MaterialPipelineDefinition::setName(std::string name) {
    if (!validateName(name)) {
        throw std::invalid_argument("The name did not ");
    }
    
    this->name = std::move(name);
}

void MaterialPipelineDefinition::setSupportedLanguages(std::vector<ShaderLanguage> supportedLanguages) {
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
}

bool MaterialPipelineDefinition::validateName(const std::string& name) const {
    if (name.empty() || (name.length() > con::MaxPipelineNameLength)) {
        return false;
    }
    
    return std::regex_match(name, SystemRegexes().FunctionAndFileNameRegex);
}

bool MaterialPipelineDefinition::validateAdditionalVertexShaderOutputs(const std::vector<ShaderVariable>& additionalVertexOutputs) const {
    std::unordered_set<std::string> uniqueNames;
    
    for (const auto& avo : additionalVertexOutputs) {
        const auto result = uniqueNames.insert(avo.getName());
        if (!result.second) {
            return false;
        }
    }
    
    return true;
}

void MaterialPipelineDefinition::serialize(Serializer& fw) const {
    fw.writeBytes(PipelineDefinitionMagicNumber.data(), PipelineDefinitionMagicNumber.size());
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
    
    // TODO update version and start writing these when tessellation and geometry shaders are finally supported by the generator
//     fw.writeUInt8(usesTessellation);
//     
//     fw.writeUInt8(usesGeometryShaders);
//     fw.writeUInt8(vertexColorDataRequiredGS);
//     fw.writeUInt64(geometryShaderDataSets.to_ullong());
}

void MaterialPipelineDefinition::deserialize(Serializer& fr) {
    std::array<char, 5> magic;
    fr.readBytes(magic.data(), magic.size());
    
    if (magic != PipelineDefinitionMagicNumber) {
        throw std::runtime_error("An invalid MaterialPipelineDefinition file was provided (magic number mismatch)");
    }
    
    std::uint16_t version = fr.readUInt16();
    
    // At the moment we only support one version
    if (version != VERSION) {
        throw std::runtime_error(fmt::format("An invalid MaterialPipelineDefinition file was provided (unknown version {})", version));
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
    
    // TODO update version and start writing these when tessellation and geometry shaders are finally supported by the generator
//     fw.writeUInt8(usesTessellation);
//     
//     fw.writeUInt8(usesGeometryShaders);
//     fw.writeUInt8(vertexColorDataRequiredGS);
//     fw.writeUInt64(geometryShaderDataSets.to_ullong());
}

hash64_t MaterialPipelineDefinition::computeHash() const {
    MemorySerializer ms(1024);
    serialize(ms);
    
    return HF(ms.data(), ms.size());
}

/// TODO move these to compilation
// std::pair<bool, std::string> MaterialPipelineDefinition::setMaterialComponents(const std::vector<MaterialComponent>& componentList) {
//     std::unordered_set<MaterialComponent> uniqueNames;
//     
//     for (const auto& c : componentList) {
//         if (util::startsWith(c.name, "padding")) {
//             const std::string error = "All names of material components that start with the word \"padding\" are reserved.";
//             LOG_E("Material component list error: " << error);
//             return {false, error};
//         }
//         
//         const auto result = uniqueNames.insert(c);
//         if (!result.second) {
//             std::string error = "All names of material components must be unique. \"";
//             error.append(c.name);
//             error.append("\" was not.");
//             LOG_E("Material component list error: " << error);
//             
//             return {false, error};
//         }
//     }
//     
//     std::vector<MaterialComponent> materialComponentsSorted = componentList;
//     if (packMaterialData(materialComponentsSorted).first != true) {
//         std::stringstream ss;
//         ss <<"The material has more than " << con::MaxMaterialComponents << " components." << "\n\tPlease note possible padding:";
//         for (const auto& c : materialComponentsSorted) {
//             ss << "\n\t\t" << c.name << "; Size: " << c.componentCount << "; Offset: " << c.offset;
//         }
//         
//         const std::string error = ss.str();
//         LOG_E("Material component packing failed: " << error);
//         
//         return {false, error};
//     }
//     
//     materialComponents = materialComponentsSorted;
//     
//     return {true, ""};
// }
// 
// std::pair<bool, std::string> MaterialPipelineDefinition::packMaterialData(std::vector<MaterialComponent>& components) const {
//     // Nothing to pack
//     if (components.size() == 0) {
//         return {true, ""};
//     }
//     
//     std::sort(components.begin(), components.end(), std::greater<MaterialComponent>());
// 
//     std::size_t paddingCount = 0;
//     std::size_t packedCount = 0;
//     
//     // 4 component data is already aligned and stored the way we want.
//     for (std::size_t i = 0; i < components.size(); ++i) {
//         if (components[i].componentCount == 4) {
//             packedCount++;
//         } else {
//             break;
//         }
//     }
//     
//     // When storing 3 component data, if possible, fill the remaining last gap with a 1 byte value. If not, create a padding component
//     for (std::size_t i = packedCount; i < components.size(); ++i) {
//         if (components[i].componentCount == 3 && components.back().componentCount == 1) {
//             const MaterialComponent last = components.back();
//             components.pop_back();
//             components.insert(components.begin() + i + 1, last);
//             
//             i++;
//         } else if (components[i].componentCount == 3) {
//             components.insert(components.begin() + i + 1, MaterialComponent("padding" + std::to_string(paddingCount), 1));
//             paddingCount++;
//             
//             i++;
//         } else {
//             packedCount = i;
//             break;
//         }
//     }
//     
//     // Nothing to do with 2 or 1 component values
// //     // We can store two 2 component elements one after another
// //     for (std::size_t i = packedCount; i < components.size(); ++i) {
// //         if (components[i].componentCount == 2 && i + 1 < components.size() && components[i + 1].componentCount == 2) {
// //             i++;
// //         }
// //     }
//     
//     // Store the offsets and compute the final size. It may have increased due to padding.
//     std::size_t totalSize = 0;
//     for (auto& c : components) {
//         c.offset = totalSize;
//         totalSize += c.componentCount;
//     }
//     
//     // See if we still fit into memory.
//     if (totalSize <= con::MaxMaterialComponents) {
//         return {true, ""};
//     } else {
//         return {false, ""};
//     }
// }

static MaterialPipelineDefinition createToonPipelineDefinition() {
    MaterialPipelineDefinition definition;
    assert(definition.getSupportedLanguages().size() == 1);
    assert(definition.getSupportedLanguages()[0] == ShaderLanguage::GLSLVulkan);
    definition.setName("CellShaded");
    definition.setNormalDataRequired(true);
    definition.setRequiredVertexColorChannelCount(1);
    definition.setLightsSupported(true);
    const std::string lightProcessingCode = 
R"glsl(    float toonStep = step(1.0f - vertColor.x, dot(normal, lightDirection));
    vec3 litDiffuse = mix(diffuseColor, tint, toonStep) * lightColor * lightIntensity;
    
    // TODO calculate specular
    vec3 finalColor = litDiffuse;

    return vec4(finalColor, 0.0f);)glsl";
    
    definition.setLightProcessingCode({lightProcessingCode});
//     definition.additionalVertexOutputs.emplace_back("TestVar", ShaderDataType::Matrix4x2, ShaderDataFormat::Float);
    
    std::vector<LightProcessingFunctionInput> inputs;
    inputs.reserve(4);
    inputs.emplace_back("diffuseColor",      ShaderDataType::Vector3D, glm::vec4(1.0f, 0.0f, 1.0f, 0.0f));
    inputs.emplace_back("specular",          ShaderDataType::Scalar,   glm::vec4(0.3f, 0.0f, 0.0f, 0.0f));
    inputs.emplace_back("tint",              ShaderDataType::Vector3D, glm::vec4(0.2f, 0.2f, 0.2f, 0.0f));
    inputs.emplace_back("specularLightness", ShaderDataType::Scalar,   glm::vec4(0.5f, 0.0f, 0.0f, 0.0f));
    
    definition.setLightProcessingFunctionInputs(inputs);
    
    return definition;
}

namespace con {

const MaterialPipelineDefinition& GetDefaultMaterialPipelineDefinition(DefaultMaterialPipeline pipeline) {
    using DefaultMaterialPipelineDefinitionArray = std::array<MaterialPipelineDefinition, static_cast<std::size_t>(DefaultMaterialPipeline::COUNT)>;
    static const DefaultMaterialPipelineDefinitionArray DefaultMaterialPipelineDefinitions = {
        MaterialPipelineDefinition(),
        createToonPipelineDefinition(),
    };
    
    return DefaultMaterialPipelineDefinitions[static_cast<std::size_t>(pipeline)];
}

}
}
