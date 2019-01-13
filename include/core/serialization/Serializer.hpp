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

#ifndef IYF_SERIALIZER_HPP
#define IYF_SERIALIZER_HPP

#include <cstdint>
#include <string>

#include "core/StringLengthIndicator.hpp"
#include "core/exceptions/SerializerException.hpp"
#include "utilities/NonCopyable.hpp"

namespace iyf {

/// \warning All serializers output data in little endian format
class Serializer : private NonCopyable {
public:
    enum class OpenMode {
        Read,
        Write,
        Append,
        ReadAndWrite
    };
    
    Serializer(OpenMode mode) : mode(mode) {}
    virtual ~Serializer() {}
    
    inline OpenMode getMode() const {
        return mode;
    }
    
    virtual bool isEnd() = 0;
    virtual std::int64_t seek(std::int64_t offset) = 0;
    virtual std::int64_t tell() const = 0;
    
    // Write methods ----------------------------
    
    virtual std::int64_t writeString(const std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None) = 0;
    virtual std::int64_t writeString(const std::string_view& stringView, StringLengthIndicator indicator = StringLengthIndicator::None) = 0;
    virtual std::int64_t writeString(const char* string, std::size_t length, StringLengthIndicator indicator = StringLengthIndicator::None) = 0;
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) = 0;
    virtual bool writeInt8(std::int8_t val) = 0;
    virtual bool writeUInt8(std::uint8_t val) = 0;
    virtual bool writeInt16(std::int16_t val) = 0;
    virtual bool writeUInt16(std::uint16_t val) = 0;
    virtual bool writeInt32(std::int32_t val) = 0;
    virtual bool writeUInt32(std::uint32_t val) = 0;
    virtual bool writeInt64(std::int64_t val) = 0;
    virtual bool writeUInt64(std::uint64_t val) = 0;
    virtual bool writeFloat(float val) = 0;
    virtual bool writeDouble(double val) = 0;
    
    // Read methods ------------------------------
    
    virtual std::int64_t readString(std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None, std::uint64_t count = 0) = 0;
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) = 0;
    virtual std::int8_t readInt8() = 0;
    virtual std::uint8_t readUInt8() = 0;
    virtual std::int16_t readInt16() = 0;
    virtual std::uint16_t readUInt16() = 0;
    virtual std::int32_t readInt32() = 0;
    virtual std::uint32_t readUInt32() = 0;
    virtual std::int64_t readInt64() = 0;
    virtual std::uint64_t readUInt64() = 0;
    virtual float readFloat() = 0;
    virtual double readDouble() = 0;
    
private:
    OpenMode mode;
};

}

#endif // IYF_SERIALIZER_HPP
