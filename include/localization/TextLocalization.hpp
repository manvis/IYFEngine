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

#ifndef TEXT_LOCALIZATION_HPP
#define TEXT_LOCALIZATION_HPP

#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>

#include "utilities/TypeHelpers.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/ForceInline.hpp"
#include "localization/LocalizationHandle.hpp"
#include "core/Constants.hpp"
#include "fmt/format.h"

#define THROW_IF_MISSING

struct sqlite3;
struct sqlite3_stmt;

namespace iyf {
class FileSystem;

/// The TextLocalizer is responsible for managing and updating a database of localization strings.
/// 
/// For performance reasons, the functions of this class are not protected by mutexes, however you can safely call the operator()(hash32_t key, Args ... args)
/// from multiple threads for as long as you can ensure that other methods are only called from the main thread and all other tasks that require localized strings
/// have already been completed for that frame.
class TextLocalizer {
public:
    TextLocalizer();
    ~TextLocalizer();
    
    enum class LoadResult {
        /// Another set of localization strings is being loaded. You should wait for a second (or a few, depending on the number of strings)
        /// before attempting to load the strings again.
        AnotherLoadInProgress,
        /// Another set of localization strings has been loaded, but it's waiting to be swapped in. This will happen before the start of the
        /// next frame and you'll be able to use the new set of strings then.
        PendingSwap,
        /// Everything went smoothly. The new strings will be swapped in before the start of the next frame
        LoadSuccessful,
        /// Self explanatory. 
        NoFilesForLocale,
        /// This error may have many reasons (check log). Some possibilites: invalid or damaged localization files, failure to access the virtual
        /// filesystem, OS errors, etc. If you get this, you should either continue with the strings that were loaded for the current locale
        /// (if any) or abort because it's very unlikely the error will resolve without human intervention.
        Failure
    };
    
    enum class StringCheckResult {
        /// Same string value was passed for locales A and B
        SameLocale,
        /// A is not a locale (you need something like en_US, lt_LT, etc.)
        AIsNotALocale,
        /// B is not a locale (you need something like en_US, lt_LT, etc.)
        BIsNotALocale,
        /// Didn't find any string filed for locale A
        NoFilesForLocaleA,
        /// Didn't find any string filed for locale B
        NoFilesForLocaleB,
        /// Creitical failure when trying to load files for locale A (check log)
        FailedToLoadLocaleA,
        /// Creitical failure when trying to load files for locale B (check log)
        FailedToLoadLocaleB,
        /// Not all strings had a corresponding value. Check the missingStrings for a full list
        MissingStringsDetected,
        /// Check passed successfully
        NoMissingStrings
    };
    
    enum class MissingFrom : std::uint32_t {
        LocaleA, LocaleB
    };
    
    struct MissingString {
        IYF_FORCE_INLINE MissingString(LocalizationHandle handle, MissingFrom missingFrom) : handle(handle), missingFrom(missingFrom) {}
        
        LocalizationHandle handle;
        MissingFrom missingFrom;
    };
    
    inline const std::string& getLocale() const {
        return localeString;
    }
    
    /// Get a localized and formatted string from the string map.
    ///
    /// \param [in] key hashed key of the localization string
    /// \param [in] args a variable list of arguments that are passed to the string formatter
    /// \return A localized string
    template<typename ... Args>
    IYF_FORCE_INLINE std::string operator()(LocalizationHandle key, Args ... args) const {
        auto str = stringMap.find(key.getHashValue());
        if (str == stringMap.end()) {
#ifdef THROW_IF_MISSING
            throw std::runtime_error("Localized string for hashed key '" + std::to_string(key.getHashValue().value()) + "' not found for locale '" + localeString + "'");
#else
            return logAndReturnMissingKey(key.getHashValue());
#endif
        } else {
            return fmt::format(str->second, args ...);
        }
    }
    
    /// Fetches strings from all files that match the specifed locale into the tempStringMap.
    ///
    /// You should always call this function from a separate thread. It will do its thing and, once done, set the swapPending
    /// flag to true. Before the start of the next frame, when (hopefully) nothing is doing any string lookups (e.g. from a long
    /// running separate thread), our friend, the Engine, will call executePendingSwap(). The function will notice the flag and 
    /// do swap the tempStringMap
    ///
    /// I don't like this. This is potentially racy and will cause bugs for someone someday. However, I ABSOLUTELY don't want to
    /// use a mutex in the string lookup operator. It would get locked many times every frame. That would be both wasteful and
    /// useless because C++ containers can be safely read from multiple threads.
    ///
    /// \todo Look into ways to make this class more thread-safe without introducing big performance penalties
    /// 
    /// \param[in] fs A filesystem pointer. Used to obtain a list of localization files that reside in the specified path
    /// \param[in] localizationFileDirectory A path to the directory (in the virtual filesystem) that stores localized string files
    /// \param[in] locale A locale that you wish to use
    /// \param[in] clearIfNone Should the current strings be removed if no strings are found for the requested locale? 
    /// LoadResult::NoFilesForLocale will be returned regardless.
    LoadResult loadStringsForLocale(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& locale, bool clearIfNone);
    
    /// A debug function that loads and compares the string maps of two locales. It returns a vector that contains the hashes of any missing strings.
    ///
    /// \todo Would be nice if this worked for multiple locales simultaneously instead of checking pairs.
    static StringCheckResult checkForMissingStrings(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& localeA, const std::string& localeB, std::vector<MissingString>& missingStrings);
    
    std::string loadResultToErrorString(LoadResult result) const; 
protected:
    static LoadResult loadToMap(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& locale, std::unordered_map<StringHash, std::string>& map);
    
    std::string logAndReturnMissingKey(StringHash hash) const;
    
    friend class Engine;
    bool executePendingSwap() {
        if (pendingSwap) {
            std::lock_guard<std::mutex> lock(mapMutex);
            
            stringMap.swap(tempStringMap);
            localeString.swap(tempLocaleString);
            
            tempStringMap.clear();
            tempLocaleString.clear();
            
            pendingSwap = false;
            
            return true;
        }
        
        return false;
    }

    std::string localeString;
    std::unordered_map<StringHash, std::string> stringMap;
    
    std::mutex mapMutex;
    std::atomic<bool> pendingSwap;
    std::string tempLocaleString;
    std::unordered_map<StringHash, std::string> tempStringMap;
};

TextLocalizer& SystemLocalizer();
TextLocalizer& GameLocalizer();

template<typename ... Args>
IYF_FORCE_INLINE std::string LOC_SYS(LocalizationHandle lh, Args ... args) {
    return SystemLocalizer()(lh, args...);
}

template<typename ... Args>
IYF_FORCE_INLINE std::string LOC(LocalizationHandle lh, Args ... args) {
    return GameLocalizer()(lh, args...);
}

}
#endif // TEXT_LOCALIZATION_HPP
