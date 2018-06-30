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

#ifndef IYF_MATERIAL_PIPELINE_DEFINITION_HPP
#define IYF_MATERIAL_PIPELINE_DEFINITION_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <bitset>

#include "core/Constants.hpp"
#include "core/interfaces/Serializable.hpp"
#include "graphics/ShaderConstants.hpp"
#include "utilities/hashing/Hashing.hpp"

namespace iyf {
struct MaterialComponent : public Serializable {
    MaterialComponent() : componentCount(1), isSigned(false), offset(0) {}
    MaterialComponent(std::string name, std::uint16_t componentCount, bool isSigned = false) : name(name), componentCount(componentCount), isSigned(isSigned), offset(0) { }
    
    std::string name;
    std::uint16_t componentCount;
    bool isSigned;
    std::uint8_t paddingByte;
    std::uint32_t offset;
    
    friend bool operator<(const MaterialComponent& left, const MaterialComponent& right) {
        return left.componentCount < right.componentCount;
    }
    
    friend bool operator>(const MaterialComponent& left, const MaterialComponent& right) {
        return left.componentCount > right.componentCount;
    }
    
    friend bool operator==(const MaterialComponent& left, const MaterialComponent& right) {
        return left.componentCount == right.componentCount && left.name == right.name;
    }
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
};

using ShaderDataSets = std::bitset<static_cast<std::size_t>(PerFrameDataSets::COUNT)>;
using ComponentsReadFromTexture = std::bitset<con::MaxMaterialComponents>;
    
struct ShaderInputOutputVariable : public Serializable {
    ShaderInputOutputVariable(std::string name, ShaderDataType type, ShaderDataFormat format) : name(name), type(type), format(format) {}
    
    std::string name;
    ShaderDataType type;
    ShaderDataFormat format;
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
};

/// A structure that defines all properties and code of a specific material pipeline family.
///
/// Uniquely identified by the name property
struct MaterialPipelineDefinition : public Serializable {
    /// The current version of this struct. Should be updated whenever the layout changes.
    static const std::uint16_t VERSION = 1;
    
    MaterialPipelineDefinition() : name("EmptyPipeline"), normalDataRequired(true), supportsMultipleLights(true), numRequiredColorChannels(0), requiresAdditionalVertexProcessing(false), usesGeometryShaders(false), vertexColorDataRequired(false), vertexColorDataRequiredGS(false), logAssembly(false), textureCoordinatesRequired(true), languages{ShaderLanguage::GLSLVulkan} {
        vertexShaderDataSets[static_cast<std::size_t>(PerFrameDataSets::ObjectTransformations)] = true;
        fragmentShaderDataSets[static_cast<std::size_t>(PerFrameDataSets::LightsAndCamera)] = true;
        fragmentShaderDataSets[static_cast<std::size_t>(PerFrameDataSets::RendererData)] = true;
        fragmentShaderDataSets[static_cast<std::size_t>(PerFrameDataSets::MaterialData)] = true;
        fragmentShaderDataSets[static_cast<std::size_t>(PerFrameDataSets::TextureData)] = true;
    }
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
    
    /// The name of the material pipeline
    ///
    /// \warning The name must be usable as both a file name and as a function name. That is, it must match this
    /// regular expression: [a-zA-Z][a-zA-Z0-9]*
    std::string name;
    
    /// Does the material pipeline require normal data to compute lighting or to perform normal mapping. Most do.
    bool normalDataRequired;
    
    /// Does the material pipeline support lights. If this is false, the light loop won't be generated and the
    /// lightProcessingCode will only be called once (without per-light parameters). Moreover, if this is false,
    /// the world space position data won't be computed
    bool supportsMultipleLights;
    
    /// The number of per vertex color data channels that this material pipeline requires.
    /// \warning For now, the maximum is 1.
    std::uint8_t numRequiredColorChannels;
    
