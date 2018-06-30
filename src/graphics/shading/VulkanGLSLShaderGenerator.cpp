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

#include "graphics/shading/VulkanGLSLShaderGenerator.hpp"
#include "graphics/Renderer.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "format/format.h"
#include "core/Logger.hpp"

#include <stdexcept>

namespace iyf {
const std::string VulkanGLSLHeader =
    "#version 450\n\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n\n";

std::string MakeVulkanGLSLVectorDataType(ShaderDataFormat format, int count) {
    if (count < 2 || count > 4) {
        throw std::logic_error("A vector must have between 2 and 4 components.");
    }
    
    switch (format) {
    case ShaderDataFormat::UnsignedInteger:
        return "uvec" + std::to_string(count);
    case ShaderDataFormat::Integer:
        return "ivec" + std::to_string(count);
    case ShaderDataFormat::Float:
        return "vec" + std::to_string(count);
    case ShaderDataFormat::Double:
        return "dvec" + std::to_string(count);
    }
    
    return "";
}

std::string MakeVulkanGLSLSquareMatrixDataType(ShaderDataFormat format, int count) {
    if (count < 2 || count > 4) {
        throw std::logic_error("A square matrix must have 2, 3 or 4 components in rows/columns.");
    }
    
    switch (format) {
    case ShaderDataFormat::UnsignedInteger:
    case ShaderDataFormat::Integer:
        throw std::logic_error("Integer matrices are not supported.");
    case ShaderDataFormat::Float:
        return "mat" + std::to_string(count);
    case ShaderDataFormat::Double:
        return "dmat" + std::to_string(count);
    }
    
    return "";
}

std::string MakeVulkanGLSLMatrixDataType(ShaderDataFormat format, int n, int m) {
    if (n < 2 || n > 4 || m < 2 || m > 4) {
        throw std::logic_error("A square must have 2, 3 or 4 components in rows/columns.");
    }
    
    switch (format) {
    case ShaderDataFormat::UnsignedInteger:
    case ShaderDataFormat::Integer:
        throw std::logic_error("Integer matrices are not supported.");
    case ShaderDataFormat::Float:
        return "mat" + std::to_string(n) + "x" + std::to_string(m);
    case ShaderDataFormat::Double:
        return "dmat" + std::to_string(n) + "x" + std::to_string(m);
    }
    
    return "";
}

std::string MakeVulkanGLSLDataType(ShaderDataFormat shaderDataFormat, ShaderDataType shaderDataType) {
    switch (shaderDataType) {
    case ShaderDataType::Scalar:
        switch (shaderDataFormat) {
        case ShaderDataFormat::UnsignedInteger:
            return "uint";
        case ShaderDataFormat::Integer:
            return "int";
        case ShaderDataFormat::Float:
            return "float";
        case ShaderDataFormat::Double:
            return "double";
        }
    case ShaderDataType::Vector2D:
        return MakeVulkanGLSLVectorDataType(shaderDataFormat, 2);
    case ShaderDataType::Vector3D:
        return MakeVulkanGLSLVectorDataType(shaderDataFormat, 3);
    case ShaderDataType::Vector4D:
        return MakeVulkanGLSLVectorDataType(shaderDataFormat, 4);
    case ShaderDataType::Matrix2x2:
        return MakeVulkanGLSLSquareMatrixDataType(shaderDataFormat, 2);
    case ShaderDataType::Matrix3x3:
        return MakeVulkanGLSLSquareMatrixDataType(shaderDataFormat, 3);
    case ShaderDataType::Matrix4x4:
        return MakeVulkanGLSLSquareMatrixDataType(shaderDataFormat, 4);
    case ShaderDataType::Matrix2x3:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 2, 3);
    case ShaderDataType::Matrix2x4:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 2, 4);
    case ShaderDataType::Matrix3x2:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 3, 2);
    case ShaderDataType::Matrix3x4:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 3, 4);
    case ShaderDataType::Matrix4x2:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 4, 2);
    case ShaderDataType::Matrix4x3:
        return MakeVulkanGLSLMatrixDataType(shaderDataFormat, 4, 3);
    case ShaderDataType::COUNT:
        throw std::runtime_error("COUNT is not a data type.");
    }
    
    return "";
}

