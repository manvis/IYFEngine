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

#include "CSVParserTests.hpp"
#include "localization/LocalizationCSVParser.hpp"

namespace iyf::test {
inline std::string printRows(const std::vector<CSVRow>& rows) {
    std::stringstream ss;
            
    if (!rows.empty()) {
        std::size_t rowNumber = 0;
        for (const CSVRow& row : rows) {
            ss << "\n\t\tROW: " << rowNumber;
            ss << "\n\t\t\tKey(" << row.key.length() << "): ";
            ss.write(row.key.data(), row.key.length());
            ss << "\n\t\t\tNamespace(" << row.stringNamespace.length() << "): ";
            ss.write(row.stringNamespace.data(), row.stringNamespace.length());
            ss << "\n\t\t\tText(" << row.getValue().length() << "): ";
            ss.write(row.getValue().data(), row.getValue().length());
            rowNumber++;
        }
    } else {
        ss << "\n\t\tNONE";
    }
    
    return ss.str();
}

CSVParserTests::CSVParserTests(bool verbose) : iyf::test::TestBase(verbose) {}
CSVParserTests::~CSVParserTests() {}

void CSVParserTests::initialize() {
    CSVs.emplace_back("", LocalizationCSVParser::Result::Success);
    
    CSVs.emplace_back(",,Test1", LocalizationCSVParser::Result::KeyEmpty);
    CSVs.emplace_back(",Namespace,Test", LocalizationCSVParser::Result::KeyEmpty);
    CSVs.emplace_back(";;Test1", LocalizationCSVParser::Result::KeyEmpty);
    CSVs.emplace_back(";Namespace;Test", LocalizationCSVParser::Result::KeyEmpty);
    
    CSVs.emplace_back("Key,,Test", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key;;Test", LocalizationCSVParser::Result::Success);
    
    CSVs.emplace_back("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111,namespace,test", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111,namespace,test", LocalizationCSVParser::Result::TooManyBytesInKey);
    CSVs.emplace_back("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111;namespace;test", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111;namespace;test", LocalizationCSVParser::Result::TooManyBytesInKey);
    
    CSVs.emplace_back("Key,Test", LocalizationCSVParser::Result::ColumnMissing);
    CSVs.emplace_back("Key;Test", LocalizationCSVParser::Result::ColumnMissing);
    
    CSVs.emplace_back("Key,,Test\nKey2,,Test2\r\nKey3,Namespace,Test3", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key,,Test\nKey2,,\"Test2\nNewline\"\r\nKey3,NS,\"Another\ntime\"\nKey4,Namespace,Test4", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key,,\"Te\nst\"", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key;;Test\nKey2;;Test2\r\nKey3;Namespace;Test3", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key;;Test\nKey2;;\"Test2\nNewline\"\r\nKey3;NS;\"Another\ntime\"\nKey4;Namespace;Test4", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("Key;;\"Te\nst\"", LocalizationCSVParser::Result::Success);
    
    CSVs.emplace_back("Key;Ultraquotes-:D;\"\"\"\"\"\"\"\"\"\"", LocalizationCSVParser::Result::Success);
    
    // This was created by Google Sheets. The CSV export there didn't have any settings to adjust.
    CSVs.emplace_back("Key1,,Value1\r\nKey2,namespace,\"Value2;\r\nWith a break\"\r\nKey3,,\"Value3, \"\"With quotes\"\"\"\r\nKey4,namespace,Value4\r\nKey5,,\"Va,lue5\"\r\nKey6,namespace2,Value;6\r\nKey7,namespace2,Value7", LocalizationCSVParser::Result::Success);
    
    // This was created by LibreOffice with default settings (, as a field delimiter and " as a text delimiter)
    CSVs.emplace_back("Key1,,Value1\nKey2,namespace,\"Value2;\nWith a break\"\nKey3,,\"Value3, \"\"With quotes\"\"\"\nKey4,namespace,Value4\nKey5,,\"Va,lue5\"\nKey6,namespace2,Value;6\nKey7,namespace2,Value7\n", LocalizationCSVParser::Result::Success);
    
    // This was created by Excel
    CSVs.emplace_back("Key1;;Value1\r\nKey2;namespace;\"Value2;\nWith a break\"\r\nKey3;;\"Value3, \"\"With quotes\"\"\"\r\nKey4;namespace;Value4\r\nKey5;;Va,lue5\r\nKey6;namespace2;\"Value;6\"\r\nKey7;namespace2;Value7", LocalizationCSVParser::Result::Success);
}

TestResults CSVParserTests::run() {
    const LocalizationCSVParser parser;
    
    for (const auto& CSV : CSVs) {
        std::vector<CSVRow> rows;
        
        auto result = parser.parse(CSV.csv.c_str(), CSV.csv.length(), rows);
        
        if (result.first != CSV.expectedResult) {
            std::stringstream ss;
            ss << "When parsing this CSV string:\n--\n" << CSV.csv << "\n--\n\t";
            ss << "expected the parser to return \"" << parser.resultToErrorString(CSV.expectedResult) << "\", it returned \"" 
               << parser.resultToErrorString(result.first) << "\" with row count " << result.second;
            ss << "\n\t\tRows that were parsed successfully: " << printRows(rows);
            
            return TestResults(false, ss.str());
        }
        
        if (isOutputVerbose()) {
            std::string printedRows = printRows(rows);
            
            LOG_V("Parsed the following CSV string:\n--\n" << CSV.csv << "\n--\n\tAs expected, the parser returned \""
                  << parser.resultToErrorString(result.first) << "\"\n\tPARSED ROWS: " << printedRows);
        }
    }
    
    return TestResults(true, "");
}

void CSVParserTests::cleanup() {
    CSVs.clear();
}

}
