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

#include "DefaultFileSystemFile.hpp"

#include <cassert>

#include "logging/Logger.hpp"
#include "io/exceptions/FileOpenException.hpp"

namespace iyf {
DefaultFileSystemFile::DefaultFileSystemFile(const Path& path, FileOpenMode openMode)
    : File(path, openMode) {
    std::ios::openmode mode = std::ios::binary;

    switch (openMode) {
    case FileOpenMode::Read:
        mode = std::ios::in;
        break;
    case FileOpenMode::Write:
        mode = std::ios::out;
        mode |= std::ios::trunc;
        break;
    case FileOpenMode::Append:
        mode = std::ios::out;
        mode |= std::ios::app;
        break;
    default:
        throw FileOpenException("Unknown or unhandled FileOpenMode");
    }

    stream.open(path.getNativeString(), mode);
    if (!stream.is_open()) {
        LOG_E("Failed to open a file {}", path);
        throw FileOpenException("Failed to open a file");
    }
}

DefaultFileSystemFile::~DefaultFileSystemFile() {}

bool DefaultFileSystemFile::close() {
    stream.close();
    return true;
}

bool DefaultFileSystemFile::flush() {
    return bool(stream.flush());
}

std::int64_t DefaultFileSystemFile::seek(std::int64_t offset, SeekFrom whence) {
    std::ios_base::seekdir dir;

    switch (whence) {
    case SeekFrom::Current:
        dir = std::ios_base::cur;
        break;
    case SeekFrom::Start:
        dir = std::ios_base::beg;
        break;
    case SeekFrom::End:
        dir = std::ios_base::end;
        break;
    default:
        return -1;
    }

    if (openMode == FileOpenMode::Read) {
        return bool(stream.seekg(offset, dir));
    } else {
        return bool(stream.seekp(offset, dir));
    }
}

std::int64_t DefaultFileSystemFile::tell() {
    if (openMode == FileOpenMode::Read) {
        return stream.tellg();
    } else {
        return stream.tellp();
    }
}

std::int64_t DefaultFileSystemFile::readBytes(void* bytes, std::uint64_t count) {
    const bool result = bool(stream.read(static_cast<char*>(bytes), count));
    if (!result) {
        return -1;
    }

    return stream.gcount();
}

bool DefaultFileSystemFile::isEOF() {
    return stream.eof();
}

std::int64_t DefaultFileSystemFile::writeBytes(const void* bytes, std::uint64_t count) {
    const bool result = bool(stream.write(static_cast<const char*>(bytes), count));
    if (!result) {
        return -1;
    }

    return static_cast<int64_t>(count);
}

}
