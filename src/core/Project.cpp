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

#include "core/Project.hpp"
#include "core/Constants.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/File.hpp"

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
static const char* BASE_LOCALE_FIELD_NAME = "base_locale";

Project::CreationResult Project::Create(fs::path newProjectPath, const std::string& projectName, std::function<void(const std::string&)> callback, const std::string& baseLocale) {
    return CreationResult::EmptyPath;
}

Project::Project(fs::path root) : root(root) {
    deserialize();
}

Project::~Project() {}

bool Project::serialize() const {
    rj::StringBuffer sb;
    PrettyStringWriter pw(sb);
    
    pw.SetIndent('\t', 1);
    serializeJSON(pw);
    
    const char* jsonString = sb.GetString();
    const std::size_t jsonByteCount = sb.GetSize();
    const fs::path finalPath = root / con::ProjectFile;
    
    File output(finalPath, File::OpenMode::Write);
    output.writeBytes(jsonString, jsonByteCount);
}

bool Project::deserialize() {
    const fs::path finalPath = root / con::ProjectFile;
    
    File jsonFile(finalPath, File::OpenMode::Read);
    const auto wholeFile = jsonFile.readWholeFile();
    
    rj::Document document;
    document.Parse(wholeFile.first.get(), wholeFile.second);
    deserializeJSON(document);
}

void Project::serializeJSON(PrettyStringWriter& pw) const {
    pw.String(VERSION_FIELD_NAME);
    pw.Uint(CURRENT_DATA_FORMAT_VERSION);
    
    pw.String(GAME_NAME_FIELD_NAME);
    pw.String(gameName.c_str(), gameName.length(), true);
    
    pw.String(COMPANY_NAME_FIELD_NAME);
    pw.String(companyName.c_str(), companyName.length(), true);
    
    pw.String(FIRST_WORLD_FIELD_NAME);
    pw.String(firstWorldName.c_str(), firstWorldName.length(), true);
    
    pw.String(GAME_VERSION_FIELD_NAME);
    pw.StartObject();
    
    pw.String(GAME_VERSION_MAJOR_FIELD_NAME);
    pw.Uint(version.getMajor());
    
    pw.String(GAME_VERSION_MINOR_FIELD_NAME);
    pw.Uint(version.getMinor());
    
    pw.String(GAME_VERSION_PATCH_FIELD_NAME);
    pw.Uint(version.getPatch());
    
    pw.EndObject();
    
    pw.String(BASE_LOCALE_FIELD_NAME);
    pw.String(baseLocale.c_str(), baseLocale.length(), true);
    
    // WARNING Do not serialize the root path. It will differ between users.
}

void Project::deserializeJSON(JSONObject& jo) {
    // This should handle multiple versions in the future
    if (jo[VERSION_FIELD_NAME].GetUint() != CURRENT_DATA_FORMAT_VERSION) {
        LOG_E("Unknown project file version" << jo[VERSION_FIELD_NAME].GetUint());
        throw std::runtime_error("Unknown project file version");
    }
    
    gameName = jo[GAME_NAME_FIELD_NAME].GetString();
    companyName = jo[COMPANY_NAME_FIELD_NAME].GetString();
    firstWorldName = jo[FIRST_WORLD_FIELD_NAME].GetString();
    
    const std::uint16_t major = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_MAJOR_FIELD_NAME].GetUint();
    const std::uint16_t minor = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_MINOR_FIELD_NAME].GetUint();
    const std::uint16_t patch = jo[GAME_VERSION_FIELD_NAME][GAME_VERSION_PATCH_FIELD_NAME].GetUint();
    
    version = Version(major, minor, patch);

    baseLocale = jo[BASE_LOCALE_FIELD_NAME].GetString();
}

void Project::setFirstWorldName(std::string name) {
    firstWorldName = name;
}

std::string Project::getFirstWorldName() const {
    return firstWorldName;
}

void Project::setCompanyName(std::string name) {
    companyName = name;
}

std::string Project::getCompanyName() const {
    return companyName;
}

void Project::setGameName(std::string name) {
    gameName = name;
}

std::string Project::getGameName() const {
    return gameName;
}

void Project::setVersion(const Version& version) {
    this->version = version;
}

Version Project::getVersion() const {
    return version;
}

std::string Project::getBaseLocale() const {
    return baseLocale;
}

void Project::setBaseLocale(std::string locale) {
    baseLocale = locale;
}

}
