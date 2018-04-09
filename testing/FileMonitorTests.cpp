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

#include "FileMonitorTests.hpp"

#include <fstream>
#include <random>

#include "core/Logger.hpp"
#include "core/filesystem/cppFilesystem.hpp"

namespace iyf::test {
/// If this is true, and the test was created in verbose mode, the file monitor backends will output
/// all events (including those that aren't sent to the callback) to log files. This may make the
/// output too noisy to read.
const bool VERBOSE_TEST_EQUALS_ALL_EVENT_LOGGING = true;

const fs::path EXTERNAL_SOURCE_DIR_NAME = "fsTestSource";
const fs::path MONITORED_DIR_NAME = "fsTestMonitoredDir";
const fs::path MONITORED_SUBDIR_NAME = "fsTestSubdir";
const fs::path MOVED_EXTERNAL_DIR_NAME = "fsMovedDir";
const fs::path MOVED_EXTERNAL_SUBDIR_A_NAME = "-fsSubdirA-";
const fs::path MOVED_EXTERNAL_SUBDIR_B_NAME = "-fsSubdirB-";
const fs::path MOVED_EXTERNAL_DIR_NAME_POST_RENAME = "fsMovedRenamedDir";
const fs::path MOVED_EXTERNAL_MULTI_NAME = "movedMulti";

const fs::path COPIED_FILE_NAME = "copiedFile.dat";
const fs::path MOVED_FILE_NAME = "movedFile.dat";
const fs::path LOCAL_FILE_NAME = "localFile.dat";

const fs::path COPIED_EXTERNAL_FILE_PATH = EXTERNAL_SOURCE_DIR_NAME / COPIED_FILE_NAME;
const fs::path MOVED_EXTERNAL_FILE_PATH = EXTERNAL_SOURCE_DIR_NAME / MOVED_FILE_NAME;
const fs::path MOVED_EXTERNAL_DIR_PATH = EXTERNAL_SOURCE_DIR_NAME / MOVED_EXTERNAL_DIR_NAME;
const fs::path MOVED_EXTERNAL_SUBDIR_A_PATH = MOVED_EXTERNAL_DIR_PATH / MOVED_EXTERNAL_SUBDIR_A_NAME;
const fs::path MOVED_EXTERNAL_SUBDIR_B_PATH = MOVED_EXTERNAL_DIR_PATH / MOVED_EXTERNAL_SUBDIR_B_NAME;

const fs::path COPIED_EXTERNAL_FILE_DESTINATION = MONITORED_DIR_NAME / COPIED_FILE_NAME;
const fs::path MOVED_EXTERNAL_FILE_DESTINATION = MONITORED_DIR_NAME / MOVED_FILE_NAME;
const fs::path MOVED_EXTERNAL_DIR_DESTINATION = MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME;
const fs::path MOVED_EXTERNAL_SUBDIR_A_DESTINATION = MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME / MOVED_EXTERNAL_SUBDIR_A_NAME; // For validation only. Moved when MOVED_EXTERNAL_DIR_NAME is moved
const fs::path MOVED_EXTERNAL_SUBDIR_B_DESTINATION = MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME / MOVED_EXTERNAL_SUBDIR_B_NAME; // For validation only. Moved when MOVED_EXTERNAL_DIR_NAME is moved
const fs::path MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME = MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME_POST_RENAME;
const fs::path MOVED_EXTERNAL_SUBDIR_A_DESTINATION_POST_RENAME =  MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME_POST_RENAME / MOVED_EXTERNAL_SUBDIR_A_NAME; // For validation only. Only the parent gets renamed
const fs::path MOVED_EXTERNAL_SUBDIR_B_DESTINATION_POST_RENAME =  MONITORED_DIR_NAME / MOVED_EXTERNAL_DIR_NAME_POST_RENAME / MOVED_EXTERNAL_SUBDIR_B_NAME; // For validation only. Only the parent gets renamed

const fs::path MONITORED_SUBDIR_PATH = MONITORED_DIR_NAME / MONITORED_SUBDIR_NAME;

const fs::path LOCAL_FILE_PATH = MONITORED_SUBDIR_PATH / LOCAL_FILE_NAME;

const std::chrono::milliseconds LAST_MODIFICATION_THRESHOLD(500);
const std::chrono::milliseconds POST_POLL_SLEEP_DURATION(50);

const std::size_t MOVED_FILE_COUNT_ROOT = 32;
const std::size_t MOVED_FILE_COUNT_SUBDIR = 16;
const std::size_t FILE_SIZE_BYTES = 1024 * 1024 * 64;

const std::size_t EXPECTED_UNIQUE_MOVED_DIR_ITEM_COUNT = MOVED_FILE_COUNT_ROOT + (MOVED_FILE_COUNT_SUBDIR * 2) + 3; // 3 events for the directories 

enum class MovedDir {
    Base, SubDirA, SubDirB
};

static std::string MakeMovedFileName(MovedDir identifier, std::size_t fileID) {
    std::stringstream ss;
    
    switch (identifier) {
    case MovedDir::Base:
        ss << MOVED_EXTERNAL_DIR_PATH.string() << "/";
        ss << fileID;
        ss << ".tst";
        break;
    case MovedDir::SubDirA:
        ss << MOVED_EXTERNAL_SUBDIR_A_PATH.string() << "/";
        ss << fileID;
        ss << ".tsa";
        break;
    case MovedDir::SubDirB:
        ss << MOVED_EXTERNAL_SUBDIR_B_PATH.string() << "/";
        ss << fileID;
        ss << ".tsb";
        break;
    }
    
    return ss.str();
}

FileMonitorTests::FileMonitorTests(bool verbose) : TestBase(verbose) { }
FileMonitorTests::~FileMonitorTests() { }

std::uint64_t FileMonitorTests::WriteRandomFile(const std::string& path, std::size_t byteCount) {
    std::random_device rd;
    std::uint64_t seed = rd();
    
    std::ofstream out(path, std::ios::binary | std::ios_base::out | std::ios_base::trunc);
    
    for (std::size_t i = 0; i < (byteCount / sizeof(seed)); ++i) {
        seed++;
        out.write(reinterpret_cast<char*>(&seed), sizeof(seed));
    }
    
    return seed;
}

void FileMonitorTests::initialize() {
    errorText.clear();
    
    if (!fs::create_directory(EXTERNAL_SOURCE_DIR_NAME) || !fs::create_directory(MONITORED_DIR_NAME) || 
        !fs::create_directory(MOVED_EXTERNAL_DIR_PATH) || !fs::create_directory(MOVED_EXTERNAL_SUBDIR_A_PATH) ||
        !fs::create_directory(MOVED_EXTERNAL_SUBDIR_B_PATH)) {
        throw std::runtime_error("Failed to create a directory required for a test");
    }
    
    WriteRandomFile(COPIED_EXTERNAL_FILE_PATH.string(), FILE_SIZE_BYTES);
    WriteRandomFile(MOVED_EXTERNAL_FILE_PATH.string(), FILE_SIZE_BYTES);
    
    for (std::size_t i = 0; i < MOVED_FILE_COUNT_ROOT; ++i) {
        WriteRandomFile(MakeMovedFileName(MovedDir::Base, i), FILE_SIZE_BYTES);
    }
    
    for (std::size_t i = 0; i < MOVED_FILE_COUNT_SUBDIR; ++i) {
        WriteRandomFile(MakeMovedFileName(MovedDir::SubDirA, i), FILE_SIZE_BYTES);
    }
    
    for (std::size_t i = 0; i < MOVED_FILE_COUNT_SUBDIR; ++i) {
        WriteRandomFile(MakeMovedFileName(MovedDir::SubDirB, i), FILE_SIZE_BYTES);
    }
    
    iyf::FileSystemWatcher::MonitoredDirectory dir;
    dir.path = MONITORED_DIR_NAME.string();
    
    // Finally, create the file system watcher
    iyf::FileSystemWatcher::CreateInfo ci;
    ci.writeChangesToLog = VERBOSE_TEST_EQUALS_ALL_EVENT_LOGGING ? isOutputVerbose() : false;
    ci.monitoredDirectories.emplace_back(std::move(dir));
    ci.handler = std::bind(&FileMonitorTests::monitorCallback, this, std::placeholders::_1);
    monitor = iyf::FileSystemWatcher::MakePlatformFilesystemWatcher(ci);
    
    if (isOutputVerbose()) {
        LOG_V("File monitor backend: " << monitor->getBackendName());
    }
    
    step = FileMonitorTestStep::Started;
}

TestResults FileMonitorTests::run() {
    // TEST STEP: We poll and check if any events have been reported. There should be none.
    step = FileMonitorTestStep::InitialCheck;
    monitor->poll();
    
    if (!errorText.empty()) {
        return TestResults(false, errorText);
    }
    
    // We should have a single monitored directory at the moment.
    auto directories = monitor->getMonitoredDirectories();
    if (directories.size() != 1) {
        std::stringstream ss;
        
        ss << "Unexpected directory(-ies) monitored by the FileSystemWatcher:\n";
        
        for (const auto& dir : directories) {
            ss << dir << "\n";
        }
        
        return TestResults(false, ss.str());
    }
    
    if (directories[0] != MONITORED_DIR_NAME) {
        std::stringstream ss;
        ss << "Monitored directory was called:\n\t" << directories[0];
        ss << "\n";
        ss << "Should have been:\n\t" << MONITORED_DIR_NAME;
        
        return TestResults(false, ss.str());
    }
    
    // TEST STEP: Try to copy an external file and get a notification about it
    step = FileMonitorTestStep::ExternalFileCopied;
    
    // Perform the copy in a separate thread to simulate an external operation
    std::thread copyThread([](){fs::copy(COPIED_EXTERNAL_FILE_PATH, COPIED_EXTERNAL_FILE_DESTINATION);});
    
    // Clear the values that will be set in the callback
    modificationNotificationCount = 0;
    copyCreatedReceived = false;
    lastModificationNotificationTime = std::chrono::steady_clock::now();
    
    while (true) {
        monitor->poll();
        
        // Abort the test if an error has been reported
        if (!errorText.empty()) {
            copyThread.join();
            return TestResults(false, errorText);
        }
        
        // Sleep for a while between polls, just like the editor does
        std::this_thread::sleep_for(std::chrono::milliseconds(POST_POLL_SLEEP_DURATION));
        
        // Modification events may continue for some time (the file is being written, after all)
        // This check ensures we catch them all and they don't mess up other tests
        auto delta = std::chrono::steady_clock::now() - lastModificationNotificationTime;
        if (delta > LAST_MODIFICATION_THRESHOLD) {
            break;
        }
    }
    
    copyThread.join();
    
    if (!copyCreatedReceived) {
        return TestResults(false, "Did not get notified about a file copy.");
    }
    
    // TEST STEP: Try to move in an external file, most logic matches 
    step = FileMonitorTestStep::ExternalFileMovedIn;
    std::thread moveThread([](){fs::rename(MOVED_EXTERNAL_FILE_PATH, MOVED_EXTERNAL_FILE_DESTINATION);});
    
    // Once again, prepare values that will be set in the callback
    modificationNotificationCount = 0;
    movedReceived = false;
    lastModificationNotificationTime = std::chrono::steady_clock::now();
    
    while (true) {
        monitor->poll();
        
        if (!errorText.empty()) {
            moveThread.join();
            return TestResults(false, errorText);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(POST_POLL_SLEEP_DURATION));
        
        auto delta = std::chrono::steady_clock::now() - lastModificationNotificationTime;
        
        if (delta > LAST_MODIFICATION_THRESHOLD) {
            break;
        }
    }
    
    moveThread.join();
    
    if (!movedReceived) {
        return TestResults(false, "Did not get notified about a file move.");
    }
    
    // TEST STEP: Try to create a new directory and check if it gets automatically added to the list of monitored directories
    step = FileMonitorTestStep::DirectoryCreated;
    fs::create_directory(MONITORED_SUBDIR_PATH);
    
    // Sleep for a few milliseconds before polling to make sure the event gets noticed.
    // This should only fire a single quick event, that's why we can do away without multiple polls
    std::this_thread::sleep_for(std::chrono::milliseconds(POST_POLL_SLEEP_DURATION));
    
    monitor->poll();
    
    if (!errorText.empty()) {
        return TestResults(false, errorText);
    }
    
    directories = monitor->getMonitoredDirectories();
    
    // Separate scope to avoid name collision with different test steps
    {
        if (directories.size() == 0) {
            // Not a joke - actually happened
            return TestResults(false, "Creation of a new directory wiped the whole tracked directory list");
        } else if (directories.size() != 2) {
            std::stringstream ss;
            
            ss << "Expected two directories to be in the list:\n\t\t";
            ss << MONITORED_DIR_NAME.string() << "\n\t\t";
            ss << MONITORED_SUBDIR_PATH.string() << "\n\t\t";
            ss << "Got this instead:";
            for (const auto& d : directories) {
                ss << "\n\t\t" << d;
            }
            
            return TestResults(false, ss.str());
        }
        
        auto findBaseDir = std::find(directories.begin(), directories.end(), MONITORED_DIR_NAME);
        auto findNewDir = std::find(directories.begin(), directories.end(), MONITORED_SUBDIR_PATH);
        
        if (findBaseDir == directories.end()) {
            return TestResults(false, "The base directory was lost after the creation of a new directory");
        }
        
        if (findNewDir == directories.end()) {
            return TestResults(false, "The newly created directory was not in the list of monitored directories");
        }
    }
    
    // TEST STEP: try to monitor a file. The File System Watcher only knows how to monitor directories, trying to monitor files should result in an error
    step = FileMonitorTestStep::TryToMonitorFile;
    
    assert(fs::exists(COPIED_EXTERNAL_FILE_DESTINATION));
    
    iyf::FileSystemWatcher::MonitoredDirectory md;
    md.path = COPIED_EXTERNAL_FILE_DESTINATION.string();
    
    // Try adding a file
    bool fileMonitorResult = monitor->addDirectory(md);
    if (fileMonitorResult) {
        return TestResults(false, "The backend allowed a FILE monitor to be added. This is forbidden because we only allow watching of directories.");
    }
    
    monitor->poll();
    
    // No events should have been returned
    if (!errorText.empty()) {
        return TestResults(false, errorText);
    }
    
    directories = monitor->getMonitoredDirectories();
    if (std::find(directories.begin(), directories.end(), COPIED_EXTERNAL_FILE_DESTINATION.string()) != directories.end()) {
        return TestResults(false, "A file was added to the list of monitored directories.");
    }
    
    // TEST STEP: try to write a local file (happens when metadata files are written)
    step = FileMonitorTestStep::LocalFileWritten;
    
    std::atomic_uint64_t lastNumber;
    
    std::thread newFileWriteThread([&lastNumber](){
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        lastNumber = WriteRandomFile(LOCAL_FILE_PATH.string(), FILE_SIZE_BYTES);
    });
    
    newFileModificationNotificationCount = 0;
    newFileCreatedReceived = false;
    newFileLastModificationNotificationTime = std::chrono::steady_clock::now();
    
    while (true) {
        monitor->poll();
        
        if (!errorText.empty()) {
            newFileWriteThread.join();
            return TestResults(false, errorText);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(POST_POLL_SLEEP_DURATION));
        
        auto delta = std::chrono::steady_clock::now() - newFileLastModificationNotificationTime;
        
        if (delta > LAST_MODIFICATION_THRESHOLD) {
            break;
        }
    }
    
    newFileWriteThread.join();
    
    // TEST STEP: Try to immediately read the file that has been written. This should not generate any events
    step = FileMonitorTestStep::LocalFileOpenedForRead;
    
    // Can be done in local thread. After all, we only care about NOT getting events here.
    std::ifstream fileIn(LOCAL_FILE_PATH.string(), std::ios::binary | std::ios_base::in);
    
    std::uint64_t lastNumberRead = 0;
    while (!fileIn.eof()) {
        fileIn.read(reinterpret_cast<char*>(&lastNumberRead), sizeof(lastNumberRead));
    }
    fileIn.close();
    
    // Ensure that the what we read matches what we wrote
    if (lastNumberRead != lastNumber) {
        std::stringstream ss;
        ss << "Read and writen numbers did not match: ";
        ss << lastNumber << " vs " << lastNumberRead;
        return TestResults(false, ss.str());
    }
    
    // Poll to NOT get events
    monitor->poll();
    
    // errorText will be not empty if an event has been received
    if (!errorText.empty()) {
        return TestResults(false, errorText);
    }
    
    // Check the result of a previous (writing) step
    if (!newFileCreatedReceived) {
        return TestResults(false, "New file creation was not monitored properly");
    }
    
    // TEST-STEP: Delete a monitored file
    step = FileMonitorTestStep::FileDeleted;
    
    fs::remove(LOCAL_FILE_PATH);
    for (std::size_t i = 0; i < 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        monitor->poll();
        
        if (!errorText.empty()) {
            return TestResults(false, errorText);
        }
    }
    
    // TEST-STEP: Try to move in an external directory.
    step = FileMonitorTestStep::DirectoryMovedIn;
    dirMoveEventMap.clear();
    
    std::thread moveDirThread([](){fs::rename(MOVED_EXTERNAL_DIR_PATH, MOVED_EXTERNAL_DIR_DESTINATION);});
    
    timeSinceLastMovedModification = std::chrono::steady_clock::now();
    while (true) {
        monitor->poll();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        
        auto delta = std::chrono::steady_clock::now() - timeSinceLastMovedModification;
        if ((dirMoveEventMap.size() == EXPECTED_UNIQUE_MOVED_DIR_ITEM_COUNT) || (delta > std::chrono::seconds(1))) {
            break;
        }
    }
    
    moveDirThread.join();
    
    // Each newly moved in file or directory should trigger a create event
    if (dirMoveEventMap.size() != EXPECTED_UNIQUE_MOVED_DIR_ITEM_COUNT) {
        std::stringstream ss;
        ss << "Failed to receive some of the expected file move events\n";
        for (const auto& e : dirMoveEventMap) {
            ss << "---FILE: " << e.first << "\n\t\t";
            
            for (const auto& i : e.second) {
                ss << printEvent(i) << "\n\t\t";
            }
        }
        
        return TestResults(false, ss.str());
    } else {
        if (isOutputVerbose()) {
            LOG_V("Successfully received all " << EXPECTED_UNIQUE_MOVED_DIR_ITEM_COUNT << " expected events after an external directory move")
        }
    }
    
    // TODO is this really OK? I think some other backends may report more events during move
    for (const auto& e : dirMoveEventMap) {
        if (e.second.size() != 1) {
            return TestResults(false, std::string("Moving in an external directory generated multiple events for item:") + e.first);
        }
    }
    
    // TODO validate names. Number is not everything
//     // Validate names and multiple events
//     for (std::size_t i = 0; i < MOVED_EXTERNAL_FILE_PATH) {
//         
//     }
    // Check to make sure that directories have been added recursively
    directories = monitor->getMonitoredDirectories();
    {
        printMonitoredDirectories(directories);
        
        auto findBaseDir = std::find(directories.begin(), directories.end(), MONITORED_DIR_NAME);
        auto findSubDir = std::find(directories.begin(), directories.end(), MONITORED_SUBDIR_PATH);
        auto movedDir = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_DIR_DESTINATION);
        auto movedSubDirA = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_A_DESTINATION);
        auto movedSubDirB = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_B_DESTINATION);
        
        if (findBaseDir == directories.end()) {
            return TestResults(false, "The base directory was lost after moving in new directories.");
        }
        
        if (findSubDir == directories.end()) {
            return TestResults(false, "The original monitored subdirectory was lost after moving in new directories.");
        }
        
        if (movedDir == directories.end()) {
            return TestResults(false, "A moved directory was not added to the list of monitored directories.");
        }
        
        if (movedSubDirA == directories.end()) {
            return TestResults(false, "A subdirectory (A) of a moved directory was not added to the list of monitored directories.");
        }
        
        if (movedSubDirB == directories.end()) {
            return TestResults(false, "A subdirectory (B) of a moved directory was not added to the list of monitored directories.");
        }
    }
    
    // TEST-STEP: Try to rename a directory
    step = FileMonitorTestStep::DirectoryRenamed;
    postRenameEventCount = 0;
    std::thread renameDirThread([](){fs::rename(MOVED_EXTERNAL_DIR_DESTINATION, MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME);});
    
    timeSinceLastMovedModification = std::chrono::steady_clock::now();
    while (true) {
        monitor->poll();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //postRenameEventCount
        auto delta = std::chrono::steady_clock::now() - timeSinceLastMovedModification;
        if (delta > std::chrono::seconds(3)) {
            break;
        }
    }
    
    renameDirThread.join();
    
    // Check if all descriptors have been updated
    directories = monitor->getMonitoredDirectories();
    {
        printMonitoredDirectories(directories);
        
        auto findBaseDir = std::find(directories.begin(), directories.end(), MONITORED_DIR_NAME);
        auto findSubDir = std::find(directories.begin(), directories.end(), MONITORED_SUBDIR_PATH);
        auto movedDir = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME);
        auto movedSubDirA = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_A_DESTINATION_POST_RENAME);
        auto movedSubDirB = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_B_DESTINATION_POST_RENAME);
        
        if (findBaseDir == directories.end()) {
            return TestResults(false, "The base directory was lost after renaming the previously moved directory.");
        }
        
        if (findSubDir == directories.end()) {
            return TestResults(false, "The original monitored subdirectory was lost after renaming the previously moved directory.");
        }
        
        if (movedDir == directories.end()) {
            return TestResults(false, "The previously moved directory was not not renamed in the monitored directory list.");
        }
        
        if (movedSubDirA == directories.end()) {
            return TestResults(false, "A subdirectory (A) of a previously moved directory was not not renamed in the monitored directory list.");
        }
        
        if (movedSubDirB == directories.end()) {
            return TestResults(false, "A subdirectory (B) of a previously moved directory was not not renamed in the monitored directory list.");
        }
    }
    
    // TEST-STEP: Delete a monitored directory
    step = FileMonitorTestStep::MonitoredDirectoryDeleted;
    std::thread deleteThread([](){fs::remove_all(MOVED_EXTERNAL_SUBDIR_B_DESTINATION_POST_RENAME);});
    
    timeSinceLastMovedModification = std::chrono::steady_clock::now();
    while (true) {
        monitor->poll();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto delta = std::chrono::steady_clock::now() - timeSinceLastMovedModification;
        if (delta > std::chrono::seconds(3)) {
            break;
        }
    }
    
    deleteThread.join();
    
    // Check if descriptors have been erased
    directories = monitor->getMonitoredDirectories();
    {
        printMonitoredDirectories(directories);
        
        auto findBaseDir = std::find(directories.begin(), directories.end(), MONITORED_DIR_NAME);
        auto findSubDir = std::find(directories.begin(), directories.end(), MONITORED_SUBDIR_PATH);
        auto movedDir = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME);
        auto movedSubDirA = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_A_DESTINATION_POST_RENAME);
        auto movedSubDirB = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_B_DESTINATION_POST_RENAME);
        
        if (findBaseDir == directories.end()) {
            return TestResults(false, "The base directory was lost after removing a subdirectory.");
        }
        
        if (findSubDir == directories.end()) {
            return TestResults(false, "The original monitored subdirectory was lost after removing a subdirectory.");
        }
        
        if (movedDir == directories.end()) {
            return TestResults(false, "The previously renamed directory was lost after removing a subdirectory.");
        }
        
        if (movedSubDirA == directories.end()) {
            return TestResults(false, "A subdirectory (A) of a previously renamed directory was lost after removing a subdirectory.");
        }
        
        if (movedSubDirB != directories.end()) {
            return TestResults(false, "A subdirectory (B) of a previously renamed directory was not removed after being explicitly deleted.");
        }
    }
    
    // TEST-STEP: Move out a monitored directory
    step = FileMonitorTestStep::DirectoryMovedOut;
    std::thread moveOutThread([](){fs::rename(MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME, EXTERNAL_SOURCE_DIR_NAME / MOVED_EXTERNAL_MULTI_NAME);});
    
    timeSinceLastMovedModification = std::chrono::steady_clock::now();
    while (true) {
        monitor->poll();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        //postRenameEventCount
        auto delta = std::chrono::steady_clock::now() - timeSinceLastMovedModification;
        if (delta > std::chrono::seconds(3)) {
            break;
        }
    }
    
    moveOutThread.join();
    
    directories = monitor->getMonitoredDirectories();
    {
        printMonitoredDirectories(directories);
        
        auto findBaseDir = std::find(directories.begin(), directories.end(), MONITORED_DIR_NAME);
        auto findSubDir = std::find(directories.begin(), directories.end(), MONITORED_SUBDIR_PATH);
        auto movedDir = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_DIR_DESTINATION_POST_RENAME);
        auto movedSubDirA = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_A_DESTINATION_POST_RENAME);
        auto movedSubDirB = std::find(directories.begin(), directories.end(), MOVED_EXTERNAL_SUBDIR_B_DESTINATION_POST_RENAME);
        
        if (findBaseDir == directories.end()) {
            return TestResults(false, "The base directory was lost after removing a subdirectory.");
        }
        
        if (findSubDir == directories.end()) {
            return TestResults(false, "The original monitored subdirectory was lost after removing a subdirectory.");
        }
        
        if (movedDir != directories.end()) {
            return TestResults(false, "The previously renamed directory was not removed after being moved to an a subdirectory.");
        }
        
        if (movedSubDirA != directories.end()) {
            return TestResults(false, "A subdirectory (A) of a previously renamed directory was not removed after being moved to an external directory.");
        }
        
        if (movedSubDirB != directories.end()) {
            return TestResults(false, "A subdirectory (B) of a previously renamed directory was not removed after being moved to an external directory.");
        }
    }
    
    // TEST-STEP: finish
    step = FileMonitorTestStep::Finished;
    //std::this_thread::sleep_for(std::chrono::milliseconds(60000));
    return TestResults(true, "");
}

