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

namespace iyf {
// enum class LineState {
//     Key,
//     Namespace,
//     Value
// };

inline bool extractColumn(const char* bytes, std::size_t length, std::size_t maxLength, std::size_t& current, std::string_view& view) {
    const char* start = bytes + current;
    
    if (start[0] == '\t') {
        current++;
        view = std::string_view(start, 0);
    }
    
    const bool inQuotes = (start[0] == '\"');
    
    
    return false;
}

bool LocalizationCSVParser::parse(const char* bytes, std::size_t length, std::vector<CSVLine>& parsedLines) const {
    if (bytes == nullptr) {
        LOG_E("The bytes pointer passed to LocalizationCSVParser can't be nullptr");
        return false;
    }
    
    std::size_t counter = 0;
    while (counter < length) {
        CSVLine line;
        bool result;
        
        result = extractColumn(bytes, length, 128, counter, line.key);
        if (!result) {
            return false;
        }
        
        result = extractColumn(bytes, length, 128, counter, line.stringNamespace);
        if (!result) {
            return false;
        }
        
        result = extractColumn(bytes, length, -1, counter, line.value);
        if (!result) {
            return false;
        }
        
        parsedLines.push_back(std::move(line));
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
    
    return false;
}
}
