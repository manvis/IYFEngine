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

#include "core/configuration/Configuration.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>

#include "core/exceptions/ConfigurationLoadError.hpp"
#include "core/exceptions/ConfigurationWriteError.hpp"
#include "core/interfaces/Configurable.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"

#include "utilities/StringUtilities.hpp"
#include "utilities/TypeHelpers.hpp"

#include "fmt/ostream.h"

namespace iyf {
namespace con {
const std::string& GetConfigurationValueNamespaceName(ConfigurationValueNamespace namespaceID) {
    static const std::array<std::string, static_cast<std::size_t>(ConfigurationValueNamespace::COUNT)> ConfigurationFamilyNames = {
        "core",         // 0
        "engine",       // 1
        "editor",       // 2
        "graphics",     // 3
        "sound",        // 4
        "controls",     // 5
        "localization", // 6
        "gameplay",     // 7
        "project",      // 8
        "other",        // 9
    };
    
    return ConfigurationFamilyNames[static_cast<std::size_t>(namespaceID)];
}

const StringHash GetConfigurationValueNamespaceNameHash(ConfigurationValueNamespace namespaceID) {
    static const std::array<StringHash, static_cast<std::size_t>(ConfigurationValueNamespace::COUNT)> ConfigurationFamilyNameHashes = {
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Core)),         // 0
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Engine)),       // 1
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Editor)),       // 2
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Graphics)),     // 3
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Sound)),        // 4
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Controls)),     // 5
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Localization)), // 6
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Gameplay)),     // 7
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Project)),      // 8
        HS(GetConfigurationValueNamespaceName(ConfigurationValueNamespace::Other)),        // 9
    };
    
    return ConfigurationFamilyNameHashes[static_cast<std::size_t>(namespaceID)];
}
}

void Configuration::fillMapsFromFiles(const std::vector<ConfigurationFile>& files, const std::vector<ConfigurationValueMap*>& maps) {
    for (const ConfigurationFile& file : files) {
        for (const ConfigurationFile::ConfigurationFileLine& line : file.getLines()) {
            if (line.index() == 1) {
                const ConfigurationValue& value = std::get<ConfigurationValue>(line);
                const ConfigurationValueHandle handle(HS(value.getName()), HS(value.getNamespaceName()));
                
                for (ConfigurationValueMap* map : maps) {
                    (map->data)[handle] = value;
                }
            }
        }
    }
    
}

ConfigurationValueHandle::ConfigurationValueHandle(StringHash nameHash, ConfigurationValueNamespace namespaceID)
    : nameHash(nameHash), namespaceHash(con::GetConfigurationValueNamespaceNameHash(namespaceID)) {}

std::string ConfigurationFile::ParseResult::printErrors() const {
    std::stringstream ss;
    
    for (const auto& line : linesWithErrors) {
        ss << "\tLINE: " << std::setw(5) << line.first << "\n\t\t";
        
        switch (line.second) {
            case ConfigurationFileError::Correct:
                ss << "No Error";
                break;
            case ConfigurationFileError::NonUnixLineEndings:
                ss << "FATAL: Non Unix line endings";
                break;
            case ConfigurationFileError::InvalidLineSyntax:
                ss << "Invalid line length or = missing. Line skipped";
                break;
            case ConfigurationFileError::NoIdentifier:
                ss << "No identifier part (name and, optionally, namespace). Line skipped";
                break;
            case ConfigurationFileError::NoName:
                ss << "No name. Line skipped";
                break;
            case ConfigurationFileError::NamespaceNotAlphanumericASCII:
                ss << "Namespace contains non ASCII characters. Line skipped";
                break;
            case ConfigurationFileError::NameNotAlphanumericASCII:
                ss << "Name contains non ASCII characters. Line skipped";
                break;
            case ConfigurationFileError::NoValue:
                ss << "No value. Line skipped";
                break;
            case ConfigurationFileError::InvalidStringParameter:
                ss << "Invalid string parameter. Line skipped";
                break;
            case ConfigurationFileError::InvalidNumericParameter:
                ss << "Invalid numeric parameter. Line skipped";
                break;
            case ConfigurationFileError::UnknownError:
                ss << "Unknown error. Line skipped";
                break;
        }
    }
    
    return ss.str();
}

