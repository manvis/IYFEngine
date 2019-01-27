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

#include "graphics/shaderGeneration/VulkanGLSLShaderGenerator.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/materials/MaterialLogicGraph.hpp"
#include "core/Engine.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "fmt/format.h"
#include "core/Logger.hpp"

#include <stdexcept>

namespace iyf {
struct PerFrameExtraData {
    const MaterialLogicGraph* graph;
    const CodeGenerationResult* code;
};

inline std::string GetShaderStageName(ShaderStageFlagBits stage) {
    switch (stage) {
        case ShaderStageFlagBits::Vertex:
            return "vertex";
        case ShaderStageFlagBits::Geometry:
            return "geometry";
        case ShaderStageFlagBits::TessControl:
            return "tessellation control";
        case ShaderStageFlagBits::TessEvaluation:
            return "tessellation evaluation";
        case ShaderStageFlagBits::Fragment:
            return "fragment";
        case ShaderStageFlagBits::Compute:
            return "compute";
        case ShaderStageFlagBits::COUNT:
            throw std::runtime_error("COUNT is not a valid shader stage.");
    }
    
    throw std::logic_error("Invalid ShaderStageFlagBit");
}

const std::string VulkanGLSLInitialization = 
    "#version 450\n\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
//     "#extension GL_GOOGLE_include_directive : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n\n";

inline std::string MakeVulkanGLSLHeader(ShaderStageFlagBits stage) {
    std::stringstream ss;
    
    ss << VulkanGLSLInitialization;
    
    ss << "// IYFEngine. Automatically generated " << GetShaderStageName(stage) << " shader.\n";
    ss << "// WARNING: DO NOT MODIFY BY HAND. Update the material definition and regenerate it instead.\n\n";
    
    return ss.str();
}

inline std::string WriteMacroDefinitions(PlatformIdentifier platform, const MaterialFamilyDefinition& definition) {
    const PlatformInfo& platforInfo = con::PlatformIdentifierToInfo(platform);
    
    std::stringstream ss;
    ss << "#define " << GetShaderMacroName(ShaderMacro::NormalTextureChannelCount) << " " << platforInfo.normalTextureChannelCount << "\n";
    
    if (definition.isWorldSpacePositionRequired()) {
        ss << "#define " << GetShaderMacroName(ShaderMacro::WorldSpacePositionAvailable) << "\n";
    }
    
    ss << "\n";
    return "// Macros that depend on the MaterialFamilyDefinition go here\n" + ss.str();
}

shaderc_shader_kind ShaderStageToKind(ShaderStageFlagBits shaderStage) {
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
    
    return shaderKind;
}

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
        default:
            throw std::runtime_error("Unknown ShaderDataFormat");
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

VulkanGLSLShaderGenerator::VulkanGLSLShaderGenerator(const FileSystem* fileSystem) : ShaderGenerator(fileSystem) {}

VulkanGLSLShaderGenerator::~VulkanGLSLShaderGenerator() {}

fs::path VulkanGLSLShaderGenerator::getShaderStageFileExtension(ShaderStageFlagBits stage) const {
    switch (stage) {
        case ShaderStageFlagBits::Vertex:
            return ".vert.glsl";
        case ShaderStageFlagBits::Geometry:
            return ".geom.glsl";
        case ShaderStageFlagBits::TessControl:
            return ".tesc.glsl";
        case ShaderStageFlagBits::TessEvaluation:
            return ".tese.glsl";
        case ShaderStageFlagBits::Fragment:
            return ".frag.glsl";
        case ShaderStageFlagBits::Compute:
            return ".comp.glsl";
        case ShaderStageFlagBits::COUNT:
            throw std::invalid_argument("COUNT is not a valid shader stage.");
    }
    
    throw std::logic_error("Invalid ShaderStageFlagBit");
}

ShaderGenerationResult VulkanGLSLShaderGenerator::generateVertexShader(PlatformIdentifier platform, const MaterialFamilyDefinition& definition) const {
    std::size_t codeID = 0;
    for (std::size_t i = 0; i < definition.getSupportedLanguages().size(); ++i) {
        if (definition.getSupportedLanguages()[i] == getShaderLanguage()) {
            codeID = i;
            break;
        }
    }
    
    std::stringstream ss;
    ss << MakeVulkanGLSLHeader(ShaderStageFlagBits::Vertex);
    ss << WriteMacroDefinitions(platform, definition);
    
    ss << generatePerFrameData(definition.getVertexShaderDataSets(), nullptr);
    
    ss << "\n";
    
    std::size_t compatibleVertexLayoutCount = 0;
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(VertexDataLayout::COUNT); ++i) {
        const VertexDataLayoutDefinition& layout = con::GetVertexDataLayoutDefinition(static_cast<VertexDataLayout>(i));
        
        ShaderGenerationResult validationResult = checkVertexDataLayoutCompatibility(definition, static_cast<VertexDataLayout>(i));
        
        ss << "#if " << GetShaderMacroName(ShaderMacro::VertexDataLayout) << " == " << i << " // " << layout.getName() << "\n";
        if (validationResult.getStatus() != ShaderGenerationResult::Status::Success) {
            ss << "#error \"This shader is incompatible with this VertexDataLayout\"\n";
        } else {
            const auto& attributes = layout.getAttributes();
            for (std::size_t j = 0; j < attributes.size(); ++j) {
                const auto& attribute = attributes[j];
                std::string type = MakeVulkanGLSLDataType(attribute.format);
                ss << "layout(location = " << j << ") in " << type << " " << con::GetVertexAttributeName(attribute.type) << ";\n";
            }
            // DO NOT DEFINE ShaderMacro::NormalMappingMode HERE. It may seem like a good idea, but it's not.
            // It HAS to be set via a macro when compiling because it HAS to be available in all shader stages.
        }
        ss << "#endif\n\n";
    
        compatibleVertexLayoutCount++;
    }
    
