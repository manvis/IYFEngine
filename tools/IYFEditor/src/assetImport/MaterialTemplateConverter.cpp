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

namespace iyf::editor {
class MaterialTemplateConverterInternalState : public InternalConverterState {
public:
    MaterialTemplateConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> code;
    std::int64_t size;
};

MaterialTemplateConverter::MaterialTemplateConverter(const ConverterManager* manager) : Converter(manager) {
    // TODO implement me
    //vulkanShaderGen = std::make_unique<VulkanGLSLShaderGenerator>(manager->getFileSystem());
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
//     MaterialTemplateConverterState& conversionState = dynamic_cast<MaterialTemplateConverterState&>(state);
//     MaterialTemplateConverterInternalState* internalState = dynamic_cast<MaterialTemplateConverterInternalState*>(state.getInternalState());
//     assert(internalState != nullptr);
//     
//     const fs::path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
//     
//     FileHash hash = HF(reinterpret_cast<const char*>(content.data()), outputByteCount);
//     MaterialTemplateMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags());
//     ImportedAssetData iad(state.getType(), metadata, outputPath);
//     state.getImportedAssets().push_back(std::move(iad));
//     
//     File fw(outputPath, File::OpenMode::Write);
//     fw.writeBytes(content.data(), outputByteCount);
    throw std::runtime_error("errr");
    
    return true;
}

}