std::string MakeVulkanGLSLDataType(Format format) {
    auto iter = con::FormatToShaderDataType.find(format);
    
    if (iter == con::FormatToShaderDataType.end()) {
        throw std::logic_error("This format cannot be used in shaders.");
    }
    
    ShaderDataFormat shaderDataFormat = iter->second.first;
    ShaderDataType shaderDataType = iter->second.second;
    
    return MakeVulkanGLSLDataType(shaderDataFormat, shaderDataType);
}

VulkanGLSLShaderGenerator::VulkanGLSLShaderGenerator(const Engine* engine) : ShaderGenerator(engine) {
    // TODO fill the compilerOptions, create an includer and fill it with reusable code, etc.
    compilerOptions.SetOptimizationLevel(shaderc_optimization_level_size);
    
    fileSystem = engine->getFileSystem();
}

std::string VulkanGLSLShaderGenerator::getVertexShaderExtension() const {
    return ".vert";
}

std::string VulkanGLSLShaderGenerator::getFragmentShaderExtension() const {
    return ".frag";
}

ShaderGenerationResultErrorPair VulkanGLSLShaderGenerator::generateVertexShaderImpl(const fs::path& path, const MaterialPipelineDefinition& definition, VertexDataLayout vertexDataLayout, bool normalMapped, bool compile) const {
    ShaderGenerationResultErrorPair validationResult = validateVertexShader(definition, vertexDataLayout);
    if (validationResult.first != ShaderGenerationResult::Success) {
        return validationResult;
    }
    
    std::size_t codeID = 0;
    for (std::size_t i = 0; i < definition.languages.size(); ++i) {
        if (definition.languages[i] == getShaderLanguage()) {
            codeID = i;
            break;
        }
    }
    
    // TODO BONE DATA
    const VertexDataLayoutDefinition& layout = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(vertexDataLayout)];
    
    bool regularNormalMapping = layout.hasAttribute(VertexAttributeType::Tangent);
    bool bitangentRecoveringNormalMapping = layout.hasAttribute(VertexAttributeType::TangentAndBias);
    
    if (normalMapped && !(regularNormalMapping || bitangentRecoveringNormalMapping)) {
        throw std::logic_error("Can't create a shader with normal mapping support for a vertex type that lacks appropriate data");
    }
    
    std::stringstream ss;
    ss << VulkanGLSLHeader;
    ss << "// IYFEngine. Automatically generated vertex shader for vertex layout called " << layout.getName() << ".";
    
    if (normalMapped) {
        ss << " With normal mapping.";
    }
    
    ss << "\n\n";
    
    ss << generatePerFrameData(definition.vertexShaderDataSets, definition);
    
    ss << "\n";
    
    const auto& attributes = layout.getAttributes();
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        const auto& attribute = attributes[i];
        std::string type = MakeVulkanGLSLDataType(attribute.format);
        ss << "layout(location = " << i << ") in " << type << " " << con::VertexShaderAttributeNames[static_cast<std::size_t>(attribute.type)] << ";\n";
    }
    
    ss << "\n// We need to redeclare outputs because of GL_ARB_separate_shader_objects\n";
    ss << "out gl_PerVertex {\n";
    ss << "    vec4 gl_Position;\n";
    ss << "};\n\n";
    
    ss << "// Per vertex output to other shaders in the pipeline\n";
    ss << "layout (location = 0) out VertexOutput {\n";
    
    if (definition.supportsMultipleLights) {
        ss << "    vec3 positionWS;\n";
    }
    
    if (definition.vertexColorDataRequired || definition.vertexColorDataRequiredGS) {
        ss << "    vec4 vertColor;\n";
    }
    
    if (definition.textureCoordinatesRequired) {
        ss << "    vec2 UV;\n";
    }
    
    if (normalMapped) {
        ss << "    mat3 TBN;\n";
    } else if (definition.normalDataRequired) {
        ss << "    vec3 normalWS;\n";
    }
    
    ss << "    uint instanceID;\n";
    
    if (definition.requiresAdditionalVertexProcessing) {
        for (const auto& o : definition.additionalVertexOutputs) {
            ss << "    " << MakeVulkanGLSLDataType(o.format, o.type) << " " << o.name << ";\n";
        }
    }
    
    ss << "} vertexOutput;\n\n";
    
    ss << "void main() {\n";
    
    ss << "    gl_Position = transformations.MVP[ID.transformation + gl_InstanceIndex] * vec4(" << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Position3D)] << ", 1.0f);\n\n";
    ss << "    // Needed for the retrieval of material data\n";
    ss << "    vertexOutput.instanceID = gl_InstanceIndex;\n\n";
    
    if (definition.supportsMultipleLights) {
        ss << "    // Used when computing world space lighting\n";
        ss << "    vertexOutput.positionWS = (transformations.M[ID.transformation + gl_InstanceIndex] * vec4(" << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Position3D)] << ", 1.0f)).xyz;\n\n";
    }
    
    if (normalMapped) {
        if (regularNormalMapping) {
            ss << "    // Creation of a TBN matrix for normal mapping\n";
            ss << "    vec3 normalWS    = normalize(mat3(transformations.M[ID.transformation + gl_InstanceIndex]) * " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Normal)] << ".xyz);\n";
            ss << "    vec3 tangentWS   = normalize(mat3(transformations.M[ID.transformation + gl_InstanceIndex]) * " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Tangent)] << ".xyz);\n";
            ss << "    vec3 bitangentWS = normalize(mat3(transformations.M[ID.transformation + gl_InstanceIndex]) * " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Bitangent)] << ".xyz);\n\n";
            
            ss << "    vertexOutput.TBN = mat3(tangentWS, bitangentWS, normalWS);\n\n";
        } else if (bitangentRecoveringNormalMapping) {
            // TODO implement this
            throw std::runtime_error("Whoops. Not implemented yet");
        } else {
            // This shouldn't happen
            assert(false);
        }
    } else if (definition.normalDataRequired) {
        ss << "    vertexOutput.normalWS = normalize(mat3(transformations.M[ID.transformation + gl_InstanceIndex]) * " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Normal)] << ".xyz);\n\n";
    }
    
    if (definition.textureCoordinatesRequired) {
        ss << "    // Output of UV coordinates for texture mapping\n";
        ss << "    vertexOutput.UV = " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::UV)] << ";\n\n";
    }
    
    if (definition.vertexColorDataRequired || definition.vertexColorDataRequiredGS) {
        ss << "    // Output of vertex color data\n";
        ss << "    vertexOutput.vertColor = " << con::VertexShaderAttributeNames[static_cast<std::size_t>(VertexAttributeType::Color)] << ";\n\n";
    }
    
    if (definition.requiresAdditionalVertexProcessing) {
        ss << "    //Additional vertex processing. This part uses code written by the author of the material\n";
        ss << definition.additionalVertexProcessingCode[codeID];
        ss << "\n";
    }
    
    ss << "}";
    
    std::string fileName = makeVertexShaderName(definition.name, layout.getName(), getVertexShaderExtension(), normalMapped);
    
    File fw((path / fileName).generic_string(), File::OpenMode::Write);
    
    const std::string shaderSource = ss.str();
    fw.writeString(shaderSource);
    
    if (compile) {
        return compileShader(definition, fileName, shaderSource, path / "spv", fileName + ".spv", ShaderStageFlagBits::Vertex);
    }
    
    return {ShaderGenerationResult::Success, ""};
}

