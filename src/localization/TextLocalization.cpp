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

#include "database/sqlite3.h"

#include <cstring>

namespace iyf {
TextLocalizer& SystemLocalizer() {
    static TextLocalizer l;
    return l;
}

TextLocalizer& GameLocalizer() {
    static TextLocalizer l;
    return l;
}

const std::string SelectLocale = "SELECT id FROM locales WHERE code=?;";
const std::string InsertLocale = "INSERT OR IGNORE INTO locales (code) VALUES (?);";
const std::string DeleteLocale = "DELETE FROM locales WHERE code = ?;";

const std::string SelectLookupKey = "SELECT id FROM stringKeys WHERE key=?;";
const std::string InsertLookupKey = "INSERT OR IGNORE INTO stringKeys (key, description, hash32, hashType) VALUES (?, ?, ?, ?);";
const std::string UpdateLookupKeyDescription = "UPDATE OR IGNORE stringKeys SET description=? WHERE key=?";
const std::string DeleteLookupKey = "DELETE FROM stringKeys WHERE key=?;";
const std::string SelectNotHashed = "SELECT id, key FROM stringKeys where hashType IS NULL OR hashType = \"\" OR hashType != ?;";
const std::string UpdateNotHashed = "UPDATE stringKeys SET hash32 = ?, hashType = ? WHERE id = ?;";

const std::string InsertLocalizedString = "INSERT OR REPLACE INTO strings (key, locale, string) VALUES (?, ?, ?);";
const std::string DeleteLocalizedString = "DELETE FROM strings WHERE key=? AND locale=?;";

// const std::string FindStringKey = "SELECT ";
const std::string SelectAllStringsForLocale = "SELECT stringKeys.hash32, strings.string FROM stringKeys LEFT OUTER JOIN strings ON strings.key = stringKeys.id LEFT OUTER JOIN locales ON locales.id  = strings.locale WHERE locales.code = ?";

TextLocalizer::TextLocalizer() : path(""), db(nullptr), selectNotHashedStmt(nullptr), updateNotHashedStmt(nullptr), selectAllStringsForLocaleStmt(nullptr), selectLocaleStmt(nullptr), insertLocaleStmt(nullptr),
    deleteLocaleStmt(nullptr), selectLookupKeyStmt(nullptr), insertLookupKeyStmt(nullptr), updateLookupKeyDescriptionStmt(nullptr), deleteLookupKeyStmt(nullptr), insertLocalizedStringStmt(nullptr), deleteLocalizedStringStmt(nullptr) {}

TextLocalizer::~TextLocalizer() {
    dispose();
}

void TextLocalizer::initialize() {
    assert(db == nullptr);
    assert(selectNotHashedStmt == nullptr);
    assert(updateNotHashedStmt == nullptr);
    assert(selectAllStringsForLocaleStmt == nullptr);
    
    // We're mutexing ourselves
    int result = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX, nullptr);

    if (result != SQLITE_OK){
        throw std::runtime_error(std::string("Failed to open localization database. ") + sqlite3_errmsg(db));
    }
    
    const std::string foreignKeyPragma = "PRAGMA foreign_keys = ON;";
    sqlite3_exec(db, foreignKeyPragma.c_str(), nullptr, nullptr, nullptr);
    
    // Selects all string keys that are missing a hash value or are using an incompatible hash type and write appropriate hashes
    result = sqlite3_prepare_v2(db, SelectNotHashed.c_str(), -1, &selectNotHashedStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a database query for selecting non-hashed strings. SQLite error: ") + sqlite3_errmsg(db));
    }