ConfigurationFile::NumberParseResult ConfigurationFile::parseNumber(const std::string_view& str, std::int64_t& intVal, double& doubleVal) {
    
    const char* lastValueChar = str.end();
    char* end;
    intVal = std::strtol(str.data(), &end, 10);
    
    if (errno == ERANGE) {
        errno = 0;
        return NumberParseResult::ParseFailed;
    }
    
    if (end == lastValueChar) {
        return NumberParseResult::Int;
    }
    
    doubleVal = std::strtod(str.data(), &end);
    if (errno == ERANGE) {
        errno = 0;
        return NumberParseResult::ParseFailed;
    }
    
    if (end == lastValueChar) {
        return NumberParseResult::Double;
    }
    
    return NumberParseResult::ParseFailed;
}

ConfigurationFile::ParsedLine ConfigurationFile::processLine(const std::string_view& line) {
    if (line.length() == 0) {
        return ParsedLine(ConfigurationFileError::Correct);
    }
    
    if (line.length() >= 2 && line[0] == '/' && line[1] == '/') {
        return ParsedLine(ConfigurationFileError::Correct);
    }
    
    const std::size_t equalsPos = line.find_first_of('=');
    
    // Shortest possible line is a=1
    if (equalsPos == std::string_view::npos || 
        line.length() < 3 ||
        equalsPos == 0 ||
        equalsPos == line.length() - 1) {
        
        return ParsedLine(ConfigurationFileError::InvalidLineSyntax);
    }
    
    const std::size_t firstBeforeEquals = equalsPos - 1;
    const std::size_t lastAfterEquals = equalsPos + 1;
    
    std::size_t identifierEnd = firstBeforeEquals;
    bool identifierExists = true;
    
    while (line[identifierEnd] == ' ') {
        if (identifierEnd != 0) {
            identifierEnd--;
        } else {
            identifierExists = false;
            break;
        }
    }
    
    if (!identifierExists) {
        return ParsedLine(ConfigurationFileError::NoIdentifier);
    }
    
    const std::string_view identifier(line.data(), identifierEnd + 1);
    const std::size_t separatorPos = identifier.find_first_of('.');
    
    std::string_view namespaceName;
    if (separatorPos != std::string_view::npos) {
        namespaceName = std::string_view(line.data(), separatorPos);
        
        if (separatorPos == identifier.length() - 1) {
            return ParsedLine(ConfigurationFileError::NoName);
        }
    }
    
    if (!namespaceName.empty() && !util::isAlphanumericASCII(namespaceName)) {
        return ParsedLine(ConfigurationFileError::NamespaceNotAlphanumericASCII);
    }
    
    const std::string_view paramName(identifier.data() + separatorPos + 1, identifier.length() - separatorPos - 1);
    if (!util::isAlphanumericASCII(paramName)) {
        return ParsedLine(ConfigurationFileError::NameNotAlphanumericASCII);
    }
    
    std::size_t valueStart = lastAfterEquals;
    const std::size_t lastCharPos = line.length() - 1;
    
    bool valueExists = true;
    while (line[valueStart] == ' ') {
        if (valueStart < lastCharPos) {
            valueStart++;
        } else {
            valueExists = false;
            break;
        }
    }
    
    if (!valueExists) {
        return ParsedLine(ConfigurationFileError::NoValue);
    }
    
    std::size_t valueEnd = lastCharPos;
    while (line[valueEnd] == ' ') {
        valueEnd--;
    }
    
    
    const std::string_view valueStr(line.data() + valueStart, valueEnd - valueStart + 1);
    
    bool isNumericParam = false;
    
    if (valueStr == std::string_view("true")) {
        return ParsedLine(ConfigurationValue(true, paramName, namespaceName), ConfigurationFileError::Correct);
    } else if (valueStr == std::string_view("false")) {
        return ParsedLine(ConfigurationValue(false, paramName, namespaceName), ConfigurationFileError::Correct);
    }
    
    const std::size_t firstQuote = valueStr.find_first_of("\"");
    if (firstQuote == std::string_view::npos) {
        isNumericParam = true;
    } else if ((valueStr[valueStr.length() - 1] != '\"') || (valueStr.length() - 1 == firstQuote)) {
        return ParsedLine(ConfigurationFileError::InvalidStringParameter);
    } else {
        return ParsedLine(ConfigurationValue(std::string(valueStr.data() + 1, valueStr.length() - 2), paramName, namespaceName), ConfigurationFileError::Correct);
    }
    
    std::int64_t intVal = 0;
    double doubleVal = 0.0;
    NumberParseResult parseResult = NumberParseResult::ParseFailed;
    if (isNumericParam) {
        parseResult = parseNumber(valueStr, intVal, doubleVal);
        
        if (parseResult == NumberParseResult::ParseFailed) {
            return ParsedLine(ConfigurationFileError::InvalidNumericParameter);
        }
    }
    
    if (parseResult == NumberParseResult::Int) {
        return ParsedLine(ConfigurationValue(intVal, paramName, namespaceName), ConfigurationFileError::Correct);
    }
    
    if (parseResult == NumberParseResult::Double) {
        return ParsedLine(ConfigurationValue(doubleVal, paramName, namespaceName), ConfigurationFileError::Correct);
    }
    
    return ParsedLine(ConfigurationFileError::UnknownError);
}

