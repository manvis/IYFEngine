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

#ifndef IYF_FILE_MONITOR_TESTS_HPP
#define IYF_FILE_MONITOR_TESTS_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "TestBase.hpp"

#include "core/filesystem/FileSystemWatcher.hpp"

namespace iyf::editor {
class FileSystemWatcher;
}

namespace iyf::test {
enum class FileMonitorTestStep {
    Started,
    InitialCheck,
    ExternalFileCopied,
    ExternalFileMovedIn,
    DirectoryCreated,
    TryToMonitorFile,
    LocalFileWritten,
    LocalFileOpenedForRead,
    FileDeleted,
    DirectoryMovedIn,
    DirectoryRenamed,
    MonitoredDirectoryDeleted,
    DirectoryMovedOut,
    Finished
};

class FileMonitorTests : public TestBase {
public:
    FileMonitorTests(bool verbose);
    virtual ~FileMonitorTests();
    
    virtual std::string getName() const final override {
        return "File system monitoring";
    }
    
    virtual void initialize() final override;
    virtual TestResults run() final override;
    virtual void cleanup() final override;
private:
    void monitorCallback(std::vector<FileSystemEvent> events);
    void printMonitoredDirectories(const std::vector<Path>& directories);
    std::string printEvent(const iyf::FileSystemEvent& event);
    static std::uint64_t WriteRandomFile(const std::string& path, std::size_t byteCount);
    
    std::unique_ptr<iyf::FileSystemWatcher> monitor;
    FileMonitorTestStep step;
    std::string errorText;
    
    std::chrono::time_point<std::chrono::steady_clock> lastModificationNotificationTime;
    std::chrono::time_point<std::chrono::steady_clock> newFileLastModificationNotificationTime;
    std::chrono::time_point<std::chrono::steady_clock> timeSinceLastMovedModification;
    std::size_t modificationNotificationCount;
    std::size_t newFileModificationNotificationCount;
    std::size_t postRenameEventCount;
    bool copyCreatedReceived;
    bool movedReceived;
    bool newFileCreatedReceived;
    bool fileDeleteReceived;
    std::unordered_map<std::string, std::vector<iyf::FileSystemEvent>> dirMoveEventMap;
};
}

#endif // IYF_FILE_MONITOR_TESTS_HPP