    LOG_V("Number of compatible vertex layouts " << compatibleVertexLayoutCount);
    
    ss << "\n// We need to redeclare outputs because of GL_ARB_separate_shader_objects\n";
    ss << "out gl_PerVertex {\n";
    ss << "    vec4 gl_Position;\n";
    ss << "};\n\n";
    
    ss << "// Per vertex output to other shaders\n";
    ss << "layout (location = 0) out VertexOutput {\n";
    
    if (definition.isWorldSpacePositionRequired()) {
        ss << "    vec3 positionWS;\n";
    }
    
    if (definition.isVertexColorDataRequired()) {
        ss << "    vec4 vertColor;\n";
    }
    
    if (definition.areTextureCoordinatesRequired()) {
        ss << "    vec2 UV;\n";
    }
    
    if (definition.isNormalDataRequired()) {
        ss << "#if (" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0)\n";
        ss << "    mat3 TBN;\n";
        ss << "#else // If normal data is required, but we don't do normal mapping, it is passed separately\n";
        ss << "    vec3 normalWS;\n";
        ss << "#endif\n";
    }
    
    ss << "    uint instanceID;\n";
    
    if (!definition.getAdditionalVertexShaderOutputs().empty()) {
        for (const auto& o : definition.getAdditionalVertexShaderOutputs()) {
            ss << "    " << MakeVulkanGLSLDataType(o.format, o.type) << " " << o.getName() << ";\n";
        }
    }
    
    ss << "} vertexOutput;\n\n";
    
    ss << "#include \"" << GetShaderIncludeName(ShaderInclude::VertexShaderHelpers) << "\"\n\n";
    
    ss << "void main() {\n";
    // TODO HANDLE BONE DATA
    ss << "    gl_Position = MVP() * vec4(" << con::GetVertexAttributeName(VertexAttributeType::Position3D) << ", 1.0f);\n\n";
    ss << "    // Needed for the retrieval of material data\n";
    ss << "    vertexOutput.instanceID = gl_InstanceIndex;\n\n";
    
    if (definition.isWorldSpacePositionRequired()) {
        ss << "    // Used when computing world space lighting\n";
        ss << "    vertexOutput.positionWS = (M() * vec4(" << con::GetVertexAttributeName(VertexAttributeType::Position3D) << ", 1.0f)).xyz;\n\n";
    }
    
    if (definition.isNormalDataRequired()) {
        ss << "#if (" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0)\n";
        ss << "    vertexOutput.TBN = TBN();\n";
        ss << "#else\n";
        ss << "    vertexOutput.normalWS = normalize(mat3(M()) * " << con::GetVertexAttributeName(VertexAttributeType::Normal) << ".xyz);\n";
        ss << "#endif\n\n";
    }
    
    if (definition.areTextureCoordinatesRequired()) {
        ss << "    // Output of UV coordinates for texture mapping\n";
        ss << "    vertexOutput.UV = " << con::GetVertexAttributeName(VertexAttributeType::UV) << ";\n\n";
    }
    
    if (definition.isVertexColorDataRequired()) {
        ss << "    // Output of vertex color data\n";
        ss << "    vertexOutput.vertColor = " << con::GetVertexAttributeName(VertexAttributeType::Color) << ";\n\n";
    }
    
    // TODO this should probably happen earlier
    const std::string& vertProcessingCode = definition.getAdditionalVertexProcessingCode()[codeID];
    if (!vertProcessingCode.empty()) {
        ss << "    // Additional vertex processing. This part uses code (if any) written by the author of the material family definition\n";
        ss << vertProcessingCode << "\n";
    }
    
