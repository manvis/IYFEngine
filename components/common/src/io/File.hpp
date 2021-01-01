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

#ifndef IYF_FILE_HPP
#define IYF_FILE_HPP

#include <string>
#include <cstdint>
#include <memory>

#include "io/exceptions/FileException.hpp"
#include "StringLengthIndicator.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/Endianess.hpp"
#include "io/Path.hpp"
#include "io/FileOpenMode.hpp"

namespace iyf {

/// \warning Files are always little endian, unless you write big endian data using writeBytes()
///
/// \todo This applies both here and (partially) in the serializers. There are 2 kinds of read functions - those that throw and
/// those that return a value. Serializers use the throwing versions. However, write functions only return a value. This is inconsistent,
/// a bit confusing and it requires the end users of these classes to check for write results. I think I should move to error codes
/// like in the filesystem library
class File {
public:
    enum class SeekFrom {
        Start = SEEK_SET,
        Current = SEEK_CUR,
        End = SEEK_END
    };

    virtual ~File();
    
    /// Closes the file. Normallly, the destructor takes care of that. Closing the file and performing any other operations on it
    /// invokes undefined behaviour
    virtual bool close() = 0;
    
    virtual bool flush() = 0;

    /// Seeks to a specified position in file.
    /// \return new position if successful, -1 if an error has occured.
    virtual std::int64_t seek(std::int64_t offset, SeekFrom whence) = 0;
    
    /// \return the current position in the file or -1 if an error occured
    virtual std::int64_t tell() = 0;

    inline const Path& getPath() const {
        return path;
    }

    inline FileOpenMode getOpenMode() const {
        return openMode;
    }
    
    // File read methods --------------------------------------------------------------------------------------
    /// Reads a string from a file and into the provided std::string.
    ///
    /// \param[out] string The string that data will be appended to.
    /// \param[in] indicator The length indicator that the string uses.
    /// \param[in] count Number of bytes to read. Ignored if length is not StringLengthIndicator::None
    /// \return Actual number of bytes read, -1 if an error has occured.
    std::int64_t readString(std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None, std::uint64_t count = 0);
    
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) = 0;

    virtual bool isEOF() = 0;

    inline bool readInt8(std::int8_t* val) {
        return readBytes(val, sizeof(std::int8_t)) == sizeof(int8_t);
    }

