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

#include "sol/sol.hpp"
#include "core/exceptions/ConfigurationLoadError.hpp"
#include "core/exceptions/ConfigurationWriteError.hpp"
#include "core/interfaces/Configurable.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"

namespace iyf {
    
bool operator==(const ConfigurationValue& a, const ConfigurationValue& b) {
    return a.variant == b.variant;
}

bool operator!=(const ConfigurationValue& a, const ConfigurationValue& b) {
    return a.variant != b.variant;
}
    
const std::array<std::string, static_cast<std::size_t>(ConfigurationValueFamily::COUNT)> con::ConfigurationFamilyNames = {
    "Core",         // 0
    "Engine",       // 1
    "Editor",       // 2
    "Graphics",     // 3
    "Sound",        // 4
    "Controls",     // 5
    "Localization", // 6
    "Gameplay",     // 7
    "Project",      // 8
    "Other",        // 9
};

static void fillMapsFromLuaState(sol::state& state, const std::vector<ConfigurationValueMap*>& maps) {
    for (std::size_t i = 0; i < static_cast<std::size_t>(ConfigurationValueFamily::COUNT); ++i) {
        const std::string& familyName = con::ConfigurationFamilyNames[i];
        
        sol::table table = state[familyName];
        
//         LOG_D(familyName << " " << table.size());
        
        for (const auto& v : table) {
            const std::string& valueName = v.first.as<std::string>();
            ConfigurationValueHandle handle(HS(valueName.c_str()), static_cast<ConfigurationValueFamily>(i));
            
//             LOG_D("FOUND CONFIG VALUE. Family: " << familyName << "; Type: " << static_cast<int>(v.second.get_type())
//                     << ";\n\tName:  " << valueName << ";\n\tValue: " << v.second.as<std::string>());
            
            switch (v.second.get_type()) {
                case sol::type::string:
                    for (ConfigurationValueMap* map : maps) {
                        (*map)[handle] = ConfigurationValue(v.second.as<std::string>(), valueName);
                    }
                    break;
                case sol::type::number:
                    for (ConfigurationValueMap* map : maps) {
                        (*map)[handle] = ConfigurationValue(v.second.as<double>(), valueName);
                    }
                    break;
                case sol::type::boolean:
                    for (ConfigurationValueMap* map : maps) {
                        (*map)[handle] = ConfigurationValue(v.second.as<bool>(), valueName);
                    }
                    break;
                default:
                    std::stringstream ss;
                    ss << "A configuration value must be a number, a boolean or a string. A value from the \"" 
                    << familyName << "\" family with a name \"" << valueName << "\" was neither";
                    throw ConfigurationLoadError(ss.str());
            }
        }
    }
}

inline void executeScript(sol::state& state, const ConfigurationFilePath& path) {
    if (path.type == ConfigurationFilePath::PathType::Virtual) {
        File in(path.path, File::OpenMode::Read);
        auto data = in.readWholeFile();
        
        state.script(std::string(data.first.get(), data.second));
    } else {
        state.script_file(path.path.string());
    }
}

void Configuration::fillConfigurationMaps(const std::vector<ConfigurationFilePath>& paths, ConfigurationValueMap& resolvedValueMap, 
                                          ConfigurationValueMap* systemValueMap, ConfigurationValueMap* userValueMap, FileSystem* filesystem,
                                          Mode mode) {
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
                    LOG_E("Failed to obtain file statistics for the user's configuration file: " << userConfigurationPath.path);
                    throw std::runtime_error("Failed to obtain file statistics for the user's configuration file.");
                }
                
                if (stat.readonly) {
                    LOG_E("The user's configuration file " << userConfigurationPath.path << " exists in the virtual filesystem, but it is not writable");
                    throw ConfigurationLoadError("The user's configuration file exists in the virtual filesystem, but it is not writable.");
                }
            } else {
                File out(userConfigurationPath.path, File::OpenMode::Write);
            }
        } else {
            std::ofstream userConfigFile(userConfigurationPath.path.string(), std::ofstream::app);
            if (!userConfigFile.is_open()) {
                throw ConfigurationLoadError("The user's configuration file does not exist and could not create it at the specifed path: " +
                                            userConfigurationPath.path.string());
            }
            userConfigFile.close();
        }
    }
    
    sol::state state;
    
    std::stringstream ss;
    for (const std::string& s : con::ConfigurationFamilyNames) {
        ss << s << " = {}\n";
    }
    
    // Clear tables
    state.script(ss.str());
    
    if (paths.size() > 1) {
        // Fill the tables with values from all files except the last one, which is considered to be the user's file
        for (std::size_t i = 0; i < paths.size() - 1; ++i) {
            executeScript(state, paths[i]);
        }
        
        std::vector<ConfigurationValueMap*> mapsToUpdate;
        mapsToUpdate.push_back(&resolvedValueMap);
        if (systemValueMap != nullptr) {
            mapsToUpdate.push_back(systemValueMap);
        }
        
        fillMapsFromLuaState(state, mapsToUpdate);
        
        // Clear tables so that we would only receive the values from user's configuration during the next step
        state.script(ss.str());
    }
    
    // Load the user's configuration values
    executeScript(state, paths.back());
    