// Not used
//     ss << "    // Material specific code (if any) that gets pulled in when compiling the vertex shader\n";
//     ss << "    #include \"vertex.inl\"\n";
    
    ss << "}";
    
//     LOG_V("\n" << ss.str());
    return ShaderGenerationResult(ShaderGenerationResult::Status::Success, ss.str());
}

inline std::string formatVectorDataForOutput(const std::string& dataTypeName, const glm::vec4& vec, std::size_t componentCount) {
    std::stringstream ss;
    
    if (componentCount > 1) {
        ss << dataTypeName << "(";
    }
    
    ss.precision(6);
    
    for (std::size_t i = 0; i < componentCount; ++i) {
        ss << std::fixed << vec[i] << "f";
        
        if (i + 1 != componentCount) {
            ss << ", ";
        }
    }
    
    if (componentCount > 1) {
        ss << ")";
    }
    
    return ss.str();
}

ShaderGenerationResult VulkanGLSLShaderGenerator::generateFragmentShader(PlatformIdentifier platform, const MaterialFamilyDefinition& definition, const MaterialLogicGraph* graph) const {
    assert(graph != nullptr);
    
    validateFamilyDefinition(definition);
    
    const CodeGenerationResult codeGenResult = graph->toCode(getShaderLanguage());
    if (!codeGenResult.isSuccessful()) {
        return ShaderGenerationResult(ShaderGenerationResult::Status::InvalidMaterialLogicGraph, codeGenResult.getCode());
    }
    
    std::size_t codeID = 0;
    for (std::size_t i = 0; i < definition.getSupportedLanguages().size(); ++i) {
        if (definition.getSupportedLanguages()[i] == getShaderLanguage()) {
            codeID = i;
            break;
        }
    }
    
    std::stringstream ss;
    ss << MakeVulkanGLSLHeader(ShaderStageFlagBits::Fragment);
    ss << WriteMacroDefinitions(platform, definition);
    
    ss << "// Data from the previous shader stages\n";
    ss << "layout (location = 0) in FragmentInput {\n";
    
    if (definition.isWorldSpacePositionRequired()) {
        ss << "    vec3 positionWS;\n";
    }
    
    if (definition.isVertexColorDataRequired()) {
        ss << "    vec4 vertColor;\n";
    }
    
    if (definition.areTextureCoordinatesRequired()) {
        ss << "    vec2 UV;\n";
    }
    
    if (definition.isNormalDataRequired()) {
        ss << "#if (" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0)\n";
        ss << "    mat3 TBN;\n";
        ss << "#else // If normal data is required, but we don't do normal mapping, it is passed separately\n";
        ss << "    vec3 normalWS;\n";
        ss << "#endif\n";
    }
    
    ss << "    uint instanceID;\n";
    
    if (!definition.getAdditionalVertexShaderOutputs().empty()) {
        for (const auto& o : definition.getAdditionalVertexShaderOutputs()) {
            ss << "    " << MakeVulkanGLSLDataType(o.format, o.type) << " " << o.getName() << ";\n";
        }
    }
    
    ss << "} fragmentInput;\n\n";
    
    ss << "layout (location = 0) out vec4 finalColor;\n\n";

    PerFrameExtraData extraData;
    extraData.graph = graph;
    extraData.code = &codeGenResult;
    
    ss << generatePerFrameData(definition.getFragmentShaderDataSets(), &extraData);
    
    ss << generateLightProcessingFunctionSignature(definition) << "{\n";
    
    ss << definition.getLightProcessingCode()[codeID] << "\n";
    
    ss << "}\n\n";
    
    ss << "#include \"" << GetShaderIncludeName(ShaderInclude::FragmentShaderHelpers) << "\"\n\n";
    
    ss << "void main() {\n";
    
    //ss << generateMaterialDataUnpacker(settings.readFromTexture, definition);
    
    for (const LightProcessingFunctionInput& lpfi : definition.getLightProcessingFunctionInputs()) {
        ss << "    ";
        
        assert(lpfi.getVariableDefinition().format == ShaderDataFormat::Float);
        
        std::string dataTypeName;
        int componentCount = 0;
        
        switch (lpfi.getVariableDefinition().type) {
            case ShaderDataType::Scalar:
                dataTypeName = "float";
                componentCount = 1;
                break;
            case ShaderDataType::Vector2D:
                dataTypeName = "vec2";
                componentCount = 2;
                break;
            case ShaderDataType::Vector3D:
                dataTypeName = "vec3";
                componentCount = 3;
                break;
            case ShaderDataType::Vector4D:
                dataTypeName = "vec4";
                componentCount = 4;
                break;
            default:
                throw std::runtime_error("Shader data type not supported");
        }
        
        ss << dataTypeName << " " << lpfi.getVariableDefinition().getName() << " = " << formatVectorDataForOutput(dataTypeName, lpfi.defaultValue, componentCount) << ";\n";
    }
    
    ss << "\n";
    
    ss << codeGenResult.getCode();
    
    if (definition.isNormalDataRequired()) {
        ss << "    // World space normal\n";
        ss << "#ifndef " << GetShaderMacroName(ShaderMacro::NormalSetByMaterialGraph) << "\n";
        ss << "#if (" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0)\n";
        ss << "    vec3 normal = normalize(fragmentInput.TBN[2]);\n";
        ss << "#else //" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << "\n";
        ss << "    vec3 normal = normalize(fragmentInput.normalWS);\n";
        ss << "#endif //" << GetShaderMacroName(ShaderMacro::NormalMappingMode) << "\n";
        ss << "#endif //"<< GetShaderMacroName(ShaderMacro::NormalSetByMaterialGraph) << "\n";
    }
    
    if (definition.isWorldSpacePositionRequired()) {
        ss << "    vec3 viewDirection = normalize(cameraAndLights.cameraPosition - fragmentInput.positionWS);\n\n";
    }
    
    ss << "    vec4 lightSum = vec4(0.0f, 0.0f, 0.0f, 0.0f);\n\n";
    
    std::string lightProcessingFunctionCall = generateLightProcessingFunctionCall(definition);
    
    if (definition.areLightsSupported()) {
        for (const RendererType rendererType : Renderer::GetAvailableRenderers()) {
            ss << "#if " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n";
            
            const RendererProperties& rendererProperties = Renderer::GetRendererProperties(rendererType);
            ss << rendererProperties.makeLightLoops(getShaderLanguage(), lightProcessingFunctionCall);
            
            ss << "#endif // " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n\n";
        }
    } else {
        ss << "   " << lightProcessingFunctionCall << "\n\n";
    }
    
    ss << "    finalColor = lightSum;\n";
    ss << "}";
    
    return ShaderGenerationResult(ShaderGenerationResult::Status::Success, ss.str());
}

