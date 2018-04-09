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

#ifndef FILE_SYSTEM_HPP
#define FILE_SYSTEM_HPP

#include <string>
#include <vector>
#include <physfs.h>

#include "core/Engine.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/hashing/Hashing.hpp"

namespace iyf {
class Engine;
class Project;

/// This is merely a C++ wrapper for the PhysFS library (version 3.0+) with some engine specific bookkeeping functions.
/// To learn more about features and limitations of certain methods, you may wish to read the PhysFS docs:
/// https://icculus.org/physfs/docs/html/
class FileSystem : private NonCopyable {
public:
    ~FileSystem();
    
    hash64_t computeFileHash(const fs::path& path) const;
    
    /// Obtain the current write directory used by the virtual file system. The path is provided in platform dependent notation.
    ///
    /// This is a real directory that serves as the root for all File objects currently open in write mode. That is, if this 
    /// path returns /home/user/ and you create a File object that writes to foo/bar.txt, the real path of the file will be
    /// /home/user/foo/bar.txt
    ///
    /// \warning If the Engine was launched in editor mode, the result of this call WILL CHANGE to match the root of the
    /// current Project. If you need to write data that must be available to the Editor regardless of Project (e.g. you're wrtiting
    /// some sort of a plug-in), use regular C++ filesystem functions and write to the directory returned by getPreferenceDirectory().
    ///
    /// If the Engine wasn't launced in editor mode, the result of this function call will always match the result of a 
    /// FileSystem::getPreferenceDirectory() call. 
    ///
    /// \warning If the Engine wasn't launched in editor mode, you should assume that this directory is the only path where the
    /// Engine can safely write.
    ///
    /// \return A path to a write directory.
    inline const fs::path& getCurrentWriteDirectory() const {
        return currentWriteDir;
    }
    
    /// Obtain a real path to the current preference directory. The exact path depends on both the OS and the Project settings that
    /// were provided when starting the engine.
    ///
    /// If the Engine wasn't launced in editor mode, the result of this function call will always match the result of a 
    /// FileSystem::getCurrentWriteDirectory() call. 
    /// 
    /// \warning If the Engine wasn't launched in editor mode, you should assume that this directory is the only path where the
    /// Engine can safely write.
    ///
    /// \return A path to the current user's home directory.
    inline const fs::path& getPreferenceDirectory() const {
        return prefDir;
    }
    
    /// Get the location of the Engine's executable in platform dependent notation. Stays constant during a run. 
    inline const fs::path& getBaseDirectory() const {
        return baseDir;
    }
    
    /// Turns a specified virtual file system path into a real path in platform dependent notation. This is typically used when
    /// files need to be opened by third-party libraries that don't know how to handle the virtual filesystem.
    ///
    /// \warning This method breaks the sandbox, so make sure that it's only used internally and not accesible to mods or other
    /// user generated content.
    ///
    /// \param[in] fileName the file to search for
    /// \return the read only real name of directory containing the fileName
    inline fs::path getRealDirectory(const fs::path& fileName) const {
        const char* dir = PHYSFS_getRealDir(fileName.generic_string().c_str());
        
        if (dir == nullptr) {
            return emptyDir;
        } else {
            return fs::path(dir) / fileName;
        }
    }
    
    /// \brief Deletes a file or an empty directory from the current write directory
    /// 
    /// \param[in] path file or directory to delete
    /// \return whether the deletion was successful, or not
    inline bool deleteFile(const fs::path& path) const {
        return PHYSFS_delete(path.generic_string().c_str());
    }
    
    /// \brief Obtains statistics for a file that exists in the virtual filesystem
    inline int getFileSystemStatistics(const fs::path& path, PHYSFS_Stat& stat) const {
        return PHYSFS_stat(path.generic_string().c_str(), &stat);
    }
    
    /// Creates a directory and all missing parent directories in the current write directory.
    ///  
    /// \param[in] path Directory to create
    /// \return Whether the creation was successful 
    inline bool createDirectory(const fs::path& path) const {
        return PHYSFS_mkdir(path.generic_string().c_str());
    }
    