    /// If this is true, the contents of additionalVertexProcessingCode will be
    /// non-empty and they will be inserted at the bottom of the vertex shader
    bool requiresAdditionalVertexProcessing;
    
    /// Does this material pipeline require a geometry shader.
    /// \todo Add geometry shader support
    bool usesGeometryShaders;
    
    /// Does the material pipeline require vertex color data to compute lighting.
    bool vertexColorDataRequired;
    
    /// Does the geometry shader require vertex color data.
    /// \todo Add geometry shader support
    bool vertexColorDataRequiredGS;
    
    /// If the shader generator is capable and compilation has been enabled, the shader assembly will be generated and written to log
    /// with verbose priority
    bool logAssembly;
    
    /// Additional vertex processing code that must be written by a human. The number of code strings in this
    /// vector and their languages must match what's defined in the 'languages' member of this struct.
    ///
    /// Contents are appended at the end of the generated vertex shaders if requiresAdditionalVertexProcessing
    /// is true
    ///
    /// \warning The syntax of the provided code will only be checked by the compiler when generateShaders(), generateVertexShader(),
    ///  generateFragmentShader() or similar functions are called with compile set to true.
    std::vector<std::string> additionalVertexProcessingCode;
    
    /// Does the material pipeline require texture data. Most do.
    /// \warning Cases without UV data are not handled well
    /// \todo Fix generation of pipelines without UV data
    bool textureCoordinatesRequired;
    
    bool usesTessellation;
    
    /// Additional vertex shader outputs. Only added to the vertex shader if requiresAdditionalVertexProcessing
    /// is true.
    ///
    /// \todo Maybe check if the name is unique
    std::vector<ShaderInputOutputVariable> additionalVertexOutputs;
    /// Data sets that are needed in the vertex shader
    ShaderDataSets vertexShaderDataSets;
    /// The code that computes the actual shading. Must be written by a human. The number of code strings in this
    /// vector and their languages must match what's defined in the 'languages' member of this struct.
    ///
    /// \warning The syntax of the provided code will only be checked by the compiler when generateShaders(), generateVertexShader(),
    ///  generateFragmentShader() or similar functions are called with compile set to true.
    std::vector<std::string> lightProcessingCode;
    /// Data sets that are needed in the fragment shader
    ShaderDataSets fragmentShaderDataSets;
    /// Data sets that are needed in the geometry shader
    ShaderDataSets geometryShaderDataSets;
    
    /// The shading languages that this pipeline supports. All additional code snippets used in this 
    /// struct must be written in these languages.
    std::vector<ShaderLanguage> languages;
    
    /// This will automatically validate, sort and pack the components into an optimal layout. Make sure to call getMaterialComponents() 
    /// to retrieve a reference to the sorted vector and base any further operations on that instead of what was provided as input
    /// to this function.
    std::pair<bool, std::string> setMaterialComponents(const std::vector<MaterialComponent>& componentList);
    
    /// Returns a sorted list of material components
    const std::vector<MaterialComponent>& getMaterialComponents() const {
        return materialComponents;
    }
private:
    /// List of all possible per-material data. Sorted and packed into an optimal layout. Created and used automatically 
    /// when 
    std::vector<MaterialComponent> materialComponents;
    std::pair<bool, std::string> packMaterialData(std::vector<MaterialComponent>& components) const;
};

enum class DefaultMaterialPipeline {
    /// \warning DO NOT USE this for shader generation. This is supposed to act as an empty starting point with some sensible defaults.
    Empty = 0,
    Toon = 1,
    // PBR = 2, // TODO implement PBR rendering
    COUNT
};

using DefaultMaterialPipelineDefinitionArray = std::array<MaterialPipelineDefinition, static_cast<std::size_t>(DefaultMaterialPipeline::COUNT)>;
extern const DefaultMaterialPipelineDefinitionArray DefaultMaterialPipelineDefinitions;
}

#endif // IYF_MATERIAL_PIPELINE_DEFINITION_HPP
