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

#include "MemorySerializerTests.hpp"

#include "core/serialization/MemorySerializer.hpp"

#define SPC_TEST(x, s, p, c) { \
    auto spc = checkSizePositionCapacity(x, s, p, c);  \
    if (!spc.isSuccessful()) { \
        return spc; \
    } \
}

#define WC_TEST(x, y) { \
    if (!x) { \
        return TestResults(false, "Failed to write " y); \
    } \
}

#define RD_TEST(x, y, z) { \
    if (x != y) { \
         LOG_V("Failed to read " z ". Expected " + std::to_string(y) + " got " + std::to_string(x));\
    } \
}

namespace iyf::test {
static const std::uint8_t uint8Val = 200;
static const std::int8_t int8Val = 120;
static const std::uint16_t uint16Val = 65000;
static const std::int16_t int16Val = 15000;
static const std::uint32_t uint32Val = 1000000;
static const std::int32_t int32Val = 1000000;
static const std::uint64_t uint64Val = 9999000000ULL;
static const std::int64_t int64Val = 1580000000ULL;
static const float floatVal = 168.9985f;
static const std::array<char, 16> CharArray = {"This is a test!"};
static const std::string TestString(CharArray.data());

MemorySerializerTests::MemorySerializerTests(bool verbose) : TestBase(verbose) {}
MemorySerializerTests::~MemorySerializerTests() {}


void MemorySerializerTests::printCompareDataBuffers(const char* bufferA, const char* bufferB, std::size_t count) {
    std::stringstream ss;
    
    for (std::size_t i = 0; i < count; ++i) {
        ss << static_cast<std::int64_t>(*(bufferA + i)) << " " << static_cast<std::int64_t>(*(bufferB + i)) << "\n\t";
    }
    
    LOG_V(ss.str())
}

void MemorySerializerTests::initialize() {
    char* data = bufferAfterIntWrites.data();
    
    std::memcpy(data, &uint8Val, 1);
    std::memcpy(data + 1, &int8Val, 1);
    
    // The serializers store data in little endian order. We need to flip the integer byte order
    // before we can build the test array
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        std::uint16_t uint16ValCor = SDL_Swap16(uint16Val);
        std::int16_t int16ValCor = SDL_Swap16(int16Val);
        std::uint32_t uint32ValCor = SDL_Swap32(uint32Val);
        std::int32_t int32ValCor = SDL_Swap32(int32Val);
        std::uint64_t uint64ValCor = SDL_Swap64(uint64Val);
        std::int64_t int64ValCor = SDL_Swap64(int64Val);
        float floatValCor = SDL_SwapFloat(floatVal);
        
        std::int16_t lenCor = SDL_Swap16(15);
    #else
        std::uint16_t uint16ValCor = uint16Val;
        std::int16_t int16ValCor = int16Val;
        std::uint32_t uint32ValCor = uint32Val;
        std::int32_t int32ValCor = int32Val;
        std::uint64_t uint64ValCor = uint64Val;
        std::int64_t int64ValCor = int64Val;
        float floatValCor = floatVal;
        
        std::int16_t lenCor = 15;
    #endif
    
    std::memcpy(data + 2, &uint16ValCor, 2);
    std::memcpy(data + 4, &int16ValCor, 2);
    std::memcpy(data + 6, &uint32ValCor, 4);
    std::memcpy(data + 10, &int32ValCor, 4);
    std::memcpy(data + 14, &uint64ValCor, 8);
    std::memcpy(data + 22, &int64ValCor, 8);
    std::memcpy(data + 30, &floatValCor, 4);
    
    char* data2 = bufferAfterStringWrites.data();
    std::memcpy(data2, data, 28);
    std::memcpy(data2 + 28, &lenCor, 2);
    std::memcpy(data2 + 30, CharArray.data(), 15);
}
void MemorySerializerTests::cleanup() {}

TestResults MemorySerializerTests::checkSizePositionCapacity(const MemorySerializer& ms, std::size_t size, std::int64_t position, std::size_t capacity) {
    if (ms.size() != size) {
        std::stringstream ss;
        ss << "Invalid size. Expected: " << size << ", was: " << ms.size();
        return TestResults(false, ss.str());
    }
    
    if (ms.tell() != position) {
        std::stringstream ss;
        ss << "Invalid position. Expected: " << position << ", was: " << ms.tell();
        return TestResults(false, ss.str());
    }
    
    if (ms.capacity() != capacity) {
        std::stringstream ss;
        ss << "Invalid capacity. Expected: " << capacity << ", was: " << ms.capacity();
        return TestResults(false, ss.str());
    }
    
    return TestResults(true, "");
}

TestResults MemorySerializerTests::run() {
    static_assert(MemorySerializer::CapacityGrowthMultiplier == 2, "This test was prepared for a multiplier that == 2");
    std::size_t expectedCapacity = 16;
    
    MemorySerializer ms(expectedCapacity);
    SPC_TEST(ms, 0, 0, expectedCapacity);
    
    WC_TEST(ms.writeUInt8(uint8Val), "uint8");
    SPC_TEST(ms, 1, 1, expectedCapacity);
    
    WC_TEST(ms.writeInt8(int8Val), "int8");
    SPC_TEST(ms, 2, 2, expectedCapacity);
    
    WC_TEST(ms.writeUInt16(uint16Val), "uint16");
    SPC_TEST(ms, 4, 4, expectedCapacity);
    
    WC_TEST(ms.writeInt16(int16Val), "int16");
    SPC_TEST(ms, 6, 6, expectedCapacity);
    
    WC_TEST(ms.writeUInt32(uint32Val), "uint32");
    SPC_TEST(ms, 10, 10, expectedCapacity);
    
    WC_TEST(ms.writeInt32(int32Val), "int32");
    SPC_TEST(ms, 14, 14, expectedCapacity);
    
    // Growth should happen here
    expectedCapacity = MemorySerializer::CapacityGrowthMultiplier * expectedCapacity;
    
    WC_TEST(ms.writeUInt64(uint64Val), "uint64");
    SPC_TEST(ms, 22, 22, expectedCapacity);
    
    WC_TEST(ms.writeInt64(int64Val), "int64");
    SPC_TEST(ms, 30, 30, expectedCapacity);
    
    // Growth should happen here
    expectedCapacity = MemorySerializer::CapacityGrowthMultiplier * expectedCapacity;
    
    WC_TEST(ms.writeFloat(floatVal), "float");
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    if (isOutputVerbose()) {
        printCompareDataBuffers(ms.data(), bufferAfterIntWrites.data(), 34);
    }
    
    // Compare the current contents with expected contents
    if (std::memcmp(ms.data(), bufferAfterIntWrites.data(), 34) != 0) {
        return TestResults(false, "Buffer written by the serializer does not match the buffer built by hand.");
    }
    
    // Check if we're currently at the end (we should be, since we did not seek)
    if (!ms.isEnd()) {
        return TestResults(false, "Failed to report the end of buffer.");
    }
    
    // Try to seek before start
    bool exceptionCaught = false;
    try {
        ms.seek(-10);
    } catch (SerializerException& se) {
        exceptionCaught = true;
    }
    
    if (!exceptionCaught) {
        return TestResults(false, "An exception wasn't thrown when seeking before start.");
    }
    
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    // Try to seek after end
    exceptionCaught = false;
    try {
        ms.seek(35);
    } catch (SerializerException& se) {
        exceptionCaught = true;
    }
    
    if (!exceptionCaught) {
        return TestResults(false, "An exception wasn't thrown when seeking after end.");
    }
    
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    // Reset to start
    std::int64_t seekPos = 0;
    if (ms.seek(seekPos) != seekPos) {
        return TestResults(false, "Failed to seek to a specified position.");
    }
    
    SPC_TEST(ms, 34, 0, expectedCapacity);
    
//     std::uint8_t a = ms.readUInt8();
//     if (a != uint8Val) {
//         return TestResults(false, "Failed to read uint8");
//     }
//     
//     std::int8_t b = ms.readInt8();
//     if (b != int8Val) {
//         return TestResults(false, "Failed to read int8");
//     }
    
    RD_TEST(ms.readUInt8(), uint8Val, "uint8_t");
    SPC_TEST(ms, 34, 1, expectedCapacity);
    
    RD_TEST(ms.readInt8(), int8Val, "int8_t");
    SPC_TEST(ms, 34, 2, expectedCapacity);
    
    RD_TEST(ms.readUInt16(), uint16Val, "uint16");
    SPC_TEST(ms, 34, 4, expectedCapacity);
    
    RD_TEST(ms.readInt16(), int16Val, "int16");
    SPC_TEST(ms, 34, 6, expectedCapacity);
    
    RD_TEST(ms.readUInt32(), uint32Val, "uint32");
    SPC_TEST(ms, 34, 10, expectedCapacity);
    
    RD_TEST(ms.readInt32(), int32Val, "int32");
    SPC_TEST(ms, 34, 14, expectedCapacity);
    
    RD_TEST(ms.readUInt64(), uint64Val, "uint64");
    SPC_TEST(ms, 34, 22, expectedCapacity);
    
    RD_TEST(ms.readInt64(), int64Val, "int64");
    SPC_TEST(ms, 34, 30, expectedCapacity);
    
    RD_TEST(ms.readFloat(), floatVal, "float");
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    // Check if an exception is thrown when trying to perform an out of bounds read
    exceptionCaught = false;
    try {
        ms.readInt8();
    } catch (SerializerException& se) {
        exceptionCaught = true;
    }
    
    if (!exceptionCaught) {
        return TestResults(false, "An exception wasn't thrown when trying to read past the end.");
    }
    
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    std::array<char, 8> tempBuff;
    tempBuff.fill(0);
    
    std::int64_t readCount = ms.readBytes(tempBuff.data(), 8);
    if (readCount != 0) {
        return TestResults(false, "We were allowed to read past the end.");
    }
    
    for (const char& c : tempBuff) {
        if (c != 0) {
            return TestResults(false, "An invalid read modified the destination buffer");
        }
    }
    
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    std::int64_t posMinus4 = ms.seek(ms.size() - 4);
    if (posMinus4 != 30) {
        return TestResults(false, "Incorrect return value of the seek function.");
    }
    
    SPC_TEST(ms, 34, 30, expectedCapacity);
    
    readCount = ms.readBytes(tempBuff.data(), 8);
    if (readCount != 4) {
        return TestResults(false, "A partial read returned more data than expected.");
    }
    SPC_TEST(ms, 34, 34, expectedCapacity);
    
    for (std::size_t i = 4; i < tempBuff.size(); ++i) {
        if (tempBuff[i] != 0) {
            return TestResults(false, "A partial read modified too many bytes of the destination buffer");
        }
    }
    
    if (isOutputVerbose()) {
        printCompareDataBuffers(bufferAfterIntWrites.data() + 30, tempBuff.data(), 4);
    }
    
    if (std::memcmp(bufferAfterIntWrites.data() + 30, tempBuff.data(), 4) != 0) {
        return TestResults(false, "The bytes returned by a partial read did not match what was expected.");
    }
    
    ms.seek(28);
    
    std::int64_t strLength = ms.writeString(TestString, StringLengthIndicator::UInt16);
    if (strLength != 15 + 2) { // 15 bytes of string data (null terminator is not written) + 2 bytes of StringLengthIndicator
        return TestResults(false, "String writing method did not write the expected number of bytes.");
    }
    SPC_TEST(ms, 45, 45, expectedCapacity);
    
    if (std::memcmp(ms.data(), bufferAfterStringWrites.data(), 45)) {
        return TestResults(false, "The contents of the buffer do not match what's expected after a string write.");
    }
    
    std::string tempStr;
    ms.seek(28);
    SPC_TEST(ms, 45, 28, expectedCapacity);
    
    readCount = ms.readString(tempStr, StringLengthIndicator::UInt16, 0); // Count is ignored if an indicator is present
    
    if (readCount != 15 + 2) {
        return TestResults(false, "String reading method read an unexpected number of bytes (" + std::to_string(readCount) + ") from the buffer.");
    }
    SPC_TEST(ms, 45, 45, expectedCapacity);
    
    if (tempStr != TestString) {
        return TestResults(false, "The string that was read did not match the string that was written");
    }
    
    /// The serializer didn't know how to resize multiple times before this
    MemorySerializer ms2(2);
    ms2.writeUInt64(65);
    SPC_TEST(ms2, 8, 8, 8);
    
    
    return TestResults(true, "");
}
}
