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

#include "assetImport/converters/LocalizationStringConverter.hpp"
#include "assetImport/converterStates/LocalizationStringConverterState.hpp"

#include "core/filesystem/VirtualFileSystem.hpp"
#include "assetImport/ConverterManager.hpp"
#include "assets/metadata/StringMetadata.hpp"
#include "io/File.hpp"
#include "logging/Logger.hpp"
#include "io/serialization/MemorySerializer.hpp"
#include "localization/LocalizationCSVParser.hpp"
#include "utilities/Regexes.hpp"
#include "utilities/hashing/HashCombine.hpp"

#include <string_view>

namespace iyf::editor {
class LocalizationConverterInternalState : public InternalConverterState {
public:
    LocalizationConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> data;
    std::size_t size;
};

std::unique_ptr<ConverterState> LocalizationStringConverter::initializeConverter(const Path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<LocalizationConverterInternalState> internalState = std::make_unique<LocalizationConverterInternalState>(this);
    
    if (!std::regex_match(inPath.filename().getGenericString(), SystemRegexes().LocalizationFileNameValidationRegex)) {
        LOG_E("The name of the string file did not match the expected format. You need something like \"filename.en_US.csv\"");
        return nullptr;
    }
    
    auto inFile = VirtualFileSystem::Instance().openFile(inPath, FileOpenMode::Read);
    auto data = inFile->readWholeFile();
    internalState->data = std::move(data.first);
    internalState->size = data.second;
    
    const FileHash sourceFileHash = HF(internalState->data.get(), internalState->size);
    auto locState = std::unique_ptr<LocalizationStringConverterState>(new LocalizationStringConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
    locState->priority = 0;
    locState->locale = inPath.stem().extension().getGenericString().substr(1);
    return locState;
}

bool LocalizationStringConverter::convert(ConverterState& state) const {
    LocalizationStringConverterState& locState = dynamic_cast<LocalizationStringConverterState&>(state);
    LocalizationConverterInternalState* internalState = dynamic_cast<LocalizationConverterInternalState*>(locState.getInternalState());
    assert(internalState != nullptr);
    
    std::vector<CSVRow> rows;
    
    const LocalizationCSVParser parser;
    auto result = parser.parse(internalState->data.get(), internalState->size, rows);
    
    if (result.first != LocalizationCSVParser::Result::Success) {
        LOG_E("Failed to parse a localization string file {}. Parser reported an error: {}", state.getSourceFilePath(), parser.resultToErrorString(result.first));
        
        return false;
    }
    
    if (state.isDebugOutputRequested()) {
        LOG_V("Loaded {} strings.", result.second);
    }
    
    std::unordered_map<StringHash, CSVRow> strings;
    
    for (const CSVRow& row : rows) {
        StringHash seed(0);
        
        const StringHash keyHash = HS(row.key.data(), row.key.length());
        util::HashCombine(seed, keyHash);
        
        if (row.stringNamespace.length() != 0) {
            const StringHash namespaceHash = HS(row.stringNamespace.data(), row.stringNamespace.length());
            util::HashCombine(seed, namespaceHash);
        }
        
        auto string = strings.find(seed);
        if (string != strings.end()) {
            std::stringstream ss;
            ss << "The string key \"" << std::string(row.key) << "\" ";
            
            if (row.stringNamespace.length() != 0) {
                ss << "from namespace \"" << std::string(row.stringNamespace) << "\" ";
            }
            
            if (row.key == string->second.key && row.stringNamespace == string->second.stringNamespace) {
                ss << "repeats multiple times. Only the last value will be recorded.";
            } else {
                ss << "has a hash collision with \"" << std::string(string->second.key) << "\" ";
                
                if (string->second.stringNamespace.length() != 0) {
                    ss << "from namespace \"" << std::string(string->second.stringNamespace) << "\" ";
                }
                
                ss << "adjust the keys or the namespaces.";
            }
            
            LOG_W("{}", ss.str());
        } else {
            strings[seed] = row;
        }
    }
    
    const Path outputPath = locState.systemTranslations ? manager->makeFinalPathForSystemStrings(state.getSourceFilePath(), state.getPlatformIdentifier())
                                                            : manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
    
    const std::array<char, 4> magicNumber = {'I', 'Y', 'F', 'S'};
    MemorySerializer ms(4096);
    
    ms.writeBytes(magicNumber.data(), magicNumber.size());
    ms.writeUInt32(1); // File format version
    ms.writeInt32(locState.priority);
    ms.writeUInt32(strings.size());
    
    for (const auto& string : strings) {
        ms.writeUInt64(string.first);
        ms.writeString(string.second.getValue(), StringLengthIndicator::UInt32);
    }
    
    LOG_D("OP: {} {}", outputPath, locState.systemTranslations);
    
    const FileHash hash = HF(ms.data(), ms.size());
    StringMetadata metadata(hash, state.getSourceFilePath(), state.getSourceFileHash(), state.isSystemAsset(), state.getTags(), locState.getLocale(), locState.priority);
    ImportedAssetData iad(state.getType(), metadata, outputPath);
    state.getImportedAssets().push_back(std::move(iad));
    
    LOG_D("OP: {} {} {}", outputPath, locState.systemTranslations, static_cast<std::size_t>(state.getType()));
    
    auto file = VirtualFileSystem::Instance().openFile(outputPath, FileOpenMode::Write);
    file->writeBytes(ms.data(), ms.size());
    
    return true;
}
}
