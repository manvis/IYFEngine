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

#include "assetImport/LocalizationStringConverter.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "localization/LocalizationCSVParser.hpp"
#include "utilities/Regexes.hpp"

#include <string_view>

namespace iyf::editor {
class LocalizationConverterInternalState : public InternalConverterState {
public:
    LocalizationConverterInternalState(const Converter* converter) : InternalConverterState(converter) {}
    
    std::unique_ptr<char[]> data;
    std::size_t size;
    
    std::vector<CSVRow> parsedRows;
};

std::unique_ptr<ConverterState> LocalizationStringConverter::initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const {
    std::unique_ptr<LocalizationConverterInternalState> internalState = std::make_unique<LocalizationConverterInternalState>(this);
    
    if (!std::regex_match(inPath.filename().string(), regex::LocalizationFileNameValidationRegex)) {
        LOG_E("The name of the string file did not match the expected format. You need something like \"filename.en_US.csv\"");
        return nullptr;
    }
    
    File inFile(inPath, File::OpenMode::Read);
    auto data = inFile.readWholeFile();
    internalState->data = std::move(data.first);
    internalState->size = data.second;
    
    const hash64_t sourceFileHash = HF(internalState->data.get(), internalState->size);
    
    return std::unique_ptr<LocalizationStringConverterState>(new LocalizationStringConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
}

bool LocalizationStringConverter::convert(ConverterState& state) const {
    return false;
}
}
