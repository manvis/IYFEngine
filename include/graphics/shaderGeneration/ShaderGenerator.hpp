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
#include "graphics/ShaderMacros.hpp"
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
        Invalid,
    };
    
    inline ShaderGenerationResult() : status(Status::Invalid), contents("This is a default constructed result with no information.") {}
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

class ShaderCompilationResult {
public:
    enum class Status {
        Success,
        NotSupported,
        CompilationFailed,
        Invalid
    };
    
    inline ShaderCompilationResult() : status(Status::Invalid), errorsAndWarnings("This is a default constructed result with no information.") {}
    inline ShaderCompilationResult(Status status, std::string errorsAndWarnings, std::vector<std::uint8_t> bytecode) : status(status), errorsAndWarnings(errorsAndWarnings), bytecode(bytecode) { }
    
    /// \return The status of the shader compilation operation.
    inline Status getStatus() const {
        return status;
    }
    
    /// \return A human readable error and warning string.
    inline const std::string& getErrorsAndWarnings() const {
        return errorsAndWarnings;
    }
    
    /// \return Compiled bytecode. Empty if getStatus() != Status::Success.
    inline const std::vector<std::uint8_t>& getBytecode() const {
        return bytecode;
    }
    
private:
    Status status;
    std::string errorsAndWarnings;
    std::vector<std::uint8_t> bytecode;
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

// /// \warning Use explicit, sequential IDs. They're written to files and having them visible makes debugging easier. Adding IDs at the end
// /// is safe. To reduce the chance of causing bugs, you should avoid removing, reusing or reordering existing IDs.
// /// 
// /// \warning Do not change the underlying type
// enum class ShaderInclude : std::uint16_t {
//     MaterialVertexProcessing = 0, /// < Material specific vertex shader code that gets injected 
//     MaterialFragmentProcessing = 1
// };

enum class ShaderOptimizationLevel : std::uint8_t {
    NoOptimization, /// < Disable all optimizations
    Size, /// < Optimize for minimal assembly size
    Performance /// < Optimize for maximum performance
};

struct ShaderCompilationSettings {
    ShaderCompilationSettings() : logAssembly(false), optimizationLevel(ShaderOptimizationLevel::Performance) {}
    
    /// If this is true and the ShaderGenerator supports it, human readable shader assembly will be written to log.
    bool logAssembly;
    
    ShaderOptimizationLevel optimizationLevel;
    
    /// Macros that need to be defined before compiling the shader.
    std::vector<ShaderMacroWithValue> macros;
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
    
    /// The shader language that this generator outputs.
    virtual ShaderLanguage getShaderLanguage() const = 0;
    
    virtual std::string getVertexShaderExtension() const = 0;
    virtual std::string getFragmentShaderExtension() const = 0;
    
    virtual std::string generateLightProcessingFunctionSignature(const MaterialPipelineDefinition& definition) const = 0;
    
    ShaderGenerationResult generateShader(ShaderStageFlagBits stage, const MaterialPipelineDefinition& definition) const;
    
    virtual ShaderCompilationResult compileShader(ShaderStageFlagBits stage, const std::string& source, const std::string& name, const ShaderCompilationSettings& settings) = 0;
    
    /// This checks for major errors that would prevent the shader from being generated.
    ShaderGenerationResult validatePipelineDefinition(const MaterialPipelineDefinition& definition) const;
    
    /// Checks if the material can use the specified vertex data layout.
    ShaderGenerationResult checkVertexDataLayoutCompatibility(const MaterialPipelineDefinition& definition, VertexDataLayout vertexDataLayout) const;
    
    /// Generates a name for a vertex shader
    virtual std::string makeVertexShaderName(const std::string& pipelineName, const std::string& vertexLayoutName, const std::string& extension, bool normalMapped) const;
    
    /// Generates a name for a fragment shader
    virtual std::string makeFragmentShaderName(const std::string& pipelineName, const ComponentsReadFromTexture& readFromTexture, const std::string& extension, bool normalMapped) const;
protected:
    virtual ShaderGenerationResult generateVertexShader(const MaterialPipelineDefinition& definition) const = 0;
    virtual ShaderGenerationResult generateFragmentShader(const MaterialPipelineDefinition& definition) const = 0;
    
    ShaderGenerationResult generateAndReportError(ShaderGenerationResult::Status status, const std::string& error) const;
protected:
    virtual std::string generatePerFrameData(const ShaderDataSets& requiredDataSets) const = 0;
    
    virtual std::string generateLightProcessingFunctionCall(const MaterialPipelineDefinition& definition) const = 0;
    
    virtual ShaderCompilationResult compileShader(const MaterialPipelineDefinition& definition, const std::string& shaderName, const std::string& shaderSource, const fs::path& savePath, const fs::path& fileName, ShaderStageFlagBits shaderStage) const = 0;
    
    const Engine* engine;
    const Renderer* renderer;
};
}


#endif // SHADER_GENERATOR_HPP
