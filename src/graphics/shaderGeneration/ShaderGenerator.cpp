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

#include <unordered_set>

namespace iyf {
ShaderGenerator::ShaderGenerator(const Engine* engine) : engine(engine), renderer(engine->getRenderer()) {}

ShaderGenerationResult ShaderGenerator::generateAndReportError(ShaderGenerationResult::Status status, const std::string& error) const {
    LOG_E("Shader generation error: " << error);
    
    return {status, error};
}

ShaderGenerationResult ShaderGenerator::generateShader(ShaderStageFlagBits stage, const MaterialPipelineDefinition& definition) const {
    ShaderGenerationResult result = validatePipelineDefinition(definition);
    if (result.getStatus() != ShaderGenerationResult::Status::Success) {
        return result;
    }
    
    switch (stage) {
        case ShaderStageFlagBits::Vertex:
            return generateVertexShader(definition);
        case ShaderStageFlagBits::Fragment:
            return generateFragmentShader(definition);
        default:
            throw std::runtime_error("Not implemented");
    }
}

ShaderGenerationResult ShaderGenerator::validatePipelineDefinition(const MaterialPipelineDefinition& definition) const {
    assert(definition.getSupportedLanguages().size() == definition.getLightProcessingCode().size());
    assert(definition.getSupportedLanguages().size() == definition.getAdditionalVertexProcessingCode().size());
    
    // TODO validate data that's important to all shaders
    
    std::unordered_set<ShaderLanguage> uniqueLanguages;
    bool found = false;
    
    for (ShaderLanguage sl : definition.getSupportedLanguages()) {
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

ShaderGenerationResult ShaderGenerator::checkVertexDataLayoutCompatibility(const MaterialPipelineDefinition& definition, VertexDataLayout vertexDataLayout) const {
    const VertexDataLayoutDefinition& layout = con::GetVertexDataLayoutDefinition(vertexDataLayout);
    
    bool hasNormals = layout.hasAttribute(VertexAttributeType::Normal);
    if (definition.isNormalDataRequired() && !hasNormals) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it lacks normals.");
    }
    
    bool hasVertexColors = layout.hasAttribute(VertexAttributeType::Color);
    if (definition.isVertexColorDataRequired() && !hasVertexColors) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it lacks vertex colors.");
    }
    
    bool hasUV = layout.hasAttribute(VertexAttributeType::UV);
    if (definition.areTextureCoordinatesRequired() && !hasUV) {
        return generateAndReportError(ShaderGenerationResult::Status::MissingVertexAttribute, "Vertex layout " + layout.getName() + " is incompatible with the material pipeline definition because it texture coordinates.");
    }
    
    return {ShaderGenerationResult::Status::Success, ""};
}

}
