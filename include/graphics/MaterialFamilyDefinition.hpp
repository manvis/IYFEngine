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

#ifndef IYF_MATERIAL_FAMILY_DEFINITION_HPP
#define IYF_MATERIAL_FAMILY_DEFINITION_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <bitset>

#include "core/Constants.hpp"
#include "core/interfaces/Serializable.hpp"
#include "graphics/ShaderConstants.hpp"
#include "utilities/hashing/Hashing.hpp"

#include "glm/vec4.hpp"

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

using ShaderDataSets = std::bitset<static_cast<std::size_t>(PerFrameDataSet::COUNT)>;
using ComponentsReadFromTexture = std::bitset<con::MaxMaterialComponents>;
    
class ShaderVariable : public Serializable {
public:
    ShaderVariable() : ShaderVariable("unnamedVariable", ShaderDataType::Scalar, ShaderDataFormat::Float) {}
    
    /// Create a new shader variable definition.
    ///
    /// \throws std::invalid_argument if the validation of the name fails.
    ShaderVariable(std::string name, ShaderDataType type, ShaderDataFormat format);
    
    /// Set the name of the shader variable.
    ///
    /// \remark The name must be start with a letter. Other symbols must be letters or numbers.
    ///
    /// \throw std::invalid_argument if name validation fails 
    inline void setName(std::string newName) {
        if (!validateName(newName)) {
            throw std::invalid_argument("The validation of the name failed");
        }
        
        name = newName;
    }
    
    /// \see setName()
    inline const std::string& getName() const {
        return name;
    }
    
    /// Checks if the name can be used for a variable.
    bool validateName(const std::string& newName) const;
    
    ShaderDataType type;
    ShaderDataFormat format;
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
private:
    std::string name;
};

class LightProcessingFunctionInput : public Serializable {
public:
    LightProcessingFunctionInput() {}
    
    LightProcessingFunctionInput(const std::string& name, ShaderDataType type, glm::vec4 defaultValue = glm::vec4()) 
        : defaultValue(std::move(defaultValue))
    {
        setVariableDefinition(ShaderVariable(name, type, ShaderDataFormat::Float));
    }
    
    /// The default value to use when the input is not connected. Only the required number of elements will be used.
    /// E.g., if ShaderDataType is ShaderDataType::Vector2D only x and y will be used.
    glm::vec4 defaultValue;
    
    /// \warning Currently, a limitation exists. You must only use ShaderDataType::Scalar, ShaderDataType::Vector2D,
    /// ShaderDataType::Vector3D or ShaderDataType::Vector4D for the type and ShaderDataFormat::Float for the format.
    ///
    /// \throws std::invalid_argument if a forbidden ShaderDataType or ShaderDataFormat was used.
    inline void setVariableDefinition(ShaderVariable variable) {
        const bool formatValid = (variable.format == ShaderDataFormat::Float);
        const bool typeValid = (variable.type == ShaderDataType::Scalar)   ||
                               (variable.type == ShaderDataType::Vector2D) ||
                               (variable.type == ShaderDataType::Vector3D) ||
                               (variable.type == ShaderDataType::Vector4D);
        
        if (!formatValid || !typeValid) {
            throw std::invalid_argument("A forbidded format or data type was used");
        }
        
        this->variable = variable;
    }
    
    const ShaderVariable& getVariableDefinition() const {
        return variable;
    }
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
private:
    ShaderVariable variable;
};

/// Each material family is a set of materials based on a specific light processing function that gives them a certain
/// look. E.g., the light processing function of a toon material family is designed to give materials derived from it a
/// cell shaded look.
class MaterialFamilyDefinition : public Serializable {
public:
    /// The current version of this class. Should be updated whenever the layout changes.
    static const std::uint16_t VERSION = 1;
    
    MaterialFamilyDefinition();
    
    virtual void serialize(Serializer& fw) const final;
    virtual void deserialize(Serializer& fr) final;
    
    /// Computes a hash that uniquely identifies this MaterialFamilyDefinition.
    ///
    /// \remark This function serializes this MaterialFamilyDefinition to a memory buffer and then computes the 
    /// hash of the said buffer. For performance reasons, you should cache the computed values.
    FileHash computeHash() const;
    
    /// Get the name of the material family
    inline const std::string& getName() const {
        return name;
    }
    
    /// Set the name of the material family
    ///
    /// \remark The name must be usable as both a file name and as a function name. That is, it must match this
    /// regular expression: [a-zA-Z][a-zA-Z0-9]* The length of the name must be between 1 and con::MaxMaterialFamilyNameLength
    /// characters.
    ///
    /// \throws std::invalid_argument if the name did not match the regex
    void setName(std::string name);
    
    /// \return true if the name matched the validation regex and length requirements and false if did not
    bool validateName(const std::string& name) const;
    
