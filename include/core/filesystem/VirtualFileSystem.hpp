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

#ifndef IYF_VIRTUAL_FILE_SYSTEM_HPP
#define IYF_VIRTUAL_FILE_SYSTEM_HPP

#include <string>
#include <vector>
#include <cassert>

#include "io/Path.hpp"
#include "io/FileSystem.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/hashing/Hashing.hpp"

namespace iyf {
class Project;

/// This is merely a C++ wrapper for the PhysFS library (version 3.0+) with some engine specific bookkeeping functions.
/// To learn more about features and limitations of certain methods, you may wish to read the PhysFS docs:
/// https://icculus.org/physfs/docs/html/
class VirtualFileSystem : public FileSystem {
public:
    NON_COPYABLE(VirtualFileSystem)

    virtual ~VirtualFileSystem();

    static VirtualFileSystem& Instance();

    virtual std::unique_ptr<File> openFile(const Path& p, FileOpenMode mode) const final override;
    
    virtual FileHash computeFileHash(const Path& path) const final override;
    
    /// Open a file in the file browser. This function expects a real path.
    virtual FileSystemResult openInFileBrowser(const Path& path) const final override;
    
    /// Obtain the current write directory used by the virtual file system. The path is provided in platform dependent notation.
    ///
    /// This is a real directory that serves as the root for all File objects currently open in write mode. That is, if this 
    /// path returns /home/user/ and you create a File object that writes to foo/bar.txt, the real path of the file will be
    /// /home/user/foo/bar.txt
    ///
    /// \warning If the Engine was launched in editor mode, the result of this call WILL CHANGE to match the root of the
    /// current Project. If you need to write data that must be available to the Editor regardless of Project (e.g. you're wrtiting
    /// some sort of a plug-in), use the DefaultFileSystem and write to the directory returned by getPreferenceDirectory().
    ///
    /// If the Engine wasn't launced in editor mode, the result of this function call will always match the result of a 
    /// VirtualFileSystem::getPreferenceDirectory() call. 
    ///
    /// \warning If the Engine wasn't launched in editor mode, you should assume that this directory is the only path where the
    /// Engine can safely write.
    ///
    /// \return A path to a write directory.
    inline const Path& getCurrentWriteDirectory() const {
        assert(initialized);

        return currentWriteDir;
    }
    
    /// Obtain a real path to the current preference directory. The exact path depends on both the OS and the Project settings that
    /// were provided when starting the engine.
    ///
    /// If the Engine wasn't launced in editor mode, the result of this function call will always match the result of a 
    /// VirtualFileSystem::getCurrentWriteDirectory() call. 
    /// 
    /// \warning If the Engine wasn't launched in editor mode, you should assume that this directory is the only path where the
    /// Engine can safely write.
    ///
    /// \return A path to the current user's home directory.
    inline const Path& getPreferenceDirectory() const {
        assert(initialized);

        return prefDir;
    }
    
    /// Get the location of the Engine's executable in platform dependent notation. Stays constant during a run. 
    inline const Path& getBaseDirectory() const {
        assert(initialized);

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
    Path getRealDirectory(const Path& fileName) const;
    
    /// \brief Deletes a file or an empty directory from the current write directory
    /// 
    /// \param[in] path file or directory to delete
    /// \return whether the deletion was successful, or not
    virtual FileSystemResult remove(const Path& path) const final override;

    virtual FileSystemResult removeRecursive(const Path& path) const final override;
    
    /// \brief Obtains statistics for a file that exists in the virtual filesystem
    virtual FileSystemResult getStats(const Path& path, FileStat& sOut) const final override;
    
    /// Creates a directory and all missing parent directories in the current write directory.
    ///  
    /// \param[in] path Directory to create
    /// \return Whether the creation was successful 
    virtual FileSystemResult createDirectory(const Path& path) const final override;
    
    /// Checks if the file exists ANYWHERE in the virtual file system
    ///
    /// \param[in] fileName what file to search for
    /// \return exists or not
    virtual bool exists(const Path& fileName, FileSystemResult& result) const final override;
    virtual bool isEmpty(const Path& path, FileSystemResult& result) const final override;
    virtual bool isDirectory(const Path& path, FileSystemResult& result) const final override;

    using FileSystem::exists;
    using FileSystem::isEmpty;
    using FileSystem::isDirectory;
    
    /// \brief Returns an std::vector of file and folder name Path in the specified directory
    /// 
    /// The search for the directory happens in read directories that were 
    /// specified during the initialization of the physfs library. Check code in
    /// Engine::setupPhysFS() if you need to change anything.
    /// 
    /// \param[in] path directory, contents of which you want to list
    /// \return std::vector of file and folder name strings in the directory
    virtual std::vector<Path> getDirectoryContents(const Path& path) const final override;
    
    /// \brief Returns char** to file and folder names in the specified directory
    /// 
    /// The search for the directory happens only in read directories which were 
    /// specified during the initialization of the physfs library. Check code in
    /// Engine::setupPhysFS() if you need to change anything.
    /// 
    /// \warning: the list MUST BE FREED using VirtualFileSystem::freeDirectoryContentsCArray()
    ///
    /// \param[in] path directory, contents of which you want to list
    /// \return char** of file and folder names in the directory that MUST BE FREED using VirtualFileSystem::freeDirectoryContents
    char** getDirectoryContentsCArray(const Path& path) const;
    
    /// \warning Not supported by PHYSFS, always returns false
    virtual FileSystemResult rename(const Path&, const Path&) const final override;
    virtual FileSystemResult copyFile(const Path& source, const Path& destination, FileCopyOption option = FileCopyOption::None) const final override;

    /// \brief Frees the list of directory contents
    /// 
    /// Frees the list of directory contents that was obtained using VirtualFileSystem::getDirectoryContentsCArray()
    ///
    /// \param[in] list list to free
    void freeDirectoryContentsCArray(char** list) const;
    
    /// A debug method that writes the current search path to a string
    std::string logSearchPath(const std::string& pathSeparator) const;
protected:
private:
    friend class Engine;
    friend class SystemAssetPacker;

    // MUST be set by the Engine or SystemAssetPacker BEFORE the first call to Instance()
    static char* argv;
    
    /// Adds a read path. Called by setResourcePathsForProject() and our friend SystemAssetPacker that needs to set
    /// up custom dirs for asset packing
    void addReadPath(Path realPath, const Path& virtualPath, bool appendToSearchPath);
    
    /// Sets the write path. Called by setResourcePathsForProject() and our friend SystemAssetPacker that needs to set
    /// up custom dirs for asset packing
    void setWritePath(Path realPath);
    
    VirtualFileSystem();
    bool initialize(const Project* project, bool editorMode, bool skipSystemPackageMounting = false);
    
    bool editorMode;
    bool initialized;
    
    Path prefDir;
    Path currentWriteDir;
    Path baseDir;
    Path emptyDir;
    Path dirSeparator;
    std::vector<Path> readPaths;
};
}

#endif // IYF_FILE_SYSTEM_HPP
