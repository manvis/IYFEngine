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

#ifndef IYF_MEMORY_SERIALIZER_TESTS_HPP
#define IYF_MEMORY_SERIALIZER_TESTS_HPP

#include "TestBase.hpp"

#include <array>

namespace iyf {
class MemorySerializer;
}

namespace iyf::test {

class MemorySerializerTests : public TestBase {
public:
    MemorySerializerTests(bool verbose);
    virtual ~MemorySerializerTests();
    
    virtual std::string getName() const final override {
        return "Memory serializer";
    }
    
    virtual void initialize() final override;
    virtual TestResults run() final override;
    virtual void cleanup() final override;
private:
    void printCompareDataBuffers(const char* bufferA, const char* bufferB, std::size_t count);
    TestResults checkSizePositionCapacity(const MemorySerializer& ms, std::size_t size, std::int64_t position, std::size_t capacity);
    std::array<char, 34> bufferAfterIntWrites;
    std::array<char, 45> bufferAfterStringWrites;
};
    
}

#endif // IYF_MEMORY_SERIALIZER_TESTS_HPP