    /// Checks if the file exists ANYWHERE in the virtual file system
    ///
    /// \param[in] fileName what file to search for
    /// \return exists or not
    inline bool exists(const fs::path& fileName) const {
        return PHYSFS_exists(fileName.generic_string().c_str());
    }
    
    /// \brief Returns an std::vector of file and folder name fs::path in the specified directory
    /// 
    /// The search for the directory happens in read directories that were 
    /// specified during the initialization of the physfs library. Check code in
    /// Engine::setupPhysFS() if you need to change anything.
    /// 
    /// \param[in] path directory, contents of which you want to list
    /// \return std::vector of file and folder name strings in the directory
    inline static std::vector<fs::path> getDirectoryContents(const fs::path& path) {
        // TODO this is probably suboptimal and might need to be replaced with PHYSFS_enumerateFilesCallback
        std::vector<fs::path> files;
        char** fileList = PHYSFS_enumerateFiles(path.generic_string().c_str());
        char** iter;

        for (iter = fileList; *iter != nullptr; iter++) {
            files.push_back(*iter);
        } 

        PHYSFS_freeList(fileList);

        return files;
    }
    
    /// \brief Returns char** to file and folder names in the specified directory
    /// 
    /// The search for the directory happens only in read directories which were 
    /// specified during the initialization of the physfs library. Check code in
    /// Engine::setupPhysFS() if you need to change anything.
    /// 
    /// \warning: the list MUST BE FREED using FileSystem::freeDirectoryContentsCArray()
    ///
    /// \param[in] path directory, contents of which you want to list
    /// \return char** of file and folder names in the directory that MUST BE FREED using FileSystem::freeDirectoryContents
    inline static char** getDirectoryContentsCArray(const fs::path& path) {
        char** fileList = PHYSFS_enumerateFiles(path.generic_string().c_str());
        return fileList;
    }

    /// \brief Frees the list of directory contents
    /// 
    /// Frees the list of directory contents that was obtained using FileSystem::getDirectoryContentsCArray()
    ///
    /// \param[in] list list to free
    inline static void freeDirectoryContentsCArray(char** list) {
        PHYSFS_freeList(list);
    }
    
    /// A debug method that writes the current search path to a log (verbose priority).
    void logSearchPath() const;

    inline PHYSFS_ErrorCode getLastErrorCode() const {
        return PHYSFS_getLastErrorCode();
    }
    
    const char* getLastErrorText() const;
    
    enum class IsFileOpenResult {
        Open,
        NotOpen,
        Error
    };
protected:
private:
    friend class Engine;
    friend class SystemAssetPacker;
    
    /// Set PHYSFS search and write paths to those required by the iyf::Project. Called by our friend Engine.
    ///.
    /// If the Engine is running in game mode, this function is called once during the startup. If the Engine is running in
    /// editor mode, it is called every time the iyf::EditorState is initialized with a new iyf::Project instance.
    ///
    /// \throw std::runtime_error If any PHYSFS operations failed
    /// \param[in] project the project
    void setResourcePathsForProject(const Project* project);
    
    /// Adds a read path. Called by setResourcePathsForProject() and our friend SystemAssetPacker that needs to set
    /// up custom dirs for asset packing
    void addReadPath(fs::path realPath, const fs::path& virtualPath, bool appendToSearchPath);
    
    /// Sets the write path. Called by setResourcePathsForProject() and our friend SystemAssetPacker that needs to set
    /// up custom dirs for asset packing
    void setWritePath(fs::path realPath);
    
    FileSystem(bool editorMode, char* argv, bool skipSystemPackageMounting = false);
    
    bool editorMode;
    
    fs::path prefDir;
    fs::path currentWriteDir;
    fs::path baseDir;
    fs::path emptyDir;
    fs::path dirSeparator;
    std::vector<fs::path> readPaths;
};
}

#endif // FILE_SYSTEM_HPP
