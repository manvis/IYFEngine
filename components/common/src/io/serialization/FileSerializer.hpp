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

#ifndef IYF_FILE_SERIALIZER_HPP
#define IYF_FILE_SERIALIZER_HPP

#include "io/serialization/Serializer.hpp"
#include "io/File.hpp"

namespace iyf {
class FileSystem;
class FileSerializer : public Serializer {
public:
    static constexpr FileOpenMode SerializerOpenModeToFileOpenMode(OpenMode mode) {
        switch (mode) {
            case OpenMode::Read:
                return FileOpenMode::Read;
            case OpenMode::Write:
                return FileOpenMode::Write;
            case OpenMode::Append:
                return FileOpenMode::Append;
            case OpenMode::ReadAndWrite:
                throw SerializerException("The FileSerializer does not support the ReadWrite mode");
        }
    }
    
    static constexpr OpenMode FileOpenModeToSerializerOpenMode(FileOpenMode mode) {
        switch (mode) {
            case FileOpenMode::Read:
                return OpenMode::Read;
            case FileOpenMode::Write:
                return OpenMode::Write;
            case FileOpenMode::Append:
                return OpenMode::Append;
            default:
                throw SerializerException("An unknown or unsupported File::OpenMode was specified.");
        }
    }
    
    FileSerializer(FileSystem& fs, const Path& path, FileOpenMode mode);
    
    virtual bool isEnd() final override {
        return file->isEOF();
    }
    
    virtual std::int64_t seek(std::int64_t offset) final override {
        return file->seek(offset, File::SeekFrom::Start);
    }
    
    virtual std::int64_t tell() const final override {
        return file->tell();
    }
    
    // Write methods ----------------------------
    
    virtual std::int64_t writeString(const std::string& string, StringLengthIndicator indicator) final override {
        return file->writeString(string, indicator);
    }
    
    virtual std::int64_t writeString(const std::string_view& stringView, StringLengthIndicator indicator = StringLengthIndicator::None) final override {
        return file->writeString(stringView, indicator);
    }
    
    virtual std::int64_t writeString(const char* string, std::size_t length, StringLengthIndicator indicator = StringLengthIndicator::None) final override {
        return file->writeString(string, length, indicator);
    }
    
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) final override {
        return file->writeBytes(bytes, count);
    }
    
    virtual bool writeInt8(std::int8_t val) final override {
        return file->writeInt8(val);
    }
    
    virtual bool writeUInt8(std::uint8_t val) final override {
        return file->writeUInt8(val);
    }
    
    virtual bool writeInt16(std::int16_t val) final override {
        return file->writeInt16(val);
    }
    
    virtual bool writeUInt16(std::uint16_t val) final override {
        return file->writeUInt16(val);
    }
    
    virtual bool writeInt32(std::int32_t val) final override {
        return file->writeInt32(val);
    }
    
    virtual bool writeUInt32(std::uint32_t val) final override {
        return file->writeUInt32(val);
    }
    
    virtual bool writeInt64(std::int64_t val) final override {
        return file->writeInt64(val);
    }
    
    virtual bool writeUInt64(std::uint64_t val) final override {
        return file->writeUInt64(val);
    }
    
    virtual bool writeFloat(float val) final override {
        return file->writeFloat(val);
    }
    
    virtual bool writeDouble(double val) final override {
        return file->writeDouble(val);
    }
    
    // Read methods ------------------------------
    
    virtual std::int64_t readString(std::string& string, StringLengthIndicator indicator, std::uint64_t count) final override {
        return file->readString(string, indicator, count);
    }
    
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) final override {
        return file->readBytes(bytes, count);
    }
    
    virtual std::int8_t readInt8() final override {
        return file->readInt8();
    }
    
    virtual std::uint8_t readUInt8() final override {
        return file->readUInt8();
    }
    
    virtual std::int16_t readInt16() final override {
        return file->readInt16();
    }
    
    virtual std::uint16_t readUInt16() final override {
        return file->readUInt16();
    }
    
    virtual std::int32_t readInt32() final override {
        return file->readInt32();
    }
    
    virtual std::uint32_t readUInt32() final override {
        return file->readUInt32();
    }
    
    virtual std::int64_t readInt64() final override {
        return file->readInt64();
    }
    
    virtual std::uint64_t readUInt64() final override {
        return file->readUInt64();
    }
    
    virtual float readFloat() final override {
        return file->readFloat();
    }
    
    virtual double readDouble() final override {
        return file->readDouble();
    }
    
private:
    std::unique_ptr<File> file;
};
}

#endif // IYF_FILE_SERIALIZER_HPP