void FileMonitorTests::cleanup() {
    monitor = nullptr;
    
    if (fs::remove_all(EXTERNAL_SOURCE_DIR_NAME) == 0 || fs::remove_all(MONITORED_DIR_NAME) == 0) {
        throw std::runtime_error("Failed to remove one or more directories used for a test");
    }
}

std::string FileMonitorTests::printEvent(const iyf::FileSystemEvent& event) {
    std::stringstream ss;
    ss << "\tFILE CHANGE CALLBACK EVENT\n";
    
    ss << "\t\tOrigin:      ";
    if (event.getOrigin() == iyf::FileSystemEventOrigin::Directory) {
        ss << "directory\n";
    } else {
        ss << "file\n";
    }
    
    ss << "\t\tType:        ";
    switch(event.getType()) {
    case iyf::FileSystemEventFlags::Created:
        ss << "created\n";
        break;
    case iyf::FileSystemEventFlags::Deleted:
        ss << "deleted\n";
        break;
    case iyf::FileSystemEventFlags::Modified:
        ss << "modified\n";
        break;
    case iyf::FileSystemEventFlags::Moved:
        ss << "moved\n";
        break;
    default:
        throw std::runtime_error("Unknown event flag was reported to the print function.");
    }
    
    ss << "\t\tSource:      " << event.getSource().string();
    
    if (event.getType() == iyf::FileSystemEventFlags::Moved) {
        ss << "\n\t\tDestination: " << event.getDestination().string();
    }
    
    return ss.str();
}

