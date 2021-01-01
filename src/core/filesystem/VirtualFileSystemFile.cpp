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

#include "core/filesystem/VirtualFileSystemFile.hpp"

#include <cassert>

#include "logging/Logger.hpp"
#include "io/exceptions/FileOpenException.hpp"

#include "fmt/ostream.h"

namespace iyf {
VirtualFileSystemFile::VirtualFileSystemFile(const Path& path, FileOpenMode openMode)
    : File(path, openMode), file(nullptr), isOpen(true) {
    switch (openMode) {
    case FileOpenMode::Read:
        file = PHYSFS_openRead(path.getGenericString().c_str());
        break;
    case FileOpenMode::Write:
        file = PHYSFS_openWrite(path.getGenericString().c_str());
        break;
    case FileOpenMode::Append:
        file = PHYSFS_openAppend(path.getGenericString().c_str());
        break;
    }
    
    if (file == nullptr) {
        throw FileOpenException(path.getGenericString());
    }
}

std::int64_t VirtualFileSystemFile::tell() {
    return PHYSFS_tell(file);
}

bool VirtualFileSystemFile::isEOF() {
    return PHYSFS_eof(file);
}

std::int64_t VirtualFileSystemFile::readBytes(void* bytes, std::uint64_t count) {
    return PHYSFS_readBytes(file, bytes, count);
}

std::int64_t VirtualFileSystemFile::writeBytes(const void* bytes, std::uint64_t count) {
    return PHYSFS_writeBytes(file, bytes, count);
}

std::int64_t VirtualFileSystemFile::seek(std::int64_t offset, SeekFrom whence) {
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

bool VirtualFileSystemFile::close() {
    bool result = PHYSFS_close(file);
    
    if (result) {
        isOpen = false;
        file = nullptr;
    } else {
        LOG_E("Failed to close a file: {}", path); 
    }
    
    return result;
}

bool VirtualFileSystemFile::flush() {
    return (PHYSFS_flush(file) > 0);
}

VirtualFileSystemFile::~VirtualFileSystemFile() {
    if (isOpen) {
        int result = PHYSFS_close(file);
        if (result == 0) {
            LOG_E("Error when closing a PHYSFS file. Path: {}", path);
        }
    }
}
    
}
