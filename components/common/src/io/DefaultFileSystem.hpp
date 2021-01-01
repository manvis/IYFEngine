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

#ifndef IYF_DEFAULT_FILE_SYSTEM_HPP
#define IYF_DEFAULT_FILE_SYSTEM_HPP

#include "FileSystem.hpp"
#include "utilities/NonCopyable.hpp"

namespace iyf {
class DefaultFileSystem : public FileSystem {
public:
    NON_COPYABLE(DefaultFileSystem)

    virtual ~DefaultFileSystem();

    static DefaultFileSystem& Instance();

    virtual std::unique_ptr<File> openFile(const Path& p, FileOpenMode mode) const final override;
    
    virtual FileHash computeFileHash(const Path& path) const final override;
    virtual FileSystemResult openInFileBrowser(const Path& path) const final override;
    virtual FileSystemResult remove(const Path& path) const final override;
    virtual FileSystemResult removeRecursive(const Path& path) const final override;
    virtual FileSystemResult getStats(const Path& path, FileStat& sOut) const final override;
    virtual FileSystemResult createDirectory(const Path& path) const final override;
    virtual bool exists(const Path& path, FileSystemResult& result) const final override;
    virtual bool isEmpty(const Path& path, FileSystemResult& result) const final override;
    virtual bool isDirectory(const Path& path, FileSystemResult& result) const final override;
    using FileSystem::exists;
    using FileSystem::isEmpty;
    using FileSystem::isDirectory;
    virtual std::vector<Path> getDirectoryContents(const Path& path) const final override;
    virtual FileSystemResult rename(const Path& source, const Path& destination) const final override;
    virtual FileSystemResult copyFile(const Path& source, const Path& destination, FileCopyOption option = FileCopyOption::None) const final override;
private:
    DefaultFileSystem();
};
}

#endif // IYF_DEFAULT_FILE_SYSTEM_HPP