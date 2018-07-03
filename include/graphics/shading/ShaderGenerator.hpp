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

#ifndef SHADER_GENERATOR_HPP
#define SHADER_GENERATOR_HPP

#include <cstdint>
#include <string>
#include <bitset>

#include "core/Constants.hpp"
#include "core/Platform.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "graphics/Lights.hpp"
#include "graphics/MaterialPipelineDefinition.hpp"
#include "core/interfaces/Serializable.hpp"

namespace iyf {
class Renderer;
class Engine;

class ShaderGenerationResult {
public:
    enum class Status {
        Success,
        InvalidPipelineName,
        InvalidGenerationSettings,
        MissingLightProcessing,
        DuplicateLanguages,
        LanguageNotSupported,
        ReservedKeyword,
        DuplicateIdentifier,
        ComponentPackingFailed,
        MissingAdditionalVertexProcessingCode,
        MissingVertexAttribute,
        CompilationFailed,
        ShaderGenerationNotAttempted,
    };
    
    inline ShaderGenerationResult() : status(Status::ShaderGenerationNotAttempted), contents("You're reading the result before even calling the shader generation function") {}
    inline ShaderGenerationResult(Status status, std::string contents) : status(status), contents(contents) { }
    
    /// \return The status of the shader generation operation.
    inline Status getStatus() const {
        return status;
    }
    
    /// \return The source code of the shader if getStatus() == Status::Success and a human readable error if it's not.
    inline const std::string& getContents() const {
        return contents;
    }
private:
    Status status;
    std::string contents;
};

struct ShaderGenerationSettings {
    ShaderGenerationSettings() : platform(con::GetCurrentPlatform()), compileShader(true), writeSource(false), normalMapped(true) {}
    
    /// The platform to compile/generate for
    PlatformIdentifier platform;
    
    /// If compileShader is true, the final compiled shader will be written to this path.
    ///
    /// This path should point to a directory. An appropriate filename will be generated automatically and appended to it.
    fs::path compiledShaderPath;
    
    /// If this is not empty, the source code of the generated shader will be written to this path.
    ///
    /// This path should point to a directory. An appropriate filename will be generated automatically and appended to it.
    fs::path shaderSourcePath;
    
    /// A pointer to the definition of a material. Must not be nullptr.
    const MaterialPipelineDefinition* materialDefinition;
    
    /// If this is true, the shader will be compiled and placed in the compiledShaderPath.
    bool compileShader;
    
    /// If this is true, the generated shader source will be placed in the shaderSourcePath.
    bool writeSource;
    
    /// Is normal mapping enabled in this variant (requires tangents and bitangents to be present in the input vertex layout)
    bool normalMapped;
};

struct VertexShaderGenerationSettings : ShaderGenerationSettings {
    VertexShaderGenerationSettings() : ShaderGenerationSettings(), vertexDataLayout(VertexDataLayout::MeshVertexColored) {}
    
    /// The vertex data layout to generate for.
    VertexDataLayout vertexDataLayout;
};

struct FragmentShaderGenerationSettings : ShaderGenerationSettings {
    FragmentShaderGenerationSettings() : ShaderGenerationSettings(), readFromTexture(0) {}
    
    /// Indicates which inputs will be read from texture and which ones won't
    ComponentsReadFromTexture readFromTexture;
};

struct MultipleShaderGenerationResult {
    MultipleShaderGenerationResult() : totalVertexShaderCount(0), generatedVertexShaderCount(0), totalFragmentShaderCount(0), generatedFragmentShaderCount(0) {}
    
    using NameErrorPair = std::pair<std::string, ShaderGenerationResult>;
    
    /// If pipelineValidationError.first is not equal to ShaderGenerationResult::Success, it means that nothing
    /// was generated because the MaterialPipelineDefinition were incorrect.
    ShaderGenerationResult pipelineValidationError;
    /// Number of vertex shaders that the generator attempted to make.
    std::size_t totalVertexShaderCount;
    /// Number of successfully generated vertex shaders.
    ///
    /// \warning The ShaderGenerator blindly attempts to make shaders for all available vertex layouts. Unless you're generating
    /// some extremely simple pipelines, some vertex shader generation attemps will fail and that's OK. Just make sure that the
    /// shaders you care about are not in vertexShaderErrors.
    std::size_t generatedVertexShaderCount;
    /// Number of vertex fragment that the generator attempted to make.
    std::size_t totalFragmentShaderCount;
    /// Number of successfully generated fragment shaders
    std::size_t generatedFragmentShaderCount;
    /// List of all vertex shaders that the ShaderGenerator failed to create. First element of the pair is the name generated by using
    /// ShaderGenerator::makeVertexShaderName(), second is the ShaderGenerationResult corresponding to it.
    ///
    /// \warning The ShaderGenerator blindly attempts to make shaders for all available vertex layouts. Unless you're generating
    /// some extremely simple pipelines, some vertex shader generation attemps will fail and that's OK. Just make sure that the
    /// shaders you care about are not in this vector.
    std::vector<NameErrorPair> vertexShaderErrors;
    /// List of all fragment shaders that the ShaderGenerator failed to create. First element of the pair is the name generated by using
    /// ShaderGenerator::makeFragmentShaderName(), second is the ShaderGenerationResult corresponding to it.
    std::vector<NameErrorPair> fragmentShaderErrors;
};

/// \brief This class generates shader code based on data provided in MaterialPipelineDefinition objects.
///
/// \remark The methods of this class are thread safe for as long as you can ensure that different invocations write to different files.
///
/// \todo Support geometry and tesselation shader generation as well.
///
/// \todo At the moment, vertex shaders are considered to be a part of the material. However, more often
/// than not, MaterialPipelineDefinition::requiresAdditionalVertexProcessing is false and they can be REUSED by 
/// different materials. Therefore, it would be nice to have a mechanism that would allow us to reuse
/// existing shaders if they are compatible. 
class ShaderGenerator {
public:
    ShaderGenerator(const Engine* engine);
    
