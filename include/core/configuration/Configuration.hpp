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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <variant>

#include "core/filesystem/cppFilesystem.hpp"
#include "core/exceptions/ConfigurationValueReadError.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/hashing/HashCombine.hpp"

namespace iyf {
class FileSystem;

/// A list of all possible configuration value families
///
/// \warning this and iyf::ConfigurationValueFamily 
enum class ConfigurationValueFamily : std::uint8_t {
    /// Low level Engine settings that should only be changed by engine developers or maintainers. Careless adjustment may
    /// cause undefined behaviour or errors.
    Core = 0,
    /// Higher level Engine settings that game developers may use to better adapt the Engine to their specific usecases.
    /// Debug options go here as well
    Engine = 1,
    /// This family contains editor specific configuration that should not change the games in any way.
    Editor = 2,
    /// This family should contain screen size, visual quality settings, fov and similar values that relate to rendering.
    Graphics = 3,
    /// This family should contain sound, music, voice and microphone settings.
    Sound = 4,
    /// This family should contain keyboard, controller and mouse bindings, mouse sensitivity and etc.
    Controls = 5,
    /// This family should contain various localization and language options
    Localization = 6,
    /// This family should contain gameplay settings common to all playthroughs (e.g., should interactive objects be highlighted
    /// or not). Things that depend on a specific playthough (e.g., difficulty) should go into savegames.
    Gameplay = 7,
    /// This family should contain project settings
    Project = 8,
    /// This family should contain configuration values that do not belong to any other family
    Other = 9,
    COUNT
};

/// A key used for lookups in the iyf::ConfigurationValueMap 
struct ConfigurationValueHandle {
    constexpr ConfigurationValueHandle(StringHash nameHash, ConfigurationValueFamily family) : nameHash(nameHash), family(family) {}
    
    const StringHash nameHash;
    const ConfigurationValueFamily family;
    
    friend bool operator==(const ConfigurationValueHandle& a, const ConfigurationValueHandle& b) {
        return (a.nameHash == b.nameHash) && (a.family == b.family);
    }
};
}

// Injecting a hash function for iyf::ConfigurationValueHandle
namespace std {
    template <>
    struct hash<iyf::ConfigurationValueHandle> {
        std::size_t operator()(const iyf::ConfigurationValueHandle& k) const {
            std::size_t seed = 0;
            iyf::util::HashCombine(seed, k.nameHash.value());
            iyf::util::HashCombine(seed, static_cast<std::size_t>(k.family));
            return seed;
        }
    };
}

namespace sol {
    class state;
}

namespace iyf {

namespace con {
const std::string& GetConfigurationValueFamilyName(ConfigurationValueFamily family);
}

/// \warning the order must match iyf::ConfigurationVariant
enum class ConfigurationValueType : std::uint8_t {
    Double = 0,
    Bool   = 1,
    String = 2
};

/// \warning the order must match iyf::ConfigurationValueType
using ConfigurationVariant = std::variant<double, bool, std::string>;

class Configurable;
class ConfigurationManifest;

class ConfigurationValue {
public:
    ConfigurationValue() = default;
    
    template <typename T>
    inline ConfigurationValue(T value, std::string name) : variant(value), name(std::move(name)) { }
    
    /// \return Real, non-hashed name of this value
    inline const std::string& getName() const {
        return name;
    }
    
    inline ConfigurationValueType getType() const {
        return static_cast<ConfigurationValueType>(variant.index());
    }
    
    // We don't need any additional type checks since variant performs static_asserts.
    template <typename T>
    inline operator T() const {
        return std::get<T>(variant);
    }
    
    friend bool operator==(const ConfigurationValue& a, const ConfigurationValue& b);
    friend bool operator!=(const ConfigurationValue& a, const ConfigurationValue& b);
private:
    ConfigurationVariant variant;
    std::string name;
};

template <>
inline ConfigurationValue::operator std::int32_t() const {
    double value = std::get<double>(variant);
    return static_cast<std::int32_t>(value);
}

template <>
inline ConfigurationValue::operator std::int64_t() const {
    double value = std::get<double>(variant);
    return static_cast<std::int64_t>(value);
}

using ConfigurationValueMap = std::unordered_map<ConfigurationValueHandle, ConfigurationValue>;

class ConfigurationEditor;

struct ConfigurationFilePath {
    enum class PathType {
        Real, Virtual
    };
    