void FileMonitorTests::monitorCallback(iyf::FileSystemWatcher::EventList events) {
    if (events.empty()) {
        errorText += "Unexpected callback call with an empty event list\n";
    }
    
    switch (step) {
        case FileMonitorTestStep::Started:
            errorText = "Callback ran before poll was called for the first time\n";
            break;
        case FileMonitorTestStep::InitialCheck:
            errorText += "Unexpected file system event detected before any operations have been performed.\n";
            break;
        case FileMonitorTestStep::ExternalFileCopied: {
                std::size_t count = 0;
                
                for (const iyf::FileSystemEvent& e : events) {
                    if (e.getOrigin() == iyf::FileSystemEventOrigin::Directory) {
                        errorText += "Expected a file copy event, got a directory event instead:\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    // We're using a lexographical compare here instead of fs::equivalent because the later causes a race condition to occur.
                    // Problem? fs::equivalent uses stat to check if paths resolve to the same file. Inotify seems to beat stat and it notifies
                    // us about the new file BEFORE stat can find it
                    if (e.getSource() != COPIED_EXTERNAL_FILE_DESTINATION) {
                        errorText += "Unexpected file event detected:\n";
                        errorText += printEvent(e);
                        errorText += std::string("\n\n\t\tEXPECTED PATH WAS: ") + COPIED_EXTERNAL_FILE_DESTINATION.string();
                        
                        return;
                    }
                    
                    if (e.getType() == iyf::FileSystemEventFlags::Created) {
                        if (copyCreatedReceived) {
                            errorText += "Multiple \"Created\" events received during a single file copy:\n";
                            errorText += printEvent(e);
                        
                            return;
                        }
                        
                        copyCreatedReceived = true;
                        lastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else if (e.getType() == iyf::FileSystemEventFlags::Modified) {
                        if (!copyCreatedReceived) {
                            errorText += "Received a file modification event before (or without) a \"Created\" event during file copy";
                        
                            return;
                        }
                        
                        modificationNotificationCount++;
                        lastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else {
                        errorText += "File event was not \"Created\" or \"Modified\" during file copy\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    count++;
                }
                
                if (isOutputVerbose()) {
                    std::stringstream ss;
                    
                    for (const auto& e : events) {
                        ss << "\n\n" << printEvent(e);
                    }
                    
                    LOG_D("Received " << count << " event(s) during file copy: " << ss.str());
                }
            }
            
            break;
        case FileMonitorTestStep::ExternalFileMovedIn: {
                std::size_t count = 0;
                
                for (const iyf::FileSystemEvent& e : events) {
                    if (e.getOrigin() == iyf::FileSystemEventOrigin::Directory) {
                        errorText += "Expected a file move event, got a directory event instead:\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    // Once again, we're using a lexographical compare
                    if (e.getSource() != MOVED_EXTERNAL_FILE_DESTINATION) {
                        errorText += "Unexpected file event detected:\n";
                        errorText += printEvent(e);
                        errorText += std::string("\n\n\t\tEXPECTED PATH WAS: ") + MOVED_EXTERNAL_FILE_DESTINATION.string();
                        
                        return;
                    }
                    
                    if (e.getType() == iyf::FileSystemEventFlags::Created) {
                        if (movedReceived) {
                            errorText += "Multiple \"Created\" events received during a single file move:\n";
                            errorText += printEvent(e);
                        
                            return;
                        }
                        
                        movedReceived = true;
                        lastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else if (e.getType() == iyf::FileSystemEventFlags::Modified) {
                        if (!movedReceived) {
                            errorText += "Received a file modification event before (or without) a \"Created\" event during file move";
                        
                            return;
                        }
                        
                        modificationNotificationCount++;
                        lastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else {
                        errorText += "File event was not \"Created\" or \"Modified\" during file move\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    count++;
                }
                
                if (isOutputVerbose()) {
                    std::stringstream ss;
                    
                    for (const auto& e : events) {
                        ss << "\n\n" << printEvent(e);
                    }
                    
                    LOG_D("Received " << count << " event(s) during file move: " << ss.str());
                }
            }
            
            break;
        case FileMonitorTestStep::DirectoryCreated: {
                if (events.size() > 1) {
                    std::stringstream ss;
                    
                    for (const auto& e : events) {
                        ss << "\n\n" << printEvent(e);
                    }
                    
                    errorText += "Expected a single event during directory creation. Got more:\n";
                    errorText += ss.str();
                }
                
                const auto& e = events[0];
                
                if (e.getOrigin() == iyf::FileSystemEventOrigin::File) {
                    errorText += "Expected a directory creation event. Got this instead:\n";
                    errorText += printEvent(e);
                    
                    return;
                }
                
                if (isOutputVerbose()) {
                    LOG_D("Directory creation event: \n" << printEvent(e));
                }
            }
        
            break;
        case FileMonitorTestStep::TryToMonitorFile:{
                std::stringstream ss;
                
                errorText += "Unexpected event(s) during attempted file tracker addition: \n";
                for (const auto& e : events) {
                    ss << printEvent(e);
                }
            }
            break;
        case FileMonitorTestStep::LocalFileWritten: {
                std::size_t count = 0;
                
                for (const iyf::FileSystemEvent& e : events) {
                    if (e.getOrigin() == iyf::FileSystemEventOrigin::Directory) {
                        errorText += "Expected a local file creation or modification event, got a directory event instead:\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    // Same deal with lexographical compare as before
                    if (e.getSource() != LOCAL_FILE_PATH) {
                        errorText += "Unexpected file event detected:\n";
                        errorText += printEvent(e);
                        errorText += std::string("\n\n\t\tEXPECTED PATH WAS: ") + LOCAL_FILE_PATH.string();
                        
                        return;
                    }
                    
                    if (e.getType() == iyf::FileSystemEventFlags::Created) {
                        if (newFileCreatedReceived) {
                            errorText += "Multiple \"Created\" events received during a single file creattion:\n";
                            errorText += printEvent(e);
                        
                            return;
                        }
                        
                        newFileCreatedReceived = true;
                        newFileLastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else if (e.getType() == iyf::FileSystemEventFlags::Modified) {
                        if (!newFileCreatedReceived) {
                            errorText += "Received a file modification event before (or without) a \"Created\" event during a single file creation";
                        
                            return;
                        }
                        
                        newFileModificationNotificationCount++;
                        newFileLastModificationNotificationTime = std::chrono::steady_clock::now();
                    } else {
                        errorText += "File event was not \"Created\" or \"Modified\" during a single file creation\n";
                        errorText += printEvent(e);
                        
                        return;
                    }
                    
                    count++;
                }
                
                if (isOutputVerbose()) {
                    std::stringstream ss;
                    
                    for (const auto& e : events) {
                        ss << "\n\n" << printEvent(e);
                    }
                    
                    LOG_D("Received " << count << " event(s) during a local file creation: " << ss.str());
                }
            }
            
            break;
        case FileMonitorTestStep::LocalFileOpenedForRead:
            errorText += "Received an unexpected event(s) during file read (should not have received any):";
            
            for (const auto& e : events) {
                errorText += printEvent(e);
            }
            break;
        case FileMonitorTestStep::FileDeleted: {
                if (fileDeleteReceived) {
                    errorText += "Received additional unexpected events after a file deletion event:";
                    
                    for (const auto& e : events) {
                        errorText += printEvent(e);
                    }
                } else {
                    fileDeleteReceived = true;
                }
                
                if (events.size() > 1) {
                    errorText += "Expected a single file delete event got this:";
                    
                    for (const auto& e : events) {
                        errorText += printEvent(e);
                    }
                    
                    return;
                }
                
                const auto& e = events[0];
                if (e.getOrigin() == iyf::FileSystemEventOrigin::Directory) {
                    errorText += "Expected a local file deletion event, got a directory event instead:\n";
                    errorText += printEvent(e);
                    
                    return;
                }
                
                // Same deal with lexographical compare as before
                if (e.getSource() != LOCAL_FILE_PATH) {
                    errorText += "Unexpected file event detected during deletion:\n";
                    errorText += printEvent(e);
                    errorText += std::string("\n\n\t\tEXPECTED PATH WAS: ") + LOCAL_FILE_PATH.string();
                    
                    return;
                }
            }
            
            break;
        case FileMonitorTestStep::DirectoryMovedIn: {
                for (const auto& e : events) {
                    auto result = dirMoveEventMap.find(e.getSource().string());
                    
                    if (result == dirMoveEventMap.end()) {
                        dirMoveEventMap.insert({e.getSource().string(), {e}});
                    } else {
                        result->second.push_back(e);
                    }
                    //LOG_D(printEvent(e));
                }
            }
            break;
        case FileMonitorTestStep::DirectoryRenamed: {
                LOG_D("TRACKED DIR RENAME");
                
                for (const auto& e: events) {
                    LOG_D(printEvent(e));
                }
                
                postRenameEventCount++;
            }
            break;
        case FileMonitorTestStep::MonitoredDirectoryDeleted: {
                LOG_D("TRACKED DIR DELETE");

                for (const auto& e: events) {
                    LOG_D(printEvent(e));
                }
            }
            break;
        case FileMonitorTestStep::DirectoryMovedOut: {
                LOG_D("TRACKED DIR MOVE OUT");

                for (const auto& e: events) {
                    LOG_D(printEvent(e));
                }
            }
            
            break;
        case FileMonitorTestStep::Finished:
            errorText = "Callback ran too long after the last poll was called\n";
            break;
    }
}

void FileMonitorTests::printMonitoredDirectories(const std::vector<fs::path>& directories) {
    if (!isOutputVerbose()) {
        return;
    }
    
    std::stringstream ss;
    
    ss << "Currently monitored directories: ";
    for (const auto& d : directories) {
        ss << "\n\t\t\t" << d.string();
    }
    
    LOG_V(ss.str());
}

}
