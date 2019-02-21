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

#include "core/filesystem/File.hpp"

#include <cassert>

#include "core/Logger.hpp"
#include "core/exceptions/FileOpenException.hpp"
#include "core/filesystem/cppFilesystem.hpp"

#include "fmt/ostream.h"

namespace iyf {
File::File(const fs::path& path, OpenMode openMode) : file(nullptr), path(path), openMode(openMode), isOpen(true) {
    switch (openMode) {
    case OpenMode::Read:
        file = PHYSFS_openRead(path.generic_string().c_str());
        break;
    case OpenMode::Write:
        file = PHYSFS_openWrite(path.generic_string().c_str());
        break;
    case OpenMode::Append:
        file = PHYSFS_openAppend(path.generic_string().c_str());
        break;
    }
    
    if (file == nullptr) {
        throw FileOpenException(path.generic_string());
    }
}

std::int64_t File::seek(std::int64_t offset, SeekFrom whence) {
    PHYSFS_sint64 position = 0;

    if (whence == SeekFrom::Start) {
        position = (PHYSFS_sint64) offset;
    } else if (whence == SeekFrom::Current) {
        const PHYSFS_sint64 current = PHYSFS_tell(file);
        
        if (current == -1) {
            return -1;
        }

        if (offset == 0) {
            return current;
        }

        position = current + ((PHYSFS_sint64) offset);
    } else if (whence == SeekFrom::End) {
        const PHYSFS_sint64 length = PHYSFS_fileLength(file);
        
        if (length == -1) {
            return -1;
        }

        position = length + ((PHYSFS_sint64) offset);
    }
    
    if (!PHYSFS_seek(file, (PHYSFS_uint64) position)) {
        return -1;
    }
    
    return position;
}

std::int64_t File::writeString(const char* string, std::size_t length, StringLengthIndicator indicator) {
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
        throw FileException("Failed to write a string length indicator to file ", path.generic_string());
    }
    
    return writeBytes(string, sizeof(char) * length) + lengthIncrement;
}

std::int64_t File::writeString(const std::string& string, StringLengthIndicator indicator) {
    return writeString(string.data(), string.length(), indicator);
}

std::int64_t File::writeString(const std::string_view& stringView, StringLengthIndicator indicator) {
    return writeString(stringView.data(), stringView.length(), indicator);
}

std::int64_t File::readString(std::string& string, StringLengthIndicator indicator, std::uint64_t count) {
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
        throw FileException("Failed read a string from file ", path.generic_string());
    }
    
    string.append(buffer, bytesToRead);
    
    delete[] buffer;
    
    return readByteCount + lengthIncrement;
}

std::pair<std::unique_ptr<char[]>, std::int64_t> File::readWholeFile() {
    // Don't forget to save the current position. We don't want to mess someone's logic
    const std::int64_t currentPos = tell();
    if (currentPos == -1) {
        throw FileException("Failed to tell the current position of ", path.generic_string());
    }
    
    std::int64_t size = seek(0, File::SeekFrom::End);
    if (size == -1) {
        throw FileException("Failed to seek to the end of ", path.generic_string());
    }
    
    std::int64_t result = seek(0, File::SeekFrom::Start);
    if (result == -1) {
        throw FileException("Failed to seek to the start of ", path.generic_string());
    }
    
    auto buffer = std::make_unique<char[]>(size + 1);
    std::int64_t byteCount = readBytes(buffer.get(), size);
    if (byteCount == -1 || (byteCount < size)) {
        const std::string errorStr = fmt::format("Failed to read the required number of bytes (read {} of {}) from ", byteCount, size);
        throw FileException(errorStr, path.generic_string());
    }
    
    buffer[size] = '\0';
    
    // Don't forget to reset the position.
    result = seek(currentPos, File::SeekFrom::Start);
    if (result == -1) {
        throw FileException("Failed to seek to the original position ", path.generic_string());
    }
    
    return {std::move(buffer), size};
}

bool File::close() {
    bool result = PHYSFS_close(file);
    
    if (result) {
        isOpen = false;
        file = nullptr;
    } else {
        LOG_E("Failed to close a file: {}", path); 
    }
    
    return result;
}

File::~File() {
    if (isOpen) {
        int result = PHYSFS_close(file);
        if (result == 0) {
            LOG_E("Error when closing a PHYSFS file. Path: {}", path);
        }
    }
}
    
}