    /// This checks for major errors that would prenvent ANY variant of the vertex shader from being generated.
    ///
    /// \warning All deriving classes MUST call this in generateVertexShader();
    ShaderGenerationResult validateVertexShader(const MaterialPipelineDefinition& definition) const;
    
    /// Checks if the material can use the specified vertex data layout.
    ///
    /// \warning All deriving classes MUST call this in generateVertexShader();
    ShaderGenerationResult checkVertexDataLayoutCompatibility(const MaterialPipelineDefinition& definition, VertexDataLayout vertexDataLayout) const;
    
    /// Generates all possible combinations of shader values based on the specified MaterialPipelineDefinition.
    ///
    /// \warning This function may take a long time to complete and should be called in a separate thread. Moreover, since this will generate ALL possible
    /// shader variants, HUNDREDS if not THOUSANDS of files may be created and compiled.
    ///
    /// \param[in] definition All features and extra code required by the pipeline you wish to generate.
    virtual MultipleShaderGenerationResult generateAllShaders(const fs::path& path, const MaterialPipelineDefinition& definition) const;
    
    /// Generates a name for a vertex shader
    virtual std::string makeVertexShaderName(const std::string& pipelineName, const std::string& vertexLayoutName, const std::string& extension, bool normalMapped) const;
    
    /// Generates a name for a fragment shader
    virtual std::string makeFragmentShaderName(const std::string& pipelineName, const ComponentsReadFromTexture& readFromTexture, const std::string& extension, bool normalMapped) const;
protected:
    /// \brief This function finds all compatible VertexDataLayout types and generates vertex shaders for them by calling generateVertexShader
    virtual void generateAllVertexShaders(const fs::path& path, MultipleShaderGenerationResult& multiResult, const MaterialPipelineDefinition& definition) const;
    
    /// \brief This function generates all possible variants of the fragment shader by calling generateFragmentShader()
    virtual void generateAllFragmentShaders(const fs::path& path, MultipleShaderGenerationResult& multiResult, const MaterialPipelineDefinition& definition) const;
    
    ShaderGenerationResult validatePipelineDefinition(const MaterialPipelineDefinition& definition) const;
    ShaderGenerationResult generateAndReportError(ShaderGenerationResult::Status status, const std::string& error) const;
    
// -------------------- PURE VIRTUAL methods start here
public:
    /// The shader language that this generator outputs.
    virtual ShaderLanguage getShaderLanguage() const = 0;
    
    virtual std::string generateLightProcessingFunctionSignature(const MaterialPipelineDefinition& definition) const = 0;
    
    /// First of all, validates the definition, then performs the actual generation of the fragment shader code in the language that's specified by the getShaderLanguage().
    /// All code will end up in files with names generated by the makeFragmentShaderName() function.
    ShaderGenerationResult generateFragmentShader(const FragmentShaderGenerationSettings& settings) const;
    
    /// First of all, validates the definition, then performs the actual generation of the vertex shader code in the language that's specified by the getShaderLanguage().
    /// All code will end up in files with names generated by the makeVertexShaderName() function.
    ShaderGenerationResult generateVertexShader(const VertexShaderGenerationSettings& settings) const;
    
    virtual std::string getVertexShaderExtension() const = 0;
    virtual std::string getFragmentShaderExtension() const = 0;
protected:
    virtual ShaderGenerationResult generateFragmentShaderImpl(const FragmentShaderGenerationSettings& settings) const = 0;
    virtual ShaderGenerationResult generateVertexShaderImpl(const VertexShaderGenerationSettings& settings) const = 0;
    
    virtual std::string generatePerFrameData(const ShaderDataSets& requiredDataSets, const MaterialPipelineDefinition& definition) const = 0;
    
    virtual std::string generateLightProcessingFunctionCall(const MaterialPipelineDefinition& definition) const = 0;
    
    virtual ShaderGenerationResult compileShader(const MaterialPipelineDefinition& definition, const std::string& shaderName, const std::string& shaderSource, const fs::path& savePath, const fs::path& fileName, ShaderStageFlagBits shaderStage) const = 0;
    
    ShaderGenerationResult validateShaderGenerationSettings(const VertexShaderGenerationSettings& settings) const;
    ShaderGenerationResult validateShaderGenerationSettings(const FragmentShaderGenerationSettings& settings) const;
    
    const Engine* engine;
    const Renderer* renderer;
};
}


#endif // SHADER_GENERATOR_HPP
