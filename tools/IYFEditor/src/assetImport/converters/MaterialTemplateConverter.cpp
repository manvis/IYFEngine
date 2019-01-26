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

#include "assetImport/converters/MaterialTemplateConverter.hpp"
#include "assetImport/ConverterManager.hpp"
#include "assetImport/converterStates/MaterialTemplateConverterState.hpp"
#include "assets/metadata/MaterialTemplateMetadata.hpp"

#include "graphics/Renderer.hpp"
#include "graphics/shaderGeneration/ShaderMacroCombiner.hpp"
#include "graphics/shaderGeneration/VulkanGLSLShaderGenerator.hpp"

#include "core/filesystem/File.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/Logger.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "format/format.h"

#include "tools/MaterialEditor.hpp"

#include "utilities/DataSizes.hpp"
#include "utilities/hashing/HashCombine.hpp"

#include "rapidjson/error/en.h"

#include "glm/gtx/string_cast.hpp"

#include <bitset>

//#define IYF_PRINT_MACRO_LIST_AND_COMBOS

namespace iyf::editor {
constexpr std::size_t VertexLayoutCount = 4;
constexpr std::size_t ShaderStageCount = 2;

constexpr std::array<VertexDataLayout, VertexLayoutCount> VertexLayouts = {
    VertexDataLayout::MeshVertex,
    VertexDataLayout::MeshVertexWithBones,
    VertexDataLayout::MeshVertexColored,
    VertexDataLayout::MeshVertexColoredWithBones
};

constexpr std::array<ShaderStageFlagBits, ShaderStageCount> ShaderStages = {
    ShaderStageFlagBits::Vertex,
    ShaderStageFlagBits::Fragment,
};

constexpr std::bitset<64> VertexDataLayoutsToBitset() {
    std::uint64_t result = 0;
    
    for (VertexDataLayout vdl : VertexLayouts) {
        result |= 1 << static_cast<std::size_t>(vdl);
    }
    
    return std::bitset<64>(result);
}

constexpr std::bitset<64> SupportedVertexLayouts = VertexDataLayoutsToBitset();

struct AvailableShaderCombos {
    StringHash versionHash;
    /// Sorted vector, used for consitent hash generation
    std::vector<std::vector<ShaderMacroWithValue>> macrosWithAllowedValues;
    
    /// Unordered map used for quick lookups
    std::unordered_map<ShaderMacro, MaterialTemplateMetadata::SupportedMacroValues> macrosWithAllowedValuesMap;
    
    std::vector<std::pair<StringHash, std::vector<ShaderMacroWithValue>>> allAvailableCombos;
    
    inline void makeMap() {
        macrosWithAllowedValuesMap.reserve(macrosWithAllowedValues.size());
        for (const auto& smvs : macrosWithAllowedValues) {
            assert(!smvs.empty());
            
            // Value 0 is the default, as the docs say.
            auto insertionResult = macrosWithAllowedValuesMap.emplace(smvs[0].getMacroIdentifier(), MaterialTemplateMetadata::SupportedMacroValues());
            assert(insertionResult.second);
            
            MaterialTemplateMetadata::SupportedMacroValues& supportedMacroValues = insertionResult.first->second;
            supportedMacroValues.defaultValue = smvs[0].getRawValue();
            
            supportedMacroValues.supportedValues.reserve(smvs.size());
            for (const ShaderMacroWithValue& v : smvs) {
                supportedMacroValues.supportedValues.emplace_back(v.getRawValue());
            }
        }
    }
};

class MaterialTemplateConverterInternalState : public InternalConverterState {
public:
    MaterialTemplateConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> code;
    std::int64_t size;
};

MaterialTemplateConverter::MaterialTemplateConverter(const ConverterManager* manager) : Converter(manager) {
    vulkanShaderGen = std::make_unique<VulkanGLSLShaderGenerator>(manager->getFileSystem());
    availableShaderCombos = std::make_unique<AvailableShaderCombos>();
    
    availableShaderCombos->macrosWithAllowedValues = ShaderMacroCombiner::MakeMacroAndValueVectors();
    
    ShaderMacroCombiner::MacroCombos combos = ShaderMacroCombiner::MakeAllCombinations(availableShaderCombos->macrosWithAllowedValues);
    availableShaderCombos->versionHash = combos.versionHash;
    availableShaderCombos->allAvailableCombos = std::move(combos.combos);
    
    availableShaderCombos->makeMap();
    
#ifdef IYF_PRINT_MACRO_LIST_AND_COMBOS
    std::stringstream ss;
    ss << "##VERSION##\n";
    ss << "\t\t" << availableShaderCombos->versionHash << "\n\n";
    
    ss << "\t##MACROS##\n";
    for (const auto& macroWithAllowedValues : availableShaderCombos->macrosWithAllowedValues) {
        for (const auto& m : macroWithAllowedValues) {
            ss << "\t\t" << m.getName() << " " << m.getStringifiedValue() << " " << m.getNameHash() << "\n";
        }
        
        ss << "\n";
    }
    
    ss << "\n\t##COMBOS##\n";
    for (const auto& combo : availableShaderCombos->allAvailableCombos) {
        for (const auto& m : combo.second) {
            ss << "\t\t" << m.getName() << " " << m.getStringifiedValue() << " " << m.getNameHash() << "\n";
        }
        
        ss << "\n";
    }
    
    LOG_D(ss.str())
#endif // IYF_PRINT_MACRO_LIST_AND_COMBOS
}

MaterialTemplateConverter::~MaterialTemplateConverter() {}

std::unique_ptr<ConverterState> MaterialTemplateConverter::initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<MaterialTemplateConverterInternalState> internalState = std::make_unique<MaterialTemplateConverterInternalState>(this);
    
