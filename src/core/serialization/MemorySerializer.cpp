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

#include "core/serialization/MemorySerializer.hpp"

#include <stdexcept>

namespace iyf {
void MemorySerializer::reserve(std::size_t newCapacity) {
    if (!ownsMemory) {
        throw SerializerException("Cannot call reserve on a MemorySerializer that does not own the memory.");
    }
    
    if (newCapacity > bufferCapacity) {
        bufferCapacity = newCapacity;
        
        char* temp = new char[bufferCapacity];
        if (temp == nullptr) {
            throw SerializerException("Memory allocation failed.");
        }
        
        if (buffer != nullptr) {
            std::memcpy(temp, buffer, bufferSize);
            
            delete[] buffer;
        }
        
        buffer = temp;
    }
}

std::int64_t MemorySerializer::writeString(const char* string, std::size_t length, StringLengthIndicator indicator) {
    // This is an almost unchanged copy from File::writeString
    bool result;
    int lengthIncrement;
    
    switch (indicator) {
        case StringLengthIndicator::None:
            result = true;
            lengthIncrement = 0;
            break;
        case StringLengthIndicator::UInt8:
            result = writeUInt8(static_cast<std::uint8_t>(length));
            lengthIncrement = 1;
            break;
        case StringLengthIndicator::UInt16:
            result = writeUInt16(static_cast<std::uint16_t>(length));
            lengthIncrement = 2;
            break;
        case StringLengthIndicator::UInt32:
            result = writeUInt32(static_cast<std::uint32_t>(length));
            lengthIncrement = 4;
            break;
        case StringLengthIndicator::UInt64:
            result = writeUInt64(static_cast<std::uint64_t>(length));
            lengthIncrement = 8;
            break;
    }
    
    if (!result) {
        throw SerializerException("Failed to serialize a string length indicator to memory");
    }
    
    return writeBytes(string, sizeof(char) * length) + lengthIncrement;
}

std::int64_t MemorySerializer::writeString(const std::string& string, StringLengthIndicator indicator) {
    return writeString(string.data(), string.length(), indicator);
}

std::int64_t MemorySerializer::writeString(const std::string_view& stringView, StringLengthIndicator indicator) {
    return writeString(stringView.data(), stringView.length(), indicator);
}

std::int64_t MemorySerializer::readString(std::string& string, StringLengthIndicator indicator, std::uint64_t count) {
    // This is an almost unchanged copy from File::readString
    std::uint64_t bytesToRead;
    int lengthIncrement;
    
    switch (indicator) {
        case StringLengthIndicator::None:
            bytesToRead = count;
            lengthIncrement = 0;
            break;
        case StringLengthIndicator::UInt8:
            bytesToRead = readUInt8();
            lengthIncrement = 1;
            break;
        case StringLengthIndicator::UInt16:
            bytesToRead = readUInt16();
            lengthIncrement = 2;
            break;
        case StringLengthIndicator::UInt32:
            bytesToRead = readUInt32();
            lengthIncrement = 4;
            break;
        case StringLengthIndicator::UInt64:
            bytesToRead = readUInt64();
            lengthIncrement = 8;
            break;
    }
    
    char* buffer = new char[bytesToRead];
    
    std::int64_t readByteCount = readBytes(buffer, bytesToRead);
    
    if (readByteCount == -1 || static_cast<std::uint32_t>(readByteCount) != bytesToRead) {
        throw SerializerException("Failed read a serialized string from memory");
    }
    
    string.append(buffer, bytesToRead);
    
    delete[] buffer;
    
    return readByteCount + lengthIncrement;
}
}
