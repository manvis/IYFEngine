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

#include "localization/TextLocalization.hpp"
#include "core/Logger.hpp"
#include "core/Constants.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "utilities/StringUtilities.hpp"
#include "utilities/Regexes.hpp"

#include <cstring>

// #define IYF_LOG_ALL_LOADED_STRINGS

namespace iyf {
TextLocalizer& SystemLocalizer() {
    static TextLocalizer l;
    return l;
}

TextLocalizer& GameLocalizer() {
    static TextLocalizer l;
    return l;
}


TextLocalizer::TextLocalizer() : pendingSwap(false) {}

TextLocalizer::~TextLocalizer() {}


TextLocalizer::LoadResult TextLocalizer::loadStringsForLocale(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& locale, bool clearIfNone) {
    if (pendingSwap) {
        return LoadResult::PendingSwap;
    }
    
    std::unique_lock<std::mutex> lock(mapMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        return LoadResult::AnotherLoadInProgress;
    }
    
    tempStringMap.clear();
    
    LoadResult result = loadToMap(fs, localizationFileDirectory, locale, tempStringMap);
    
    // If the user asked for it, make sure that existing strings aren't removed.
    if (result == LoadResult::NoFilesForLocale && !clearIfNone) {
        return result;
    }
    
    pendingSwap = true;
    
    return result;
}

TextLocalizer::StringCheckResult TextLocalizer::checkForMissingStrings(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& localeA, const std::string& localeB, std::vector<MissingString>& missingStrings) {
    if (localeA == localeB) {
        return StringCheckResult::SameLocale;
    }
    
    if (!std::regex_match(localeA, regex::LocaleValidationRegex)) {
        return StringCheckResult::AIsNotALocale;
    }
    
    if (!std::regex_match(localeB, regex::LocaleValidationRegex)) {
        return StringCheckResult::AIsNotALocale;
    }
    
    std::unordered_map<hash32_t, std::string> mapA;
    std::unordered_map<hash32_t, std::string> mapB;
    
    LoadResult resultA = loadToMap(fs, localizationFileDirectory, localeA, mapA);
    if (resultA == LoadResult::Failure) {
        return StringCheckResult::FailedToLoadLocaleA;
    }
    if (resultA == LoadResult::NoFilesForLocale) {
        return StringCheckResult::NoFilesForLocaleA;
    }
    
    LoadResult resultB = loadToMap(fs, localizationFileDirectory, localeB, mapB);
    if (resultB == LoadResult::Failure) {
        return StringCheckResult::FailedToLoadLocaleB;
    }
    if (resultB == LoadResult::NoFilesForLocale) {
        return StringCheckResult::NoFilesForLocaleB;
    }
    
    bool missingStringsDetected = false;
    for (const auto& s : mapA) {
        if (mapB.count(s.first) == 0) {
            missingStrings.emplace_back(LH(s.first), MissingFrom::LocaleB);
            missingStringsDetected = true;
        }
    }
    
    for (const auto& s : mapB) {
        if (mapA.count(s.first) == 0) {
            missingStrings.emplace_back(LH(s.first), MissingFrom::LocaleB);
            missingStringsDetected = true;
        }
    }
    
    return missingStringsDetected ? StringCheckResult::MissingStringsDetected : StringCheckResult::NoMissingStrings;
}

struct FilePriorityCount {
    FilePriorityCount(File file, std::int32_t priority, std::uint32_t count) : file(std::move(file)), priority(priority), count(count) {}
    
    File file;
    std::int32_t priority;
    std::uint32_t count;
};

TextLocalizer::LoadResult TextLocalizer::loadToMap(const FileSystem* fs, const fs::path& localizationFileDirectory, const std::string& locale, std::unordered_map<hash32_t, std::string>& map) {
    assert(map.empty());
    
    auto fileNames = fs->getDirectoryContents(localizationFileDirectory);
    
    const std::array<char, 4> magicNumber = {'I', 'Y', 'F', 'S'};
    
    std::size_t stringTotal = 0;
    
    std::vector<FilePriorityCount> filesWithPriorities;
    
    for (const auto& name : fileNames) {
        // Making sure to skip metadata files
        const bool stringFile = util::startsWith(name.stem().string(), locale) &&
                                (name.extension() != con::MetadataExtension) && 
                                (name.extension() != con::TextMetadataExtension);
        
        if (stringFile) {
            File file(localizationFileDirectory / name, File::OpenMode::Read);
            
            std::array<char, 4> magicTest{};
            file.readBytes(magicTest.data(), 4);
            
            if (magicNumber != magicTest) {
                LOG_E("Failed to load a string file: " << localizationFileDirectory / name << ". Incorrect magic number");
                return LoadResult::Failure;
            }
            
            const std::uint32_t version = file.readUInt32();
            if (version != 1) {
                LOG_E("Failed to load a string file: " << localizationFileDirectory / name << ". Unsupported version: " << version);
                return LoadResult::Failure;
            }
            
            const std::int32_t priority = file.readInt32();
            
            const std::uint32_t count = file.readUInt32();
            stringTotal += count;
            
            filesWithPriorities.emplace_back(std::move(file), priority, count);
        }
    }
    
    if (filesWithPriorities.empty()) {
        return LoadResult::NoFilesForLocale;
    }
    
    std::sort(filesWithPriorities.begin(), filesWithPriorities.end(), [](const FilePriorityCount& a, const FilePriorityCount& b){ return a.priority < b.priority; });
    
    for (auto& f : filesWithPriorities) {
        LOG_V("Loading strings from " << f.file.getPath() << "; PRIORITY: " << f.priority);
        
        //buffer.reserve(1024);
        for (std::size_t i = 0; i < f.count; ++i) {
            const hash32_t strHash(f.file.readUInt32());
            
            std::string buffer;
            f.file.readString(buffer, StringLengthIndicator::UInt32);
            
            map[strHash] = buffer;
        }
        
        assert(f.file.isEOF());
    }

#ifdef IYF_LOG_ALL_LOADED_STRINGS
    std::stringstream ss;
    ss << "All loaded strings (comment out IYF_LOG_ALL_LOADED_STRINGS from TextLocalization.cpp to hide):";
    for (const auto& s : map) {
        ss << "\n\tHASH: " << s.first.value() << "; CONTENT: " << s.second;
    }
    
    LOG_V(ss.str());
#endif
    
    return LoadResult::LoadSuccessful;
}

std::string TextLocalizer::loadResultToErrorString(LoadResult result) const {
    switch (result) {
        case LoadResult::AnotherLoadInProgress:
            return "Another set of strings is being loaded at the moment";
        case LoadResult::PendingSwap:
            return "A locale swap is pending";
        case LoadResult::NoFilesForLocale:
            return "No files were found for the specified locale";
        case LoadResult::Failure:
            return "Failed to load localized strings (check earlier log entries)";
        case LoadResult::LoadSuccessful:
            return "Strings loaded successfully";
    }
}

std::string TextLocalizer::logAndReturnMissingKey(hash32_t key) const {
    std::string str = fmt::format("MISSING STRING {}##", key.value());
    
    LOG_W(str);
    return str;
}

}
