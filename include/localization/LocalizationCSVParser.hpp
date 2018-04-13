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
#include <string_view>

namespace iyf {
struct CSVRow {
    std::string_view key;
    std::string_view stringNamespace;
    std::string_view value;
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
    /// 1. The files must use the UTF-8 encoding.
    /// 2. The key, namespace and translated string columns must be separated by a tab character.
    /// 3. The length of the key must be between 1 and 128 **bytes**. That is, be careful when using mutibyte UTF-8 characters
    /// (e.g. CJK).
    /// 4. The length of the namespace must be between 0 (since it's optional) and 128 **bytes**.
    /// 5. The key and the namespace **MUST NOT** conain newlines, tabs or regular ASCII double quotes (").
    /// 6. The translated string column must be followed by a newline. It indicates the end of the row.
    /// 7. The translated string may contain tabs or newlines, however, if it does, it must be delimited by double quotes.
    /// The first double quote goes directly after the tab that separated the namespace from the translated string. The second
    /// one goes right before the newline that indicates the end of the row.
    /// 8. The translated string must not contain regular ASCII double quotes ("). Pretty UTF-8 double quotes that are inserted
    /// automatically (usually) by your spreadsheet software are OK.
    /// 
    /// Most of the formatting requirements will be automatically fulfilled by LibreOffice. When saving the csv:
    ///
    /// 1. Set the character set to Unicode (UTF-8)
    /// 2. Set the field delimiter to {Tab}
    /// 3. Set the text delimiter to double quotes ("). Use regular double quotes provided in the drop-down. Do not manually
    /// type in pretty unicode ones.
    /// 4. Check "Save cell content as shown" and uncheck everything else.
    ///
    /// \warning This doesn't work on files exported by excel or Google Docs
    ///
    /// \todo Either improve this parser, tell people (in the docs) to use LibreOffice or build a custom localization file editor 
    /// that would save in the right format
    ///
    /// \return a pair of the result and a number. If the result is Success, it will be the number of rows that were parsed successfully
    /// if result reported a failure, you'll obtain the number of the row where the error occurded.
    std::pair<Result, std::size_t> parse(const char* bytes, std::size_t length, std::vector<CSVRow>& parsedRows) const;
    
    std::string resultToErrorString(Result result) const;
};

}

#endif // IYF_LOCALIZATION_CSV_PARSER_HPP