ConfigurationFile::ParseResult ConfigurationFile::parse(const std::string& fileContents) {
    lines.clear();
    
    ParseResult result;
    
    if (fileContents.empty()) {
        return result;
    }
    
    const std::size_t nonUnixLineEnding = fileContents.find("\r\n");
    
    if (nonUnixLineEnding != std::string::npos) {
        throw std::runtime_error("A non unix line ending was detected in file\n");
        result.linesWithErrors.emplace_back(std::numeric_limits<std::size_t>::max(), ConfigurationFileError::NonUnixLineEndings);
    }
    
    std::size_t lineCount = 0;
    std::size_t lineEnd = 0;
    std::size_t lineStart = 0;
    bool allLinesParsed;
    
    while (true) {
        lineEnd = fileContents.find_first_of('\n', lineStart);
        if (lineEnd != 0)
        
        if (lineEnd == std::string::npos) {
            allLinesParsed = true;
            
            if (fileContents.length() - lineStart == 0) {
                break;
            } else {
                lineEnd = fileContents.length();
            }
        }
        
        const std::string_view line(fileContents.data() + lineStart, lineEnd - lineStart);
        ParsedLine parsedLine = processLine(line);
        
        if (parsedLine.error != ConfigurationFileError::Correct) {
            result.linesWithErrors.emplace_back(lineCount, parsedLine.error);
        }
        
        lines.emplace_back(parsedLine.line);
        
        
        lineCount++;
        lineStart = lineEnd + 1;
        
        if (allLinesParsed) {
            break;
        }
    }
    
    return result;
}

std::string Configuration::loadFile(const ConfigurationFilePath& path) {
    if (path.type == ConfigurationFilePath::PathType::Virtual) {
        File in(path.path, File::OpenMode::Read);
        auto data = in.readWholeFile();
        
        const std::string fileContents = std::string(data.first.get(), data.second);
        return fileContents;
    } else {
        std::ifstream in(path.path, std::ios::binary | std::ios::ate);
        const std::size_t size = in.tellg();
        in.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        
        if (buffer.empty()) {
            LOG_W("The config file was empty. Was this intentional?");
        }
        
        const bool readResult = bool(in.read(buffer.data(), size));
        if (!readResult) {
            throw ConfigurationLoadError(std::string("Failed to read a configuration file: ") + path.path.generic_string());
        }
        
        if (buffer.empty()) {
            LOG_W("The config file was empty. Was this intentional?");
        }
        
        return std::string(buffer.data(), buffer.size());
    }
}

