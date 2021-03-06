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

#include "assetImport/converters/FontConverter.hpp"
#include "assetImport/converterStates/FontConverterState.hpp"

#include "core/filesystem/VirtualFileSystem.hpp"
#include "assets/metadata/FontMetadata.hpp"
#include "assetImport/ConverterManager.hpp"
#include "logging/Logger.hpp"
#include "io/File.hpp"

#include "stb_truetype.h"

namespace iyf::editor {
class FontConverterInternalState : public InternalConverterState {
public:
    FontConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<unsigned char[]> data;
    std::size_t size;
};

std::unique_ptr<ConverterState> FontConverter::initializeConverter(const Path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<FontConverterInternalState> internalState = std::make_unique<FontConverterInternalState>(this);
    
    // This is done to check if the file is a valid font file.
    auto inFile = VirtualFileSystem::Instance().openFile(inPath, FileOpenMode::Read);
    std::size_t size = inFile->seek(0, File::SeekFrom::End);
    inFile->seek(0, File::SeekFrom::Start);
    
    internalState->data = std::make_unique<unsigned char[]>(size);
    internalState->size = size;
    unsigned char* buffer = internalState->data.get();
    
    inFile->readBytes(buffer, size);
    
    int numFonts = stbtt_GetNumberOfFonts(buffer);
    if (numFonts < 1) {
        LOG_W("The file {} is not a valid .ttf/.otf font file", inPath);
        return nullptr;
    }
    
    if (numFonts > 1) {
        LOG_W("The file {} has too many fonts. Only one font per file is supported by the engine", inPath);
        return nullptr;
    }
    
    const FileHash sourceFileHash = HF(reinterpret_cast<const char*>(internalState->data.get()), internalState->size);
    
    return std::unique_ptr<FontConverterState>(new FontConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
}

bool FontConverter::convert(ConverterState& state) const {
    FontConverterInternalState* internalState = dynamic_cast<FontConverterInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    Path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
    
    FileHash hash = HF(reinterpret_cast<const char*>(internalState->data.get()), internalState->size);
    FontMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags());
    ImportedAssetData iad(state.getType(), metadata, outputPath);
    state.getImportedAssets().push_back(std::move(iad));
    
    auto fw = VirtualFileSystem::Instance().openFile(outputPath, FileOpenMode::Write);
    fw->writeBytes(internalState->data.get(), internalState->size);
    
    return true;
}
}
