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

#ifndef IYF_DEFAULT_FILE_SYSTEM_FILE_HPP
#define IYF_DEFAULT_FILE_SYSTEM_FILE_HPP

#include <cstdint>
#include <fstream>

#include "utilities/NonCopyable.hpp"
#include "io/DefaultFileSystem.hpp"
#include "io/File.hpp"

namespace iyf {

/// Encapsulates PhysFS file access that is limited to the virtual file system. If you wish to access files
/// from anywhere, you'll need to rely on C++'s file system functions, however, that is strongly discouraged.
///
/// \todo This applies both here and (partially) in the serializers. There are 2 kinds of read functions - those that throw and
/// those that return a value. Serializers use the throwing versions. However, write functions only return a value. This is inconsistent,
/// a bit confusing and it requires the end users of these classes to check for write results. I think I should move to error codes
/// like in the filesystem library
///
/// \todo Start using size_t here and in serializers (for sizes)
class DefaultFileSystemFile : public File {
public:
    NON_COPYABLE(DefaultFileSystemFile)

    virtual ~DefaultFileSystemFile();
    virtual bool close() final override;    
    virtual bool flush() final override;

    virtual std::int64_t seek(std::int64_t offset, SeekFrom whence) final override;
    virtual std::int64_t tell() final override;
    virtual std::int64_t readBytes(void* bytes, std::uint64_t count) final override;

    virtual bool isEOF() final override;
    virtual std::int64_t writeBytes(const void* bytes, std::uint64_t count) final override;
protected:
    friend class DefaultFileSystem;
    DefaultFileSystemFile(const Path& path, FileOpenMode openMode);

    std::fstream stream;
};

}

#endif // IYF_DEFAULT_FILE_SYSTEM_FILE_HPP