void Configuration::fillConfigurationMaps(const std::vector<ConfigurationFilePath>& paths, ConfigurationValueMap& resolvedValueMap, 
                                          ConfigurationValueMap* systemValueMap, ConfigurationValueMap* userValueMap, ConfigurationFile* userConfigFile, 
                                          FileSystem* filesystem, Mode mode) {
    if (paths.size() == 0) {
        throw ConfigurationLoadError("At least one path must be set.");
    }
    
    // Don't create any files if the configuration instance was created in read-only mode
    if (mode == Mode::Editable) {
        // As specified in the documentation of the Configuration constructor, the user's configuration file is automatically
        // created if it does not exist, which is very likely in a new installation.
        const ConfigurationFilePath& userConfigurationPath = paths.back();
        
        if (userConfigurationPath.type == ConfigurationFilePath::PathType::Virtual) {
            assert(filesystem != nullptr);
            
            if (filesystem->exists(userConfigurationPath.path)) {
                // File exists. Check if it's writable
                PHYSFS_Stat stat;
                int result = filesystem->getFileSystemStatistics(userConfigurationPath.path, stat);
                if (!result) {
                    LOG_E("Failed to obtain file statistics for the user's configuration file: {}", userConfigurationPath.path);
                    throw std::runtime_error("Failed to obtain file statistics for the user's configuration file.");
                }
                
                if (stat.readonly) {
                    LOG_E("The user's configuration file {} exists in the virtual filesystem, but it is not writable", userConfigurationPath.path);
                    throw ConfigurationLoadError("The user's configuration file exists in the virtual filesystem, but it is not writable.");
                }
            } else {
                File out(userConfigurationPath.path, File::OpenMode::Write);
            }
        } else {
            std::ofstream writableConfigFile(userConfigurationPath.path.string(), std::ofstream::app);
            if (!writableConfigFile.is_open()) {
                throw ConfigurationLoadError("The user's configuration file does not exist and could not create it at the specifed path: " +
                                            userConfigurationPath.path.string());
            }
            writableConfigFile.close();
        }
    }
    
    std::stringstream errors;
    if (paths.size() > 1) {
        std::vector<ConfigurationFile> configFiles;
        configFiles.reserve(paths.size() - 1);
        
        // Fill the tables with values from all files except the last one, which is considered to be the user's file
        for (std::size_t i = 0; i < paths.size() - 1; ++i) {
            std::string loadedFile = Configuration::loadFile(paths[i]);
            
            configFiles.emplace_back();
            ConfigurationFile::ParseResult result = configFiles.back().parse(loadedFile);
            
            if (result.hasErrors()) {
                errors << "\tERRORS IN FILE: (" << ((paths[i].type == ConfigurationFilePath::PathType::Real) ? "Real path" : "Virtual path") << ") " << paths[i].path << "\n";
                errors << result.printErrors();
            }
        }
        
        std::vector<ConfigurationValueMap*> mapsToUpdate;
        mapsToUpdate.push_back(&resolvedValueMap);
        if (systemValueMap != nullptr) {
            mapsToUpdate.push_back(systemValueMap);
        }
        
        Configuration::fillMapsFromFiles(configFiles, mapsToUpdate);
    }
    
    std::string loadedFile = Configuration::loadFile(paths.back());
    ConfigurationFile cf;
    ConfigurationFile::ParseResult result = cf.parse(loadedFile);
    
    if (result.hasErrors()) {
        errors << "\tERRORS IN FILE: (" << ((paths.back().type == ConfigurationFilePath::PathType::Real) ? "Real path" : "Virtual path") << ") " << paths.back().path << "\n";
        errors << result.printErrors();
    }
    
    std::vector<ConfigurationValueMap*> mapsToUpdate;
    mapsToUpdate.push_back(&resolvedValueMap);
    if (userValueMap != nullptr) {
        mapsToUpdate.push_back(userValueMap);
    }
    
    Configuration::fillMapsFromFiles({cf}, mapsToUpdate);
    
    if (const std::string errorStr = errors.str(); !errorStr.empty()) {
        LOG_W("{}", errorStr);
    }
    
    if (userConfigFile != nullptr) {
        (*userConfigFile) = std::move(cf);
    }
}

Configuration::Configuration(const std::vector<ConfigurationFilePath> paths, Mode mode, FileSystem* filesystem) 
    : filesystem(filesystem), paths(std::move(paths)), mode(mode) {
    
    if (filesystem == nullptr) {
        for (const auto& p : this->paths) {
            if (p.type == ConfigurationFilePath::PathType::Virtual) {
                std::logic_error("The filesystem parameter can't be nullptr when virtual paths are used.");
            }
        }
    }
    
    Configuration::fillConfigurationMaps(this->paths, resolvedConfigurationValues, &systemValues, &userValues, &userConfigFile, filesystem, mode);
    
    LOG_I("Number of loaded configuration values: {}", resolvedConfigurationValues.data.size());
}

void Configuration::addListener(Configurable* listener) {
    listeners.push_back(listener);
}