    ConfigurationFilePath(fs::path path, PathType type)
        : path(std::move(path)), type(type) {}
    
    fs::path path;
    PathType type;
};

class Configuration : private NonCopyable {
public:
    enum class Mode {
        Editable,
        ReadOnly
    };
    
    /// Creates a new iyf::Configuration instance by reading and executing all specified lua script files. The order 
    /// of the files is very important because the configuration values that are specified later will automatically
    /// override the earlier values.
    ///
    /// If the mode is Mode::Editable, the last path in the list is assumed to point to the user's configuration data 
    /// that will be updated whenever Configuration::serialize() is called. Therefore, ensure that it is writable.
    /// An empty configuration file will be created automatically if it does not exist.
    ///
    /// \warning To ensure thread safety and prevent name collisions, each iyf::Configuration instance uses a
    /// separate lua state.
    ///
    /// \throws ConfigurationLoadError if the vector of paths was empty, contained non-existing files or the lua 
    /// scripts had errors.
    ///
    /// \throws std::logic_error if virtual filesystem paths are used, but the provided virtual filesystem pointer is 
    /// nullptr.
    ///
    /// \param[in] paths A list of configuration files to open.
    /// \param[in] mode The mode of operations. If mode == Mode::ReadOnly, the Configuration instance cannot be changed or
    /// serialized.
    /// \param[in] filesystem A pointer to a virtual filesystem instance. Can be nullptr if all paths are real (isVirtualFileSystemPath is false)
    Configuration(const std::vector<ConfigurationFilePath> paths, Mode mode, FileSystem* filesystem);
    
    inline Mode getMode() const {
        return mode;
    }
    
    inline FileSystem* getFileSystem() const {
        return filesystem;
    }
    
    /// Get the iyf::ConfigurationValue using a pre-built ConfigurationValueHandle.
    ///
    /// \remark This function is thread safe.
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found.
    ///
    /// \param[in] handle A pre-built ConfigurationValueHandle
    inline ConfigurationValue getValue(const ConfigurationValueHandle& handle) const {
        std::lock_guard<std::mutex> lock(configurationValueMutex);
        
        auto result = resolvedConfigurationValues.find(handle);
        
        if (result == resolvedConfigurationValues.end()) {
            throw ConfigurationValueReadError("Unknown configuration value with hash: " + std::to_string(handle.nameHash));
        }
        
        return result->second;
    }

    /// Get the iyf::ConfigurationValue using a pre-hashed name and family.
    ///
    /// \remark This function is thread safe.
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found.
    ///
    /// \param[in] nameHash A pre-hashed name
    /// \param[in] family The family of configuration values that you want to querying
    inline ConfigurationValue getValue(StringHash nameHash, ConfigurationValueFamily family) const {
        ConfigurationValueHandle handle(nameHash, family);
        return getValue(handle);
    }
    
    /// Get the iyf::ConfigurationValue using an std::string name name and family.
    ///
    /// \remark This function is thread safe.
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found.
    ///
    /// \param[in] name Name of the parameter
    /// \param[in] family The family of configuration values that you want to querying
    inline ConfigurationValue getValue(const std::string& name, ConfigurationValueFamily family) const {
        return getValue(HS(name.c_str()), family);
    }
    
    /// Adds a listener that will get notified when Configuration changes. For requirements and best performance tips,
    /// please check the iyf::Configurable documentation
    void addListener(Configurable* listener);
    
    /// Removes an iyf:Configurable from the list of listeners
    void removeListener(Configurable* listener);
    
    /// Saves the configuration to the last file that was provided to the constructor.
    ///
    /// \remark This method is thread safe
    ///
    /// \warning This method writes to the file system and may block reading configuration values for quite some time,
    /// therefore, you should NOT call it mid-game.
    ///
    /// \throw std::logic_error if getMode() == Mode::ReadOnly.
    bool serialize();
    
    /// Creates a new ConfigurationEditor that can be used for thread safe configuration updates.
    ///
    /// \throw std::logic_error if getMode() == Mode::ReadOnly.
    std::unique_ptr<ConfigurationEditor> makeConfigurationEditor();
protected:
private:
    friend class ConfigurationEditor;
    