    File shaderFile(inPath, File::OpenMode::Read);
    auto result = shaderFile.readWholeFile();
    
    internalState->code = std::move(result.first);
    internalState->size = result.second;
    
    const FileHash shaderFileHash = HF(internalState->code.get(), internalState->size);
    auto converterState = std::unique_ptr<MaterialTemplateConverterState>(new MaterialTemplateConverterState(platformID, std::move(internalState), inPath, shaderFileHash));
    return converterState;
}

bool MaterialTemplateConverter::convert(ConverterState& state) const {
    MaterialTemplateConverterState& conversionState = dynamic_cast<MaterialTemplateConverterState&>(state);
    MaterialTemplateConverterInternalState* internalState = dynamic_cast<MaterialTemplateConverterInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    // Use any MaterialFamily here. Everything's going to be overriden when deserialization completes anyway.
    std::unique_ptr<MaterialLogicGraph> mlg = std::make_unique<MaterialLogicGraph>(MaterialFamily::Toon);
    
    rj::Document jo;
    rj::ParseResult parseResult = jo.Parse(internalState->code.get(), internalState->size);
    
    if (!parseResult) {
        LOG_W("Failed to parse the JSON from " << state.getSourceFilePath() <<
              "\n\tError: " << rj::GetParseError_En(parseResult.Code()) << "(" << parseResult.Offset() << ")");
        return false;
    }
    
    mlg->deserializeJSON(jo);
    
    // Input variables
    std::vector<const VariableNode*> variables = mlg->getVariableNodes();
    std::sort(variables.begin(), variables.end(), [](const VariableNode* a, const VariableNode* b) {
        return a->getName() < b->getName();
    });
    
    std::vector<MaterialInputVariable> inputVariables;
    for (const VariableNode* vn : variables) {
        inputVariables.emplace_back(vn->getName(), vn->getDefaultValue(), vn->getComponentCount());
    }
    
    // Input textures
    std::vector<const TextureInputNode*> textures = mlg->getTextureNodes();
    std::sort(textures.begin(), textures.end(), [](const TextureInputNode* a, const TextureInputNode* b) {
        return a->getName() < b->getName();
    });
    
    std::vector<MaterialInputTexture> inputTextures;
    for (const TextureInputNode* tn : textures) {
        inputTextures.emplace_back(tn->getName(), tn->getDefaultTexture());
    }
    
    //LOG_D("CODE_GEN:\n" << mlg->toCode(ShaderLanguage::GLSLVulkan).getCode());
    
    // Use the MaterialLogicGraph to build the ubershaders with tons of #ifdef cases
    const ShaderGenerationResult vertResult = vulkanShaderGen->generateShader(conversionState.getPlatformIdentifier(),
                                                                              ShaderStageFlagBits::Vertex,
                                                                              con::GetMaterialFamilyDefinition(mlg->getMaterialFamily()),
                                                                              nullptr);
    if (!vertResult) {
        LOG_W("Failed to generate the vertex shader for " << state.getSourceFilePath() <<
              "\n\tError: " << vertResult.getContents() << ")");
        return false;
    }
    
    const ShaderGenerationResult fragResult = vulkanShaderGen->generateShader(conversionState.getPlatformIdentifier(),
                                                                              ShaderStageFlagBits::Fragment,
                                                                              con::GetMaterialFamilyDefinition(mlg->getMaterialFamily()),
                                                                              mlg.get());
    if (!fragResult) {
        LOG_W("Failed to generate the fragment shader for " << state.getSourceFilePath() <<
              "\n\tError: " << fragResult.getContents() << ")");
        return false;
    }
    
    LOG_D("------------------\n" << vertResult.getContents() << "----------------\n" << fragResult.getContents());
    
    // Compile all variants of the ubershaders
    ShaderCompilationSettings scs;
    scs.optimizationLevel = ShaderOptimizationLevel::Performance;
    
    std::size_t totalShaders = 0;
    std::size_t estimatedTotalShaders = availableShaderCombos->allAvailableCombos.size() * VertexLayoutCount * ShaderStageCount;
    
    std::uint64_t size = Bytes(Mebibytes(2)).count();
    MemorySerializer ms(size);
    
    const std::array<char, 5> magicNumber = {'I', 'Y', 'F', 'M', 'T'};
    ms.writeBytes(magicNumber.data(), magicNumber.size());
    ms.writeUInt16(1); // Version
    ms.writeUInt32(estimatedTotalShaders);

#ifndef NDEBUG
    std::unordered_set<StringHash> finalHashes;
    std::stringstream ssc;
#endif // NDEBUG
    
    for (const auto& combo : availableShaderCombos->allAvailableCombos) {
        scs.macros = combo.second;
        
        for (std::size_t i = 0; i < VertexLayouts.size(); ++i) {
            const VertexDataLayout vdl = VertexLayouts[i];
            
            scs.vertexDataLayout = vdl;
            
            const VertexDataLayoutDefinition& layoutDefinition = con::GetVertexDataLayoutDefinition(vdl);
            
            for (ShaderStageFlagBits stage : ShaderStages) {
                const std::string* code = nullptr;
                switch (stage) {
                    case ShaderStageFlagBits::Vertex:
                        code = &vertResult.getContents();
                        break;
                    case ShaderStageFlagBits::Fragment:
                        code = &fragResult.getContents();
                        break;
                    default:
                        throw std::runtime_error("The MaterialTemplateConverter can't handle this shader stage");
                }
                
                std::optional<StringHash> compilationResult = compileShader(combo.first, stage, *code, layoutDefinition.getName(), scs, ms); ;
                if (!compilationResult) {
                    return false;
                }
                
#ifndef NDEBUG
                auto uniqueCheck = finalHashes.emplace(*compilationResult);
                assert(uniqueCheck.second);
#endif // NDEBUG
                
                totalShaders++;
            }
        }
    }
    assert(estimatedTotalShaders == totalShaders);
    
     const fs::path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
     
    FileHash hash = HF(ms.data(), ms.size());
    
    MaterialFamily materialFamily = mlg->getMaterialFamily();
    const MaterialFamilyDefinition& materialFamilyDefinition = con::GetMaterialFamilyDefinition(materialFamily);
    MaterialTemplateMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags(), materialFamily,
                                      materialFamilyDefinition.computeHash(), availableShaderCombos->versionHash, SupportedVertexLayouts, inputVariables, inputTextures);
    ImportedAssetData iad(state.getType(), metadata, outputPath);
    state.getImportedAssets().push_back(std::move(iad));
    
    File fw(outputPath, File::OpenMode::Write);
    fw.writeBytes(ms.data(), ms.size());
    return true;
}

