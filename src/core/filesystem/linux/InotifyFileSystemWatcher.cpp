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

#include "core/filesystem/linux/InotifyFileSystemWatcher.hpp"

#include <stdexcept>
#include <sys/select.h>
#include <errno.h>
#include <unistd.h>
#include "core/filesystem/cppFilesystem.hpp"
#include "threading/ThreadProfiler.hpp"

#include "core/Logger.hpp"
#include "utilities/StringUtilities.hpp"

namespace iyf {
struct PathReplacement {
    int wd;
    std::string source;
    std::string destination;
};

InotifyFileSystemWatcher::InotifyFileSystemWatcher(FileSystemWatcherCreateInfo createInfo) : FileSystemWatcher(std::move(createInfo)), fd(-1) {
    fd = inotify_init1(IN_NONBLOCK);
    
    if (fd < 0) {
        std::string errStr = "Failed to initialize inotify file change watcher. ERROR ID: ";
        errStr.append(std::to_string(errno));
        
        LOG_E(errStr);
        throw std::runtime_error(errStr);
    }
    
    movePaths.reserve(30);
    events.reserve(30);
    
    LOG_D("INOTIFY initializing file system watcher with " << getCreateInfo().monitoredDirectories.size() << " initial directories")
    
    for (const auto& d : getCreateInfo().monitoredDirectories) {
        if (!addDirectory(d)) {
            throw std::runtime_error("Failed to add one of initial directories to inotify file system watcher");
        }
    }
}

bool InotifyFileSystemWatcher::addDirectoryImplNoLock(const fs::path& path, FileSystemEventFlags flags) {
    auto existingLookupResult = pathToDescriptor.find(path.string());
    if (existingLookupResult != pathToDescriptor.end()) {
        LOG_W("INOTIFY tried to re-add a tracked path");
        return false;
    }
    
    if (fs::exists(path)) {
        int mask = 0;
        
//         if (static_cast<bool>(flags & FileSystemEventFlags::Created)) {
//             mask |= IN_CREATE;
//         }
//         
//         if (static_cast<bool>(flags & FileSystemEventFlags::Deleted)) {
//             mask |= IN_DELETE;
//         }
//         
//         if (static_cast<bool>(flags & FileSystemEventFlags::Moved)) {
//             mask |= IN_MOVE;
//         }
//         
//         if (static_cast<bool>(flags & FileSystemEventFlags::Modified)) {
//             mask |= IN_MODIFY;
//         }
// 
//        if (static_cast<bool>(flags & FileSystemEventFlags::Closed)) {
//            mask |= IN_CLOSE;
//        }
//        
//        if (static_cast<bool>(flags & FileSystemEventFlags::Opened)) {
//            mask |= IN_OPEN;
//        }
        // Monitor all and filter later because we need to track things like open status all the time
        mask = // IN_ACCESS | // Disabled because it's very noisy and "accessed for reading" does not give us much useful info
               IN_MODIFY | // Detects writes (translates into FileSystemEventFlags::Modified)
               // IN_ATTRIB | // TODO file attribute changes LIKELY need to be enabled and turned into a FileSystemEventFlags::Modified event
               IN_CLOSE_WRITE | // Keeping it on because it may be possible to turn this into a useful extra flag, however, as far as I know, 
                                // WinDOHs does not have this functionality and I'd be fragmenting the API
               // IN_CLOSE_NOWRITE | // Disabled because it's noisy and does not give us much useful info
               // IN_OPEN | // Disabled because the same event is used for read and write. Does not give us much useful info.
               IN_MOVED_FROM | // Detects items moved into external directories (translates into FileSystemEventFlags::Deleted) and
                               // reports the names of files before the rename.
               IN_MOVED_TO | // Detects items moved into the folder from external sources (translates into FileSystemEventFlags::Created)
                             // and reports new names after a file gets renamed (translates into FileSystemEventFlags::Modified).
               IN_CREATE | // Creation of a new file or directory (translates into FileSystemEventFlags::Created)
               IN_DELETE | // Deletion of a file or directory (translates into FileSystemEventFlags::Deleted)
               IN_DELETE_SELF | // Reports a deletion of a tracked dir
               IN_MOVE_SELF | // Reports a move of a tracked dir
               IN_ONLYDIR; // This ensures only directories are tracked
        int wd = inotify_add_watch(fd, path.c_str(), mask);
        
        if (wd < 0) {
            std::stringstream ss;
            ss << "INOTIFY: Failed to add a watcher for path: ";
            ss << path;
            ss << "; ERROR ID: ";
            ss << std::to_string(errno);
            ss << "; ERROR: ";
            ss << strerror(errno);

            LOG_W(ss.str());
            return false;
        }
        
        pathToDescriptor[path.string()] = wd;
        descriptorToPath[wd] = path.string();
        
        if (createInfo.writeChangesToLog) {
            LOG_V("INOTIFY-NEW-DIR\n\t\t" << path << "; WD: " << wd);
        }
        
        return true;
    } else {
        std::string errorString = "Non existing path: ";
        errorString.append(path.string());
        LOG_E(errorString)
        throw std::runtime_error(errorString);
    }
}

bool InotifyFileSystemWatcher::addDirectoryImpl(const fs::path& path, FileSystemEventFlags flags) {
    std::lock_guard<std::mutex> lock(inotifyMutex);
    
    return addDirectoryImplNoLock(path, flags);
}

bool InotifyFileSystemWatcher::addDirectory(const MonitoredDirectory& monitoredDirectory) {
    IYFT_PROFILE(AddMonitoredDirectory, iyft::ProfilerTag::AssetConversion);
    
    if (monitoredDirectory.recursive) {
        bool result = true;
        
        // Add self
        if (!addDirectoryImpl(monitoredDirectory.path, monitoredDirectory.monitoredEvents)) {
            return false;
        }
        
        for (const auto& p: fs::recursive_directory_iterator(monitoredDirectory.path)) {
            if (!fs::is_directory(p)) {
                // Only monitor directories
                continue;
            }
            
            if (!addDirectoryImpl(p.path().string(), monitoredDirectory.monitoredEvents)) {
                result = false;
                
                // TODO should I return immediately? Throw an exception
                LOG_W("Failed to add path to inotify watch list: " << p.path());
            }
        }
        
        return result;
    } else {
        return addDirectoryImpl(monitoredDirectory.path, monitoredDirectory.monitoredEvents);
    }
}

bool InotifyFileSystemWatcher::removeDirectory(const fs::path& path) {
    IYFT_PROFILE(RemoveMonitoredDirectory, iyft::ProfilerTag::AssetConversion);
    
    std::lock_guard<std::mutex> lock(inotifyMutex);
    
    return removeDirectoryImplNoLock(path, true, true);
}

// for (auto it = pathToDescriptor.begin(); it != pathToDescriptor.end();) {
//     if (util::startsWith(it->first, sourceStr)) {
//         std::string postRename = destinationStr + it->first.substr(sourceStr.length());
//         //LOG_D("Map element needs a rename from " << it->first << " to " << postRename);
//         
//         PathReplacement replacement;
//         replacement.wd = it->second;
//         replacement.source = it->first;
//         replacement.destination = std::move(postRename);
//         replacements.push_back(std::move(replacement));
//         
//         it = pathToDescriptor.erase(it);
//     } else {
//         it++;
//     }
// }
// 
// for (const auto& r : replacements) {
//     auto ptdResult = pathToDescriptor.insert({r.destination, r.wd});
//     assert(ptdResult.second);
//     
//     auto dtpElement = descriptorToPath.find(r.wd);
//     assert(dtpElement != descriptorToPath.end());
//     
//     dtpElement->second = r.destination;
// }
std::pair<std::unordered_map<std::string, int>::iterator, bool> InotifyFileSystemWatcher::removeDirectoryByIterator(std::unordered_map<std::string, int>::iterator it, bool descriptorNeedsDeletion) {
    int descriptor = it->second;
    it = pathToDescriptor.erase(it);
    
    auto resultDTP = descriptorToPath.find(descriptor);
    assert(resultDTP != descriptorToPath.end());
    
    bool removalSucceeded = true;
    if (descriptorNeedsDeletion) {
        int result = inotify_rm_watch(fd, descriptor);
        if (result < 0) {
            LOG_W("INOTIFY failed to remove a path: " << resultDTP->second);
            
            removalSucceeded = false;
        }
    }
    
    descriptorToPath.erase(resultDTP);
    
    return {it, removalSucceeded};
}

bool InotifyFileSystemWatcher::removeDirectoryImplNoLock(const fs::path& path, bool descriptorNeedsDeletion, bool recursive) {
    if (recursive) {
        bool removalFailed = false;
        std::size_t removalCount = 0;
        
        for (auto it = pathToDescriptor.begin(); it != pathToDescriptor.end();) {
            if (util::startsWith(it->first, path.string())) {
                auto removalResult = removeDirectoryByIterator(it, descriptorNeedsDeletion);
                it = removalResult.first;
                
                if (!removalResult.second) {
                    removalFailed = true;
                }
                removalCount++;
            } else {
                it++;
            }
        }
        
        if (removalCount == 0 || removalFailed) {
            return false;
        } else {
            return true;
        }
    } else {
        auto resultPTD = pathToDescriptor.find(path.string());
        if (resultPTD == pathToDescriptor.end()) {
            return false;
        }
        
        auto result = removeDirectoryByIterator(resultPTD, descriptorNeedsDeletion);
        
        return result.second;
    }
    
//     auto resultPTD = pathToDescriptor.find(path);
//     if (resultPTD == pathToDescriptor.end()) {
//         return false;
//     }
//     
//     int descriptor = resultPTD->second;
//     
//     auto resultDTP = descriptorToPath.find(descriptor);
//     assert(resultDTP != descriptorToPath.end());
//     
//     bool removalSucceeded = true;
//     
//     // We don't need to remove the descriptor if it has already been closed by inotify. That happens when IN_DELETE_SELF is reported.
//     if (descriptorNeedsDeletion) {
//         int result = inotify_rm_watch(fd, descriptor);
//         if (result < 0) {
//             LOG_W("INOTIFY failed to remove a path: " << resultDTP->second);
//             removalSucceeded = false;
//         }
//     }
//     
//     pathToDescriptor.erase(resultPTD);
//     descriptorToPath.erase(resultDTP);
//     
//     return removalSucceeded;
}

void InotifyFileSystemWatcher::addNewlyCreatedOrMovedDirectories(const fs::path& dir) {
    // The mutex has already been locked here
    bool result = addDirectoryImplNoLock(dir.string(), FileSystemEventFlags::All);
    
    if (!result) {
        LOG_W("Failed to add a dir to tracked list: " << dir.string());
    }
    
    for (const auto& p : fs::recursive_directory_iterator(dir)) {
        if (fs::is_directory(p)) {
            result = addDirectoryImplNoLock(p.path().string(), FileSystemEventFlags::All);
            
            if (!result) {
                LOG_W("Failed to add a dir to tracked list: " << p.path().string());
            }
        }
        
        events.emplace_back(FileSystemEventFlags::Created, FileSystemEventOrigin::File, p.path().string(), fs::path());
    }
}

void InotifyFileSystemWatcher::poll() {
    IYFT_PROFILE(PollFileSystemChanges, iyft::ProfilerTag::AssetConversion);
    
    std::lock_guard<std::mutex> lock(inotifyMutex);
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    
    // Zeroes to return immediately
    timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;

    int result = pselect(fd + 1, &fds, nullptr, nullptr, &timeout, nullptr);
    
    if (result < 0) {
        std::string errStr = "pselect experienced an error while waiting for inotify. ERROR ID: ";
        errStr.append(std::to_string(errno));

        LOG_E(errStr);
        throw std::runtime_error(errStr);
    } else if (result > 0) {
        int length = read(fd, eventBuffer, EVENT_BUFFER_SIZE); 
        if (length < 0) {
            std::string errStr = "Experienced an error while reading inotify events. ERROR ID: ";
            errStr.append(std::to_string(errno));

            LOG_E(errStr);
            throw std::runtime_error(errStr);
        }
        
        // According to a Facebook engineer (http://stackoverflow.com/a/39807418/1723459) :
        // "MOVE_FROM may be the last data that fit in your read buffer on one read and the MOVE_TO may be the first data of a subsequent read."
        // This is NOT handled here because we assume the buffer is always big enough to read everything. The assumption will hold UNTIL someone
        // decides to move a ton of files at once. Reading in a cycle and cleaning afterwards should prevent this
        movePaths.clear();
        events.clear();
        
        for (int i = 0; i < length;) {
            struct inotify_event* e = (struct inotify_event*) &eventBuffer[i];
            
            if (e->wd == -1 || e->mask & IN_Q_OVERFLOW) {
                // TODO recover from this somehow?
                std::string errStr = "Experienced an inotify event buffer overflow.";
                
                LOG_E(errStr);
                throw std::runtime_error(errStr);
            }
            
            processEvent(e);
            
            i += sizeof(struct inotify_event) + e->len;
        }
        
//         if ((e->mask & IN_ISDIR) && ((e->mask & IN_CREATE) || (e->mask & IN_MOVED_TO)) && createInfo.automaticallyAddNewDirectories) {
//             // The mutex has already been locked here. Calling addDirectoryImpl would result in a deadlock.
//             addDirectoryImplNoLock(sourcePath.string(), FileSystemEventFlags::All);
//             
//             for (const auto& p : fs::recursive_directory_iterator(sourcePath)) {
//                 if (fs::is_directory(p)) {
//                     addDirectoryImplNoLock(p.path().string(), FileSystemEventFlags::All);
//                 }
//                 
//                 events.emplace_back(FileSystemEventFlags::Created, FileSystemEventOrigin::File, p.path().string(), fs::path());
//             }
//         }
        
        for (auto& ap : movePaths) {
            auto& p = ap.second;
            
            if (p.source.empty() && p.destination.empty()) {
                throw std::runtime_error("INOTIFY file move event with empty source and destination paths.");
            } else if (p.source.empty()) {
                // Move from an untracked directory is equivalent to the creation of a new item.
                events.emplace_back(FileSystemEventFlags::Created, p.origin, p.destination, fs::path());
                
                // If a directory has been moved in from an untracked directory, all directories and files inside
                // it are "new" and need to be added and reported
                if (p.origin == FileSystemEventOrigin::Directory) {
                    addNewlyCreatedOrMovedDirectories(p.destination);
                }
            } else if (p.destination.empty()) {
                // Move to an untracked directory is equivalent to the deletion of an item.
                events.emplace_back(FileSystemEventFlags::Deleted, p.origin, p.source, fs::path());
                
                removeDirectoryImplNoLock(p.source.string(), true, true);
            } else {
                // Presence of both a source and a destination means a move from one tracked directory to another or a rename
                events.emplace_back(FileSystemEventFlags::Moved, p.origin, p.source, p.destination);
                
                // We need to update the descriptors
                if (p.origin == FileSystemEventOrigin::Directory) {
                    const std::string& sourceStr = p.source.string();
                    const std::string& destinationStr = p.destination.string();
                    // Get the descriptor
                    auto result = pathToDescriptor.find(sourceStr);
                    
                    assert(result != pathToDescriptor.end());
                    
                    // We use an unordered map for extremely quick retrieval of the directory names. Unfortunately, it means
                    // the names aren't sorted and we need to iterate through them to determine what needs changing
                    std::vector<PathReplacement> replacements;
                    replacements.reserve(20);
                    for (auto it = pathToDescriptor.begin(); it != pathToDescriptor.end();) {
                        if (util::startsWith(it->first, sourceStr)) {
                            std::string postRename = destinationStr + it->first.substr(sourceStr.length());
                            //LOG_D("Map element needs a rename from " << it->first << " to " << postRename);
                            
                            PathReplacement replacement;
                            replacement.wd = it->second;
                            replacement.source = it->first;
                            replacement.destination = std::move(postRename);
                            replacements.push_back(std::move(replacement));
                            
                            it = pathToDescriptor.erase(it);
                        } else {
                            it++;
                        }
                    }
                    
                    for (const auto& r : replacements) {
                        auto ptdResult = pathToDescriptor.insert({r.destination, r.wd});
                        assert(ptdResult.second);
                        
                        auto dtpElement = descriptorToPath.find(r.wd);
                        assert(dtpElement != descriptorToPath.end());
                        
                        dtpElement->second = r.destination;
                    }
                }
            }
        }
        
        // Don't bother the callback if we don't have anything to report
        if (events.size() > 0) {
            createInfo.handler(events);
        }
    }
}

void InotifyFileSystemWatcher::processEvent(struct inotify_event* e) {
    if (createInfo.writeChangesToLog) {
        std::string mask;

        if (e->mask & IN_ACCESS) {
            mask.append("ACCESS; ");
        }
        
        if (e->mask & IN_MODIFY) {
            mask.append("MODIFY; ");
        }
        
        if (e->mask & IN_ATTRIB) {
            mask.append("ATTRIB; ");
        }
        
        if (e->mask & IN_CLOSE_WRITE) {
            mask.append("CLOSE_WRITE; ");
        }
        
        if (e->mask & IN_CLOSE_NOWRITE) {
            mask.append("CLOSE_NOWRITE; ");
        }
        
        if (e->mask & IN_OPEN) {
            mask.append("OPEN; ");
        }
        
        if (e->mask & IN_MOVED_FROM) {
            mask.append("MOVED_FROM; ");
        }
        
        if (e->mask & IN_MOVED_TO) {
            mask.append("MOVED_TO; ");
        }
        
        if (e->mask & IN_CREATE) {
            mask.append("CREATE; ");
        }
        
        if (e->mask & IN_DELETE) {
            mask.append("DELETE; ");
        }
        
        if (e->mask & IN_DELETE_SELF) {
            mask.append("DELETE_SELF; ");
        }
        
        if (e->mask & IN_MOVE_SELF) {
            mask.append("MOVE_SELF; ");
        }
        
        if (e->mask & IN_IGNORED) {
            mask.append("IGNORED; ");
        }
        
        if (e->mask & IN_ISDIR) {
            mask.append("ISDIR; ");
        }
        
        if (e->mask & IN_UNMOUNT) {
            mask.append("UNMOUNT; ");
        }
        
        if (e->len == 0) {
            LOG_V("INOTIFY-EVENT\n\t\tFD:     " << e->wd << 
                               "\n\t\tPATH:   " << descriptorToPath[e->wd] <<
                               "\n\t\tMASK:   " << mask << 
                               "\n\t\tCOOKIE: " << e->cookie);
        } else {
            LOG_V("INOTIFY-EVENT\n\t\tFD:     " << e->wd << 
                               "\n\t\tPATH:   " << descriptorToPath[e->wd] << "/" << e->name <<
                               "\n\t\tMASK:   " << mask << 
                               "\n\t\tCOOKIE: " << e->cookie);
        }
    }
    
    FileSystemEventOrigin origin = (e->mask & IN_ISDIR) ? FileSystemEventOrigin::Directory : FileSystemEventOrigin::File;
    
    if (createInfo.ignoreHidden && e->len > 0 && e->name[0] == '.') {
        return;
    }
    
    fs::path sourcePath = descriptorToPath[e->wd];
    if (e->len != 0) {
        sourcePath /= std::string(e->name);
    }
    
    if (e->mask & IN_DELETE_SELF) {
        // A deletion starts from the deepest subdirectory and goes up, which means all monitored directories
        // will trigger IN_DELETE_SELF events automatically. This is thr reason why the last parameter is false.
        removeDirectoryImplNoLock(sourcePath.string(), false, false);
    }
    
    if (e->mask & IN_CREATE) {
        events.emplace_back(FileSystemEventFlags::Created, origin, sourcePath, fs::path());
        
        if (origin == FileSystemEventOrigin::Directory) {
            addNewlyCreatedOrMovedDirectories(sourcePath);
        }
    } else if (e->mask & IN_DELETE) {
        events.emplace_back(FileSystemEventFlags::Deleted, origin, sourcePath, fs::path());
    } else if (e->mask & IN_MODIFY) {
        events.emplace_back(FileSystemEventFlags::Modified, origin, sourcePath, fs::path());
//     } else if (e->mask & IN_CLOSE) {
//          events.emplace_back(FileSystemEventFlags::Closed, origin, sourcePath);
//     } else if (e->mask & IN_OPEN) {
//          events.emplace_back(FileSystemEventFlags::Opened, origin, sourcePath);
    } else if (e->mask & IN_MOVED_FROM) {
        // IN_MOVED_FROM always comes before IN_MOVED_TO (if it even exists), therefore no need to check map
        movePaths.emplace(e->cookie, Paths(origin, sourcePath, "")); 
    } else if (e->mask & IN_MOVED_TO) {
        auto p = movePaths.find(e->cookie);
        if (p != movePaths.end()) { // Already exists? Just add the destination
            p->second.destination = sourcePath;
        } else { // Seems that file is being moved from an untracked directory
            movePaths.emplace(e->cookie, Paths(origin, "", sourcePath));
        }
    }
}

std::vector<fs::path> InotifyFileSystemWatcher::getMonitoredDirectories() const {
    IYFT_PROFILE(ListMonitoredDirectories, iyft::ProfilerTag::AssetConversion);
    
    std::lock_guard<std::mutex> lock(inotifyMutex);
    
    std::vector<fs::path> dirs;
    dirs.reserve(pathToDescriptor.size());
    for (const auto& p : pathToDescriptor) {
        dirs.push_back(p.first);
    }
    
    return dirs;
}

InotifyFileSystemWatcher::~InotifyFileSystemWatcher() {
    if (fd != -1) {
        close(fd);
    }
}
}
