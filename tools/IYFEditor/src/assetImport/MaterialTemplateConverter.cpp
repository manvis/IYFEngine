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

#include "assetImport/MaterialTemplateConverter.hpp"
#include "assetImport/ConverterManager.hpp"

#include "graphics/shaderGeneration/VulkanGLSLShaderGenerator.hpp"

#include "core/filesystem/File.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/Logger.hpp"
#include "format/format.h"

#include "tools/MaterialEditor.hpp"

#include "rapidjson/error/en.h"

namespace iyf::editor {
class MaterialTemplateConverterInternalState : public InternalConverterState {
public:
    MaterialTemplateConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> code;
    std::int64_t size;
};

MaterialTemplateConverter::MaterialTemplateConverter(const ConverterManager* manager) : Converter(manager) {
    vulkanShaderGen = std::make_unique<VulkanGLSLShaderGenerator>(manager->getFileSystem());
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
    
    // Use whatever. It's going to be overriden when deserialized
    std::unique_ptr<MaterialLogicGraph> mlg = std::make_unique<MaterialLogicGraph>(MaterialFamily::Toon);
    
    rj::Document jo;
    rj::ParseResult parseResult = jo.Parse(internalState->code.get(), internalState->size);
    
    if (!parseResult) {
        LOG_W("Failed to parse the JSON from " << state.getSourceFilePath() <<
              "\n\tError: " << rj::GetParseError_En(parseResult.Code()) << "(" << parseResult.Offset() << ")");
        return false;
    }
    
    mlg->deserializeJSON(jo);
    
    //LOG_D("CODE_GEN:\n" << mlg->toCode(ShaderLanguage::GLSLVulkan).getCode());
    
    const ShaderGenerationResult vertResult = vulkanShaderGen->generateShader(conversionState.getPlatformIdentifier(),
                                                                              RendererType::ForwardClustered,
                                                                              ShaderStageFlagBits::Vertex,
                                                                              con::GetMaterialFamilyDefinition(mlg->getMaterialFamily()),
                                                                              nullptr);
    if (!vertResult) {
        LOG_W("Failed to generate the vertex shader for " << state.getSourceFilePath() <<
              "\n\tError: " << vertResult.getContents() << ")");
        return false;
    }
    
    const ShaderGenerationResult fragResult = vulkanShaderGen->generateShader(conversionState.getPlatformIdentifier(),
                                                                              RendererType::ForwardClustered,
                                                                              ShaderStageFlagBits::Fragment,
                                                                              con::GetMaterialFamilyDefinition(mlg->getMaterialFamily()),
                                                                              mlg.get());
    if (!fragResult) {
        LOG_W("Failed to generate the fragment shader for " << state.getSourceFilePath() <<
              "\n\tError: " << fragResult.getContents() << ")");
        return false;
    }
    
    LOG_D("------------------\n" << vertResult.getContents() << "----------------\n" << fragResult.getContents());
    
    ShaderCompilationSettings scs;
    scs.optimizationLevel = ShaderOptimizationLevel::Performance;
    
    // TODO more complicated variant generation
    
    std::array<VertexDataLayout, 4> vertexLayouts = {
        VertexDataLayout::MeshVertex,
        VertexDataLayout::MeshVertexWithBones,
        VertexDataLayout::MeshVertexColored,
        VertexDataLayout::MeshVertexColoredWithBones
    };
    
    std::size_t totalShaders = 0;
    for (const VertexDataLayout vdl : vertexLayouts) {
        scs.vertexDataLayout = vdl;
        const VertexDataLayoutDefinition& layoutDefinition = con::GetVertexDataLayoutDefinition(vdl);
        
        // Compile the vertex shader
        const ShaderCompilationResult scrVS = vulkanShaderGen->compileShader(ShaderStageFlagBits::Vertex, vertResult.getContents(), layoutDefinition.getName() + "VertexShader", scs);
        if (!scrVS) {
            LOG_W("Material template conversion failed\n\t" << scrVS.getErrorsAndWarnings());
            return false;
        } else if (!scrVS.getErrorsAndWarnings().empty()) {
            LOG_W(scrVS.getErrorsAndWarnings());
        }
        totalShaders++;
        
        // Compile the fragment shader
        const ShaderCompilationResult scrFS = vulkanShaderGen->compileShader(ShaderStageFlagBits::Fragment, fragResult.getContents(), layoutDefinition.getName() + "FragmentShader", scs);
        if (!scrFS) {
            LOG_W("Material template conversion failed\n\t" << scrFS.getErrorsAndWarnings());
            return false;
        } else if (!scrFS.getErrorsAndWarnings().empty()) {
            LOG_W(scrFS.getErrorsAndWarnings());
        }
        totalShaders++;
    }
//     const fs::path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
//     
//     FileHash hash = HF(reinterpret_cast<const char*>(content.data()), outputByteCount);
//     MaterialTemplateMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags());
//     ImportedAssetData iad(state.getType(), metadata, outputPath);
//     state.getImportedAssets().push_back(std::move(iad));
//     
//     File fw(outputPath, File::OpenMode::Write);
//     fw.writeBytes(content.data(), outputByteCount);
//     throw std::runtime_error("errr");
    LOG_V("Compiled " << totalShaders << " shader permutations for a material template called " << state.getSourceFilePath().stem());
    LOG_W("MaterialTemplateConverter::convert() NOT YET IMPLEMENTED")
    return false;
}

}

