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

#include "assets/AssetConstants.hpp"
#include "core/Project.hpp"
#include "core/Constants.hpp"
#include "logging/Logger.hpp"
#include "utilities/ReadWholeFile.hpp"
#include "utilities/Regexes.hpp"
#include "io/DefaultFileSystem.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include <fstream>

// TODO validate the inputs
namespace iyf {
static const unsigned int CURRENT_DATA_FORMAT_VERSION = 1;
static const char* VERSION_FIELD_NAME = "version";
static const char* GAME_NAME_FIELD_NAME = "game_name";
static const char* COMPANY_NAME_FIELD_NAME = "company_name";
static const char* FIRST_WORLD_FIELD_NAME = "first_world";
static const char* GAME_VERSION_FIELD_NAME = "game_version";
static const char* GAME_VERSION_MAJOR_FIELD_NAME = "game_version_major";
static const char* GAME_VERSION_MINOR_FIELD_NAME = "game_version_minor";
static const char* GAME_VERSION_PATCH_FIELD_NAME = "game_version_patch";
static const char* ENGINE_VERSION_FIELD_NAME = "engine_version";
static const char* ENGINE_VERSION_MAJOR_FIELD_NAME = "engine_version_major";
static const char* ENGINE_VERSION_MINOR_FIELD_NAME = "engine_version_minor";
static const char* ENGINE_VERSION_PATCH_FIELD_NAME = "engine_version_patch";
static const char* BASE_LOCALE_FIELD_NAME = "base_locale";

Project::CreationResult Project::Create(const Path& newProjectPath, const std::string& projectName, const std::string& companyName,
                                        std::function<void(const std::string&)> callback, const std::string& baseLocale) {
    if (callback != nullptr) {
        callback("Validating parameters");
    }
    
    if (newProjectPath.empty()) {
        return CreationResult::EmptyPath;
    }
    
    if (projectName.empty()) {
        return CreationResult::EmptyName;
    }

    auto& fs = DefaultFileSystem::Instance();
    
    FileStat stat;
    if (fs.getStats(newProjectPath, stat) != FileSystemResult::Success ||
        (stat.type != FileType::Directory)) {
        return CreationResult::NotADirectory;
    }
    
    const Path finalPath = newProjectPath / projectName;

    FileSystemResult result;
    const bool finalPathExists = fs.exists(finalPath, result);

    if (result != FileSystemResult::Success) {
        return CreationResult::NotADirectory;
    }
    
    if (callback != nullptr) {
        callback("Looking for existing directories");
    }
    
    const bool isEmpty = fs.isEmpty(finalPath, result);
    if (finalPathExists && isEmpty) {
        return CreationResult::NonEmptyDirectory;
    } else if (!finalPathExists) {
        fs.createDirectory(finalPath);
    }
    
    if (callback != nullptr) {
        callback("Creating new directories");
    }
    
    if (!CreateImportedAssetDirectories(finalPath, con::GetCurrentPlatform()) || !CreateImportsDirectory(finalPath)) {
        return CreationResult::FolderCreationFailed;
    }
    
    if (callback != nullptr) {
        callback("Creating the project file");
    }
    
    if (!CreateProjectFile(finalPath, projectName, companyName, baseLocale)) {
        return CreationResult::ProjectFileCreationFailed;
    }
    
    if (callback != nullptr) {
        callback("Done");
    }
    
    return CreationResult::CreatedSuccessfully;
}

bool Project::CreateProjectFile(const Path& path, const std::string& projectName, const std::string& companyName, const std::string& baseLocale, const Version& version) {
    if (path.empty() || projectName.empty() || companyName.empty() || baseLocale.empty()) {
        LOG_E("Couldn't create a new project file. At least one of the provided parameters was empty.");
        return false;
    }
    
    if (!std::regex_match(baseLocale, SystemRegexes().LocaleValidationRegex)) {
        LOG_E("Couldn't create a new project file. The base locale did not match the regex.");
        return false;
    }
    
    Project project(path, false);
    project.setGameName(projectName);
    project.setCompanyName(companyName);
    project.setVersion(version);
    project.setBaseLocale(baseLocale);
    project.setFirstWorldName(con::DefaultWorldFile());
    project.setEngineVersion(con::EngineVersion);
    
    if (!project.serialize()) {
        return false;
    } else {
        return true;
    }
}

inline Path MakeAssetTypeSubdirPath(const Path& path, PlatformIdentifier platformID, AssetType assetType) {
    if (platformID == con::GetCurrentPlatform()) {
        return path / con::AssetTypeToPath(assetType);
    } else {
        return path / con::PlatformIdentifierToName(platformID) / con::AssetTypeToPath(assetType);
    }
}

bool Project::CreateImportedAssetDirectories(const Path& path, PlatformIdentifier platformID) {
    auto& fs = DefaultFileSystem::Instance();

    for (std::size_t i = 0; i < static_cast<std::size_t>(AssetType::COUNT); ++i) {
        const Path assetTypeSubdir = MakeAssetTypeSubdirPath(path, platformID, static_cast<AssetType>(i));

        if (fs.createDirectory(assetTypeSubdir) != FileSystemResult::Success) {
            return false;
        }
    }
    
    // Create the system string path
    FileSystemResult result = FileSystemResult::Success;
    if (platformID == con::GetCurrentPlatform()) {
        result = fs.createDirectory(path / con::SystemStringPath());
    } else {
        result = fs.createDirectory(path / con::PlatformIdentifierToName(platformID) / con::SystemStringPath());
    }
    
    return (result == FileSystemResult::Success);
}

bool Project::CreateImportsDirectory(const Path& path) {
    auto& fs = DefaultFileSystem::Instance();

    return (fs.createDirectory(path / con::ImportPath()) == FileSystemResult::Success);
}

Project::Project(Path root, bool deserializeFile) : root(std::move(root)), valid(false) {
    if (deserializeFile && !deserialize()) {
        LOG_E("Failed to deserialize the project file  from {}", (this->root / con::ProjectFile()));
        return;
    }
    
    if (!CreateImportedAssetDirectories(this->root, con::GetCurrentPlatform()) || !CreateImportsDirectory(this->root)) {
        return;
    }
    
    valid = true;
}

Project::Project(Path root) : Project(std::move(root), true) {}

Project::~Project() {}

void Project::validOrFatalError() const {
    if (!isValid()) {
        throw std::runtime_error("Trying to access an invalid Project.");
    }
}

bool Project::makesJSONRoot() const {
    return false;
}

const Path& Project::getRootPath() const {
    validOrFatalError();
    
    return root;
}

bool Project::serialize() const {
    validOrFatalError();
    
    std::string jsonString = getJSONString();
    const Path finalPath = root / con::ProjectFile();
    
    std::ofstream jsonFile(finalPath.getNativeString(), std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
    
    if (!jsonFile.is_open()) {
        LOG_E("Failed to open a project file {} for serialization.", finalPath);
        
        return false;
    }
    
    jsonFile.write(jsonString.data(), jsonString.length());
    if (!jsonFile.good()) {
        LOG_E("Failed to serialize data to the project file {}", finalPath);
        
        return false;
    }
    
    return true;
}

bool Project::deserialize() {
    const Path finalPath = root / con::ProjectFile();
    
    std::ifstream jsonFile(finalPath.getNativeString(), std::ios_base::in|std::ios_base::binary);
    
    if (!jsonFile.is_open()) {
        LOG_E("Failed to open a project file {} for deserialization.", finalPath);
        
        return false;
    }
    
    const auto wholeFile = util::ReadWholeFile(jsonFile);
    
    rj::Document document;
    document.Parse(wholeFile.first.get(), wholeFile.second);
    deserializeJSON(document);
    
    return true;
}

void Project::serializeJSON(PrettyStringWriter& pw) const {
    validOrFatalError();
    
    pw.Key(VERSION_FIELD_NAME);
    pw.Uint(CURRENT_DATA_FORMAT_VERSION);
    
    pw.Key(GAME_NAME_FIELD_NAME);
    pw.String(gameName.c_str(), gameName.length(), true);
    
    pw.Key(COMPANY_NAME_FIELD_NAME);
    pw.String(companyName.c_str(), companyName.length(), true);
    
    pw.Key(FIRST_WORLD_FIELD_NAME);
    pw.String(firstWorldName.c_str(), firstWorldName.length(), true);
    
    pw.Key(GAME_VERSION_FIELD_NAME);
    pw.StartObject();
    
    pw.Key(GAME_VERSION_MAJOR_FIELD_NAME);
    pw.Uint(version.getMajor());
    
    pw.Key(GAME_VERSION_MINOR_FIELD_NAME);
    pw.Uint(version.getMinor());
    
    pw.Key(GAME_VERSION_PATCH_FIELD_NAME);
    pw.Uint(version.getPatch());
    
    pw.EndObject();

    pw.Key(ENGINE_VERSION_FIELD_NAME);
    pw.StartObject();
    
    pw.Key(ENGINE_VERSION_MAJOR_FIELD_NAME);
    pw.Uint(engineVersion.getMajor());
    
    pw.Key(ENGINE_VERSION_MINOR_FIELD_NAME);
    pw.Uint(engineVersion.getMinor());
    
    pw.Key(ENGINE_VERSION_PATCH_FIELD_NAME);
    pw.Uint(engineVersion.getPatch());

    pw.EndObject();
    
    pw.Key(BASE_LOCALE_FIELD_NAME);
    pw.String(baseLocale.c_str(), baseLocale.length(), true);
    
    // WARNING Do not serialize the root path. It will differ between users.
}

void Project::deserializeJSON(JSONObject& jo) {
    // This should handle multiple versions in the future
    if (jo[VERSION_FIELD_NAME].GetUint() != CURRENT_DATA_FORMAT_VERSION) {
        LOG_E("Unknown project file version {}", jo[VERSION_FIELD_NAME].GetUint());
        throw std::runtime_error("Unknown project file version");
    }
    
    gameName = jo[GAME_NAME_FIELD_NAME].GetString();
    companyName = jo[COMPANY_NAME_FIELD_NAME].GetString();
    firstWorldName = jo[FIRST_WORLD_FIELD_NAME].GetString();
    
    const std::uint16_t major = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_MAJOR_FIELD_NAME].GetUint();
    const std::uint16_t minor = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_MINOR_FIELD_NAME].GetUint();
    const std::uint16_t patch = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_PATCH_FIELD_NAME].GetUint();
    
    version = Version(major, minor, patch);
    
    const std::uint16_t engMajor = jo[ENGINE_VERSION_FIELD_NAME][ENGINE_VERSION_MAJOR_FIELD_NAME].GetUint();
    const std::uint16_t engMinor = jo[ENGINE_VERSION_FIELD_NAME][ENGINE_VERSION_MINOR_FIELD_NAME].GetUint();
    const std::uint16_t engPatch = jo[ENGINE_VERSION_FIELD_NAME][ENGINE_VERSION_PATCH_FIELD_NAME].GetUint();

    baseLocale = jo[BASE_LOCALE_FIELD_NAME].GetString();
}

void Project::setFirstWorldName(std::string name) {
    validOrFatalError();
    
    firstWorldName = name;
}

std::string Project::getFirstWorldName() const {
    validOrFatalError();
    
    return firstWorldName;
}

void Project::setCompanyName(std::string name) {
    validOrFatalError();
    
    companyName = name;
}

std::string Project::getCompanyName() const {
    validOrFatalError();
    
    return companyName;
}

void Project::setGameName(std::string name) {
    validOrFatalError();
    
    gameName = name;
}

std::string Project::getGameName() const {
    validOrFatalError();
    
    return gameName;
}

void Project::setVersion(const Version& version) {
    validOrFatalError();
    
    this->version = version;
}

Version Project::getVersion() const {
    validOrFatalError();
    
    return version;
}

std::string Project::getBaseLocale() const {
    validOrFatalError();
    
    return baseLocale;
}

void Project::setBaseLocale(std::string locale) {
    validOrFatalError();
    
    baseLocale = locale;
}

Version Project::getEngineVersion() const {
    validOrFatalError();
    
    return engineVersion;
}

void Project::setEngineVersion(Version engineVersion) {
    validOrFatalError();
    
    this->engineVersion = engineVersion;
}

}