std::optional<StringHash> MaterialTemplateConverter::compileShader(StringHash macroHash, ShaderStageFlagBits stage, const std::string& shaderCode, const std::string& name, const ShaderCompilationSettings& settings, MemorySerializer& serializer) const {
    const char* stageName = nullptr;
    std::uint8_t stageID = 255;
    
    switch (stage) {
        case ShaderStageFlagBits::Vertex:
            stageName = "VertexShader";
            stageID = 0;
            break;
        case ShaderStageFlagBits::Geometry:
            stageName = "GeometryShader";
            stageID = 1;
            break;
        case ShaderStageFlagBits::TessControl:
            stageName = "TessControlShader";
            stageID = 2;
            break;
        case ShaderStageFlagBits::TessEvaluation:
            stageName = "TessEvaluationShader";
            stageID = 3;
            break;
        case ShaderStageFlagBits::Fragment:
            stageName = "FragmentShader";
            stageID = 4;
            break;
        default:
            break;
    }
    
    assert(stageName != nullptr && stageID != 255);
    
    const ShaderCompilationResult compResult = vulkanShaderGen->compileShader(stage, shaderCode, name + stageName, settings);
    if (!compResult) {
        LOG_W("Material template conversion failed\n\t" << compResult.getErrorsAndWarnings());
        return {};
    } else if (!compResult.getErrorsAndWarnings().empty()) {
        LOG_W(compResult.getErrorsAndWarnings());
    }
    
    StringHash lookupHash(macroHash);
    util::HashCombine(lookupHash, HS(reinterpret_cast<const char*>(&settings.vertexDataLayout), sizeof(settings.vertexDataLayout)));
    util::HashCombine(lookupHash, HS(reinterpret_cast<const char*>(&stage), sizeof(ShaderStageFlagBits)));
    
    serializer.writeUInt8(static_cast<std::uint8_t>(settings.vertexDataLayout));
    serializer.writeUInt8(stageID);
    serializer.writeUInt64(macroHash);
    serializer.writeUInt64(lookupHash);
    serializer.writeUInt32(compResult.getBytecode().size());
    serializer.writeBytes(compResult.getBytecode().data(), compResult.getBytecode().size());
    
    return lookupHash;
}

}