std::string VulkanGLSLShaderGenerator::generatePerFrameData(const ShaderDataSets& requiredDataSets, const MaterialPipelineDefinition& definition) const {
    // First of all, the specialization constants
    std::stringstream ss;
    
    const std::vector<SpecializationConstant>& constants = renderer->getShaderSpecializationConstants();
    ss << "// Engine constants\n";
    
    std::size_t rendererConstantStart = (constants.size() == con::DefaultSpecializationConstants.size()) ? static_cast<std::size_t>(-1) : con::DefaultSpecializationConstants.size();
    for (std::size_t i = 0; i < constants.size(); ++i) {
        if (i == rendererConstantStart) {
            ss << "\n// Renderer constants\n";
        }
        const std::string type = MakeVulkanGLSLDataType(constants[i].format);
        
        std::string value;
        //std::int32_t, std::uint32_t, float, double
        switch (constants[i].value.index()) {
            case 0:
                value = std::to_string(std::get<std::int32_t>(constants[i].value));
                break;
            case 1:
                value = std::to_string(std::get<std::uint32_t>(constants[i].value));
                break;
            case 2:
                value = std::to_string(std::get<float>(constants[i].value));
                break;
            case 3:
                value = std::to_string(std::get<double>(constants[i].value));
                break;
        }
        ss << "layout (constant_id = " <<  i << ") const " << type << " " << constants[i].name << " = " << value << ";\n";
    }
    
    ss << "\nlayout(std140, push_constant) uniform IDPushConstants {\n";
    ss << "    uint transformation;\n";
    ss << "    uint material;\n";
    ss << "} ID;\n\n";
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSets::LightsAndCamera)]) {
        ss << "// Light data layouts in memory MUST match those in graphics/Lights.hpp.\n";
        ss << "struct PointLight {\n";
        ss << "    vec3 position;\n";
        ss << "    float radius;\n";
        ss << "    vec3 color;\n";
        ss << "    float intensity;\n";
        ss << "};\n\n";
        
        ss << "struct DirectionalLight {\n";
        ss << "    vec3 direction;\n";
        ss << "    float padding;\n";
        ss << "    vec3 color;\n";
        ss << "    float intensity;\n";
        ss << "};\n\n";
        
        ss << "struct SpotLight {\n";
        ss << "    vec3 position;\n";
        ss << "    float radius;\n";
        ss << "    vec3 direction;\n";
        ss << "    float angle;\n";
        ss << "    vec3 color;\n";
        ss << "    float intensity;\n";
        ss << "};\n\n";
        
        ss << "// Camera and light buffer data layouts in memory MUST match those in graphics/CameraAndLightBufferLayout.hpp.\n";
        ss << "layout(std" << "140" << ", set = " << con::CameraAndLightBuffer.set << ", binding = " << con::CameraAndLightBuffer.binding << ") buffer CameraAndLightBuffer {\n";
        ss << "    mat4 V;\n";
        ss << "    mat4 P;\n";
        ss << "    mat4 VP;\n";
        ss << "    vec3 cameraPosition;\n";
        ss << "    uint directionalLightCount;\n";
        ss << "    float zNear;\n";
        ss << "    float zFar;\n";
        ss << "    uvec2 framebufferDimensions;\n";
        ss << "    uint pointLightCount;\n";
        ss << "    uint spotLightCount;\n";
        ss << "    float padding1;\n";
        ss << "    float padding2;\n";
        ss << "    DirectionalLight directionalLights[" << con::MaxDirectionalLightsConstName << "];\n";
        ss << "    PointLight pointLights[" << con::MaxPointLightsConstName << "];\n";
        ss << "    SpotLight spotLights[" << con::MaxSpotLightsConstName << "];\n";
        ss << "} cameraAndLights; \n\n";
    }
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSets::ObjectTransformations)]) {
        ss << "layout(std" << "140" << ", set = " << con::TransformationDataBuffer.set << ", binding = " << con::TransformationDataBuffer.binding << ") buffer TransformationBuffer {\n";
        ss << "    mat4 M[" << con::MaxTransformationsConstName << "];\n";
        ss << "    mat4 MVP[" << con::MaxTransformationsConstName << "];\n";
        ss << "} transformations; \n\n";
    }
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSets::RendererData)]) {
        ss << renderer->makeRenderDataSet(getShaderLanguage()) << "\n";
    }
    
    const std::vector<MaterialComponent>& materialComponents = definition.getMaterialComponents();
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSets::MaterialData)] && materialComponents.size() != 0) {
        assert(con::MaxMaterialComponents % 4 == 0);
        const std::size_t dataVectorCount = con::MaxMaterialComponents / 4;
        
        ss << "// Material data\n";
        ss << "struct Material {\n";
        for (std::size_t i = 0; i < dataVectorCount; ++i) {
            ss << "    vec4 data" << i << ";\n";
        }
        ss << "};\n\n";
        
        ss << "layout(std" << "140" << ", set = " << con::MaterialDataBuffer.set << ", binding = " << con::MaterialDataBuffer.binding << ") buffer MaterialDataBuffer {\n";
        ss << "    Material materials[" << con::MaxMaterialsConstName << "];\n";
        ss << "} materials;\n\n";
    }
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSets::TextureData)] && materialComponents.size() != 0) {
        std::size_t offset = 0;
        
        if (definition.normalDataRequired) {
            ss << "layout(set = " << con::TextureDataBuffer.set << ", binding = " << (con::TextureDataBuffer.binding) << ") uniform sampler2D NormalTexture;\n";
            
            offset++;
        }
        
        for (std::size_t i = 0; i < materialComponents.size(); ++i) {
            ss << "layout(set = " << con::TextureDataBuffer.set << ", binding = " << (con::TextureDataBuffer.binding + i + offset) << ") uniform sampler2D " << materialComponents[i].name << "Texture;\n";
        }
    }
    
    ss << "\n";

    return ss.str();
}

