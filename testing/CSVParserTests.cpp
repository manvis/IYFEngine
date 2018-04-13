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
// These tests check if 
CSVParserTests::CSVParserTests(bool verbose) : iyf::test::TestBase(verbose) {}
CSVParserTests::~CSVParserTests() {}

void CSVParserTests::initialize() {
    CSVs.emplace_back("", LocalizationCSVParser::Result::Success);
    
    CSVs.emplace_back("\t\tTest1", LocalizationCSVParser::Result::KeyEmpty);
    CSVs.emplace_back("\tNamespace\tTest", LocalizationCSVParser::Result::KeyEmpty);
    
    CSVs.emplace_back("Key\t\tTest", LocalizationCSVParser::Result::Success);
    
    CSVs.emplace_back("11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111\tnamespace\ttest", LocalizationCSVParser::Result::Success);
    CSVs.emplace_back("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111\tnamespace\ttest", LocalizationCSVParser::Result::TooManyBytesInKey);
    
    CSVs.emplace_back("Key\tTest", LocalizationCSVParser::Result::ColumnMissing);
    
    CSVs.emplace_back("Key\t\tTest\nKey2\t\tTest2\r\nKey3\tNamespace\tTest3", LocalizationCSVParser::Result::Success);
}

TestResults CSVParserTests::run() {
    const LocalizationCSVParser parser;
    
    for (const auto& CSV : CSVs) {
        std::vector<CSVLine> lines;
        
        auto result = parser.parse(CSV.csv.c_str(), CSV.csv.length(), lines);
        
        if (result.first != CSV.expectedResult) {
            std::stringstream ss;
            ss << "When parsing this CSV string:\n--\n" << CSV.csv << "\n--\n\t";
            ss << "expected the parser to return \"" << parser.resultToErrorString(CSV.expectedResult) << "\", it returned \"" 
               << parser.resultToErrorString(result.first) << "\"";
            
            return TestResults(false, ss.str());
        }
        
        if (isOutputVerbose()) {
            std::stringstream sl;
            
            if (!lines.empty()) {
                std::size_t lineNumber = 0;
                for (const CSVLine& line : lines) {
                    sl << "\n\t\tLINE: " << lineNumber;
                    sl << "\n\t\t\tKey(" << line.key.length() << "): ";
                    sl.write(line.key.data(), line.key.length());
                    sl << "\n\t\t\tNamespace(" << line.stringNamespace.length() << "): ";
                    sl.write(line.stringNamespace.data(), line.stringNamespace.length());
                    sl << "\n\t\t\tText(" << line.value.length() << "): ";
                    sl.write(line.value.data(), line.value.length());
                    lineNumber++;
                }
            } else {
                sl << "\n\t\tNONE";
            }
            
            LOG_V("Parsed the following CSV string:\n--\n" << CSV.csv << "\n--\n\tAs expected, the parser returned \""
                  << parser.resultToErrorString(result.first) << "\"\n\tPARSED LINES: " << sl.str());
        }
    }
    
    return TestResults(true, "");
}

void CSVParserTests::cleanup() {
    CSVs.clear();
}

}
