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

#ifndef INOTIFYFILESYSTEMWATCHER_HPP
#define INOTIFYFILESYSTEMWATCHER_HPP

#include "core/filesystem/FileSystemWatcher.hpp"

#include <sys/inotify.h>
#include <limits.h> // NAME_MAX

#include <unordered_map>
#include <mutex>

/// Max 4096 events
#define EVENT_BUFFER_SIZE ((sizeof(struct inotify_event) + NAME_MAX + 1) * 4096)

namespace iyf {
class InotifyFileSystemWatcher : public FileSystemWatcher {
public:
    InotifyFileSystemWatcher(FileSystemWatcher::CreateInfo createInfo);
    
    virtual bool addDirectory(const MonitoredDirectory& monitoredDirectory) final override;
    virtual bool removeDirectory(const fs::path& path) override final;
    
    virtual void poll() final override;
    
    virtual ~InotifyFileSystemWatcher();
    
    virtual std::vector<fs::path> getMonitoredDirectories() const final override;
private:
    bool addDirectoryImpl(const fs::path& path, FileSystemEventFlags flags);
    
    /// For use in addDirectoryImpl() and poll() ONLY. Required by poll(), since it and addDirectoryImpl() lock the same mutex
    /// and that results in a deadlock. poll() needs add directory functionality to automatically add newly created sub-directories
    /// to the watch list.
    /// 
    /// \remark I know about recursive mutexes, but they're slower and have been a source of bugs in the past so I'd rather do this.
    bool addDirectoryImplNoLock(const fs::path& path, FileSystemEventFlags flags);
    
    /// For use in removeDirectory() and poll() ONLY. Required by poll(), since it and removeDirectory() lock the same mutex
    /// and that results in a deadlock. poll() needs directory removal functionality to automatically remove moved or renamed
    /// directories from the watch list.
    bool removeDirectoryImplNoLock(const fs::path& path, bool descriptorNeedsDeletion, bool recursive);
    
    /// Used internally by removeDirectoryImplNoLock().
    std::pair<std::unordered_map<std::string, int>::iterator, bool> removeDirectoryByIterator(std::unordered_map<std::string, int>::iterator it, bool descriptorNeedsDeletion);
    
    /// Adds newly created (or copied, or moved from an untracked location into a tracked one) directories 
    void addNewlyCreatedOrMovedDirectories(const fs::path& dir);
    
    struct Paths {
        Paths(FileSystemEventOrigin origin, fs::path source, fs::path destination) : origin(origin), source(source), destination(destination) {}
        
        FileSystemEventOrigin origin;
        fs::path source;
        fs::path destination;
    };
    
    virtual std::string getBackendName() const final override {
        return "inotify";
    }
    
    void processEvent(struct inotify_event* e);
    
    int fd;
    char eventBuffer[EVENT_BUFFER_SIZE];
    std::unordered_map<int, std::string> descriptorToPath;
    std::unordered_map<std::string, int> pathToDescriptor;
    
    std::unordered_map<std::uint32_t, Paths> movePaths; // Using cookies to map moves
    std::vector<FileSystemEvent> events;
    mutable std::mutex inotifyMutex;
};
}
#endif /* INOTIFYFILESYSTEMWATCHER_HPP */

