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
#include <set>

#include "core/Constants.hpp"
#include "core/Platform.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "graphics/Lights.hpp"
#include "graphics/materials/MaterialFamilyDefinition.hpp"
#include "graphics/ShaderMacros.hpp"
#include "graphics/RendererProperties.hpp"
#include "io/interfaces/Serializable.hpp"
#include "io/Path.hpp"

namespace iyf {
class FileSystem;
class MaterialLogicGraph;

class ShaderGenerationResult {
public:
    enum class Status {
        Success,
        InvalidFamilyName,
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
        MissingMaterialLogicGraph,
        MaterialLogicGraphPresent,
        InvalidMaterialLogicGraph,
        Invalid,
    };
    
    inline ShaderGenerationResult() : status(Status::Invalid), contents("This is a default constructed result with no information.") {}
    inline ShaderGenerationResult(Status status, std::string contents) : status(status), contents(contents) { }
    
    inline operator bool() const {
        return status == Status::Success;
    }
    
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
    
    inline operator bool() const {
        return status == Status::Success;
    }
    
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

enum class ShaderOptimizationLevel : std::uint8_t {
    NoOptimization, /// < Disable all optimizations
    Size, /// < Optimize for minimal assembly size
    Performance /// < Optimize for maximum performance
};

class ShaderVariantPicker {
public:
    ShaderVariantPicker() : identifierNeedsRebuilding(true) {}
    
    inline StringHash getIdentifier() const {
        if (identifierNeedsRebuilding) {
            lastIdentifier = generateIdentifier();
        }
        
        return lastIdentifier;
    }
private:
    StringHash generateIdentifier() const;
    
    std::set<ShaderMacroWithValue> macros;
    
    mutable StringHash lastIdentifier;
    bool identifierNeedsRebuilding;
};

struct ShaderCompilationSettings {
    ShaderCompilationSettings() : logAssembly(false), optimizationLevel(ShaderOptimizationLevel::Performance), vertexDataLayout(VertexDataLayout::MeshVertex) {}
    
    /// Macros that need to be defined before compiling the shader.
    ///
    /// \remark The order is important. If the macro is defined multiple times, the last definition wins
    std::vector<ShaderMacroWithValue> macros;
    
    /// If this is true and the ShaderGenerator supports it, human readable shader assembly will be written to log.
    bool logAssembly;
    
    /// The optimization level to use when commpiling the shader. Not all ShaderGenerators support all levels
    ShaderOptimizationLevel optimizationLevel;
    
    VertexDataLayout vertexDataLayout;
    
    bool isMacroDefined(const std::string& macroName) const;
    const ShaderMacroWithValue* getMacro(const std::string& macroName) const;
    bool isMacroDefined(ShaderMacro macro) const;
    const ShaderMacroWithValue* getMacro(ShaderMacro macro) const;
};

/// \brief This class generates shader code based on data provided in MaterialFamilyDefinition objects.
///
/// \remark The methods of this class are thread safe for as long as you can ensure that different invocations write to different files.
///
/// \todo Support geometry and tesselation shader generation as well.
class ShaderGenerator {
public:
    ShaderGenerator(const FileSystem* fileSystem);
    virtual ~ShaderGenerator() {}
    
    /// Generate a shader of the specified shader stage based on the provided MaterialFamilyDefinition
    ///
    /// \param stage A shader stage to generate for.
    /// \param definition A MaterialFamilyDefinition that will be used to generate common code.
    /// \param graph A pointer to a MaterialLogicGraph that will be used to generate material specific code. Must be not null if stage == ShaderStageFlagBits::Fragment
    /// and null otherwise.
    ShaderGenerationResult generateShader(PlatformIdentifier platform, ShaderStageFlagBits stage, const MaterialFamilyDefinition& definition, const MaterialLogicGraph* graph) const;
    
    /// Compile the generated shader
    virtual ShaderCompilationResult compileShader(ShaderStageFlagBits stage, const std::string& source, const std::string& name, const ShaderCompilationSettings& settings) const = 0;
    
    /// \return An identifier of the shader language that this generator outputs.
    virtual ShaderLanguage getShaderLanguage() const = 0;
    
    /// The material database can use this to determine if the helper functions provided by the engine have been updated and the cache
    /// needs to be regenerated.
    ///
    /// \return The version of helper functions provided via includes.
    virtual std::uint32_t GetHelperFunctionVersion() const = 0;
    
    /// \return A file extension that should be used for shaders generated by this generator
    virtual Path getShaderStageFileExtension(ShaderStageFlagBits stage) const = 0;
    
    /// This checks for major errors that would prevent the shader from being generated.
    ShaderGenerationResult validateFamilyDefinition(const MaterialFamilyDefinition& definition) const;
    
    /// Checks if the material can use the specified vertex data layout.
    ShaderGenerationResult checkVertexDataLayoutCompatibility(const MaterialFamilyDefinition& definition, VertexDataLayout vertexDataLayout) const;
protected:
    /// Generate the vertex shader. Called by generateShader()
    virtual ShaderGenerationResult generateVertexShader(PlatformIdentifier platform, const MaterialFamilyDefinition& definition) const = 0;
    
    /// Generate the fragment shader. Called by generateShader()
    virtual ShaderGenerationResult generateFragmentShader(PlatformIdentifier platform, const MaterialFamilyDefinition& definition, const MaterialLogicGraph* graph) const = 0;
    
    /// Generate the per frame data inputs (e.g., camera, light and material data).
    virtual std::string generatePerFrameData(const ShaderDataSets& requiredDataSets, const void* extraData) const = 0;
    
    virtual std::string generateLightProcessingFunctionCall(const MaterialFamilyDefinition& definition, bool global) const = 0;
    virtual std::string generateLightProcessingFunctionSignature(const MaterialFamilyDefinition& definition, bool global) const = 0;
    
    ShaderGenerationResult generateAndReportError(ShaderGenerationResult::Status status, const std::string& error) const;
    
    const FileSystem* fileSystem;
};
}


#endif // SHADER_GENERATOR_HPP
