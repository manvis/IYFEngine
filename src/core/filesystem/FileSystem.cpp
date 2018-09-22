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

#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "core/filesystem/File.hpp"
#include "core/Logger.hpp"
#include "core/Project.hpp"
#include "core/Constants.hpp"

#ifdef __linux__
#include <stdio.h>
#elif _WIN64
// TODO what to include here
#endif // OS CHECK

namespace iyf {
FileSystem::FileSystem(bool editorMode, char* argv, bool skipSystemPackageMounting) : editorMode(editorMode) {
    PHYSFS_init(argv);
    
    if (editorMode) {
        // TODO Symlinks should only be enabled when running straight from the build folder (since data folder is symlinked then)
        PHYSFS_permitSymbolicLinks(1);
    }

    dirSeparator = PHYSFS_getDirSeparator();
    baseDir = PHYSFS_getBaseDir();

    // Only skipped when running the SystemAssetPackager. It is a tool that makes the SystemAssetPacks we want to mount here
    if (!skipSystemPackageMounting) {
        // System assets should always be stored next to the executable
        fs::path systemAssetRealPath = baseDir / con::SystemAssetPackName;
        
        if (PHYSFS_mount(systemAssetRealPath.c_str(), "", 1) == 0) {
            LOG_E("Failed to mount the system asset archive (" << systemAssetRealPath << "). PHYSFS error: " << getLastErrorText());
            throw std::runtime_error(std::string("PHYSFS failed to mount a mandatory archive"));
        }
    }
}

hash64_t FileSystem::computeFileHash(const fs::path& path) const {
    File file(path, File::OpenMode::Read);
    const auto data = file.readWholeFile();
    
    hash64_t hash = HF(data.first.get(), data.second);
    return hash;
}

void FileSystem::setWritePath(fs::path realPath) {
    if (PHYSFS_setWriteDir(realPath.c_str()) == 0) {
        LOG_E("Failed to set the write directory to (" << realPath << "). PHYSFS error: " << getLastErrorText());
        throw std::runtime_error("Failed to set the write directory");
    }
    
    currentWriteDir = std::move(realPath);
}

bool FileSystem::rename(const fs::path& source, const fs::path& destination) const {
    boost::system::error_code ec;
    fs::rename(source, destination, ec);
    
    if (ec) {
        LOG_E("Failed to move a file from " << source << " to " << destination << "\n\tERROR (CODE): " << ec.message() << " (" << ec.value() << ")");
        return false;
    } else {
        return true;
    }
}

void FileSystem::addReadPath(fs::path realPath, const fs::path& virtualPath, bool appendToSearchPath) {
    if (PHYSFS_mount(realPath.c_str(), virtualPath.c_str(), appendToSearchPath ? 1 : 0) == 0)  {
        LOG_E("Failed to mount a real path (" << realPath << ") for reading as virtual path (" << virtualPath << "). PHYSFS error: " << getLastErrorText());
        throw std::runtime_error("Failed to mount a path");
    }
    
    readPaths.push_back(std::move(realPath));
}

void FileSystem::setResourcePathsForProject(const Project* project) {
    // First of all, handle the write directory
    if (editorMode) {
        // When running in editor mode, the write path matches the Project root
        const auto& projectRoot = project->getRootPath();
        setWritePath(projectRoot.string());
        
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
    
    // Unmount all existing read paths, except for the system asset path.
    if (!readPaths.empty()) {
        for (const auto& rp : readPaths) {
            if (PHYSFS_unmount(rp.c_str()) == 0) {
                LOG_E("Failed to unmount a read path (" << rp << ") that was used by a previous project. PHYSFS error: " << getLastErrorText());
                throw std::runtime_error("Failed to unmount a read path");
            }
        }
        
        readPaths.clear();
    }
    
    // Next, handle the read directories
    if (editorMode) {
        // When running in editor mode, the read path matches the project root 
        const auto& projectRoot = project->getRootPath();
        addReadPath(projectRoot.string(), "/", true);
        
        const fs::path platformAssetPath = projectRoot / con::PlatformIdentifierToName(con::GetCurrentPlatform());
        
        if (fs::exists(platformAssetPath)) {
            LOG_V("Mounting the asset folder for the current platform: " << platformAssetPath);
            addReadPath(platformAssetPath, "/", true);
        } else {
            /// This can happen before the first real project is loaded and, therefore, it's not an error.
            LOG_I("The asset path for the current platform (" << platformAssetPath << ") does not exist. Skipping its mounnting.");
        }
    } else {
        // When running in game mode, mount the data archive that contains the contents of the asset path
        fs::path assetPackPath = project->getRootPath();
        assetPackPath /= con::DefaultReleasePackName;
        addReadPath(assetPackPath.string(), con::AssetPath.generic_string(), true);
        
        // TODO mount patches, mods and DLCs
    }
}

const char* FileSystem::getLastErrorText() const {
    return PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
}

std::string FileSystem::logSearchPath(const std::string& pathSeparator) const {
    std::stringstream ss;
    char **i;

    for (i = PHYSFS_getSearchPath(); *i != NULL; i++) {
        ss << *i << pathSeparator;
    }
    
    return ss.str();
}

FileSystem::~FileSystem() {
    PHYSFS_deinit();
}

}
