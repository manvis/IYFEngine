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

#ifndef IYF_FILE_SYSTEM_EVENT_HPP
#define IYF_FILE_SYSTEM_EVENT_HPP

#include <string>
#include <vector>

#include "cppFilesystem.hpp"
#include "utilities/enum_flag_ops.hpp"

namespace iyf {

enum class FileSystemEventFlags : std::uint32_t {
    /// Created events are sent when a new item is created in a tracked directory, copied into it or moved into it from an
    /// UNTRACKED directory.
    ///
    /// \todo Can I report inotify's WRITE_CLOSE in a portable way? That would allow us to avoid most troubles mentioned in the
    /// warning below. Or would it? If the file isn't locked, can there be multiple writers?
    ///
    /// \warning A file that has been recently created or modified may not be complete and suitable for reading or modification.
    /// For example, it's entirely possible that a copy of a large file is not complete by the time this event is sent 
    /// and processed. Therefore, you should check if all writing operations have been completed before trying to access a file.
    /// The exact process depends on your OS and requirements. The simplest cross platform way is to wait until the file modification
    /// events stop. Set a timer, a threshold and, if the file hasn't been modified in a certain amount of time, it's LIKELY safe
    /// to write. That's what the IYFengine does. Then again, there's no guarantee someone else won't open the file before you do.
    /// There are more solutions. On Linux, you could try to popen "lsof -F p FILENAME", which is quite simple, but the same
    /// problem persits. You could also look into file locking mechanisms. However, since most programs don't check for locks,
    /// there's a chance your locks would simply get ignored. However, if you control all reads and writes into the monitored directories,
    /// they may be extremely useful
    Created = 0x1,
    /// Deleted events are sent when a file or a directory located in a tracked directory is deleted or moved into an UNTRACKED
    /// directory.
    ///
    /// \warning If you receive a Delete event for a directory, you should assume that ALL files inside it are gone even if their deletion
    /// has not been explicitly reported
    Deleted = 0x2,
    /// Moved events are sent when a file is moved from one TRACKED directory to another TRACKED directory or renamed.
    Moved = 0x4,
    /// Modified events are sent when a file or its metadata (e.g., permissions) change
    /// 
    /// \warning Same problem applies as with file creation, so check the warning there.
    Modified = 0x8,
//     /// Closed events are sent when a tracked file is closed.
//     /// \warning Not all backends support this flag.
//     Closed = 0x10,
//     /// Open events are sent when a tracked file is opened.
//     /// \warning Not all backends support this flag.
//     Opened = 0x20,
//     /// This flag represents all events common to all backends (Created, Deleted, Moved, Modified) and should be the default one
//     /// in most cases.
//     AllCommon = 0xF,
//     /// This flag represents all events, including those that only specific backends can track.
//     All = 0x3F 
    All = 0xF
};

IYF_DEFINE_ENUM_FLAG_OPS(FileSystemEventFlags)

enum class FileSystemEventOrigin : std::uint32_t {
    File,
    Directory
};

/// Data about a single file system event
/// \todo these explanations below MAY BE INCORRECT on Windows
class FileSystemEvent {
public:
    FileSystemEvent(FileSystemEventFlags eventType, FileSystemEventOrigin eventOrigin, fs::path source, fs::path destination)
        : eventType(eventType), eventOrigin(eventOrigin), source(std::move(source)), destination(std::move(destination)) {}
public:
    inline FileSystemEventFlags getType() const {
        return eventType;
    }
    
    inline FileSystemEventOrigin getOrigin() const {
        return eventOrigin;
    }
    
    /// Returns the path to the item that triggered this event.
    ///
    /// \remark Can be empty when getType() == FileSystemEventFlags::Moved. It happens when an item is moved from an untracked directory
    /// into a tracked one.
    const fs::path& getSource() const {
        return source;
    }
    
    /// Returns the destination path of a move operation. Unless getType() == FileSystemEventFlags::Moved, this path is always empty.
    ///
    /// \remark This path may be empty even if getType() == FileSystemEventFlags::Moved. It happens when an item is moved from a tracked
    /// directory into an untracked one.
    const fs::path& getDestination() const {
        return destination;
    }
private:
    /// Type of the event
    FileSystemEventFlags eventType;
    FileSystemEventOrigin eventOrigin;
    fs::path source;
    fs::path destination;
};
}

#endif // IYF_FILE_SYSTEM_EVENT_HPP