ShaderGenerationResultErrorPair VulkanGLSLShaderGenerator::generateFragmentShaderImpl(const fs::path& path, const ComponentsReadFromTexture& readFromTexture, const MaterialPipelineDefinition& definition, bool normalMapped, bool compile) const {
    std::size_t codeID = 0;
    for (std::size_t i = 0; i < definition.languages.size(); ++i) {
        if (definition.languages[i] == getShaderLanguage()) {
            codeID = i;
            break;
        }
    }
    
    std::stringstream ss;
    ss << VulkanGLSLHeader;
    ss << "// IYFEngine. Automatically generated fragment shader for a material called " << definition.name << ".\n\n";
    
    ss << "// Per vertex output to other shaders in the pipeline\n";
    ss << "layout (location = 0) in FragmentInput {\n";
    
    if (definition.supportsMultipleLights) {
        ss << "    vec3 positionWS;\n";
    }
    
    if (definition.vertexColorDataRequired) {
        ss << "    vec4 vertColor;\n";
    }
    
    if (definition.textureCoordinatesRequired) {
        ss << "    vec2 UV;\n";
    }
    
    if (normalMapped) {
        ss << "    mat3 TBN;\n";
    } else if (definition.normalDataRequired) {
        ss << "    vec3 normalWS;\n";
    }
    
    ss << "    uint instanceID;\n";
    
    if (definition.requiresAdditionalVertexProcessing) {
        for (const auto& o : definition.additionalVertexOutputs) {
            ss << "    " << MakeVulkanGLSLDataType(o.format, o.type) << " " << o.name << ";\n";
        }
    }
    
    ss << "} fragmentInput;\n\n";
    
    ss << "layout (location = 0) out vec4 finalColor;\n\n";
    
    ss << generatePerFrameData(definition.fragmentShaderDataSets, definition);
    
    ss << generateLightProcessingFunctionSignature(definition) << "{\n";
    
    ss << definition.lightProcessingCode[codeID] << "\n";
    
    ss << "}\n\n";
    
    ss << "void main() {\n";
    
    ss << generateMaterialDataUnpacker(readFromTexture, definition);
    
    if (normalMapped) {
        ss << "    vec2 normalXY = texture(NormalTexture, fragmentInput.UV).xy;\n";
        ss << "    normalXY = 2.0f * normalXY - vec2(1.0f, 1.0f);\n";
        ss << "    float normalZ = sqrt(1 - (normalXY.x * normalXY.x) - (normalXY.y * normalXY.y));\n";
        ss << "    vec3 normal = normalize(vec3(normalXY, normalZ));\n";
        ss << "    normal = normalize(fragmentInput.TBN * normal);\n\n";
    } else if (definition.normalDataRequired) {
        ss << "    vec3 normal = fragmentInput.normalWS;\n\n";
    }
    
    if (definition.supportsMultipleLights) {
        ss << "    vec3 viewDirection = normalize(cameraAndLights.cameraPosition - fragmentInput.positionWS);\n\n";
    }
    
    ss << "    vec4 lightSum = vec4(0.0f, 0.0f, 0.0f, 0.0f);\n\n";
    
    std::string lightProcessingFunctionCall = generateLightProcessingFunctionCall(definition);
    
    if (definition.supportsMultipleLights) {
        ss << renderer->makeLightLoops(getShaderLanguage(), lightProcessingFunctionCall);
    } else {
        ss << "   " << lightProcessingFunctionCall << "\n\n";
    }
    
    ss << "    finalColor = lightSum;\n";
    ss << "}";
    
    std::string fileName = makeFragmentShaderName(definition.name, readFromTexture.to_ullong(), getFragmentShaderExtension(), normalMapped);
    
    File fw((path / fileName).generic_string(), File::OpenMode::Write);
    
    std::string shaderSource = ss.str();
    fw.writeString(shaderSource);
//     const BindingAndSet CameraAndLightBuffer(0, 0);
//     const BindingAndSet MaterialDataBuffer(0, 1);
//     const BindingAndSet TransformationDataBuffer(0, 2);
//     const std::size_t FirstAvailableDescriptorSet = 1;
    //layout (std140, set = 1, binding = 0) buffer lightAndCameraDataBuffer {
    
    if (compile) {
        return compileShader(definition, fileName, shaderSource, path / "spv", fileName + ".spv", ShaderStageFlagBits::Fragment);
    }
    
    return {ShaderGenerationResult::Success, ""};
}

