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

#include "graphics/shaderGeneration/ShaderGenerator.hpp"
#include "graphics/Renderer.hpp"
#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/File.hpp"
#include "format/format.h"
#include "utilities/StringUtilities.hpp"
#include "utilities/Regexes.hpp"

#include <unordered_set>

namespace iyf {
ShaderGenerator::ShaderGenerator(const Engine* engine) : engine(engine), renderer(engine->getRenderer()) {}

ShaderGenerationResult ShaderGenerator::generateAndReportError(ShaderGenerationResult::Status status, const std::string& error) const {
    LOG_E("Shader generation error: " << error);
    
    return {status, error};
}

MultipleShaderGenerationResult ShaderGenerator::generateAllShaders(const fs::path& path, const MaterialPipelineDefinition& definition) const {
    MultipleShaderGenerationResult multiResult;
    
    ShaderGenerationResult result = validatePipelineDefinition(definition);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        multiResult.pipelineValidationError = result;
        return multiResult;
    }
    
    generateAllVertexShaders(path, multiResult, definition);
    generateAllFragmentShaders(path, multiResult, definition);
    
    
    LOG_I("Generated " << multiResult.generatedVertexShaderCount << " vertex shaders out of " << multiResult.totalVertexShaderCount << " and " << multiResult.generatedFragmentShaderCount << " fragment shaders out of " << multiResult.generatedFragmentShaderCount << " for a material pipeline called \"" << definition.name << "\"");
    
    return multiResult;
}

ShaderGenerationResult ShaderGenerator::validatePipelineDefinition(const MaterialPipelineDefinition& definition) const {
    if (definition.name.empty()) {
        return generateAndReportError(ShaderGenerationResult::Status::InvalidPipelineName, "The name of the material pipeline can't be empty");
    }
    
    LOG_I("Generating shaders for a material pipeline called \"" << definition.name << "\"");
    
    if (!std::regex_match(definition.name, regex::FunctionAndFileNameRegex)) {
        return generateAndReportError(ShaderGenerationResult::Status::InvalidPipelineName, "The name does not match the regex [a-zA-Z][a-zA-Z0-9]*");
    }
    
    if (definition.languages.size() != definition.lightProcessingCode.size()) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingLightProcessing, "The number of components in the lightProcessingCode vector must match the number of components in the languages vector.");
    }
    
    // TODO validate data that's important to all shaders
    
    std::unordered_set<ShaderLanguage> uniqueLanguages;
    bool found = false;
    
    for (ShaderLanguage sl : definition.languages) {
        if (sl == getShaderLanguage()) {
            found = true;
            
            const auto result = uniqueLanguages.insert(sl);
            if (!result.second) {
                return generateAndReportError(ShaderGenerationResult::Status::DuplicateLanguages, "The list of supported shading lanuages must not contain duplicates.");
            }
        }
    }
    
    if (!found) {
        return generateAndReportError(ShaderGenerationResult::Status::LanguageNotSupported, "This generator does not support any of the required shading lanuages.");
    }
    
    return {ShaderGenerationResult::Status::Success, ""};
}

ShaderGenerationResult ShaderGenerator::validateVertexShader(const MaterialPipelineDefinition& definition) const {
    if (definition.requiresAdditionalVertexProcessing && (definition.additionalVertexProcessingCode.size() != definition.languages.size())) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingAdditionalVertexProcessingCode, "RequiresAdditionalVertexProcessing is marked as true, however, the additionalVertexProcessingCode vector is empty");
    }
    
    return {ShaderGenerationResult::Status::Success, ""};
}

ShaderGenerationResult ShaderGenerator::checkVertexDataLayoutCompatibility(const MaterialPipelineDefinition& definition, VertexDataLayout vertexDataLayout) const {
    const VertexDataLayoutDefinition& layout = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(vertexDataLayout)];
    
    bool hasNormals = layout.hasAttribute(VertexAttributeType::Normal);
    if (definition.normalDataRequired && !hasNormals) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it lacks normals.");
    }
    
    bool hasVertexColors = layout.hasAttribute(VertexAttributeType::Color);
    if (definition.vertexColorDataRequired && !hasVertexColors) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it lacks vertex colors.");
    }
    
    bool hasUV = layout.hasAttribute(VertexAttributeType::UV);
    if (definition.textureCoordinatesRequired && !hasUV) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it texture coordinates.");
    }
    
    return {ShaderGenerationResult::Status::Success, ""};
}

void ShaderGenerator::generateAllVertexShaders(const fs::path& path, MultipleShaderGenerationResult& multiResult, const MaterialPipelineDefinition& definition) const {
    std::size_t count = 0;
    std::size_t successCount = 0;
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(VertexDataLayout::COUNT); ++i) {
        VertexDataLayout vertexDataLayout = static_cast<VertexDataLayout>(i);
        const VertexDataLayoutDefinition& layout = con::VertexDataLayoutDefinitions[i];
        
        bool hasNormals = layout.hasAttribute(VertexAttributeType::Normal);
        bool normalMappable = hasNormals && layout.hasAttribute(VertexAttributeType::UV) && (layout.hasAttribute(VertexAttributeType::Tangent) || layout.hasAttribute(VertexAttributeType::TangentAndBias));
        
        VertexShaderGenerationSettings genSettings;
        genSettings.compiledShaderPath = path;
        genSettings.materialDefinition = &definition;
        genSettings.vertexDataLayout = vertexDataLayout;
        genSettings.compileShader = true;
        genSettings.writeSource = false;
        
        if (normalMappable && definition.normalDataRequired) {
            // We still need a version without normal mapping when no normal map texture is attached to the shader
            genSettings.normalMapped = false;
            ShaderGenerationResult result = generateVertexShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.vertexShaderErrors.push_back({makeVertexShaderName(definition.name, layout.getName(), getVertexShaderExtension(), false), result});
            }
            
            genSettings.normalMapped = true;
            result = generateVertexShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.vertexShaderErrors.push_back({makeVertexShaderName(definition.name, layout.getName(), getVertexShaderExtension(), true), result});
            }
            
            count += 2;
        } else {
             ShaderGenerationResult result = generateVertexShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.vertexShaderErrors.push_back({makeVertexShaderName(definition.name, layout.getName(), getVertexShaderExtension(), false), result});
            }
            
            count++;
        }
    }
    
    multiResult.totalVertexShaderCount = count;
    multiResult.generatedVertexShaderCount = successCount;
}

