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

#ifndef IYF_FILE_SYSTEM_WATCHER_HPP
#define IYF_FILE_SYSTEM_WATCHER_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>

#include "utilities/NonCopyable.hpp"
#include "core/filesystem/FileSystemWatcherCreateInfo.hpp"
#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {

class FileSystemWatcher : private iyf::NonCopyable {
public:
    /// Creates a FileSystemWatcher appropriate for the current platform
    static std::unique_ptr<FileSystemWatcher> MakePlatformFilesystemWatcher(FileSystemWatcherCreateInfo createInfo);
    
    inline FileSystemWatcher(FileSystemWatcherCreateInfo createInfo) : createInfo(std::move(createInfo)) {
        if (this->createInfo.handler == nullptr) {
            throw std::invalid_argument("The handler function passed to the FileSystemWatcher cannot be nullptr");
        }
    }
    
    inline const FileSystemWatcherCreateInfo& getCreateInfo() const {
        return createInfo;
    }
    
    virtual bool addDirectory(const MonitoredDirectory& monitoredDirectory) = 0;
    
    virtual bool removeDirectory(const fs::path& path) = 0;
    
    virtual std::string getBackendName() const = 0;
    
    /// Parses the file system event queue and sends out the callbacks. Make sure to call it regularly.
    /// Alternatively, for best performance, you may want to create a separate thread and call run() in it,
    ///
    /// \remark This method is NOT thread safe. Calling poll from multiple threads or when run() has been called in
    /// a different thread will lead to undefined behaviour and/or crashes.
    virtual void poll() = 0;
    
    /// Runs poll() in a loop unitl stop() is called. This blocks and should be called in a SINGLE separate thread.
    ///
    /// \throws std::logic_error if run() has already been called in another thread.
    void run() {
        if (running) {
            throw std::logic_error("Run can only be called from a single thread.");
        }
        
        while (running) {
            poll();
            
            std::this_thread::sleep_for(createInfo.sleepDuration);
        }
    }
    
    void stop() {
        running = false;
    }
    
    virtual std::vector<fs::path> getMonitoredDirectories() const = 0;
    
    virtual ~FileSystemWatcher() { }
protected:
    const FileSystemWatcherCreateInfo createInfo;
    std::atomic<bool> running;
};
}

#endif // IYF_FILE_SYSTEM_WATCHER_HPP

