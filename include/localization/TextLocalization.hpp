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

#include "utilities/TypeHelpers.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "localization/LocalizationHandle.hpp"
#include "core/Constants.hpp"
#include "format/format.h"

#define THROW_IF_MISSING

struct sqlite3;
struct sqlite3_stmt;

namespace iyf {
/// The TextLocalizer is responsible for managing and updating a database of localization strings.
/// 
/// For performance reasons, the functions of this class are not protected by mutexes, however you can safely call the operator()(hash32_t key, Args ... args)
/// from multiple threads for as long as you can ensure that other methods are only called from the main thread and all other tasks that require localized strings
/// have already been completed for that frame.
///
/// \todo Retrieval of paginated data for string editor
/// \todo The functions of this class like to throw exceptions and do not return results. This needs to change because the editor will have to gracefully react and
/// handle all these cases
/// \todo test, test, test
class TextLocalizer {
public:
    TextLocalizer();
    ~TextLocalizer();
    
    /// Get a localized and formatted string from the string map.
    ///
    /// \param [in] key hashed key of the localization string
    /// \param [in] args a variable list of arguments that are passed to the string formatter
    /// \return A localized string
    template<typename ... Args>
    std::string operator()(LocalizationHandle key, Args ... args) const {
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
    
    /// Fetches all strings that match the specifed locale into an unordered_map used for quick string lookups. This function will automatically hash all strings that
    /// are missing hash values or have hashes of an incompatible type (may happen after compile flag changes).
    /// 
    /// \throws std::runtime_error thrown if the database file wasn't found, the locale is not supported by the database or sqlite fails to perform any of the required database operations.
    /// 
    /// \param[in] databasePath path to the string database
    /// \param[in] locale a locale that will be used after update
    void loadStringDatabase(const std::string& databasePath, const std::string& locale);
    
    /// Adds or updates a lookup key.
    ///
    /// \param[in] key The key that will be used during localized string lookups.
    void addLookupKey(const std::string& key);
    
    /// Adds a description to a lookup key if such key exists. Does nothing otherwise.
    ///
    /// \param[in] key The key that will be used during localized string lookups.
    /// \param[in] description A description of the key. Typically used to provided more context to the localization teams. 
    void setLookupKeyDescription(const std::string& key, const std::string& description);
    
    /// Removes a specifed lookup key.
    ///
    /// \warning This will clean up all values that depend on this key.
    void removeLookupKey(const std::string& key);
    
    /// Adds or updates a specifed localized string. This will affect both the SQLite database (always) and the string map (if the locale matches the current one).
    ///
    /// \param[in] key The key that will be used during localized string lookups. If this key hasn't been used before, it will be added to the database automatically.
    /// \param[in] value The localized string for the specifed locale.
    /// \param[in] locale The locale of the value.
    void addLocalizedString(const std::string& key, const std::string& value, const std::string& locale);
    
    void removeLocalizedString(const std::string& key, const std::string& locale);
    
    /// Creates a new locale.
    void addLocale(const std::string& locale);
    
    /// Removes a specifed locale.
    ///
    /// \throw std::invalid_argument If locale matches the currently active locale.
    ///
    /// \warning This will clean up all values that depend on this locale.
    void removeLocale(const std::string& locale);
    
    /// Imports localization strings of the specifed locale from a CSV file
    /// \warning This method has not been implemented yet and calling it will crash the engine
    /// \todo implement
    void importCSV(const std::string& filePath, const std::string& locale);
    
    /// Exports localization strings of the specifed locale into a CSV file
    /// \warning This method has not been implemented yet and calling it will crash the engine
    /// \todo implement
    void exportCSV(const std::string& filePath, const std::string& locale);
    
    std::string logAndReturnMissingKey(hash32_t hash) const;
protected:
    void hashAndUpdate();
    void initialize();
    void dispose();

    std::string localeString;
    std::unordered_map<hash32_t, std::string> stringMap;
    std::string path;
    
    sqlite3* db;
    
    sqlite3_stmt* selectNotHashedStmt;
    sqlite3_stmt* updateNotHashedStmt;
    sqlite3_stmt* selectAllStringsForLocaleStmt;
    sqlite3_stmt* selectLocaleStmt;
    sqlite3_stmt* insertLocaleStmt;
    sqlite3_stmt* deleteLocaleStmt;
    sqlite3_stmt* selectLookupKeyStmt;
    sqlite3_stmt* insertLookupKeyStmt;
    sqlite3_stmt* updateLookupKeyDescriptionStmt;
    sqlite3_stmt* deleteLookupKeyStmt;
    sqlite3_stmt* insertLocalizedStringStmt;
    sqlite3_stmt* deleteLocalizedStringStmt;
};

TextLocalizer& SystemLocalizer();
TextLocalizer& GameLocalizer();

template<typename ... Args>
inline std::string LOC_SYS(LocalizationHandle lh, Args ... args) {
    return SystemLocalizer()(lh, args...);
}

template<typename ... Args>
inline std::string LOC(LocalizationHandle lh, Args ... args) {
    return GameLocalizer()(lh, args...);
}

}
#endif // TEXT_LOCALIZATION_HPP