    void setChangedValues(const ConfigurationValueMap& changedValues, bool notify);
    void notifyChanged(const ConfigurationValueMap& changedValues);
    
    static void fillConfigurationMaps(const std::vector<ConfigurationFilePath>& paths, ConfigurationValueMap& resolvedValueMap,
                                      ConfigurationValueMap* systemValueMap, ConfigurationValueMap* userValueMap, FileSystem* filesystem,
                                      Mode mode);

    FileSystem* filesystem;
    const std::vector<ConfigurationFilePath> paths;
    
    /// A mutex that protects resolvedConfigurationValues
    mutable std::mutex configurationValueMutex;
    /// Final values that will be returned whenever getValue() is called.
    ConfigurationValueMap resolvedConfigurationValues;
    /// Contains configuration values from all files except the last one that was passed to the constructor.
    /// Used during a Configuration::serialize() call when when determining which values need to be saved to the user's
    /// configuration file and which don't.
    ConfigurationValueMap systemValues;
    /// Contains configuration values from the last file that was passed to the constructor. This map is updated whenever
    /// resolvedConfigurationValues are updated because all newly set configuration values are assumed to be tied to the users
    /// or their system. The values contained in this map are used during a Configuration::serialize() call when when determining 
    /// which ones need to be saved to the user's configuration file and which don't.
    ConfigurationValueMap userValues;
    
    std::vector<Configurable*> listeners;
    
    Mode mode;
};

/// Allows to update groups of configuration values.
///
/// \remark The methods inside this class are NOT thread safe
class ConfigurationEditor : private NonCopyable {
public:
    /// Executes a specified file and fills the internal map of changed settings with values that were stored in it.
    ///
    /// As the name implies, this should be used for graphics quality presets.
    ///
    /// \param [in] presetFilePath A path to a preset file that needs to be loaded
    void setToPreset(const ConfigurationFilePath& presetFilePath);
    
    /// Updates or inserts a specified value into the Configuration
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \param [in] name Name of the parameter
    /// \param [in] family The family of configuration values that you want to insert into
    /// \param [in] value A value to insert
    template <typename T>
    inline void setValue(const std::string& name, ConfigurationValueFamily family, T value) {
        pendingUpdate = true;
        const ConfigurationValueHandle handle(HS(name.c_str()), family);
        updatedValues[handle] = ConfigurationValue(value, name);
    }
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found in the maps
    ///
    /// \param [in] handle A pre-built ConfigurationValueHandle
    ConfigurationValue getValue(const ConfigurationValueHandle& handle) const;
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found in the maps
    ///
    /// \param [in] nameHash A pre-hashed name
    /// \param [in] family The family of configuration values that you want to querying
    inline ConfigurationValue getValue(StringHash nameHash, ConfigurationValueFamily family) const {
        return getValue(ConfigurationValueHandle(nameHash, family));
    }
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and family wasn't found in the maps
    ///
    /// \param [in] name Name of the parameter
    /// \param [in] family The family of configuration values that you want to querying
    inline ConfigurationValue getValue(const std::string& name, ConfigurationValueFamily family) const {
        return getValue(HS(name.c_str()), family);
    }
    
    /// This function saves the changes to the iyf::Configuration that created this editor. Once done, it will automatically clear
    /// the map of updated values and prepare this instance for reuse.
    ///
    /// \warning When many values are updated, this method may block reading from iyf::Configuration for a while. Multiple 
    /// iyf::Configurable listeners can also slow things down. Therefore, be careful when calling this function mid-game.
    ///
    /// \param [in] notify Should a notification be sent or not. If you updated a rarely queried value or something that needs a
    /// restart to change, you may not need to report anything to the listeners.
    void commit(bool notify = true);
    
    /// Clears the internal map of updated values and resets this instance for reuse.
    ///
    /// \warning This will NOT undo changes 
    void rollback();
    
    inline bool isUpdatePending() const {
        return pendingUpdate;
    }
    
    ~ConfigurationEditor();
private:
    friend class Configuration;
    ConfigurationEditor(Configuration* configuration) : configuration(configuration), pendingUpdate(false) {}
    
    Configuration* configuration;
    ConfigurationValueMap updatedValues;
    
    bool pendingUpdate;
};

}
#endif // CONFIGURATION_H