    result = sqlite3_prepare_v2(db, UpdateNotHashed.c_str(), -1, &updateNotHashedStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a database query for selecting non-hashed strings. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, SelectLookupKey.c_str(), -1, &selectLookupKeyStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a a lookup key counter. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, InsertLookupKey.c_str(), -1, &insertLookupKeyStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a a lookup key insertion statement. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, UpdateLookupKeyDescription.c_str(), -1, &updateLookupKeyDescriptionStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a a lookup key description update statement. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, DeleteLookupKey.c_str(), -1, &deleteLookupKeyStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a a lookup key deletion statement. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, SelectAllStringsForLocale.c_str(), -1, &selectAllStringsForLocaleStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a string selection query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, SelectLocale.c_str(), -1, &selectLocaleStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a locale selection query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, InsertLocale.c_str(), -1, &insertLocaleStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a locale insertion query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, DeleteLocale.c_str(), -1, &deleteLocaleStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a locale deletion query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, InsertLocalizedString.c_str(), -1, &insertLocalizedStringStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a localized string insertion query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    result = sqlite3_prepare_v2(db, DeleteLocalizedString.c_str(), -1, &deleteLocalizedStringStmt, 0);
    if (result != SQLITE_OK) {
        throw std::runtime_error(std::string("Failed to create a localized string deletion query. SQLite error: ") + sqlite3_errmsg(db));
    }
    
    // Update all string keys that don't have hash values.
    hashAndUpdate();
}

void TextLocalizer::dispose() {
    if (selectNotHashedStmt != nullptr) {
        sqlite3_finalize(selectNotHashedStmt);
        selectNotHashedStmt = nullptr;
    }
    
    if (updateNotHashedStmt != nullptr) {
        sqlite3_finalize(updateNotHashedStmt);
        updateNotHashedStmt = nullptr;
    }
    
    if (selectLookupKeyStmt != nullptr) {
        sqlite3_finalize(selectLookupKeyStmt);
        selectLookupKeyStmt = nullptr;
    }
    
    if (selectAllStringsForLocaleStmt != nullptr) {
        sqlite3_finalize(selectAllStringsForLocaleStmt);
        selectAllStringsForLocaleStmt = nullptr;
    }
    
    if (insertLookupKeyStmt != nullptr) {
        sqlite3_finalize(insertLookupKeyStmt);
        insertLookupKeyStmt = nullptr;
    }
    
    if (updateLookupKeyDescriptionStmt != nullptr) {
        sqlite3_finalize(updateLookupKeyDescriptionStmt);
        updateLookupKeyDescriptionStmt = nullptr;
    }
    
    if (deleteLookupKeyStmt != nullptr) {
        sqlite3_finalize(deleteLookupKeyStmt);
        deleteLookupKeyStmt = nullptr;
    }
    
    if (selectLocaleStmt != nullptr) {
        sqlite3_finalize(selectLocaleStmt);
        selectLocaleStmt = nullptr;
    }
    
    if (insertLocaleStmt != nullptr) {
        sqlite3_finalize(insertLocaleStmt);
        insertLocaleStmt = nullptr;
    }
    
    if (deleteLocaleStmt != nullptr) {
        sqlite3_finalize(deleteLocaleStmt);
        deleteLocaleStmt = nullptr;
    }
    
    if (insertLocalizedStringStmt != nullptr) {
        sqlite3_finalize(insertLocalizedStringStmt);
        insertLocalizedStringStmt = nullptr;
    }
    
    if (deleteLocalizedStringStmt != nullptr) {
        sqlite3_finalize(deleteLocalizedStringStmt);
        deleteLocalizedStringStmt = nullptr;
    }
    
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
}

void TextLocalizer::hashAndUpdate() {
    sqlite3_bind_text(selectNotHashedStmt, 1, IYF_HASH_32_NAME, std::strlen(IYF_HASH_32_NAME), SQLITE_STATIC);

    std::size_t i = 0;
    while (sqlite3_step(selectNotHashedStmt) == SQLITE_ROW) {
        sqlite3_int64 id = sqlite3_column_int64(selectNotHashedStmt, 0);
        const char* str = reinterpret_cast<const char*>(sqlite3_column_text(selectNotHashedStmt, 1));
        
        hash32_t hashed = HS(str);
        static_assert(sizeof(hashed.value()) == 4, "Hashed value must be 32 bit");
        
        sqlite3_bind_int(updateNotHashedStmt, 1, hashed.value());
        sqlite3_bind_text(updateNotHashedStmt, 2, IYF_HASH_32_NAME, std::strlen(IYF_HASH_32_NAME), SQLITE_STATIC);
        sqlite3_bind_int64(updateNotHashedStmt, 3, id);
        
        if (sqlite3_step(updateNotHashedStmt) != SQLITE_DONE) {
            throw std::runtime_error("Failed to update hash values");
        }
        
        sqlite3_reset(updateNotHashedStmt);
        
        LOG_D("Number of updated string hashes: " << i);
    }

    sqlite3_reset(selectNotHashedStmt);
}

void TextLocalizer::loadStringDatabase(const std::string& databasePath, const std::string& locale) {
    stringMap.clear();

    path = databasePath;
    localeString = locale;
    
    dispose();
    initialize();
    
    // Load strings into the map
    sqlite3_bind_text(selectAllStringsForLocaleStmt, 1, localeString.c_str(), localeString.size(), SQLITE_STATIC);
    
    std::size_t j = 0;
    while (sqlite3_step(selectAllStringsForLocaleStmt) == SQLITE_ROW) {
        std::uint32_t id = sqlite3_column_int(selectAllStringsForLocaleStmt, 0);
        const char* str = reinterpret_cast<const char*>(sqlite3_column_text(selectAllStringsForLocaleStmt, 1));

        stringMap[hash32_t(id)] = str;
        j++;
    }
    
    sqlite3_reset(selectAllStringsForLocaleStmt);
    
    LOG_D("Localization string count: " << j);
}

void TextLocalizer::addLookupKey(const std::string& key) {
    hash32_t hashed = HS(key.c_str());
    
    sqlite3_bind_text(insertLookupKeyStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
    sqlite3_bind_text(insertLookupKeyStmt, 2, nullptr, 0, SQLITE_STATIC);
    sqlite3_bind_int(insertLookupKeyStmt, 3, hashed.value());
    sqlite3_bind_text(insertLookupKeyStmt, 4, IYF_HASH_32_NAME, std::strlen(IYF_HASH_32_NAME), SQLITE_STATIC);
    
    if (sqlite3_step(insertLookupKeyStmt) != SQLITE_DONE) {
        throw std::runtime_error("Insertion of a new lookup key \"" + key + "\" failed");
    }
    
    sqlite3_reset(insertLookupKeyStmt);
}

void TextLocalizer::removeLookupKey(const std::string& key) {
    sqlite3_bind_text(deleteLookupKeyStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
    if (sqlite3_step(deleteLookupKeyStmt) != SQLITE_DONE) {
        throw std::runtime_error("Deletion of a lookup key \"" + key + "\" failed");
    }
    sqlite3_reset(deleteLookupKeyStmt);
}

void TextLocalizer::setLookupKeyDescription(const std::string& key, const std::string& description) {
    sqlite3_bind_text(updateLookupKeyDescriptionStmt, 1, description.c_str(), description.size(), SQLITE_STATIC);
    sqlite3_bind_text(updateLookupKeyDescriptionStmt, 2, key.c_str(), key.size(), SQLITE_STATIC);
    
    if (sqlite3_step(updateLookupKeyDescriptionStmt) != SQLITE_DONE) {
        throw std::runtime_error("Couldn't update a description for key: " + key);
    }
    
    sqlite3_reset(updateLookupKeyDescriptionStmt);
}

void TextLocalizer::addLocalizedString(const std::string& key, const std::string& value, const std::string& locale) {
    hash32_t hashedKey = HS(key.c_str());
    
    if (locale == localeString) {
        stringMap[hashedKey] = value;
    }
    
    // Check if the locale has been included
    sqlite3_bind_text(selectLocaleStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
    if (sqlite3_step(selectLocaleStmt) != SQLITE_ROW) {
        throw std::invalid_argument("An invalid locale has been specifed.");
    }
    
    sqlite3_int64 localeID = sqlite3_column_int64(selectLocaleStmt, 0);
    sqlite3_reset(selectLocaleStmt);
    
    // Check if key exists.
    sqlite3_bind_text(selectLookupKeyStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
    
    // If this key does not exist. We need to insert it.
    if (sqlite3_step(selectLookupKeyStmt) != SQLITE_ROW) {
        sqlite3_reset(selectLookupKeyStmt);
        
        addLookupKey(key);
        
        // Look the key up again to get the ID.
        sqlite3_bind_text(selectLookupKeyStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
        
        if (sqlite3_step(selectLookupKeyStmt) != SQLITE_ROW) {
            throw std::runtime_error("Failed to find the key that has just been inserted.");
        }
    }
    
    sqlite3_int64 keyID = sqlite3_column_int64(selectLookupKeyStmt, 0);
    sqlite3_reset(selectLookupKeyStmt);
    
    sqlite3_bind_int64(insertLocalizedStringStmt, 1, keyID);
    sqlite3_bind_int64(insertLocalizedStringStmt, 2, localeID);
    sqlite3_bind_text(insertLocalizedStringStmt, 3, value.c_str(), value.length(), SQLITE_STATIC);
    
    if (sqlite3_step(insertLocalizedStringStmt) != SQLITE_DONE) {
        throw std::runtime_error("Insertion of a new localized string \"" + value + "\" has failed.");
    }
    
    sqlite3_reset(insertLocalizedStringStmt);
}

void TextLocalizer::removeLocalizedString(const std::string& key, const std::string& locale) {
    sqlite3_bind_text(updateLookupKeyDescriptionStmt, 1, key.c_str(), key.size(), SQLITE_STATIC);
    sqlite3_bind_text(updateLookupKeyDescriptionStmt, 2, locale.c_str(), locale.size(), SQLITE_STATIC);
    
    if (sqlite3_step(updateLookupKeyDescriptionStmt) != SQLITE_DONE) {
        throw std::runtime_error("Failed to remove a localized string with key \"" + key + "\" for locale " + locale);
    }
    
    sqlite3_reset(deleteLocalizedStringStmt);
}

void TextLocalizer::addLocale(const std::string& locale) {
    if (locale == localeString) {
        return;
    }
    
    sqlite3_bind_text(insertLocaleStmt, 1, locale.c_str(), locale.size(), SQLITE_STATIC);
    if (sqlite3_step(insertLocaleStmt) != SQLITE_DONE) {
        throw std::runtime_error("Locale insertion failed.");
    }
    
    sqlite3_reset(insertLocaleStmt);
}

void TextLocalizer::removeLocale(const std::string& locale) {
    if (locale == localeString) {
        throw std::invalid_argument("You can't remove the active locale.");
    }
    
    sqlite3_bind_text(deleteLocaleStmt, 1, locale.c_str(), locale.size(), SQLITE_STATIC);
    if (sqlite3_step(deleteLocaleStmt) != SQLITE_DONE) {
        throw std::runtime_error("Locale deletion failed.");
    }
    
    sqlite3_reset(deleteLocaleStmt);
}

void TextLocalizer::importCSV(const std::string& filePath, const std::string& locale) {
    throw std::runtime_error("This method has not been implemented yet.");
}

void TextLocalizer::exportCSV(const std::string& filePath, const std::string& locale) {
    throw std::runtime_error("This method has not been implemented yet.");
}

std::string TextLocalizer::logAndReturnMissingKey(hash32_t key) const {
    std::string str = fmt::format("MISSING STRING {}##", key.value());
    
    LOG_W(str);
    return str;
}

}
