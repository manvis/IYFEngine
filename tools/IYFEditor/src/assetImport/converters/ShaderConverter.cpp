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

#include "assetImport/converters/ShaderConverter.hpp"
#include "assetImport/ConverterManager.hpp"
#include "assetImport/converterStates/ShaderConverterState.hpp"
#include "assets/metadata/ShaderMetadata.hpp"

#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "fmt/format.h"

namespace iyf::editor {
static ShaderStageFlagBits StageBitsFromPath(const fs::path& path) {
    const std::string extension = path.extension().generic_string();
    return static_cast<ShaderStageFlagBits>(con::ExtensionToShaderStage(extension).uint64());
}

class ShaderConverterInternalState : public InternalConverterState {
public:
    ShaderConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> code;
    std::int64_t size;
};

ShaderConverter::ShaderConverter(const ConverterManager* manager) : Converter(manager) {
    compilerOptions.SetOptimizationLevel(shaderc_optimization_level_size);
}

std::unique_ptr<ConverterState> ShaderConverter::initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<ShaderConverterInternalState> internalState = std::make_unique<ShaderConverterInternalState>(this);
    
    File shaderFile(inPath, File::OpenMode::Read);
    auto result = shaderFile.readWholeFile();
    
    internalState->code = std::move(result.first);
    internalState->size = result.second;
    
    const FileHash shaderFileHash = HF(internalState->code.get(), internalState->size);
    auto converterState = std::unique_ptr<ShaderConverterState>(new ShaderConverterState(platformID, std::move(internalState), inPath, shaderFileHash));
    converterState->stage = StageBitsFromPath(inPath);
    converterState->determinedStage = converterState->stage;
    return converterState;
}

bool ShaderConverter::convert(ConverterState& state) const {
    ShaderConverterState& conversionState = dynamic_cast<ShaderConverterState&>(state);
    ShaderConverterInternalState* internalState = dynamic_cast<ShaderConverterInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    if (conversionState.stage != conversionState.determinedStage) {
        LOG_W("The determined shader stage does not match the user specified shader stage. You should "
              "adjust the file extension instead of forcing a different stage");
    }
    
    ShaderStageFlagBits shaderStage = conversionState.stage;
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
    
    const std::string filePath = state.getSourceFilePath().generic_string();
    if (state.isDebugOutputRequested()) {
        shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(internalState->code.get(),
                                                                                      shaderKind,
                                                                                      filePath.c_str(),
                                                                                      compilerOptions);
        
        if (result.GetCompilationStatus() == shaderc_compilation_status_success) {
            LOG_V("Assembly for shader " << filePath << "\n\n" << std::string(result.begin(), result.end()));
        }
    }
    
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(internalState->code.get(), shaderKind, filePath.c_str(), compilerOptions);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        LOG_E(fmt::format("Shader \"{}\" compilation failed with error {}", filePath, result.GetErrorMessage()));
        return false;
    }
    
    LOG_W("Successfully compiled a shader \"" << filePath << "\"\n\tWarnings(" << result.GetNumWarnings() << "): " << result.GetErrorMessage());
    
    const std::vector<std::uint32_t> content(result.begin(), result.end());
    const std::size_t outputByteCount = content.size() * sizeof(std::uint32_t);
    
    const fs::path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
    
    FileHash hash = HF(reinterpret_cast<const char*>(content.data()), outputByteCount);
    ShaderMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags(), shaderStage);
    ImportedAssetData iad(state.getType(), metadata, outputPath);
    state.getImportedAssets().push_back(std::move(iad));
    
    File fw(outputPath, File::OpenMode::Write);
    fw.writeBytes(content.data(), outputByteCount);
    
    return true;
}

}