std::string VulkanGLSLShaderGenerator::generatePerFrameData(const ShaderDataSets& requiredDataSets, const void* extraData) const {
    // First of all, the specialization constants
    std::stringstream ss;
    
    for (const RendererType rendererType : Renderer::GetAvailableRenderers()) {
        ss << "#if " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n";
        
        const RendererProperties& rendererProperties = Renderer::GetRendererProperties(rendererType);
        const std::vector<SpecializationConstant>& constants = rendererProperties.getShaderSpecializationConstants();
        ss << "// Engine constants\n";
        
        std::size_t rendererConstantStart = (constants.size() == con::DefaultSpecializationConstants.size()) ? static_cast<std::size_t>(-1) : con::DefaultSpecializationConstants.size();
        for (std::size_t i = 0; i < constants.size(); ++i) {
            if (i == rendererConstantStart) {
                ss << "\n// Renderer constants\n";
            }
            const std::string type = MakeVulkanGLSLDataType(constants[i].format);
            
            std::string value;
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
        
        ss << "#endif // " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n\n";
    }
    
    ss << "#ifndef " << GetShaderMacroName(ShaderMacro::Renderer) << "\n";
    ss << "#error \"No valid renderer has been set\"\n";
    ss << "#endif // " << GetShaderMacroName(ShaderMacro::Renderer) << "\n";
    
    ss << "\nlayout(std140, push_constant) uniform IDPushConstants {\n";
    ss << "    uint transformation;\n";
    ss << "    uint material;\n";
    ss << "} ID;\n\n";
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSet::LightsAndCamera)]) {
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
        ss << "    float fieldOfView;\n";
        ss << "    float time;\n";
        ss << "    DirectionalLight directionalLights[" << con::MaxDirectionalLightsConstName << "];\n";
        ss << "    PointLight pointLights[" << con::MaxPointLightsConstName << "];\n";
        ss << "    SpotLight spotLights[" << con::MaxSpotLightsConstName << "];\n";
        ss << "} cameraAndLights; \n\n";
    }
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSet::ObjectTransformations)]) {
        ss << "layout(std" << "140" << ", set = " << con::TransformationDataBuffer.set << ", binding = " << con::TransformationDataBuffer.binding << ") buffer TransformationBuffer {\n";
        ss << "    mat4 M[" << con::MaxTransformationsConstName << "];\n";
        ss << "    mat4 MVP[" << con::MaxTransformationsConstName << "];\n";
        ss << "} transformations; \n\n";
    }
    
    for (const RendererType rendererType : Renderer::GetAvailableRenderers()) {
        ss << "#if " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n";
        
        if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSet::RendererData)]) {
            const RendererProperties& rendererProperties = Renderer::GetRendererProperties(rendererType);
            ss << rendererProperties.makeRenderDataSet(getShaderLanguage()) << "\n";
        }
        
        ss << "#endif // " << GetShaderMacroName(ShaderMacro::Renderer) << " == " << static_cast<std::int64_t>(rendererType) << "\n\n";
    }
    
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSet::MaterialData)]) {
        assert(extraData != nullptr);
        
        const PerFrameExtraData* data = static_cast<const PerFrameExtraData*>(extraData);
        
        ss << "// Material data\n";
        ss << data->code->getMaterialStructCode();
        ss << "\n";
        
        ss << "layout(std" << "140" << ", set = " << con::MaterialDataBuffer.set << ", binding = " << con::MaterialDataBuffer.binding << ") buffer MaterialDataBuffer {\n";
        ss << "    Material materials[" << con::MaxMaterialsConstName << "];\n";
        ss << "} materials;\n\n";
    }
    