std::string VulkanGLSLShaderGenerator::generateLightProcessingFunctionSignature(const MaterialPipelineDefinition& definition) const {
    std::vector<std::string> parameters;
    parameters.reserve(30);
    
    if (definition.normalDataRequired) {
        parameters.emplace_back("vec3 normal");
    }
    
    if (definition.supportsMultipleLights) {
        parameters.emplace_back("vec3 viewDirection");
        parameters.emplace_back("vec3 lightDirection");
        parameters.emplace_back("vec3 lightColor");
        parameters.emplace_back("float lightIntensity");
    }
    
    if (definition.vertexColorDataRequired) {
        parameters.emplace_back("vec4 vertColor");
    }
    
    for (const MaterialComponent& mc : definition.getMaterialComponents()) {
        std::string type;
        switch (mc.componentCount) {
        case 1:
            type = "float";
            break;
        case 2:
            type = "vec2";
            break;
        case 3:
            type = "vec3";
            break;
        case 4:
            type = "vec4";
            break;
        }
        
        parameters.emplace_back(type + " " + mc.name);
    }
    
    std::stringstream ss;
    
    ss << "vec4 " << definition.name << "(";
    
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i];
        
        if (i + 1 < parameters.size()) {
            ss << ", ";
        }
    }
    
    ss << ")";
    
    return ss.str();
}

