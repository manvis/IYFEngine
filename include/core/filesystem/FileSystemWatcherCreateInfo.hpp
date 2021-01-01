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

#ifndef IYF_FILE_SYSTEM_WATCHER_CREATE_INFO_HPP
#define IYF_FILE_SYSTEM_WATCHER_CREATE_INFO_HPP

#include <chrono>
#include <functional>

#include "core/filesystem/FileSystemEvent.hpp"

namespace iyf {
struct MonitoredDirectory {
    MonitoredDirectory() : monitoredEvents(FileSystemEventFlags::All), recursive(true) {}
    
    /// The path to the directory to monitor
    Path path;
    /// Flags indicating events to monitor. FileSystemEventFlags::All is the default.
    FileSystemEventFlags monitoredEvents;
    /// Should subdirectories (if any) be monitored?
    bool recursive;
};

struct FileSystemWatcherCreateInfo {
    FileSystemWatcherCreateInfo() : ignoreHidden(true), writeChangesToLog(false), automaticallyAddNewDirectories(true), sleepDuration(std::chrono::milliseconds(100)) {}
    
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
    /// A handler function that will receive all callbacks
    std::function<void(std::vector<FileSystemEvent>)> handler;
    ///.A vector of directories that will be monitored.
    std::vector<MonitoredDirectory> monitoredDirectories;
};
}

#endif // IYF_FILE_SYSTEM_WATCHER_CREATE_INFO_HPP