void ShaderGenerator::generateAllFragmentShaders(const fs::path& path, MultipleShaderGenerationResult& multiResult, const MaterialPipelineDefinition& definition) const {
    std::size_t materialComponentCount = definition.getMaterialComponents().size();
    
    std::size_t count = 0;
    std::size_t successCount = 0;
    
    FragmentShaderGenerationSettings genSettings;
    genSettings.compiledShaderPath = path;
    genSettings.materialDefinition = &definition;
    genSettings.compileShader = true;
    genSettings.writeSource = false;
    
    // Generate something even if the shader has no required material data
    // TODO what if no UV coordinates are present in vertex layout? ComponentsReadFromTexture == 0?
    if (materialComponentCount == 0) {
        if (definition.normalDataRequired) {
            genSettings.readFromTexture = 0;
            genSettings.normalMapped = false;
            ShaderGenerationResult result = generateFragmentShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, 0, getFragmentShaderExtension(), false), result});
            }
            
            genSettings.readFromTexture = 0;
            genSettings.normalMapped = true;
            result = generateFragmentShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, 0, getFragmentShaderExtension(), true), result});
            }
            
            count += 2;
        } else {
            genSettings.readFromTexture = 0;
            genSettings.normalMapped = false;
            
            ShaderGenerationResult result = generateFragmentShaderImpl(genSettings);
            if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                successCount++;
            } else {
                multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, 0, getFragmentShaderExtension(), false), result});
            }
            
            count += 1;
        }
    } else {
        ComponentsReadFromTexture maxVal;
        for (std::size_t i = 0; i < definition.getMaterialComponents().size(); ++i) {
            maxVal.set(i);
        }
        
        for (std::size_t i = 0; i <= maxVal.to_ullong(); ++i) {
            if (definition.normalDataRequired) {
                genSettings.readFromTexture = ComponentsReadFromTexture(i);
                genSettings.normalMapped = false;
                ShaderGenerationResult result = generateFragmentShaderImpl(genSettings);
                if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                    successCount++;
                } else {
                    multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, i, getFragmentShaderExtension(), false), result});
                }
                
                genSettings.readFromTexture = ComponentsReadFromTexture(i);
                genSettings.normalMapped = true;
                result = generateFragmentShaderImpl(genSettings);
                if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                    successCount++;
                } else {
                    multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, i, getFragmentShaderExtension(), true), result});
                }
            
                count += 2;
            } else {
                genSettings.readFromTexture = ComponentsReadFromTexture(i);
                genSettings.normalMapped = false;
                ShaderGenerationResult result = generateFragmentShaderImpl(genSettings);
                if (result.getStatus() == ShaderGenerationResult::Status::Success) {
                    successCount++;
                } else {
                    multiResult.fragmentShaderErrors.push_back({makeFragmentShaderName(definition.name, i, getFragmentShaderExtension(), false), result});
                }
                
                count++;
            }
        }
    }
    
    multiResult.totalFragmentShaderCount = count;
    multiResult.generatedFragmentShaderCount = successCount;
}

std::string ShaderGenerator::makeVertexShaderName(const std::string& pipelineName, const std::string& vertexLayoutName, const std::string& extension, bool normalMapped) const {
    return fmt::format("{}_{}_{}{}{}", pipelineName, renderer->getName(), vertexLayoutName, normalMapped ? "_normalMapped" : "", extension);
}

std::string ShaderGenerator::makeFragmentShaderName(const std::string& pipelineName, const ComponentsReadFromTexture& readFromTexture, const std::string& extension, bool normalMapped) const {
    return fmt::format("{}_{}_{}{}{}", pipelineName, renderer->getName(), readFromTexture.to_ullong(), normalMapped ? "_normalMapped" : "", extension);
}

ShaderGenerationResult ShaderGenerator::generateFragmentShader(const FragmentShaderGenerationSettings& settings) const {
    ShaderGenerationResult result = validateShaderGenerationSettings(settings);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        return result;
    }
    
    result = validatePipelineDefinition(*settings.materialDefinition);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        return result;
    }
    return generateFragmentShaderImpl(settings);
}

ShaderGenerationResult ShaderGenerator::generateVertexShader(const VertexShaderGenerationSettings& settings) const {
    ShaderGenerationResult result = validateShaderGenerationSettings(settings);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        return result;
    }
    
    result = validatePipelineDefinition(*settings.materialDefinition);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        return result;
    }
    
    return generateVertexShaderImpl(settings);
}

ShaderGenerationResult ShaderGenerator::validateShaderGenerationSettings(const VertexShaderGenerationSettings& settings) const {
    if (settings.materialDefinition == nullptr) {
        return {ShaderGenerationResult::Status::InvalidGenerationSettings, "The materialDefinition cannot be nullptr"};
    }
    
    return {ShaderGenerationResult::Status::Success, ""};
}

ShaderGenerationResult ShaderGenerator::validateShaderGenerationSettings(const FragmentShaderGenerationSettings&) const {
    return {ShaderGenerationResult::Status::Success, ""};
}

}