    /// Sets the shading languages that this material family supports. All additional code snippets used in this 
    /// struct must be written in these languages.
    ///
    /// \warning Setting this value will resize and clear additionalVertexProcessingCode and lightProcessingCode.
    /// Therefore, make sure to use this function BEFORE setting any code snippets.
    ///
    /// \throws std::invalid_argument if supportedLanguages vector was empty or a single language was specified
    /// multiple times.
    void setSupportedLanguages(std::vector<ShaderLanguage> supportedLanguages);
    
    /// \see setSupportedLanguages()
    inline const std::vector<ShaderLanguage>& getSupportedLanguages() const {
        return supportedLanguages;
    }
    
    /// Additional vertex processing code that must be written by a human. The number of code strings in this
    /// vector and their languages must match what's defined in the 'languages' member of this struct.
    ///
    /// \warning The syntax of the provided code will only be checked when compiling the material shaders.
    ///
    /// \throws std::invalid_argument if the size of the provided vector did not match the size of the supportedLanguages
    /// vector
    inline void setAdditionalVertexProcessingCode(std::vector<std::string> code) {
        if (code.size() != supportedLanguages.size()) {
            throw std::invalid_argument("The size of the code vector must match the size of the supportedLanguages vector");
        }
        
        this->additionalVertexProcessingCode = std::move(code);
    }
    
    /// \see setAdditionalVertexProcessingCode()
    inline const std::vector<std::string>& getAdditionalVertexProcessingCode() const {
        return additionalVertexProcessingCode;
    }
    
    /// The code that computes the actual shading. Must be written by a human. The number of code strings in this
    /// vector and their languages must match what's defined in the 'languages' member of this struct.
    ///
    /// \warning The syntax of the provided code will only be checked when compiling the material shaders.
    ///
    /// \throws std::invalid_argument if the size of the provided vector did not match the size of the supportedLanguages
    /// vector
    inline void setLightProcessingCode(std::vector<std::string> code) {
        if (code.size() != supportedLanguages.size()) {
            throw std::invalid_argument("The size of the code vector must match the size of the supportedLanguages vector");
        }
        
        this->lightProcessingCode = std::move(code);
    }
    
    /// \see setLightProcessingCode()
    inline const std::vector<std::string>& getLightProcessingCode() const {
        return lightProcessingCode;
    }
    
    /// Does the material family require normal data (e.g., to compute lighting)?
    ///
    /// \remark Setting this to true will make all VertexDataLayout variants that lack a normal vector incompatible
    /// with this material family.
    inline void setNormalDataRequired(bool required) {
        setFlagValue(Flag::NormalDataRequired, required);
    }
    
    /// \see setNormalDataReguired()
    inline bool isNormalDataRequired() const {
        return getFlagValue(Flag::NormalDataRequired);
    }
    
    /// Should world space position be computed in the vertex shader and passed to the light processing function.
    inline void setWorldSpacePositionRequired(bool required) {
        setFlagValue(Flag::WorldSpacePositionRequired, required);
    }
    
    /// \see setWorldSpacePositionRequired()
    inline bool isWorldSpacePositionRequired() const {
        return getFlagValue(Flag::WorldSpacePositionRequired);
    }
    
    /// Does the material family support lights? If this is false, the light loops won't be generated and the
    /// lightProcessingCode will only be called once (without per-light parameters).
    inline void setLightsSupported(bool supported) {
        setFlagValue(Flag::LightsSupported, supported);
    }
    
    inline bool areLightsSupported() const {
        return getFlagValue(Flag::LightsSupported);
    }
    
    /// Sets the number of required per vertex color data channels.
    ///
    /// \warning For now, the maximum is 1.
    ///
    /// \throws std::invalid_argument if the number was greater than the maximum.
    inline bool setRequiredVertexColorChannelCount(std::uint8_t count) {
        if (count > 1) {
            throw std::invalid_argument("Count was greater than maximum");
        }
        
        requiredVertexColorChannelCount = count;
        return true;
    }
    
    /// \see setRequiredVertexColorChannelCount()
    inline std::uint8_t getRequiredVertexColorChannelCount() const {
        return requiredVertexColorChannelCount;
    }
    
    /// \see setRequiredVertexColorChannelCount()
    inline bool isVertexColorDataRequired() const {
        return requiredVertexColorChannelCount > 0;
    }
    
    /// Does the material family require texture data?
    ///
    /// \warning Cases without UV data are not handled well
    ///
    /// \todo Fix generation of families without UV data
    inline void setTextureCoordinatesRequired(bool required) {
        setFlagValue(Flag::TextureCoordinatesRequired, required);
    }
    
    /// \see setTextureCoordinatesRequired()
    inline bool areTextureCoordinatesRequired() const {
        return getFlagValue(Flag::TextureCoordinatesRequired);
    }
    
    /// Set which data sets need to be included in the vertex shader.
    ///
    /// \remark For performance reasons, it's better to only include the ones you need.
    ///
    /// \warning Don't disable the data sets that are enabled by default
    inline void setVertexShaderDataSet(PerFrameDataSet dataSet, bool enabled) {
        vertexShaderDataSets[static_cast<std::size_t>(dataSet)] = enabled;
    }
    
