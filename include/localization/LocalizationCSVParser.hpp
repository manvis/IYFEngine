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

#ifndef IYF_LOCALIZATION_CSV_PARSER_HPP
#define IYF_LOCALIZATION_CSV_PARSER_HPP

#include <vector>
#include <variant>
#include <string>
#include <string_view>

namespace iyf {
struct CSVRow {
    std::string_view key;
    std::string_view stringNamespace;
    /// Using a variant because it's impossible to escape quotes within the string view
    std::variant<std::string_view, std::string> value;
    
    inline std::string_view getValue() const {
        if (value.index() == 0) {
            return std::get<std::string_view>(value);
        } else {
            // strings can be implicitly converted to string views
            return std::get<std::string>(value);
        }
    }
};

class LocalizationCSVParser {
public:
    enum class Result {
        UnknownError,
        NullPointer,
        TooManyBytesInKey,
        TooManyBytesInNamespace,
        InvalidCharacterInKey,
        InvalidCharacterInNamespace,
        KeyEmpty,
        ColumnMissing,
        Success
    };
    
    LocalizationCSVParser() {}
    
    /// Parses a preloaded CSV file that contains localized strings. All such files must conform to certain specific rules
    /// that are defined below.
    ///
    /// 0. Each file must contain 3 columns. The first column is the key, the second is an optional namespace, the third is
    /// the translated string.
    /// 1. The files must use the UTF-8 encoding **WITHOUT A BOM**.
    /// 2. The key, namespace and translated string columns must be separated by commas (0x2c) or semicolons (0x3b). Semicolons
    /// are an Excel specific quirk on many locales. They will parse successfully, but commas are preferred.
    /// 3. The length of the key must be between 1 and 128 **bytes**. That is, be careful when using mutibyte UTF-8 characters
    /// (e.g. CJK).
    /// 4. The length of the namespace must be between 0 (since it's optional) and 128 **bytes**.
    /// 5. The key and the namespace **MUST NOT** conain newlines (0x0a), carriage returns (0x0d), horizontal tabs (0x09) or double
    /// quotes (0x22). Likewise, they shouldn't contain commas (0x2c) or semicolons (0x3b) because they will be interpreted as
    /// column delimiters.
    /// 6. The translated string column must be followed by a newline (0x0a) or a carriage return and a newline (0x0d 0x0a). 
    /// Those symbols indicate the end of the row.
    /// 7. If the translated string contains newlines (0x0a), carriage returns (0x0d), double quotes (0x22) or the delimiters that
    /// were used to separate the columns of this line (either semicolons (0x3b) or commas (0x2c)), it must be delimited by double
    /// quotes (0x22). The first double quote goes directly after the separator that separated the namespace from the translated
    /// string. The second one goes right before the newline (0x0a) or a carriage return and a newline (0x0d 0x0a) that indicate the
    /// end of the row.
    /// 8. The translated string may contain double quotes (0x22). However, each double quote chatacter must be immediately followed
    /// by an another double quote chatacter (0x22), e.g., this is a full line: "Start ""quoted name "" end"
    ///
    /// \return a pair of the result and a number. If the result is Success, it will be the number of rows that were parsed successfully
    /// if result reported a failure, you'll obtain the number of the row where the error occurded.
    std::pair<Result, std::size_t> parse(const char* bytes, std::size_t length, std::vector<CSVRow>& parsedRows) const;
    
    std::string resultToErrorString(Result result) const;
};

}

#endif // IYF_LOCALIZATION_CSV_PARSER_HPP