std::string VulkanGLSLShaderGenerator::generateLightProcessingFunctionCall(const MaterialPipelineDefinition& definition) const {
    std::vector<std::string> parameters;
    parameters.reserve(30);
    
    if (definition.normalDataRequired) {
        parameters.emplace_back("normal");
    }
    
    if (definition.supportsMultipleLights) {
        parameters.emplace_back("viewDirection");
        
        parameters.emplace_back("lightDirection");
        parameters.emplace_back("lightColor");
        parameters.emplace_back("lightIntensity");
    }
    
    if (definition.vertexColorDataRequired) {
        parameters.emplace_back("fragmentInput.vertColor");
    }
    
    for (const MaterialComponent& mc : definition.getMaterialComponents()) {
        parameters.emplace_back(mc.name);
    }
    
    std::stringstream ss;
    
    ss << "lightSum += " << definition.name << "(";
    
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i];
        
        if (i + 1 < parameters.size()) {
            ss << ", ";
        }
    }
    
    ss << ");";
    
    return ss.str();
}

std::string VulkanGLSLShaderGenerator::generateMaterialDataUnpacker(const ComponentsReadFromTexture& readFromTexture, const MaterialPipelineDefinition& definition) const {
    if (definition.getMaterialComponents().size() == 0) {
        return "";
    }
    
    std::stringstream ss;
    
    for (std::size_t i = 0; i < definition.getMaterialComponents().size(); ++i) {
        const MaterialComponent& c = definition.getMaterialComponents()[i];
        
        if (readFromTexture[i]) {
            std::string typeName;
            std::string textureComponents;
            
            switch (c.componentCount) {
            case 1:
                typeName = "float";
                textureComponents = ".r";
                break;
            case 2:
                typeName = "vec2";
                textureComponents = ".rg";
                break;
            case 3:
                typeName = "vec3";
                textureComponents = ".rgb";
                break;
            case 4:
                typeName = "vec4";
                textureComponents = ".rgba";
                break;
            }
            
            if (c.isSigned) {
                ss << "    " << typeName << " " << c.name << " = " << "(texture(" << c.name << "Texture, fragmentInput.UV)" << textureComponents << " * 2) - " << typeName << "(1.0f)" << ";\n";
            } else { 
                ss << "    " << typeName << " " << c.name << " = " << "texture(" << c.name << "Texture, fragmentInput.UV)" << textureComponents << ";\n";
            }
        } else {
            std::size_t vector  = c.offset / 4;
            
            std::string typeName;
            std::string unpackedElement;
            std::size_t element = c.offset % 4;
            
            switch (c.componentCount) {
            case 1:
                typeName = "float";
                
                switch (element) {
                case 0:
                    unpackedElement = ".x";
                    break;
                case 1:
                    unpackedElement = ".y";
                    break;
                case 2:
                    unpackedElement = ".z";
                    break;
                case 3:
                    unpackedElement = ".w";
                    break;
                }
                break;
            case 2:
                assert(element == 0 || element == 2);
                
                typeName = "vec2";
                
                if (element == 0) {
                    unpackedElement = ".xy";
                } else if (element == 2) {
                    unpackedElement = ".zw";
                }
                break;
            case 3:
                assert(element == 0);
                
                typeName = "vec3";
                unpackedElement = ".xyz";
                break;
            case 4:
                assert(element == 0);
                
                typeName = "vec4";
                unpackedElement = ".xyzw";
                break;
            }
            
            ss << "    " << typeName << " " << c.name << " = " << "materials.materials[ID.material + fragmentInput.instanceID].data" << vector << unpackedElement << ";\n";
        }
        
        ss << "\n";
    }
    
    return ss.str();
}

