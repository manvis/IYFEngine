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

#ifndef FILESYSTEMWATCHER_HPP
#define FILESYSTEMWATCHER_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>

#include "utilities/NonCopyable.hpp"
#include "core/filesystem/FileSystemEvent.hpp"
#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {

class FileSystemWatcher : private iyf::NonCopyable {
public:
    using EventList = std::vector<FileSystemEvent>;
    using Handler = std::function<void(EventList)>;
    
    struct MonitoredDirectory {
        MonitoredDirectory() : monitoredEvents(FileSystemEventFlags::All), recursive(true) {}
        
        /// The path to the directory to monitor
        fs::path path;
        /// Flags indicating events to monitor. FileSystemEventFlags::All is the default.
        FileSystemEventFlags monitoredEvents;
        /// Should subdirectories (if any) be monitored?
        bool recursive;
    };
    
    struct CreateInfo {
        CreateInfo() : ignoreHidden(true), writeChangesToLog(false), automaticallyAddNewDirectories(true), sleepDuration(std::chrono::milliseconds(100)) {}
        
        /// Should hidden files be monitored or not?
        bool ignoreHidden;
        /// Should changes be written to log? This does not affect change reports to callbacks in any way, however, there's a performance penalty.
        ///
        /// Logging is performed differently by each backend and may differ in format or provided data (or may not even work at all).
        bool writeChangesToLog;
        /// Should new directories be automatically added to the list of monitored directories? They will be reported to callbacks regardless.
        bool automaticallyAddNewDirectories;
        /// The number of milliseconds that FileSystemWatcher::run() should sleep for between calls to FileSystemWatcher::poll()
        /// Does nothing if you're calling FileSystemWatcher::poll() manually.
        std::chrono::milliseconds sleepDuration;
        /// The handler function that will receive all callbacks
        Handler handler;
        ///.A vector of directories that will be monitored.
        std::vector<MonitoredDirectory> monitoredDirectories;
    };
    
    /// Creates a FileSystemWatcher appropriate for the current platform
    static std::unique_ptr<FileSystemWatcher> MakePlatformFilesystemWatcher(CreateInfo createInfo);
    
    inline FileSystemWatcher(CreateInfo createInfo) : createInfo(std::move(createInfo)) {
        if (this->createInfo.handler == nullptr) {
            throw std::invalid_argument("The handler function passed to the FileSystemWatcher cannot be nullptr");
        }
    }
    
    inline const CreateInfo& getCreateInfo() const {
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
    const CreateInfo createInfo;
    std::atomic<bool> running;
};
}

#endif /* FILESYSTEMWATCHER_HPP */