//     PASS WITHOUT REGENERATING CODE
    if (requiredDataSets[static_cast<std::size_t>(PerFrameDataSet::TextureData)]) {
        assert(extraData != nullptr);
        
        const PerFrameExtraData* data = static_cast<const PerFrameExtraData*>(extraData);
        const std::vector<const TextureInputNode*> textureNodes = data->graph->getTextureNodes();
        
        for (std::size_t i = 0; i < textureNodes.size(); ++i) {
            const TextureInputNode* node = textureNodes[i];
            assert(node->hasName());
            
            ss << "layout(set = " << con::TextureDataBuffer.set << ", binding = " << (con::TextureDataBuffer.binding + i) << ") uniform sampler2D " << node->getName() << ";";
            ss << " // Key: " << node->getKey() << "\n";
        }
    }
    
    ss << "\n";

    return ss.str();
}

std::string VulkanGLSLShaderGenerator::generateLightProcessingFunctionSignature(const MaterialFamilyDefinition& definition) const {
    std::vector<std::string> parameters;
    parameters.reserve(30);
    
    if (definition.isNormalDataRequired()) {
        parameters.emplace_back("vec3 normal");
    }
    
    if (definition.isWorldSpacePositionRequired()) {
        parameters.emplace_back("vec3 viewDirection");
    }
    
    if (definition.areLightsSupported()) {
        parameters.emplace_back("vec3 lightDirection");
        parameters.emplace_back("vec3 lightColor");
        parameters.emplace_back("float lightIntensity");
    }
    
    if (definition.isVertexColorDataRequired()) {
        parameters.emplace_back("vec4 vertColor");
    }
    
    for (const LightProcessingFunctionInput& lpfi : definition.getLightProcessingFunctionInputs()) {
        assert(lpfi.getVariableDefinition().format == ShaderDataFormat::Float);
        
        std::string type;
        switch (lpfi.getVariableDefinition().type) {
        case ShaderDataType::Scalar:
            type = "float";
            break;
        case ShaderDataType::Vector2D:
            type = "vec2";
            break;
        case ShaderDataType::Vector3D:
            type = "vec3";
            break;
        case ShaderDataType::Vector4D:
            type = "vec4";
            break;
        default:
            throw std::runtime_error("Unsupported ShaderDataType");
        }
        
        parameters.emplace_back(type + " " + lpfi.getVariableDefinition().getName());
    }
    
    std::stringstream ss;
    
    ss << "vec4 " << definition.getName() << "(";
    
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i];
        
        if (i + 1 < parameters.size()) {
            ss << ", ";
        }
    }
    
    ss << ") ";
    
    return ss.str();
}

std::string VulkanGLSLShaderGenerator::generateLightProcessingFunctionCall(const MaterialFamilyDefinition& definition) const {
    std::vector<std::string> parameters;
    parameters.reserve(30);
    
    if (definition.isNormalDataRequired()) {
        parameters.emplace_back("normal");
    }
    
    if (definition.isWorldSpacePositionRequired()) {
        parameters.emplace_back("viewDirection");
    }
    
    if (definition.areLightsSupported()) {
        parameters.emplace_back("lightDirection");
        parameters.emplace_back("lightColor");
        parameters.emplace_back("lightIntensity");
    }
    
    if (definition.isVertexColorDataRequired()) {
        parameters.emplace_back("fragmentInput.vertColor");
    }
    
    for (const LightProcessingFunctionInput& lpfi : definition.getLightProcessingFunctionInputs()) {
        parameters.emplace_back(lpfi.getVariableDefinition().getName());
    }
    
    std::stringstream ss;
    
    ss << "lightSum += " << definition.getName() << "(";
    
    for (std::size_t i = 0; i < parameters.size(); ++i) {
        ss << parameters[i];
        
        if (i + 1 < parameters.size()) {
            ss << ", ";
        }
    }
    
    ss << ");";
    
    return ss.str();
}

