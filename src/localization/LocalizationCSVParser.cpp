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
// WARNING. When editing this file, make sure to run the test cases after each change. The parser code is a bit messy
// and it's easy to blow things up.

inline std::pair<LocalizationCSVParser::Result, std::variant<std::string_view, std::string>> extractColumn(const char* bytes, const char* lastByte, std::size_t maxLength, std::size_t& current, int columnID) {
    std::size_t count = 0;
    const char* start = bytes + current;
    std::size_t quoteCount = 0;
    
    if (columnID == 2) {
        const bool inQuotes = (*start == '\"');
        const char delimiter = inQuotes ? '\"' : '\n';
        
        // Skip the current quote
        if (inQuotes) {
            count++;
            current++;
        }
        
        while ((start + count < lastByte)) {
            if (*(start + count) == delimiter) {
                if (inQuotes) { // We're delimited by quotes, which means we may have more quotes that delimit even more quotes
                    std::size_t currentAppend = 1;
                    if (start + count + 1 < lastByte) {
                        if (*(start + count + 1) == '\n') { // The line ends with \n. Skip one char.
                            currentAppend += 1;
                        } else if (*(start + count + 1) == '\r') {
                            if ((start + count + 2 < lastByte) && (*(start + count + 2) == '\n')) { // The line ends with \r\n. Skip two chars
                                currentAppend += 2;
                            } else {
                                // \r without an accompanying \n? Probably impossible, unless the source string was created by hand. No clue
                                // what to do. Bail.
                                return {LocalizationCSVParser::Result::UnknownError, std::string_view()};
                            }
                        } else {
                            // Seems that we're going through double quotes that the translator added in the translation string. Keep advancing
                            // until we're out.
                            count++;
                            current++;
                            quoteCount++;
                            continue;
                        }
                    }
                    
                    // Skip the delimiting double quotes and newline
                    start++;
                    count--;
                    current += currentAppend;
                    
                    break;
                } else { // No quotes. Just deal with the newline
                    // Make sure carriage returns don't end up in the translated string
                    if (*(start + count - 1) == '\r') {
                        count--;
                    }
                    
                    // Move away from the current newline character
                    current++;
                    
                    break;
                }
            }
            
            count++;
            current++;
        }
    } else if (columnID == 0 || columnID == 1) {
        while ((start + count < lastByte) && (*(start + count) != ',') && (*(start + count) != ';')) {
            char c = *(start + count);
            if (c == '\"' || c == '\r' || c == '\n' || c == '\t') {
                if (columnID == 0) {
                    return {LocalizationCSVParser::Result::InvalidCharacterInKey, std::string_view()};
                } else {
                    return {LocalizationCSVParser::Result::InvalidCharacterInNamespace, std::string_view()};
                }
            }
            
            count++;
            current++;
        }
        
        if (count > maxLength) {
            if (columnID == 0) {
                return {LocalizationCSVParser::Result::TooManyBytesInKey, std::string_view()};
            } else {
                return {LocalizationCSVParser::Result::TooManyBytesInNamespace, std::string_view()};
            }
        }
        
        if (columnID == 0 && count == 0) {
            return {LocalizationCSVParser::Result::KeyEmpty, std::string_view()};
        }
        
        if ((*(start + count) != ',') && (*(start + count) != ';')) {
            return {LocalizationCSVParser::Result::ColumnMissing, std::string_view()};
        }
        
        // Move away from the current delimiter character
        current++;
    } else {
        assert(0);
    }
    
    if (quoteCount != 0) {
        assert(columnID == 2);
        
        const std::string_view tempView(start, count);
        
        std::string finalString;
        finalString.reserve(count + 1);
        
        for (std::size_t i = 0; i < count; ++i) {
            if ((tempView[i] == '\"') && (i + 1 < count) && (tempView[i + 1] == '\"')) {
                // Quotes eat each other and only one remains
                finalString += '\"';
                i++;
                continue;
            } else {
                finalString += tempView[i];
            }
        }
        
        return {LocalizationCSVParser::Result::Success, std::move(finalString)};
    } else {
        return {LocalizationCSVParser::Result::Success, std::string_view(start, count)};
    }
    
}

std::pair<LocalizationCSVParser::Result, std::size_t> LocalizationCSVParser::parse(const char* bytes, std::size_t length, std::vector<CSVRow>& parsedRows) const {
    if (bytes == nullptr) {
        LOG_E("The bytes pointer passed to LocalizationCSVParser can't be nullptr");
        return {Result::NullPointer, 0};
    }
    
    std::size_t counter = 0;
    std::size_t rowNumber = 0;
    
    while (counter < length) {
        CSVRow row;
        
        auto result = extractColumn(bytes, bytes + length, 128, counter, 0);
        if (result.first != Result::Success) {
            return {result.first, rowNumber};
        } else {
            assert(result.second.index() == 0);
            row.key = std::get<std::string_view>(result.second);
        }
        
        result = extractColumn(bytes, bytes + length, 128, counter, 1);
        if (result.first != Result::Success) {
            return {result.first, rowNumber};
        } else {
            assert(result.second.index() == 0);
            row.stringNamespace = std::get<std::string_view>(result.second);
        }
        
        result = extractColumn(bytes, bytes + length, -1, counter, 2);
        if (result.first != Result::Success) {
            return {result.first, rowNumber};
        } else {
            row.value = result.second;
        }
        
        parsedRows.push_back(std::move(row));
        rowNumber++;
    }
    
    return {Result::Success, rowNumber};
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
