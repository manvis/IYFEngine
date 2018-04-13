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

#include "localization/LocalizationCSVParser.hpp"
#include "core/Logger.hpp"
#include <cassert>

namespace iyf {
// enum class LineState {
//     Key,
//     Namespace,
//     Value
// };

inline LocalizationCSVParser::Result extractColumn(const char* bytes, const char* lastByte, std::size_t maxLength, std::size_t& current, std::string_view& view, int columnID) {
    std::size_t count = 0;
    const char* start = bytes + current;
    
    if (columnID == 2) {
        const bool inQuotes = (*start == '\"');
        const char delimiter = inQuotes ? '\"' : '\n';
        
        while ((start + count < lastByte) && (*(start + count) != delimiter)) {
            count++;
            current++;
        }
        
        // Move one (or more, if quoted) symbols forward or back 
        if (inQuotes) {
            //
        } else {
            // Make sure carriage returns (if used in the file) don't end up in the translated string
            if (*(start + count - 1) == '\r') {
                count--;
            }
            
            // Move away from the current newline character
            current++;
        }
    } else if (columnID == 0 || columnID == 1) {
        while ((start + count < lastByte) && (*(start + count) != '\t')) {
            char c = *(start + count);
            if (c == '\"' || c == '\r' || c == '\n') {
                if (columnID == 0) {
                    return LocalizationCSVParser::Result::InvalidCharacterInKey;
                } else {
                    return LocalizationCSVParser::Result::InvalidCharacterInNamespace;
                }
            }
            
            count++;
            current++;
        }
        
        if (count > maxLength) {
            if (columnID == 0) {
                return LocalizationCSVParser::Result::TooManyBytesInKey;
            } else {
                return LocalizationCSVParser::Result::TooManyBytesInNamespace;
            }
        }
        
        if (columnID == 0 && count == 0) {
            return LocalizationCSVParser::Result::KeyEmpty;
        }
        
        if (*(start + count) != '\t') {
            return LocalizationCSVParser::Result::ColumnMissing;
        }
        
        // Move away from the current tab character
        current++;
    } else {
        assert(0);
    }
    
    view = std::string_view(start, count);
    
    return LocalizationCSVParser::Result::Success;
}

std::pair<LocalizationCSVParser::Result, std::size_t> LocalizationCSVParser::parse(const char* bytes, std::size_t length, std::vector<CSVLine>& parsedLines) const {
    if (bytes == nullptr) {
        LOG_E("The bytes pointer passed to LocalizationCSVParser can't be nullptr");
        return {Result::NullPointer, 0};
    }
    
    std::size_t counter = 0;
    std::size_t lineNumber = 0;
    
    while (counter < length) {
        CSVLine line;
        
        Result result = extractColumn(bytes, bytes + length, 128, counter, line.key, 0);
        if (result != Result::Success) {
            return {result, lineNumber};
        }
        
        result = extractColumn(bytes, bytes + length, 128, counter, line.stringNamespace, 1);
        if (result != Result::Success) {
            return {result, lineNumber};
        }
        
        result = extractColumn(bytes, bytes + length, -1, counter, line.value, 2);
        if (result != Result::Success) {
            return {result, lineNumber};
        }
        
        parsedLines.push_back(std::move(line));
        lineNumber++;
    }
//     LineState state = LineState::Key;
//     std::size_t counter = 0;
//     for (std::size_t i = 0; i < length; ++i) {
//         const char byte = bytes[i];
//         
//         switch (state) {
//             case LineState::Key:
//                 break;
//             case LineState::Namespace:
//                 break;
//             case LineState::Value:
//                 break;
//         }
//     }
    
    return {Result::Success, lineNumber};
}

std::string LocalizationCSVParser::resultToErrorString(Result result) const {
    switch (result) {
        case Result::UnknownError:
            return "Unknown Error";
        case Result::NullPointer:
            return "A nullptr was passed to the function";
        case Result::TooManyBytesInKey:
            return "Key names can't be longer than 128 bytes";
        case Result::TooManyBytesInNamespace:
            return "Namespace names can't be longer than 128 bytes";
        case Result::InvalidCharacterInKey:
            return "Found an invalid character in the key column";
        case Result::InvalidCharacterInNamespace:
            return "Found an invalid character in the namespace column";
        case Result::KeyEmpty:
            return "The key column can't be empty";
        case Result::ColumnMissing:
            return "The row had less than 3 columns";
        case Result::Success:
            return "Success";
    }
    
    throw std::runtime_error("Unknown value was provided to the resultToErrorString function");
}
}
