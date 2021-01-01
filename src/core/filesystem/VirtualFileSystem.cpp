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

#ifdef __linux__
#define _XOPEN_SOURCE
#include <stdio.h>
#elif _WIN64
// TODO what to include here
#endif // OS CHECK

#include <filesystem>
#include <physfs.h>

#include "assets/AssetConstants.hpp"
#include "core/filesystem/VirtualFileSystem.hpp"
#include "core/filesystem/VirtualFileSystemFile.hpp"
#include "io/File.hpp"
#include "logging/Logger.hpp"
#include "core/Project.hpp"
#include "core/Constants.hpp"

#include "fmt/ostream.h"

namespace iyf {
inline const char* GetLastErrorText() {
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

char* VirtualFileSystem::argv = nullptr;
VirtualFileSystem& VirtualFileSystem::Instance() {
    static VirtualFileSystem vfs;
    return vfs;
}

VirtualFileSystem::VirtualFileSystem() : editorMode(false), initialized(false) {
    if (argv == nullptr) {
        throw std::runtime_error("static argv* not initialized");
    }

    PHYSFS_init(argv);
    
    if (editorMode) {
        // TODO Symlinks should only be enabled when running straight from the build folder (since data folder is symlinked then)
        PHYSFS_permitSymbolicLinks(1);
    }

    dirSeparator = PHYSFS_getDirSeparator();
    baseDir = PHYSFS_getBaseDir();
}

std::unique_ptr<File> VirtualFileSystem::openFile(const Path& p, FileOpenMode mode) const {
    return std::unique_ptr<VirtualFileSystemFile>(new VirtualFileSystemFile(p, mode));
}

bool VirtualFileSystem::initialize(const Project* project, bool editorMode, bool skipSystemPackageMounting) {
    this->editorMode = editorMode;

    // The rest of the constructor is only skipped when running the SystemAssetPackager.
    // It is a tool that makes the SystemAssetPacks we have to mount here
    if (skipSystemPackageMounting) {
        assert(project == nullptr);
        
        initialized = true;
        return true;
    }
    
    assert(project != nullptr);
    
    // First of all, handle the system file package
    // System assets should always be stored next to the executable
    Path systemAssetRealPath = baseDir / con::SystemAssetPackName();
    
    if (PHYSFS_mount(systemAssetRealPath.getCString(), "", 1) == 0) {
        LOG_E("Failed to mount the system asset archive ({}). PHYSFS error: {}", systemAssetRealPath, GetLastErrorText());
        throw std::runtime_error(std::string("PHYSFS failed to mount a mandatory archive"));
    }
    
    // Next, handle the write directory
    if (editorMode) {
        // When running in editor mode, the write path matches the Project root
        const auto& projectRoot = project->getRootPath();
        setWritePath(projectRoot.getGenericString());
        
        // This function creates the folders automatically if they don't exist.
        // We use editor specific constants when running in editor mode.
        const char* prefPath = PHYSFS_getPrefDir("IYFTeam", "IYFEngine");
        if (prefPath == nullptr) {
            LOG_E("Couldn't create the save directory");
            throw std::runtime_error("Couldn't create the save directory");
        }
        
        prefDir = prefPath;
    } else {
        // When running in game mode, use the constants provided by the user to create a game specific preference directory
        const char* prefPath = PHYSFS_getPrefDir(project->getCompanyName().c_str(), project->getGameName().c_str());
        if (prefPath == nullptr) {
            LOG_E("Couldn't create the save directory");
            throw std::runtime_error("Couldn't create the save directory");
        }
        
        prefDir = prefPath;
        
        setWritePath(prefPath);
    }
    
    assert(readPaths.empty());
    
    // Next, handle the read directories
    if (editorMode) {
        // When running in editor mode, the read path matches the project root 
        const auto& projectRoot = project->getRootPath();
        addReadPath(projectRoot.getNativeString(), "/", true);
        
        const Path platformAssetPath = projectRoot / con::PlatformIdentifierToName(con::GetCurrentPlatform());
        
        if (std::filesystem::exists(platformAssetPath.getNativeString())) {
            LOG_V("Mounting the asset folder for the current platform: {}", platformAssetPath);
            addReadPath(platformAssetPath, "/", true);
        } else {
            /// This can happen before the first real project is loaded and, therefore, it's not an error.
            LOG_I("The asset path for the current platform ({}) does not exist. Skipping its mounnting.", platformAssetPath);
        }
    } else {
        // When running in game mode, mount the data archive that contains the contents of the asset path
        Path assetPackPath = project->getRootPath();
        assetPackPath /= con::DefaultReleasePackName();
        addReadPath(assetPackPath.getNativeString(), con::BaseAssetPath().getGenericString(), true);
        
        // TODO mount patches, mods and DLCs
    }

    initialized = true;
    return true;
}

FileHash VirtualFileSystem::computeFileHash(const Path& path) const {
    assert(initialized);
    
    auto file = openFile(path, FileOpenMode::Read);
    const auto data = file->readWholeFile();
    
    FileHash hash = HF(data.first.get(), data.second);
    return hash;
}

void VirtualFileSystem::setWritePath(Path realPath) {
    assert(initialized);

    if (PHYSFS_setWriteDir(realPath.getCString()) == 0) {
        LOG_E("Failed to set the write directory to ({}). PHYSFS error: {}", realPath, GetLastErrorText());
        throw std::runtime_error("Failed to set the write directory");
    } else {
        LOG_V("Setting write directory to {}", realPath);
    }
    
    currentWriteDir = std::move(realPath);
}

void VirtualFileSystem::addReadPath(Path realPath, const Path& virtualPath, bool appendToSearchPath) {
    assert(initialized);
    
    if (PHYSFS_mount(realPath.getCString(), virtualPath.getCString(), appendToSearchPath ? 1 : 0) == 0)  {
        LOG_E("Failed to mount a real path ({}) for reading as virtual path ({}). PHYSFS error: {}", realPath, virtualPath, GetLastErrorText());
        throw std::runtime_error("Failed to mount a path");
    }
    
    readPaths.push_back(std::move(realPath));
}

FileSystemResult VirtualFileSystem::openInFileBrowser(const Path&) const {
    assert(initialized);

    return FileSystemResult::NotSupported;
}

std::string VirtualFileSystem::logSearchPath(const std::string& pathSeparator) const {
    assert(initialized);

    std::stringstream ss;
    char **j = PHYSFS_getSearchPath();

    for (char **i = j; *i != NULL; i++) {
        ss << *i << pathSeparator;
    }
    
    PHYSFS_freeList(j);
    
    return ss.str();
}

FileSystemResult VirtualFileSystem::getStats(const Path& path, FileStat& sOut) const {
    assert(initialized);

    PHYSFS_Stat stat;
    const int result = PHYSFS_stat(path.getGenericString().c_str(), &stat);

    if (result == 0) {
        return FileSystemResult::NotFound;
    }

    sOut.accessTime = stat.accesstime;
    sOut.createTime = stat.createtime;
    sOut.updateTime = stat.modtime;

    sOut.fileSize = stat.filesize;
    sOut.permissions = FilePermission::None;

    sOut.access = stat.readonly ? FileAccess::ReadOnly : FileAccess::Writable;
    switch (stat.filetype) {
    case PHYSFS_FILETYPE_REGULAR:
        sOut.type = FileType::Regular;
        break;
    case PHYSFS_FILETYPE_DIRECTORY:
        sOut.type = FileType::Directory;
        break;
    case PHYSFS_FILETYPE_SYMLINK:
        sOut.type = FileType::Symlink;
        break;
    case PHYSFS_FILETYPE_OTHER:
        sOut.type = FileType::Other;
        break;
    }

    return FileSystemResult::Success;
}

std::vector<Path> VirtualFileSystem::getDirectoryContents(const Path& path) const {
    assert(initialized);

    // TODO this is probably suboptimal and might need to be replaced with PHYSFS_enumerateFilesCallback
    std::vector<Path> files;
    char** fileList = PHYSFS_enumerateFiles(path.getGenericString().c_str());
    char** iter;

    for (iter = fileList; *iter != nullptr; iter++) {
        files.push_back(*iter);
    } 

    PHYSFS_freeList(fileList);

    return files;
}

bool VirtualFileSystem::isEmpty(const Path& path, FileSystemResult& result) const {
    assert(initialized);

    char** fileList = PHYSFS_enumerateFiles(path.getGenericString().c_str());
    char** iter;

    bool empty = true;
    for (iter = fileList; *iter != nullptr; iter++) {
        empty = false;
        break;
    } 

    PHYSFS_freeList(fileList);

    result = FileSystemResult::Success;
    return empty;
}

bool VirtualFileSystem::isDirectory(const Path& path, FileSystemResult& result) const {
    assert(initialized);

    PHYSFS_Stat stat;
    const int r = PHYSFS_stat(path.getGenericString().c_str(), &stat);

    const PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();

    if (ec == PHYSFS_ERR_NOT_FOUND) {
        result = FileSystemResult::NotFound;
        return false;
    } else if (r == 0 || ec != PHYSFS_ERR_OK) {
        result = FileSystemResult::Error;
        return false;
    }

    result = FileSystemResult::Success;
    return stat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

VirtualFileSystem::~VirtualFileSystem() {
    PHYSFS_deinit();
}
    
Path VirtualFileSystem::getRealDirectory(const Path& fileName) const {
    assert(initialized);

    const char* dir = PHYSFS_getRealDir(fileName.getGenericString().c_str());
    
    if (dir == nullptr) {
        return emptyDir;
    } else {
        return Path(dir) / fileName;
    }
}
    
FileSystemResult VirtualFileSystem::remove(const Path& path) const {
    assert(initialized);

    const int r = PHYSFS_delete(path.getGenericString().c_str());
    const PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();

    if (ec == PHYSFS_ERR_NOT_FOUND) {
        return FileSystemResult::NotFound;
    } else if (r == 0 || ec != PHYSFS_ERR_OK) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}

FileSystemResult VirtualFileSystem::removeRecursive(const Path& path) const {
    assert(initialized);

    char** fileList = PHYSFS_enumerateFiles(path.getGenericString().c_str());
    char** iter;

    FileSystemResult result = FileSystemResult::Success;

    for (iter = fileList; *iter != nullptr; iter++) {
        PHYSFS_Stat stat;
        const int r = PHYSFS_stat(path.getGenericString().c_str(), &stat);

        if (r == 0) {
            PHYSFS_freeList(fileList);
            return FileSystemResult::Error;
        }

        if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY) {
            result = removeRecursive(*iter);
            if (result != FileSystemResult::Success) {
                PHYSFS_freeList(fileList);
                return result;
            }
        }

        const int rDel = PHYSFS_delete(path.getGenericString().c_str());
        const PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();

        if (rDel == 0 || ec != PHYSFS_ERR_OK) {
            PHYSFS_freeList(fileList);
            return FileSystemResult::Error;
        }
    } 

    PHYSFS_freeList(fileList);

    return result;
}

FileSystemResult VirtualFileSystem::createDirectory(const Path& path) const {
    assert(initialized);

    const int r = PHYSFS_mkdir(path.getGenericString().c_str());
    const PHYSFS_ErrorCode ec = PHYSFS_getLastErrorCode();

    if (ec == PHYSFS_ERR_NOT_FOUND) {
        return FileSystemResult::NotFound;
    } else if (r == 0 || ec != PHYSFS_ERR_OK) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}
    
bool VirtualFileSystem::exists(const Path& fileName, FileSystemResult& result) const {
    assert(initialized);

    const int r = PHYSFS_exists(fileName.getGenericString().c_str());
    const bool ret = (r != 0);

    result = ret ? FileSystemResult::Success : FileSystemResult::NotFound;
    return ret;
}
    
char** VirtualFileSystem::getDirectoryContentsCArray(const Path& path) const {
    assert(initialized);

    char** fileList = PHYSFS_enumerateFiles(path.getGenericString().c_str());
    return fileList;
}
    
FileSystemResult VirtualFileSystem::rename(const Path&, const Path&) const {
    assert(initialized);

    return FileSystemResult::NotSupported;
}

FileSystemResult VirtualFileSystem::copyFile(const Path& source, const Path& destination, FileCopyOption option) const {
    assert(initialized);

    const bool sourceExists = exists(source);
    if (!sourceExists) {
        return FileSystemResult::Error;
    }

    const bool destinationExists = exists(destination);

    switch (option)
    {
    case FileCopyOption::None:
        if (destinationExists) {
            return FileSystemResult::Error;
        }
        break;
    case FileCopyOption::SkipExisting:
        if (destinationExists) {
            return FileSystemResult::Success;
        }
        break;
    case FileCopyOption::OverwriteExisting:
    default:
        break;
    }

    auto input = openFile(source, FileOpenMode::Read);
    auto inputData = input->readWholeFile();

    if (inputData.second < 0) {
        return FileSystemResult::Error;
    }

    auto output = openFile(destination, FileOpenMode::Write);
    auto written = output->writeBytes(inputData.first.get(), inputData.second);
    if (written != inputData.second) {
        return FileSystemResult::Error;
    }

    return FileSystemResult::Success;
}

void VirtualFileSystem::freeDirectoryContentsCArray(char** list) const {
    assert(initialized);

    if (list == nullptr) {
        return;
    }

    PHYSFS_freeList(list);
}

}
