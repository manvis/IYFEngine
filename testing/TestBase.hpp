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

#ifndef IYF_TEST_BASE_HPP
#define IYF_TEST_BASE_HPP

#include <string>
#include "core/Logger.hpp"

namespace iyf::test {

class TestResults {
public:
    TestResults(bool success, std::string notes) : notes(std::move(notes)), success(success) {}
    
    bool isSuccessful() const {
        return success;
    }
    
    const std::string& getNotes() const {
        return notes;
    }
private:
    std::string notes;
    bool success;
};

class TestBase {
public:
    /// Test constuctors shuold not perform initialization. It should be delayed until initialize() is called because not all tests
    /// may get to run.
    /// 
    /// \param [in] verboseOutput Indicates that the test should output additional data to LOG_ macros. This may affect performance.
    TestBase(bool verboseOutput) : verbose(verboseOutput) {}
    virtual ~TestBase() {}
    
    /// Returns a unique name for the test(s) performed by the deriving class
    virtual std::string getName() const = 0;
    
    /// Initialize the test(s)
    virtual void initialize() = 0;
    
    /// Execute the test(s)
    virtual TestResults run() = 0;
    
    /// Clean up data. Should destroy what initialize() created
    virtual void cleanup() = 0;
    
    inline void log(const std::string& output) {
        if (!verbose) {
            LOG_D("{}", output);
        }
    }
    inline bool isOutputVerbose() {
        return verbose;
    }
private:
    bool verbose;
};

}

#endif // IYF_TEST_BASE_HPP