ShaderGenerationResultErrorPair VulkanGLSLShaderGenerator::compileShader(const MaterialPipelineDefinition& definition, const std::string& shaderName, const std::string& shaderSource, const fs::path& savePath, const fs::path& fileName, ShaderStageFlagBits shaderStage) const {
    shaderc_shader_kind shaderKind;
    switch (shaderStage) {
        case ShaderStageFlagBits::Vertex:
            shaderKind = shaderc_vertex_shader;
            break;
        case ShaderStageFlagBits::TessControl:
            shaderKind = shaderc_tess_control_shader;
            break;
        case ShaderStageFlagBits::TessEvaluation:
            shaderKind = shaderc_tess_evaluation_shader;
            break;
        case ShaderStageFlagBits::Geometry:
            shaderKind = shaderc_geometry_shader;
            break;
        case ShaderStageFlagBits::Fragment:
            shaderKind = shaderc_fragment_shader;
            break;
        case ShaderStageFlagBits::Compute:
            shaderKind = shaderc_vertex_shader;
            break;
        default:
            throw std::invalid_argument("An unknown shader stage has been specified.");
    }
    
    if (definition.logAssembly) {
        shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(shaderSource, shaderKind, shaderName.c_str(), compilerOptions);
        
        if (result.GetCompilationStatus() == shaderc_compilation_status_success) {
            LOG_V("Assembly for shader " << shaderName << "\n\n" << std::string(result.begin(), result.end()));
        }
    }
    
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(shaderSource, shaderKind, shaderName.c_str(), compilerOptions);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        return generateAndReportError(ShaderGenerationResult::CompilationFailed, fmt::format("Shader \"{}\" compilation failed with error {}", shaderName, result.GetErrorMessage()));
    } else {
        LOG_V("Successfully compiled a shader called \"" << shaderName << "\"\n\tWarnings: " << result.GetNumWarnings());
        // TODO output warnings if any
        
        std::vector<std::uint32_t> content(result.begin(), result.end());
        
        fileSystem->createDirectory(savePath);
        
        File fw(savePath / fileName, File::OpenMode::Write);
        fw.writeBytes(content.data(), content.size() * sizeof(std::uint32_t));
        
        return {ShaderGenerationResult::Success, ""};
    }
}
}