// std::string VulkanGLSLShaderGenerator::generateMaterialDataUnpacker(const ComponentsReadFromTexture& readFromTexture, const MaterialFamilyDefinitionMaterialFamilyDefinition& definition) const {
//     if (definition.getMaterialComponents().size() == 0) {
//         return "";
//     }
//     
//     std::stringstream ss;
//     
//     for (std::size_t i = 0; i < definition.getMaterialComponents().size(); ++i) {
//         const MaterialComponent& c = definition.getMaterialComponents()[i];
//         
//         if (readFromTexture[i]) {
//             std::string typeName;
//             std::string textureComponents;
//             
//             switch (c.componentCount) {
//             case 1:
//                 typeName = "float";
//                 textureComponents = ".r";
//                 break;
//             case 2:
//                 typeName = "vec2";
//                 textureComponents = ".rg";
//                 break;
//             case 3:
//                 typeName = "vec3";
//                 textureComponents = ".rgb";
//                 break;
//             case 4:
//                 typeName = "vec4";
//                 textureComponents = ".rgba";
//                 break;
//             }
//             
//             if (c.isSigned) {
//                 ss << "    " << typeName << " " << c.name << " = " << "(texture(" << c.name << "Texture, fragmentInput.UV)" << textureComponents << " * 2) - " << typeName << "(1.0f)" << ";\n";
//             } else { 
//                 ss << "    " << typeName << " " << c.name << " = " << "texture(" << c.name << "Texture, fragmentInput.UV)" << textureComponents << ";\n";
//             }
//         } else {
//             std::size_t vector  = c.offset / 4;
//             
//             std::string typeName;
//             std::string unpackedElement;
//             std::size_t element = c.offset % 4;
//             
//             switch (c.componentCount) {
//             case 1:
//                 typeName = "float";
//                 
//                 switch (element) {
//                 case 0:
//                     unpackedElement = ".x";
//                     break;
//                 case 1:
//                     unpackedElement = ".y";
//                     break;
//                 case 2:
//                     unpackedElement = ".z";
//                     break;
//                 case 3:
//                     unpackedElement = ".w";
//                     break;
//                 }
//                 break;
//             case 2:
//                 assert(element == 0 || element == 2);
//                 
//                 typeName = "vec2";
//                 
//                 if (element == 0) {
//                     unpackedElement = ".xy";
//                 } else if (element == 2) {
//                     unpackedElement = ".zw";
//                 }
//                 break;
//             case 3:
//                 assert(element == 0);
//                 
//                 typeName = "vec3";
//                 unpackedElement = ".xyz";
//                 break;
//             case 4:
//                 assert(element == 0);
//                 
//                 typeName = "vec4";
//                 unpackedElement = ".xyzw";
//                 break;
//             }
//             
//             ss << "    " << typeName << " " << c.name << " = " << "materials.materials[ID.material + fragmentInput.instanceID].data" << vector << unpackedElement << ";\n";
//         }
//         
//         ss << "\n";
//     }
//     
//     return ss.str();
// }

