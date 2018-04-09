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
#include "core/Logger.hpp"
#include "utilities/StringUtilities.hpp"

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

void ShaderInputOutputVariable::serialize(Serializer& fw) const {
    if (name.length() > con::MaxPipelineNameLength) {
        throw std::length_error("The length of the shader input variable name can't be longer than con::MaxPipelineNameLength");
    }
    
    fw.writeString(name, StringLengthIndicator::UInt8);
    fw.writeUInt8(static_cast<std::uint8_t>(type));
    fw.writeUInt8(static_cast<std::uint8_t>(format));
}

void ShaderInputOutputVariable::deserialize(Serializer& fr) {
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

void MaterialPipelineDefinition::serialize(Serializer& fw) const {
    if (name.length() > con::MaxPipelineNameLength) {
        throw std::length_error("The length of the pipeline name can't be longer than con::MaxPipelineNameLength");
    }
    
    if (languages.size() != additionalVertexProcessingCode.size()) {
        std::string error = fmt::format("The length of the supported languages vector ({}) must match the length of the additionalVertexProcessingCode vector ({})", languages.size(), additionalVertexProcessingCode.size());
        
        LOG_E(error);
        throw std::logic_error(error);
    }
    
    fw.writeBytes(PipelineDefinitionMagicNumber.data(), PipelineDefinitionMagicNumber.size());
    fw.writeUInt16(VERSION);
    
    fw.writeString(name, StringLengthIndicator::UInt8);
    fw.writeUInt8(logAssembly);
    
    fw.writeUInt8(static_cast<std::uint8_t>(languages.size()));
    for (ShaderLanguage l : languages) {
        fw.writeUInt8(static_cast<std::uint8_t>(l));
    }
    
    fw.writeUInt8(normalDataRequired);
    fw.writeUInt8(supportsMultipleLights);
    fw.writeUInt8(textureCoordinatesRequired);
    fw.writeUInt8(vertexColorDataRequired);
    fw.writeUInt8(numRequiredColorChannels);
    
    fw.writeUInt8(static_cast<std::uint8_t>(materialComponents.size()));
    for (const MaterialComponent& mc : materialComponents) {
        mc.serialize(fw);
    }
    
    fw.writeUInt64(vertexShaderDataSets.to_ullong());
    fw.writeUInt8(requiresAdditionalVertexProcessing);
    
    fw.writeUInt8(static_cast<std::uint8_t>(additionalVertexOutputs.size()));
    for (const ShaderInputOutputVariable& v : additionalVertexOutputs) {
        v.serialize(fw);
    }
    
    // Number of elements is known implicitly because the number must match languages.size()
    for (const std::string& code : additionalVertexProcessingCode) {
        fw.writeString(code, StringLengthIndicator::UInt32);
    }
    
    fw.writeUInt64(fragmentShaderDataSets.to_ullong());
    
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
    logAssembly = fr.readUInt8();
    
    std::size_t languageCount = fr.readUInt8();
    
    languages.clear();
    languages.reserve(languageCount);
    
    for (std::size_t i = 0; i < languageCount; ++i) {
        languages.push_back(static_cast<ShaderLanguage>(fr.readUInt8()));
    }
    
    normalDataRequired = fr.readUInt8();
    supportsMultipleLights = fr.readUInt8();
    textureCoordinatesRequired = fr.readUInt8();
    vertexColorDataRequired = fr.readUInt8();
    numRequiredColorChannels = fr.readUInt8();
    
    std::size_t materialComponentCount = fr.readUInt8();
    
    materialComponents.clear();
    materialComponents.reserve(materialComponentCount);
    
    for (std::size_t i = 0; i < materialComponentCount; ++i) {
        materialComponents.emplace_back();
        materialComponents.back().deserialize(fr);
    }
    
    vertexShaderDataSets = ShaderDataSets(fr.readUInt64());
    requiresAdditionalVertexProcessing = fr.readUInt8();
    
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

std::pair<bool, std::string> MaterialPipelineDefinition::setMaterialComponents(const std::vector<MaterialComponent>& componentList) {
    std::unordered_set<MaterialComponent> uniqueNames;
    
    for (const auto& c : componentList) {
        if (util::startsWith(c.name, "padding")) {
            const std::string error = "All names of material components that start with the word \"padding\" are reserved.";
            LOG_E("Material component list error: " << error);
            return {false, error};
        }
        
        const auto result = uniqueNames.insert(c);
        if (!result.second) {
            std::string error = "All names of material components must be unique. \"";
            error.append(c.name);
            error.append("\" was not.");
            LOG_E("Material component list error: " << error);
            
            return {false, error};
        }
    }
    
    std::vector<MaterialComponent> materialComponentsSorted = componentList;
    if (packMaterialData(materialComponentsSorted).first != true) {
        std::stringstream ss;
        ss <<"The amount of required per-material data exceeds " << con::MaxBytesPerMaterial << " bytes." << "\n\tPlease note possible padding:";
        for (const auto& c : materialComponentsSorted) {
            ss << "\n\t\t" << c.name << "; Size: " << c.componentCount << "; Offset: " << c.offset;
        }
        
        const std::string error = ss.str();
        LOG_E("Material component packing failed: " << error);
        
        return {false, error};
    }
    
    materialComponents = materialComponentsSorted;
    
    return {true, ""};
}

std::pair<bool, std::string> MaterialPipelineDefinition::packMaterialData(std::vector<MaterialComponent>& components) const {
    // Nothing to pack
    if (components.size() == 0) {
        return {true, ""};
    }
    
    std::sort(components.begin(), components.end(), std::greater<MaterialComponent>());

    std::size_t paddingCount = 0;
    std::size_t packedCount = 0;
    
    // 4 component data is already aligned and stored the way we want.
    for (std::size_t i = 0; i < components.size(); ++i) {
        if (components[i].componentCount == 4) {
            packedCount++;
        } else {
            break;
        }
    }
    
    // When storing 3 component data, if possible, fill the remaining last gap with a 1 byte value. If not, create a padding component
    for (std::size_t i = packedCount; i < components.size(); ++i) {
        if (components[i].componentCount == 3 && components.back().componentCount == 1) {
            const MaterialComponent last = components.back();
            components.pop_back();
            components.insert(components.begin() + i + 1, last);
            
            i++;
        } else if (components[i].componentCount == 3) {
            components.insert(components.begin() + i + 1, MaterialComponent("padding" + std::to_string(paddingCount), 1));
            paddingCount++;
            
            i++;
        } else {
            packedCount = i;
            break;
        }
    }
    
    // Nothing to do with 2 or 1 component values
//     // We can store two 2 component elements one after another
//     for (std::size_t i = packedCount; i < components.size(); ++i) {
//         if (components[i].componentCount == 2 && i + 1 < components.size() && components[i + 1].componentCount == 2) {
//             i++;
//         }
//     }
    
    // Store the offsets and compute the final size. It may have increased due to padding.
    std::size_t totalSize = 0;
    for (auto& c : components) {
        c.offset = totalSize;
        totalSize += c.componentCount;
    }
    
    // See if we still fit into memory.
    if (totalSize <= con::MaxBytesPerMaterial) {
        return {true, ""};
    } else {
        return {false, ""};
    }
}

static MaterialPipelineDefinition createToonPipelineDefinition() {
    MaterialPipelineDefinition definition;
    assert(definition.languages.size() == 1);
    assert(definition.languages[0] == ShaderLanguage::GLSLVulkan);
    definition.normalDataRequired = true;
    definition.vertexColorDataRequired = true;
    definition.supportsMultipleLights = true;
    definition.name = "CellShaded";
    definition.requiresAdditionalVertexProcessing = false;
    definition.additionalVertexProcessingCode.emplace_back("");
    definition.lightProcessingCode.emplace_back(
R"glsl(    vec3 diffuseColor = ColorAndSpecular.xyz;
    vec3 tintColor = TintAndSpecularLightness.xyz;

    float toonStep = step(1.0f - vertColor.x, dot(normal, lightDirection));
    vec3 litDiffuse = mix(diffuseColor, tintColor, toonStep) * lightColor * lightIntensity;
    
    // TODO calculate specular
    vec3 finalColor = litDiffuse;

    return vec4(finalColor, 0.0f);)glsl" );
//     definition.additionalVertexOutputs.emplace_back("TestVar", ShaderDataType::Matrix4x2, ShaderDataFormat::Float);
    
    std::vector<MaterialComponent> materialComponents;
    materialComponents.reserve(2);
    materialComponents.emplace_back("ColorAndSpecular", 4);
    materialComponents.emplace_back("TintAndSpecularLightness", 4);
    
    auto result = definition.setMaterialComponents(materialComponents);
    assert(result.first == true);
    
    return definition;
}

const DefaultMaterialPipelineDefinitionArray DefaultMaterialPipelineDefinitions = {
    MaterialPipelineDefinition(),
    createToonPipelineDefinition(),
};
}
