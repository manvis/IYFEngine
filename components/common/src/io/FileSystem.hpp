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

#ifndef IYF_FILE_SYSTEM_HPP
#define IYF_FILE_SYSTEM_HPP

#include <string>
#include <vector>
#include <memory>

#include "io/Path.hpp"
#include "io/FileOpenMode.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/Flags.hpp"

namespace iyf {
class File;
enum class FileType : std::uint32_t {
    Regular,
    Directory,
    Symlink,
    Other,
    Unknown,
    NotFound
};

enum class FilePermission : std::uint32_t {
    None = 0,

    OwnerRead = 0400,
    OwnerWrite = 0200,
    OwnerExec = 0100,
    OwnerAll = 0700,

    GroupRead = 040,
    GroupWrite = 020,
    GroupExec = 010,
    GroupAll = 070,
    
    OthersRead = 04,
    OthersWrite = 02,
    OthersExec = 01,
    OthersAll = 07,

    All = 0777,
};

using FilePermissionFlags = Flags<FilePermission>;

enum class FileAccess : std::uint32_t {
    None = 0x00,
    Writable = 0x01,
    ReadOnly = 0x02,
    All = 0x03
};

enum class FileCopyOption : std::uint32_t {
    None, ///< Fail if file exists
    SkipExisting, ///< Return success even if file exists and no copy is performed. This mode does NOT check if files are identical
    OverwriteExisting ///< Replace the file
};

using FileAccessFlags = Flags<FileAccess>;

struct FileStat {
    FileStat() : fileSize(-1), accessTime(-1), createTime(-1), updateTime(-1), type(FileType::NotFound),
        permissions(FilePermission::None), access(FileAccess::None) {}
    
    /// \brief Size in bytes or < 0 if non applicable (e.g., a dir) or an error
    std::int64_t fileSize;

    /// \brief Unix timestamp in seconds or < 0 if non applicable, not available or an error
    std::int64_t accessTime;

    /// \brief Unix timestamp in seconds or < 0 if non applicable, not available or an error
    std::int64_t createTime;

    /// \brief Unix timestamp in seconds or < 0 if non applicable, not available or an error
    std::int64_t updateTime;

    FileType type;

    /// \warning May not be supported by certain backends or OSes. You should check "access",
    /// which is always available
    FilePermissionFlags permissions;

    FileAccessFlags access;
};

struct NameAndID {
    std::string name;

#ifdef __linux__
    std::int64_t id;
#else
    std::string id;
#endif
};

struct UserInfo {
    NameAndID user;
    std::vector<NameAndID> groups;
};

enum class FileSystemResult {
    Success,
    NotFound,
    NotSupported,
    Error
};

/// A FileSystem implementation enacapsulates file access and hides different backends
/// from other Engine components.
///
/// Two main backends exist:
/// 1. DefaultFileSystem, which wraps the std::filesystem and allows access to actual
/// files.
/// 2. VirtualFileSystem (not part of the IYFCommon library), which is used to access
/// compressed files in a virtual PhysFS filesystem
class FileSystem {
public:
    virtual ~FileSystem() = 0;
    
    virtual std::unique_ptr<File> openFile(const Path& p, FileOpenMode mode) const = 0;

    virtual FileHash computeFileHash(const Path& path) const = 0;
    
    /// Open a file in the file browser. Not all FileSystem implementations support it.
    virtual FileSystemResult openInFileBrowser(const Path& path) const = 0;
    
    /// \brief Deletes a file or an empty directory
    /// 
    /// \param[in] path file or directory to delete
    /// \return whether the deletion was successful, or not
    virtual FileSystemResult remove(const Path& path) const = 0;

    /// \brief Deletes a directory recursively
    /// 
    /// \param[in] path file or directory to delete
    /// \return whether the deletion was successful, or not
    virtual FileSystemResult removeRecursive(const Path& path) const = 0;
    
    /// \brief Obtains statistics for a file or directory
    virtual FileSystemResult getStats(const Path& path, FileStat& sOut) const = 0;
    
    /// Creates a directory and all missing parent directories
    ///  
    /// \param[in] path Directory to create
    /// \return Whether the creation was successful 
    virtual FileSystemResult createDirectory(const Path& path) const = 0;
    
    /// \brief Checks if the file exists
    ///
    /// \param[in] path what file to search for
    /// \return exists or not
    virtual bool exists(const Path& path, FileSystemResult& result) const = 0;
    virtual bool isEmpty(const Path& path, FileSystemResult& result) const = 0;
    virtual bool isDirectory(const Path& path, FileSystemResult& result) const = 0;

    virtual bool exists(const Path& path) const {
        FileSystemResult result = FileSystemResult::Success;
        const bool r = exists(path, result);
        return (r && result == FileSystemResult::Success);
    }

    virtual bool isEmpty(const Path& path) const {
        FileSystemResult result = FileSystemResult::Success;
        const bool r = isEmpty(path, result);
        return (r && result == FileSystemResult::Success);
    }

    virtual bool isDirectory(const Path& path) const {
        FileSystemResult result = FileSystemResult::Success;
        const bool r = isDirectory(path, result);
        return (r && result == FileSystemResult::Success);
    }
    
    /// \brief Lists all files in Path
    ///
    /// \param[in] path Path to examine
    /// \return Files and folders in Path. An empty vector may be returned on error
    virtual std::vector<Path> getDirectoryContents(const Path& path) const = 0;
    
    /// \brief Renames (moves) a file.
    ///
    /// \remark Since PHYSFS (as of this writing) doesn't have a rename function, this uses the APIs from the fs namespace,
    /// which is why real paths are required here.
    ///
    /// \warning You should use this instead of fs::rename for all asset management operations.
    virtual FileSystemResult rename(const Path& source, const Path& destination) const = 0;

    virtual FileSystemResult copyFile(const Path& source, const Path& destination, FileCopyOption option = FileCopyOption::None) const = 0;

    virtual const UserInfo& getUserInfo() const; 
    
    enum class IsFileOpenResult {
        Open,
        NotOpen,
        Error
    };
};
}

#endif // IYF_FILE_SYSTEM_HPP