    /// \see setVertexShaderDataSet()
    inline bool isVertexShaderDataSetEnabled(PerFrameDataSet dataSet) const {
        return vertexShaderDataSets[static_cast<std::size_t>(dataSet)];
    }
    
    /// Returns the status of vertex shader data sets as a bitset
    inline const ShaderDataSets& getVertexShaderDataSets() const {
        return vertexShaderDataSets;
    }
    
    /// Set which data sets need to be included in the fragment shader.
    ///
    /// \remark For performance reasons, it's better to only include the ones you need.
    ///
    /// \warning Don't disable the data sets that are enabled by default
    inline void setFragmentShaderDataSet(PerFrameDataSet dataSet, bool enabled) {
        fragmentShaderDataSets[static_cast<std::size_t>(dataSet)] = enabled;
    }
    
    /// \see setFragmentShaderDataSet()
    inline bool isFragmentShaderDataSetEnabled(PerFrameDataSet dataSet) const {
        return fragmentShaderDataSets[static_cast<std::size_t>(dataSet)];
    }
    
    /// Returns the status of fragment shader data sets as a bitset
    inline const ShaderDataSets& getFragmentShaderDataSets() const {
        return fragmentShaderDataSets;
    }
    
    bool validateAdditionalVertexShaderOutputs(const std::vector<ShaderVariable>& additionalVertexOutputs) const;
    
    /// Additional vertex shader outputs. Don't forget to actually SET these values using additionalVertexProcessingCode
    ///
    /// \throw std::invalid_argument if the validation failed.
    inline bool setAdditionalVertexShaderOutputs(std::vector<ShaderVariable> additionalVertexOutputs) {
        if (!validateAdditionalVertexShaderOutputs(additionalVertexOutputs)) {
            throw std::invalid_argument("The validation of the additionalVertexOutputs vector failed.");
        }
        
        this->additionalVertexOutputs = std::move(additionalVertexOutputs);
        return true;
    }
    
    /// \see setAdditionalVertexShaderOutputs()
    inline const std::vector<ShaderVariable>& getAdditionalVertexShaderOutputs() const {
        return additionalVertexOutputs;
    }
    
    /// Sets additional inputs of the light processing function. Typically, things that depend on the material.
    inline void setLightProcessingFunctionInputs(std::vector<LightProcessingFunctionInput> inputs) {
        lightProcessingFunctionInputs = std::move(inputs);
    }
    
    /// \see setLightProcessingFunctionInputs()
    inline const std::vector<LightProcessingFunctionInput>& getLightProcessingFunctionInputs() const {
        return lightProcessingFunctionInputs;
    }
    
//     /// This will automatically validate, sort and pack the components into an optimal layout. Make sure to call getMaterialComponents() 
//     /// to retrieve a reference to the sorted vector and base any further operations on that instead of what was provided as input
//     /// to this function.
//     std::pair<bool, std::string> setMaterialComponents(const std::vector<MaterialComponent>& componentList);
//     
//     /// Returns a sorted list of material components
//     const std::vector<MaterialComponent>& getMaterialComponents() const {
//         return materialComponents;
//     }
private:
    enum class Flag {
        NormalDataRequired         = 0,
        WorldSpacePositionRequired = 1,
        LightsSupported            = 2,
        TextureCoordinatesRequired = 3,
//         // TODO FIXME I should require specific FEATURES that force the generation of tessellation and geometry shaders
//         GeometryShaderRequired     = 62,
//         TessellationShaderRequired = 63,
    };
    
    inline bool getFlagValue(Flag flag) const {
        return flagValues[static_cast<std::size_t>(flag)];
    };
    
    inline void setFlagValue(Flag flag, bool value) {
        flagValues[static_cast<std::size_t>(flag)] = value;
    }
    
    std::bitset<64> flagValues;
    
    ShaderDataSets vertexShaderDataSets;
    ShaderDataSets fragmentShaderDataSets;
    
    std::string name;
    std::uint8_t requiredVertexColorChannelCount;
    
    std::vector<ShaderLanguage> supportedLanguages;
    std::vector<std::string> additionalVertexProcessingCode;
    std::vector<std::string> lightProcessingCode;
    
    std::vector<ShaderVariable> additionalVertexOutputs;
    
    std::vector<LightProcessingFunctionInput> lightProcessingFunctionInputs;
    
//     /// List of all possible per-material data. Sorted and packed into an optimal layout. Created and used automatically 
//     /// when 
//     std::vector<MaterialComponent> materialComponents;
//     std::pair<bool, std::string> packMaterialData(std::vector<MaterialComponent>& components) const;
};

/// List of all material families supported by the engine
enum class MaterialFamily {
    Toon = 0,
    // PBR = 1, // TODO implement PBR rendering
    COUNT
};

namespace con {
    const MaterialFamilyDefinition& GetMaterialFamilyDefinition(MaterialFamily family);
}
}

#endif // IYF_MATERIAL_FAMILY_DEFINITION_HPP
