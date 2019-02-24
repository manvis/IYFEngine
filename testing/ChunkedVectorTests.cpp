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

#include "ChunkedVectorTests.hpp"
#include "utilities/ChunkedVector.hpp"
#include <atomic>
#include <initializer_list>

namespace iyf::test {
struct TestStruct {
    TestStruct() : id(0), name("Unnamed"), counter(0) {}
    TestStruct(std::uint64_t id, std::string name, std::uint32_t counter) : id(id), name(name), counter(counter) {}
    TestStruct(const TestStruct& other) : id(other.id), name(other.name), counter(other.counter.load()) { }
    TestStruct& operator=(const TestStruct& other) {
        if(&other == this) {
            return *this;
        }
        
        counter = other.counter.load();
        name = other.name;
        id = other.id;
        
        return *this;
    } 
    
    std::uint64_t id;
    std::string name;
    std::atomic<std::uint32_t> counter;
    
    friend bool operator==(const TestStruct& a, const TestStruct& b);
    friend bool operator!=(const TestStruct& a, const TestStruct& b);
};

inline bool operator==(const TestStruct& a, const TestStruct& b) {
    return (a.id == b.id) && (a.name == b.name) && (a.counter == b.counter);
}

inline bool operator!=(const TestStruct& a, const TestStruct& b) {
    return !(a == b);
}

#define IYF_CHECK_CHUNKED_SIZE_CAP(name, vec, vecSize, vecCapacity, vecChunkCount) \
    if (vec.size() != vecSize || vec.capacity() != vecCapacity || vec.chunkCount() != vecChunkCount) {\
        return TestResults(false, fmt::format("TEST {}.\n\tWrong size or capacity.\n\t\t"\
                                              "EXPECTED\n\t\t\tsize: {}\n\t\t\tcapacity: {}\n\t\t\tchunk count: {}\n\t\t"\
                                              "ACTUAL\n\t\t\tsize: {}\n\t\t\tcapacity: {}\n\t\t\tchunk count: {}",\
                                              name, vecSize, vecCapacity, vecChunkCount, vec.size(), vec.capacity(), vec.chunkCount()));\
    }

#define IYF_CHECK_CHUNKED_ITER(name, vec, ...) \
    {\
        const std::initializer_list<TestStruct> valueList = __VA_ARGS__; \
        const std::size_t expectedCount = valueList.size();\
        \
        if (expectedCount != vec.size()) { \
            return TestResults(false, fmt::format("The number of expected values ({}) didn't match the actual size of the chunked vector ({})", expectedCount, vec.size()));\
        }\
        \
        std::size_t iterCount = 0;\
        auto checkBegin = valueList.begin();\
        for (const auto& i : vec) {\
            if (checkBegin == valueList.end()) { \
                return TestResults(false, fmt::format("TEST {}.\n\t\t\tIterator ran past the end ({})", name, iterCount));\
            }\
            \
            if (i != *checkBegin) {\
                return TestResults(false, fmt::format("TEST {}.\n\t\t\tUnexpected value at element {}", name, iterCount));\
            }\
            checkBegin++;\
            iterCount++;\
        }\
    }

ChunkedVectorTests::ChunkedVectorTests(bool verbose) : TestBase(verbose) { }
ChunkedVectorTests::~ChunkedVectorTests() {}

void ChunkedVectorTests::initialize() {}
TestResults ChunkedVectorTests::run() {
    TestStruct t1(16, "T1", 256);
    TestStruct t2(8192, "T2", 4096);
    TestStruct t3(1024, "T3", 512);
    
    ChunkedVector<TestStruct, 5> cv;
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterInit", cv, 0, 0, 0);
    IYF_CHECK_CHUNKED_ITER("AfterInit", cv, {});
    
    cv.reserve(10);
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterReserve", cv, 0, 10, 2);
    IYF_CHECK_CHUNKED_ITER("AfterReserve", cv, {});
    
    cv.resize(3, t1);
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterResize", cv, 3, 10, 2);
    IYF_CHECK_CHUNKED_ITER("AfterResize", cv, {t1, t1, t1});
    
    cv.emplace_back(1024, "T3", 512); // same as t3
    cv.emplace_back(t2);
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterEmplacement", cv, 5, 10, 2);
    IYF_CHECK_CHUNKED_ITER("AfterEmplacement", cv, {t1, t1, t1, t3, t2});
    
    cv.push_back(t2);
    cv.push_back(t3);
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterPushBack", cv, 7, 10, 2);
    IYF_CHECK_CHUNKED_ITER("AfterPushBack", cv, {t1, t1, t1, t3, t2, t2, t3});
    
    cv.emplace_back(t1);
    cv.push_back(t2);
    cv.emplace_back(t3);
    cv.push_back(t1);
    IYF_CHECK_CHUNKED_SIZE_CAP("AfterPushBack", cv, 11, 15, 3);
    IYF_CHECK_CHUNKED_ITER("AfterPushBack", cv, {t1, t1, t1, t3, t2, t2, t3, t1, t2, t3, t1});
    
    if (cv.at(7) != t1) {
        return TestResults(true, "at returned an incorrect value");
    }
    
    if (cv[7] != t1) {
        return TestResults(true, "[] returned an incorrect value");
    }
    
    if (&(cv[7]) != &(cv.at(7))) {
        return TestResults(true, "Addresses for the same value differ");
    }
    
    cv.clear();
    IYF_CHECK_CHUNKED_SIZE_CAP("PostClear", cv, 0, 0, 0);
    
    return TestResults(true, "");
}
void ChunkedVectorTests::cleanup() {}
}