ShaderCompilationResult VulkanGLSLShaderGenerator::compileShader(ShaderStageFlagBits stage, const std::string& source, const std::string& name, const ShaderCompilationSettings& settings) const {
    const shaderc_shader_kind shaderKind = ShaderStageToKind(stage);
    
    shaderc::CompileOptions compileOptions;
    
    shaderc_optimization_level optimizationLevel;
    switch (settings.optimizationLevel) {
        case ShaderOptimizationLevel::NoOptimization:
            optimizationLevel = shaderc_optimization_level_zero;
            break;
        case ShaderOptimizationLevel::Size:
            optimizationLevel = shaderc_optimization_level_size;
            break;
        case ShaderOptimizationLevel::Performance:
            optimizationLevel = shaderc_optimization_level_performance;
            break;
        default:
            throw std::runtime_error("Unknown optimization level");
    }
    
    compileOptions.SetOptimizationLevel(optimizationLevel);
    
    const VertexDataLayout layout = settings.vertexDataLayout;
    const VertexDataLayoutDefinition& layoutDefinition = con::GetVertexDataLayoutDefinition(layout);
    
    std::vector<ShaderMacroWithValue> macros = settings.macros;
    macros.reserve(macros.size() + 10);
    
    macros.emplace_back(ShaderMacro::VertexDataLayout, static_cast<std::int64_t>(layout));
    macros.emplace_back(ShaderMacro::NormalMappingMode, static_cast<std::int64_t>(layoutDefinition.getSupportedNormalMappingMode()));
    
    if (layoutDefinition.hasAttribute(VertexAttributeType::Normal)) {
        macros.emplace_back(ShaderMacro::NormalAvailable);
    }
    
    if (layoutDefinition.hasAttribute(VertexAttributeType::Color)) {
        macros.emplace_back(ShaderMacro::VertexColorAvailable);
    }
    
    if (layoutDefinition.hasAttribute(VertexAttributeType::UV)) {
        macros.emplace_back(ShaderMacro::TextureCoordinatesAvailable);
    }
    
    for (const ShaderMacroWithValue& m : macros) {
        const std::string stringifiedValue = m.getStringifiedValue();
        std::string_view name = m.getName();
        
        if (stringifiedValue.empty()) {
            compileOptions.AddMacroDefinition(name.data(), name.length(), nullptr, 0);
        } else {
            compileOptions.AddMacroDefinition(name.data(), name.length(), stringifiedValue.data(), stringifiedValue.length());
        }
    }
    
    compileOptions.SetIncluder(std::make_unique<VulkanGLSLIncluder>());
    
    if (settings.logAssembly) {
        shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(source, shaderKind, name.c_str(), compileOptions);
        
        if (result.GetCompilationStatus() == shaderc_compilation_status_success) {
            LOG_V("Assembly for shader " << name << "\n\n" << std::string(result.begin(), result.end()));
        }
    }
    
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(source, shaderKind, name.c_str(), compileOptions);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        return ShaderCompilationResult(ShaderCompilationResult::Status::CompilationFailed,
                                       fmt::format("Shader \"{}\" compilation failed with error(s):\n {}", name, result.GetErrorMessage()),
                                       {});
    } else {
        std::vector<std::uint32_t> content(result.begin(), result.end());
        const std::size_t sizeInBytes = content.size() * sizeof(std::uint32_t);
        
        std::vector<std::uint8_t> byteResult(sizeInBytes, 0);
        assert(byteResult.size() == sizeInBytes);
        
        // Convert to bytes
        std::memcpy(byteResult.data(), content.data(), sizeInBytes);
        
        return ShaderCompilationResult(ShaderCompilationResult::Status::Success, (result.GetNumWarnings() != 0) ? result.GetErrorMessage() : "", byteResult);
    }
}

const std::string VulkanGLSLIncluder::EmptyString = "";
const std::string VulkanGLSLIncluder::UnknownNameError = "Unknown include";

shaderc_include_result* VulkanGLSLIncluder::GetInclude(const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth) {
    shaderc_include_result* includeResult = new shaderc_include_result;
    
    if (debugIncludes) {
        LOG_V("REQUESTED_SRC: " << requested_source << "; TYPE: " << type << "; REQUESTING_SRC:" << requesting_source << "; DEPTH: " << include_depth);
    }
    
//     LOG_V(VertexShaderHelperFunctions);
//     LOG_V(FragmentShaderHelperFunctions);
    
    // TODO start using an unordered_map if the number of includes grows even more
    if (std::strncmp(requested_source, GetShaderIncludeName(ShaderInclude::CommonHelpers), 64) == 0) {
        const char* name = "CommonHelpers";
        const std::size_t nameLength = strlen(name);
        
        includeResult->source_name = name;
        includeResult->source_name_length = nameLength;
        
        includeResult->content = CommonHelperFunctions.data();
        includeResult->content_length = CommonHelperFunctions.size();
    } else if (std::strncmp(requested_source, GetShaderIncludeName(ShaderInclude::VertexShaderHelpers), 64) == 0) {
        const char* name = "VertexShaderHelpers";
        const std::size_t nameLength = strlen(name);
        
        includeResult->source_name = name;
        includeResult->source_name_length = nameLength;
        
        includeResult->content = VertexShaderHelperFunctions.data();
        includeResult->content_length = VertexShaderHelperFunctions.size();
    } else if (std::strncmp(requested_source, GetShaderIncludeName(ShaderInclude::FragmentShaderHelpers), 64) == 0) {
        const char* name = "FragmentShaderHelpers";
        const std::size_t nameLength = strlen(name);
        
        includeResult->source_name = name;
        includeResult->source_name_length = nameLength;
        
        includeResult->content = FragmentShaderHelperFunctions.data();
        includeResult->content_length = FragmentShaderHelperFunctions.size();
    } else {
        includeResult->source_name = EmptyString.data();
        includeResult->source_name_length = EmptyString.size();
        includeResult->content = UnknownNameError.data();
        includeResult->content_length = UnknownNameError.size();
    }
    includeResult->user_data = nullptr;
    
    return includeResult;
}

