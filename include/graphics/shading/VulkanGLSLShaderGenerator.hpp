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

#ifndef VULKAN_GLSL_SHADER_GENERATOR_HPP
#define VULKAN_GLSL_SHADER_GENERATOR_HPP

#include "graphics/shading/ShaderGenerator.hpp"
#include "libshaderc/shaderc.hpp"

namespace iyf {
class FileSystem;

class VulkanGLSLShaderGenerator : public ShaderGenerator {
public:
    VulkanGLSLShaderGenerator(const Engine* engine);
    
    virtual ShaderLanguage getShaderLanguage() const final override {
        return ShaderLanguage::GLSLVulkan;
    }
    
    virtual std::string generateLightProcessingFunctionSignature(const MaterialPipelineDefinition& definition) const final override;
    
    virtual std::string getVertexShaderExtension() const final override;
    virtual std::string getFragmentShaderExtension() const final override;
protected:
    virtual ShaderGenerationResult generateFragmentShaderImpl(const FragmentShaderGenerationSettings& settings) const final override;
    virtual ShaderGenerationResult generateVertexShaderImpl(const VertexShaderGenerationSettings& settings) const final override;
    
    virtual std::string generatePerFrameData(const ShaderDataSets& requiredDataSets, const MaterialPipelineDefinition& definition) const final override;
    
//     std::string gatherMaterial
    std::string generateMaterialDataUnpacker(const ComponentsReadFromTexture& readFromTexture, const MaterialPipelineDefinition& definition) const;
    
    virtual std::string generateLightProcessingFunctionCall(const MaterialPipelineDefinition& definition) const final override;
    
    virtual ShaderGenerationResult compileShader(const MaterialPipelineDefinition& definition, const std::string& shaderName, const std::string& shaderSource, const fs::path& savePath, const fs::path& fileName, ShaderStageFlagBits shaderStage) const final override;
    
    shaderc::Compiler compiler;
    shaderc::CompileOptions compilerOptions;
    FileSystem* fileSystem;
};
}

#endif // VULKAN_GLSL_SHADER_GENERATOR_HPP
