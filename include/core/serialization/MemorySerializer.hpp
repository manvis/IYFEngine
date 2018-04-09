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

#ifndef IYF_MEMORY_SERIALIZER_HPP
#define IYF_MEMORY_SERIALIZER_HPP

#include "core/serialization/Serializer.hpp"

#include <SDL2/SDL_endian.h>
#include <cstring>
#include <cassert>
#include <memory>

namespace iyf {
/// A serializer that serializes or deserializes data to or from a memory buffer
class MemorySerializer : public Serializer {
public:
    static constexpr float CapacityGrowthMultiplier = 2.0f;
    
    /// Create a memory serializer in ReadAndWrite mode and explicitly transfer the ownership of a memory buffer to it. A serializer created with this 
    /// constructor owns the memory and will resize the internal buffer automatically to accomodate new writes (like an std::vector would).
    MemorySerializer(std::unique_ptr<char[]> buffer, std::size_t size) : Serializer(Serializer::OpenMode::ReadAndWrite), bufferSize(size), bufferCapacity(size), position(0), ownsMemory(true) {
        this->buffer = buffer.release();
    }
    
    /// Create a memory serializer in ReadAndWrite mode. A serializer created with this constructor does not own the memory. Calling reserve() or using a write
    /// operation that would trigger a resize will throw SerializerException
    MemorySerializer(char* buffer, std::size_t size) : Serializer(Serializer::OpenMode::ReadAndWrite), bufferSize(size), bufferCapacity(size), position(0), buffer(buffer), ownsMemory(false) {}
    
    /// Create a memory serializer in ReadAndWrite mode. A serializer created with this consturctor owns the memory and will resize the internal buffer automatically
    /// to accomodate new writes (like an std::vector would).
    MemorySerializer(std::size_t capacity) : Serializer(Serializer::OpenMode::ReadAndWrite), bufferSize(0), bufferCapacity(0), position(0), buffer(nullptr), ownsMemory(true) {
        reserve(capacity);
    }
    
    virtual ~MemorySerializer() {
        if (ownsMemory) {
            delete[] buffer;
        }
    }
    
    const char* data() const {
        return buffer;
    }
    
    void reserve(std::size_t newCapacity);
    
    std::size_t size() const {
        return bufferSize;
    }
    
    std::size_t capacity() const {
        return bufferCapacity;
    }
    
    virtual bool isEnd() final override {
        assert(position <= bufferSize);
        
        return (position == bufferSize);
    }
    
    virtual std::int64_t seek(std::int64_t offset) final override {
        if (offset < 0 || offset > static_cast<std::int64_t>(bufferSize)) {
            throw SerializerException("Tried to seek out of bounds");
        }
        
        position = offset;
        
        return offset;
    }
    
    virtual std::int64_t tell() const final override {
        return position;
    }
    
    // Write methods ----------------------------
    
    virtual std::int64_t writeString(const std::string& string, StringLengthIndicator indicator) final override;
    
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) final override {
        std::size_t newPosition = position + count;
        
        if (newPosition > bufferCapacity) {
            reserve(bufferCapacity * CapacityGrowthMultiplier);
        }
        
        if (newPosition > bufferSize) {
            bufferSize = newPosition;
        }
        
        std::memcpy(buffer + position, bytes, count);
        
        position = newPosition;
        
        return count;
    }
    
    virtual bool writeInt8(std::int8_t val) final override {
        return writeBytes(&val, 1) == 1;
    }
    
    virtual bool writeUInt8(std::uint8_t val) final override {
        return writeBytes(&val, 1) == 1;
    }
    
    virtual bool writeInt16(std::int16_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap16(val);
        #endif
        
        return writeBytes(&val, 2) == 2;
    }
    
    virtual bool writeUInt16(std::uint16_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap16(val);
        #endif
        
        return writeBytes(&val, 2) == 2;
    }
    
    virtual bool writeInt32(std::int32_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap32(val);
        #endif
        
        return writeBytes(&val, 4) == 4;
    }
    
    virtual bool writeUInt32(std::uint32_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap32(val);
        #endif
        
        return writeBytes(&val, 4) == 4;
    }
    
    virtual bool writeInt64(std::int64_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap64(val);
        #endif
        
        return writeBytes(&val, 8) == 8;
    }
    
    virtual bool writeUInt64(std::uint64_t val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_Swap64(val);
        #endif
        
        return writeBytes(&val, 8) == 8;
    }
    
    virtual bool writeFloat(float val) final override {
        #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            val = SDL_SwapFloat(val);
        #endif
        
        return writeBytes(&val, 4) == 4;
    }
    
    // Read methods ------------------------------
    
    virtual std::int64_t readString(std::string& string, StringLengthIndicator indicator, std::uint64_t count) final override;
    
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) final override {
        std::size_t newPosition = position + count;
        std::size_t bytesToRead;
        
        if (newPosition > bufferSize) {
            bytesToRead = bufferSize - position;
            std::memcpy(bytes, buffer + position, bytesToRead);
            
            position = bufferSize;
        } else {
            bytesToRead = count;
            std::memcpy(bytes, buffer + position, bytesToRead);
            
            position = newPosition;
        }
        
        return bytesToRead;
    }
    
    virtual std::int8_t readInt8() final override {
        return numberRead<std::int8_t>();
    }
    
    virtual std::uint8_t readUInt8() final override {
        return numberRead<std::uint8_t>();
    }
    
    virtual std::int16_t readInt16() final override {
        return SDL_SwapLE16(numberRead<std::int16_t>());
    }
    
    virtual std::uint16_t readUInt16() final override {
        return SDL_SwapLE16(numberRead<std::uint16_t>());
    }
    
    virtual std::int32_t readInt32() final override {
        return SDL_SwapLE32(numberRead<std::int32_t>());
    }
    
    virtual std::uint32_t readUInt32() final override {
        return SDL_SwapLE32(numberRead<std::uint32_t>());
    }
    
    virtual std::int64_t readInt64() final override {
        return SDL_SwapLE64(numberRead<std::int64_t>());
    }
    
    virtual std::uint64_t readUInt64() final override {
        return SDL_SwapLE64(numberRead<std::uint64_t>());
    }
    
    virtual float readFloat() final override {
        return SDL_SwapFloatLE(numberRead<float>());
    }
private:
    template <typename T>
    T numberRead() {
        T value;
        std::size_t valueSize = sizeof(T);
        
        if (readBytes(&value, valueSize) != static_cast<std::int64_t>(valueSize)) {
            throw SerializerException("Failed to read a " + std::to_string(valueSize) + " byte value from the memory buffer");
        }
        
        return value;
    }
    
    std::size_t bufferSize;
    std::size_t bufferCapacity;
    std::size_t position;
    char* buffer;
    bool ownsMemory;
};
}

#endif // IYF_MEMORY_SERIALIZER_HPP
