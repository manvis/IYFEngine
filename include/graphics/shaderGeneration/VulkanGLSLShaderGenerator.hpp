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

#include "graphics/shaderGeneration/ShaderGenerator.hpp"
#include "libshaderc/shaderc.hpp"

namespace iyf {
class FileSystem;

class VulkanGLSLIncluder : public shaderc::CompileOptions::IncluderInterface {
public:
    inline VulkanGLSLIncluder(bool debugIncludes = false) : debugIncludes(debugIncludes) {}
    
    virtual shaderc_include_result* GetInclude(const char* requested_source,
                                               shaderc_include_type type,
                                               const char* requesting_source,
                                               size_t include_depth) final override;

    virtual void ReleaseInclude(shaderc_include_result* data) final override;
    
    static std::uint32_t GetHelperFunctionVersion() {
        return 1;
    }
private:
    bool debugIncludes;
    
    static const std::string EmptyString;
    static const std::string UnknownNameError;
    static const std::string CommonHelperFunctions;
    static const std::string VertexShaderHelperFunctions;
    static const std::string FragmentShaderHelperFunctions;
};

class VulkanGLSLShaderGenerator : public ShaderGenerator {
public:
    VulkanGLSLShaderGenerator(const Engine* engine);
    virtual ~VulkanGLSLShaderGenerator();
    
    virtual ShaderLanguage getShaderLanguage() const final override {
        return ShaderLanguage::GLSLVulkan;
    }
    
    std::uint32_t GetHelperFunctionVersion() const final override {
        return VulkanGLSLIncluder::GetHelperFunctionVersion();
    }
    
    virtual ShaderCompilationResult compileShader(ShaderStageFlagBits stage, const std::string& source, const std::string& name, const ShaderCompilationSettings& settings) const final override;
    
    virtual std::string generateLightProcessingFunctionSignature(const MaterialFamilyDefinition& definition) const final override;
    
    virtual fs::path getShaderStageFileExtension(ShaderStageFlagBits stage) const final override;
protected:
    virtual ShaderGenerationResult generateVertexShader(const MaterialFamilyDefinition& definition) const final override;
    virtual ShaderGenerationResult generateFragmentShader(const MaterialFamilyDefinition& definition) const final override;
    
    virtual std::string generatePerFrameData(const ShaderDataSets& requiredDataSets) const final override;
    
    virtual std::string generateLightProcessingFunctionCall(const MaterialFamilyDefinition& definition) const final override;
    
    shaderc::Compiler compiler;
    FileSystem* fileSystem;
};
}

#endif // VULKAN_GLSL_SHADER_GENERATOR_HPP