void Configuration::removeListener(Configurable* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

std::unique_ptr<ConfigurationEditor> Configuration::makeConfigurationEditor() {
    if (getMode() == Mode::ReadOnly) {
        throw std::runtime_error("This Configuration instance is read only ");
    }
    
    return std::unique_ptr<ConfigurationEditor>(new ConfigurationEditor(this));
}

void Configuration::setChangedValues(const ConfigurationValueMap& changedValues, bool notify) {
    // Lock before modifying
    std::lock_guard<std::mutex> lock(configurationValueMutex);
    
    for (const auto& value : changedValues.data) {
        resolvedConfigurationValues.data[value.first] = value.second;
        userValues.data[value.first] = value.second;
    }
    
    if (notify) {
        notifyChanged(changedValues);
    }
}

void Configuration::notifyChanged(const ConfigurationValueMap& changedValues) {
    for (auto l : listeners) {
        l->handleConfigChange(changedValues);
    }
}

bool Configuration::serialize() {
    if (getMode() == Mode::ReadOnly) {
        throw std::runtime_error("This Configuration instance is read only ");
    }
    
    std::lock_guard<std::mutex> lock(configurationValueMutex);
    
    std::string outputPath = paths.back().path.string();
    std::ofstream out(outputPath);
    
    if (!out.is_open()) {
        throw ConfigurationWriteError("Failed to open configuration file " + outputPath + " for writing.");
    }
    
    LOG_D("Writing configuration values to: {}", outputPath);
    
    for (const ConfigurationFile::ConfigurationFileLine& line : userConfigFile.getLines()) {
        std::visit([&out](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, NonConfigLine>) {
                out << arg.getLine() << "\n";
            } else if constexpr (std::is_same_v<T, ConfigurationValue>) {
                out << arg.getNamespaceName() << "." << arg.getName() << " = ";
                
                //TODO output variant, update user's file
                std::visit([&out](auto&& var) {
                    using U = std::decay_t<decltype(var)>;
                    
                    if constexpr (std::is_same_v<U, double> || std::is_same_v<U, std::int64_t>) {
                        out << var;
                    } else if constexpr (std::is_same_v<U, bool>) {
                        out << std::boolalpha << var;
                    } else if constexpr (std::is_same_v<U, std::string>) {
                        out << "\"" << var << "\"";
                    } else {
                        static_assert(util::always_false_type<T>::value, "Some value types not handled.");
                    }
                }, arg.getValue());
                
                out << "\n";
            } else {
                static_assert(util::always_false_type<T>::value, "Some options not handled.");
            }
        }, line);
    }
    
//     for (std::size_t i = 0; i < static_cast<std::size_t>(ConfigurationValueNamespace::COUNT); ++i) {
//         const std::string& familyName = con::GetConfigurationValueNamespaceName(static_cast<ConfigurationValueNamespace>(i));
//         
//         std::vector<ConfigurationValue>& currentFamilyValues = valuesToWrite[i];
//         
//         if (currentFamilyValues.size() == 0) {
//             LOG_D("Configuration value family \"{}\" has no values that need to be written", familyName);
//         } else {
//             LOG_D("Configuration value family \"{}\" has {} value(s) that need to be written", familyName, currentFamilyValues.size());
//         }
//         
//         std::sort(currentFamilyValues.begin(), currentFamilyValues.end(), [](const ConfigurationValue& a, const ConfigurationValue& b) {
//             return a.getName() < b.getName();
//         });
//         
//         for (const auto& v : currentFamilyValues) {
//             out << familyName << "[\"" << v.getName() << "\"] = ";
//             
//             switch (v.getType()) {
//                 case ConfigurationValueType::Double: {
//                     double value = v;
//                     out << value;
//                     break;
//                 }
//                 case ConfigurationValueType::Bool: {
//                     bool value = v;
//                     out << (value ? "true" : "false");
//                     break;
//                 }
//                 case ConfigurationValueType::String: {
//                     std::string value = v;
//                     out << "\"" << value << "\"";
//                     break;
//                 }
//                 default:
//                     throw std::runtime_error("Unknown type: " + std::to_string(static_cast<std::size_t>(v.getType())));
//             }
//             
//             out << "\n";
//         }
//         
//         if (currentFamilyValues.size() > 0) {
//             out << "\n";
//         }
//     }
    
    return true;
}

ConfigurationValue ConfigurationEditor::getValue(const ConfigurationValueHandle& handle) const {
    auto result = updatedValues.data.find(handle);
    if (result != updatedValues.data.end()) {
        return result->second;
    }
    
    return configuration->getValue(handle);
}

void ConfigurationEditor::commit(bool notify) {
    if (!pendingUpdate) {
        return;
    }
    
    configuration->setChangedValues(updatedValues, notify);
    
    rollback();
}

void ConfigurationEditor::rollback() {
    pendingUpdate = false;
    updatedValues.data.clear();
}

void ConfigurationEditor::setToPreset(const ConfigurationFilePath& presetFilePath) {
    pendingUpdate = true;
    Configuration::fillConfigurationMaps({presetFilePath}, updatedValues, nullptr, nullptr, nullptr, configuration->getFileSystem(), configuration->getMode());
}

ConfigurationEditor::~ConfigurationEditor() {
    if (pendingUpdate) {
        LOG_W("A ConfigurationEditor with noncommited values has been destroyed");
    }
}

}
