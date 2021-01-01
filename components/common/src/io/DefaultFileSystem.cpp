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

#include "DefaultFileSystem.hpp"
#include "DefaultFileSystemFile.hpp"
#include "utilities/ReadWholeFile.hpp"

#include <filesystem>
#include <fstream>

#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>
#endif // OS CHECK

namespace iyf {
namespace fs = std::filesystem;

DefaultFileSystem& DefaultFileSystem::Instance() {
    static DefaultFileSystem dfs;
    return dfs;
}

DefaultFileSystem::DefaultFileSystem() {}
DefaultFileSystem::~DefaultFileSystem() {}

std::unique_ptr<File> DefaultFileSystem::openFile(const Path& p, FileOpenMode mode) const {
    return std::unique_ptr<DefaultFileSystemFile>(new DefaultFileSystemFile(p, mode));
}

FileSystemResult DefaultFileSystem::openInFileBrowser(const Path& path) const {
#ifdef __linux__
    std::string command = fmt::format("xdg-open \"{}\"", path);

    int result = system(command.c_str());
    
    if (WIFEXITED(result) && (WEXITSTATUS(result) == 0)) {
        return FileSystemResult::Success;
    }

    return FileSystemResult::Error;
#else
    // TODO implement
    throw std::runtime_error("Not yet implemented");
    static_assert(false, "Not implemented");
#endif // OS CHECK
}

FileSystemResult DefaultFileSystem::remove(const Path& path) const {
    std::error_code ec;
    bool result = fs::remove(path.path, ec);

    if (ec) {
        return FileSystemResult::Error;
    }

    if (!result) {
        return FileSystemResult::NotFound;
    }

    return FileSystemResult::Success;
}

FileSystemResult DefaultFileSystem::removeRecursive(const Path& path) const {
    std::error_code ec;
    std::uintmax_t removals = fs::remove_all(path.path, ec);

    if (removals == static_cast<std::uintmax_t>(-1) || ec) {
        return FileSystemResult::Error;
    }

    if (removals == 0) {
        return FileSystemResult::NotFound;
    }

    return FileSystemResult::Success;
}

FileSystemResult DefaultFileSystem::getStats(const Path& path, FileStat& sOut) const {
#ifdef __linux__
    struct stat sb {};

     // lstat Gets us more data in a single call than c++ functions
    const int result = lstat(path.getCString(), &sb);

    if (result != 0)
    {
        if (errno == ENOENT) {
            return FileSystemResult::NotFound;
        }
        
        return FileSystemResult::Error;
    }

    fs::exists(path.getCString());

    sOut.fileSize = sb.st_size;

    sOut.createTime = -1;
    sOut.accessTime = sb.st_atim.tv_sec;
    sOut.updateTime = sb.st_mtim.tv_nsec;

    switch (sb.st_mode & S_IFMT)
    {
        case S_IFREG:
            sOut.type = FileType::Regular;
            break;
        case S_IFDIR:
            sOut.type = FileType::Directory;
            break;
        case S_IFLNK:
            sOut.type = FileType::Symlink;
            break;
        default:
            sOut.type = FileType::Other;
            break;
    }

    sOut.permissions = FilePermissionFlags(static_cast<FilePermission>(sb.st_mode & 0777));

    const UserInfo& user = getUserInfo();

    sOut.access = FileAccess::None;

    if (sb.st_uid == user.user.id) {
        if (sOut.permissions & FilePermission::OwnerWrite) {
            sOut.access |= FileAccess::Writable;
        }
        
        if (sOut.permissions & FilePermission::OwnerRead) {
            sOut.access |= FileAccess::ReadOnly;
        }
    }

    for (const auto& g : user.groups) {
        if (sb.st_gid == g.id) {
            if (sOut.permissions & FilePermission::GroupWrite) {
                sOut.access |= FileAccess::Writable;
            }
            
            if (sOut.permissions & FilePermission::GroupRead) {
                sOut.access |= FileAccess::ReadOnly;
            }

            break;
        }
    }
    
#else 
    // TODO implement
    throw std::runtime_error("Not yet implemented");
    static_assert(false, "Not implemented");
#endif // OS CHECK

    return FileSystemResult::Success;
}

FileSystemResult DefaultFileSystem::createDirectory(const Path& path) const {
    std::error_code ec;
    const bool result = fs::create_directories(path.path, ec);

    if (!result || ec) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}

bool DefaultFileSystem::exists(const Path& path, FileSystemResult& result) const {
    std::error_code ec;
    const bool r = fs::exists(path.path, ec);

    if (ec) {
        result = FileSystemResult::Error;
        return false;
    }
    
    if (!r) {
        result = FileSystemResult::NotFound;
        return false;
    }

    result = FileSystemResult::Success;
    return r;
}

bool DefaultFileSystem::isDirectory(const Path& path, FileSystemResult& result) const {
    std::error_code ec;
    const auto s = fs::status(path.path, ec);

    if (ec) {
        result = FileSystemResult::Error;
        return false;
    }

    if (!fs::exists(s)) {
        result = FileSystemResult::NotFound;
        return false;
    }

    result = FileSystemResult::Success;
    return (s.type() == fs::file_type::directory);
}

bool DefaultFileSystem::isEmpty(const Path& path, FileSystemResult& result) const {
    std::error_code ec;
    const bool r = fs::is_empty(path.path, ec);

    if (ec) {
        result = FileSystemResult::Error;
        return false;
    }

    result = FileSystemResult::Success;
    if (!r) {
        return false;
    }

    return true;
}

FileSystemResult DefaultFileSystem::rename(const Path& source, const Path& destination) const {
    std::error_code ec;
    fs::rename(source.path, destination.path, ec);

    if (ec) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}

FileSystemResult DefaultFileSystem::copyFile(const Path& source, const Path& destination, FileCopyOption option) const {
    std::error_code ec;

    auto options = std::filesystem::copy_options::none;

    switch (option) {
    case FileCopyOption::SkipExisting:
        options |= std::filesystem::copy_options::skip_existing;
        break;
    case FileCopyOption::OverwriteExisting:
        options |= std::filesystem::copy_options::overwrite_existing;
        break;
    case FileCopyOption::None:
        options |= std::filesystem::copy_options::none;
        break;
    }

    bool success = fs::copy_file(source.path, destination.path, options, ec);

    if (!success || ec) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}

std::vector<Path> DefaultFileSystem::getDirectoryContents(const Path& path) const {
    std::vector<Path> p;
    p.reserve(64);

    for (auto dir : fs::directory_iterator(path.path)) {
        if (dir.is_regular_file() || dir.is_directory() || dir.is_symlink()) {
            p.emplace_back(dir.path());
        }
    }

    return p;
}

FileHash DefaultFileSystem::computeFileHash(const Path& path) const {
    std::ifstream file(path.path, std::ios::in | std::ios::binary);
    const auto data = util::ReadWholeFile(file);
    
    FileHash hash = HF(data.first.get(), data.second);
    return hash;
}
}