    inline std::int8_t readInt8() {
        std::int8_t temp;
        if (readInt8(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readUInt8(std::uint8_t* val) {
        return readBytes(val, sizeof(std::uint8_t)) == sizeof(uint8_t);
    }

    inline std::uint8_t readUInt8() {
        std::uint8_t temp;
        if (readUInt8(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readInt16(std::int16_t* val) {
        const bool result = readBytes(val, sizeof(std::int16_t)) == sizeof(int16_t);
        *val = static_cast<std::int16_t>(util::SwapLE(static_cast<std::uint16_t>(*val)));
        return result;
    }

    inline std::int16_t readInt16() {
        std::int16_t temp;
        if (readInt16(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readUInt16(std::uint16_t* val) {
        const bool result = readBytes(val, sizeof(std::uint16_t)) == sizeof(uint16_t);
        *val = util::SwapLE(*val);
        return result;
    }

    inline std::uint16_t readUInt16() {
        std::uint16_t temp;
        if (readUInt16(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readInt32(std::int32_t* val) {
        const bool result = readBytes(val, sizeof(std::int32_t)) == sizeof(int32_t);
        *val = static_cast<std::int32_t>(util::SwapLE(static_cast<std::uint32_t>(*val)));
        return result;
    }

    inline std::int32_t readInt32() {
        std::int32_t temp;
        if (readInt32(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readUInt32(std::uint32_t* val) {
        const bool result = readBytes(val, sizeof(std::uint32_t)) == sizeof(uint32_t);
        *val = util::SwapLE(*val);
        return result;
    }

    inline std::uint32_t readUInt32() {
        std::uint32_t temp;
        if (readUInt32(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readInt64(std::int64_t* val) {
        const bool result = readBytes(val, sizeof(std::int64_t)) == sizeof(int64_t);
        *val = static_cast<std::int64_t>(util::SwapLE(static_cast<std::uint64_t>(*val)));
        return result;
    }

    inline std::int64_t readInt64() {
        std::int64_t temp;
        if (readInt64(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readUInt64(std::uint64_t* val) {
        const bool result = readBytes(val, sizeof(std::uint64_t)) == sizeof(uint64_t);
        *val = util::SwapLE(*val);
        return result;
    }

    inline std::uint64_t readUInt64() {
        std::uint64_t temp;
        if (readUInt64(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }

    inline bool readFloat(float* val) {
        if (readBytes(val, sizeof(float)) != sizeof(float)) {
            return false;
        }

        *val = util::SwapLE(*val);
        return true;
    }

    inline float readFloat() {
        float temp;
        if (readFloat(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }
    inline bool readDouble(double* val) {
        if (readBytes(val, sizeof(double)) != sizeof(double)) {
            return false;
        }

        *val = util::SwapLE(*val);
        return true;
    }

    inline double readDouble() {
        double temp;
        if (readDouble(&temp)) {
            return temp;
        } else {
            throw FileException("Failed to read from file ", path.getGenericString());
        }
    }
    
    /// Reads the entire file into a memory buffer and returns a buffer and size pair. The actual size of the buffer is 
    /// size + 1 and it is null terminated. However, do not rely on that when reading binary files because they may contain
    /// nulls earlier.
    std::pair<std::unique_ptr<char[]>, std::int64_t> readWholeFile();
    
    // File write methods -------------------------------------------------------------------------
    
    /// Writes a string with an optional length indicator prefix.
    ///
    /// \remark This method never writes the null terminators to the file.
    ///
    /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
    std::int64_t writeString(const std::string& string, StringLengthIndicator indicator = StringLengthIndicator::None);
    
    /// Writes a string_view with an optional length indicator prefix.
    ///
    /// \remark This method never writes the null terminators to the file.
    ///
    /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
    std::int64_t writeString(const std::string_view& stringView, StringLengthIndicator indicator = StringLengthIndicator::None);
    
    /// Writes a char array with an optional length indicator prefix.
    ///
    /// \remark This method never writes the null terminators to the file and they should not be included in length
    ///
    /// \return Number of bytes that were written. Should be equal to StringLengthIndicator length + the length of the string.
    std::int64_t writeString(const char* string, std::size_t length, StringLengthIndicator indicator = StringLengthIndicator::None);
    

    /// \return number of bytes written if successful, -1 if an error has occured.
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) = 0;

    inline bool writeInt8(std::int8_t val) {
        return writeBytes(&val, sizeof(std::int8_t)) == 1;
    }

    inline bool writeUInt8(std::uint8_t val) {
        return writeBytes(&val, sizeof(std::uint8_t)) == 1;
    }

    inline bool writeInt16(std::int16_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(int16_t)) == sizeof(int16_t);
    }

    inline bool writeUInt16(std::uint16_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(uint16_t)) == sizeof(uint16_t);
    }

    inline bool writeInt32(std::int32_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(int32_t)) == sizeof(int32_t);
    }

    inline bool writeUInt32(std::uint32_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(uint32_t)) == sizeof(uint32_t);
    }

    inline bool writeInt64(std::int64_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(int64_t)) == sizeof(int64_t);
    }

    inline bool writeUInt64(std::uint64_t val) {
        val = util::SwapLE(val);
        return writeBytes(&val, sizeof(uint64_t)) == sizeof(uint64_t);
    }

    inline bool writeFloat(float val) {
        val = util::SwapLE(val);
        if (writeBytes(&val, sizeof(float)) != sizeof(float)) {
            return false;
        }

        return true;
    }

    inline bool writeDouble(double val) {
        val = util::SwapLE(val);
        if (writeBytes(&val, sizeof(double)) != sizeof(double)) {
            return false;
        }

        return true;
    }
protected:
    File(const Path& path, FileOpenMode openMode);

    Path path;
    FileOpenMode openMode;
};

}

#endif // IYF_FILE_HPP
