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

#ifndef IYF_CSV_PARSER_TESTS_HPP
#define IYF_CSV_PARSER_TESTS_HPP

#include "TestBase.hpp"
#include "localization/LocalizationCSVParser.hpp"

#include <vector>
#include <string>

namespace iyf::test {
    
class CSVParserTests : public TestBase {
public:
    CSVParserTests(bool verbose);
    virtual ~CSVParserTests();
    
    virtual std::string getName() const final override {
        return "CSV parser tests";
    }
    
    virtual void initialize() final override;
    virtual TestResults run() final override;
    virtual void cleanup() final override;
private:
    struct ExpectedValues {
        ExpectedValues(std::string key, std::string stringNamespace, std::string value) 
            : key(key), stringNamespace(stringNamespace), value(value) {}
        
        std::string key;
        std::string stringNamespace;
        std::string value;
    };
    
    struct ParseTest {
        ParseTest(std::string csv, LocalizationCSVParser::Result expectedResult) 
            : csv(std::move(csv)), expectedResult(expectedResult) {}
        
        std::string csv;
        LocalizationCSVParser::Result expectedResult;
        std::vector<ExpectedValues> expectedValues;
    };
    
    std::vector<ParseTest> CSVs;
};

}

#endif // IYF_CSV_PARSER_TESTS_HPP