//     LOG_D("User's data path: " << paths.back())
    
    std::vector<ConfigurationValueMap*> mapsToUpdate;
    mapsToUpdate.push_back(&resolvedValueMap);
    if (userValueMap != nullptr) {
        mapsToUpdate.push_back(userValueMap);
    }
    
    fillMapsFromLuaState(state, mapsToUpdate);
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
    
    Configuration::fillConfigurationMaps(this->paths, resolvedConfigurationValues, &systemValues, &userValues, filesystem, mode);
    
    LOG_I("Number of loaded configuration values: " << resolvedConfigurationValues.size());
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
    
    for (const auto& value : changedValues) {
        resolvedConfigurationValues[value.first] = value.second;
        userValues[value.first] = value.second;
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
    
    // TODO only the user's values are used here. Perhaps a separate mutex is needed?
    std::lock_guard<std::mutex> lock(configurationValueMutex);
    
    std::array<std::vector<ConfigurationValue>, static_cast<std::size_t>(ConfigurationValueFamily::COUNT)> valuesToWrite;
    
    for (auto& v : valuesToWrite) {
        // TODO can probably do something better
        v.reserve(100);
    }
    
    for (const auto& value : userValues) {
        auto result = systemValues.find(value.first);
        
        // Only values that are not in the systemValues or differ from them are added to the final file
        if (result == systemValues.end() || result->second != value.second) {
            valuesToWrite[static_cast<std::size_t>(value.first.family)].push_back(value.second);
        }
    }
    
    std::string outputPath = paths.back().path.string();
    std::ofstream out(outputPath);
    
    if (!out.is_open()) {
        throw ConfigurationWriteError("Failed to open configuration file " + outputPath + " for writing.");
    }
    
    LOG_D("Writing updated configuration values to: " << outputPath);
    
    for (std::size_t i = 0; i < static_cast<std::size_t>(ConfigurationValueFamily::COUNT); ++i) {
        const std::string& familyName = con::ConfigurationFamilyNames[i];
        std::vector<ConfigurationValue>& currentFamilyValues = valuesToWrite[i];
        
        if (currentFamilyValues.size() == 0) {
            LOG_D("Configuration value family \"" << familyName << "\" has no values that need to be written");
        } else {
            LOG_D("Configuration value family \"" << familyName << "\" has " << currentFamilyValues.size() << " value(s) that need to be written");
        }
        
        std::sort(currentFamilyValues.begin(), currentFamilyValues.end(), [](const ConfigurationValue& a, const ConfigurationValue& b) {
            return a.getName() < b.getName();
        });
        
        for (const auto& v : currentFamilyValues) {
            out << familyName << "[\"" << v.getName() << "\"] = ";
            
            switch (v.getType()) {
                case ConfigurationValueType::Double: {
                    double value = v;
                    out << value;
                    break;
                }
                case ConfigurationValueType::Bool: {
                    bool value = v;
                    out << (value ? "true" : "false");
                    break;
                }
                case ConfigurationValueType::String: {
                    std::string value = v;
                    out << "\"" << value << "\"";
                    break;
                }
                default:
                    throw std::runtime_error("Unknown type: " + std::to_string(static_cast<std::size_t>(v.getType())));
            }
            
            out << "\n";
        }
        
        if (currentFamilyValues.size() > 0) {
            out << "\n";
        }
    }
    
    return true;
}

ConfigurationValue ConfigurationEditor::getValue(const ConfigurationValueHandle& handle) const {
    auto result = updatedValues.find(handle);
    if (result != updatedValues.end()) {
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
    updatedValues.clear();
}

void ConfigurationEditor::setToPreset(const ConfigurationFilePath& presetFilePath) {
    pendingUpdate = true;
    Configuration::fillConfigurationMaps({presetFilePath}, updatedValues, nullptr, nullptr, configuration->getFileSystem(), configuration->getMode());
}

ConfigurationEditor::~ConfigurationEditor() {
    if (pendingUpdate) {
        LOG_W("A ConfigurationEditor with noncommited values has been destroyed");
    }
}

}
