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

#include "assetImport/converters/MaterialInstanceConverter.hpp"
#include "assetImport/converterStates/MaterialInstanceConverterState.hpp"

#include "assets/metadata/MaterialInstanceMetadata.hpp"
#include "graphics/materials/MaterialInstanceDefinition.hpp"
#include "assetImport/ConverterManager.hpp"
#include "logging/Logger.hpp"
#include "io/File.hpp"
#include "io/serialization/MemorySerializer.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

namespace iyf::editor {
class MaterialInstanceInternalState : public InternalConverterState {
public:
    MaterialInstanceInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> data;
    std::size_t size;
};

std::unique_ptr<ConverterState> MaterialInstanceConverter::initializeConverter(const Path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<MaterialInstanceInternalState> internalState = std::make_unique<MaterialInstanceInternalState>(this);
    
    auto inFile = VirtualFileSystem::Instance().openFile(inPath, FileOpenMode::Read);
    std::size_t size = inFile->seek(0, File::SeekFrom::End);
    inFile->seek(0, File::SeekFrom::Start);
    
    internalState->data = std::make_unique<char[]>(size);
    internalState->size = size;
    char* buffer = internalState->data.get();
    
    inFile->readBytes(buffer, size);
    
    const FileHash sourceFileHash = HF(reinterpret_cast<const char*>(internalState->data.get()), internalState->size);
    
    return std::unique_ptr<MaterialInstanceConverterState>(new MaterialInstanceConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
}

bool MaterialInstanceConverter::convert(ConverterState& state) const {
    MaterialInstanceInternalState* internalState = dynamic_cast<MaterialInstanceInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    Path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
    
    rj::Document document;
    document.Parse(internalState->data.get(), internalState->size);
    
    MaterialInstanceDefinition mid;
    mid.deserializeJSON(document);
    
    FileHash hash = HF(reinterpret_cast<const char*>(internalState->data.get()), internalState->size);
    MaterialInstanceMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags(), mid.getMaterialTemplateDefinition());
    ImportedAssetData iad(state.getType(), metadata, outputPath);
    state.getImportedAssets().push_back(std::move(iad));
    
    MemorySerializer ms(4096);
    mid.serialize(ms);
    
    auto fw = VirtualFileSystem::Instance().openFile(outputPath, FileOpenMode::Write);
    fw->writeBytes(ms.data(), ms.size());
    
    return true;
}
}