void VulkanGLSLIncluder::ReleaseInclude(shaderc_include_result* data) {
    delete data;
}

inline std::string MakeVertexShaderHelperFunctionInclude() {
    std::stringstream ss;
    ss << "#include \"" << GetShaderIncludeName(ShaderInclude::CommonHelpers) << "\"\n";
    
    ss << "// Returns an MVP matrix that should be used for this instance\n"
          "mat4 MVP() {\n"
          "    return transformations.MVP[ID.transformation + gl_InstanceIndex];\n"
          "}\n\n";
    
    ss << "// Returns the M matrix that should be used for this instance\n"
          "mat4 M() {\n"
          "    return transformations.M[ID.transformation + gl_InstanceIndex];\n"
          "}\n\n";

    ss << "// Create the TBN matrix from vertex attributes\n";
    ss << "#if " << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " == 1\n";
    ss << "mat3 TBN() {\n";
    ss << "    mat3 M3x3 = mat3(M());\n\n";
//     ss << "    vec3 normalWS    = normalize(M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Normal) << ".xyz);\n";
//     ss << "    vec3 tangentWS   = normalize(M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Tangent) << ".xyz);\n";
//     ss << "    vec3 bitangentWS = normalize(M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Bitangent) << ".xyz);\n\n";
    // It's usually sufficient to normalize the final value only.
    ss << "    vec3 normalWS    = M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Normal) << ".xyz;\n";
    ss << "    vec3 tangentWS   = M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Tangent) << ".xyz;\n";
    ss << "    vec3 bitangentWS = M3x3 * " << con::GetVertexAttributeName(VertexAttributeType::Bitangent) << ".xyz;\n\n";
    ss << "    return mat3(tangentWS, bitangentWS, normalWS);\n";
    ss << "}\n";
    ss << "#elif " << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " == 2\n";
    ss << "mat3 TBN() {\n";
    ss << "    #error \"Not yet implemented\"\n";
    ss << "}\n";
    ss << "#endif\n";

    return ss.str();
}

inline std::string MakeFragmentShaderHelperFunctionInclude() {
    std::stringstream ss;
    ss << "#include \"" << GetShaderIncludeName(ShaderInclude::CommonHelpers) << "\"\n";
    
    ss << "vec3 GetNormalFromTextureData(vec3 valueFromTexture) {\n";
    ss << "#if " << GetShaderMacroName(ShaderMacro::NormalTextureChannelCount) << " == 2\n";
    ss << "    // Normal maps are packed to two channels when using BC formats\n";
    ss << "    vec2 normalXY = valueFromTexture.xy;\n";
    ss << "    normalXY = 2.0f * normalXY - vec2(1.0f, 1.0f);\n\n";
    ss << "    float normalZ = sqrt(1 - (normalXY.x * normalXY.x) - (normalXY.y * normalXY.y));\n";
    ss << "    return normalize(vec3(normalXY, normalZ));\n";
    ss << "#elif " << GetShaderMacroName(ShaderMacro::NormalTextureChannelCount) << " == 3\n";
    ss << "    // The data is already in the normal map. Do nothing to it\n";
    ss << "    return valueFromTexture;\n";
    ss << "#else\n";
    ss << "#error \"ShaderMacro::NormalTextureChannelCount is mandatory macro, but it wasn't set\"\n";
    ss << "#endif\n";
    ss << "}\n\n";
    
    ss << "#if " << GetShaderMacroName(ShaderMacro::NormalMappingMode) << " != 0\n";
    ss << "vec3 ApplyNormalMap(vec3 valueFromTexture) {\n";
    ss << "    vec3 finalNormal = GetNormalFromTextureData(valueFromTexture);\n";
    ss << "    return normalize(fragmentInput.TBN * finalNormal);\n";
    ss << "}\n";
    ss << "#endif\n\n";
    
    return ss.str();
}

const std::string VulkanGLSLIncluder::VertexShaderHelperFunctions = MakeVertexShaderHelperFunctionInclude();

const std::string VulkanGLSLIncluder::FragmentShaderHelperFunctions = MakeFragmentShaderHelperFunctionInclude();

const std::string VulkanGLSLIncluder::CommonHelperFunctions = R"glsl(// First of all, make sure all defines are present
#ifndef NORMAL_MAPPING_MODE
#error "NORMAL_MAPPING_MODE was not defined"
#endif
)glsl";

}
