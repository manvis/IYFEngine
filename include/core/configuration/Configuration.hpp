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

#ifndef IYF_CONFIGURATION_HPP
#define IYF_CONFIGURATION_HPP

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

/// A list of default configuration value namespaces
///
/// \warning Update GetConfigurationValueNamespaceName() and GetConfigurationValueNamespaceNameHash() when updating this.
enum class ConfigurationValueNamespace : std::uint8_t {
    /// Low level Engine settings that should only be changed by engine developers or maintainers. Careless adjustment may
    /// cause undefined behaviour or errors.
    Core = 0,
    /// Higher level Engine settings that game developers may use to better adapt the Engine to their specific usecases.
    /// Debug options go here as well
    Engine = 1,
    /// This namespace contains editor specific configuration that should not change the games in any way.
    Editor = 2,
    /// This namespace should contain screen size, visual quality settings, fov and similar values that relate to rendering.
    Graphics = 3,
    /// This namespace should contain sound, music, voice and microphone settings.
    Sound = 4,
    /// This namespace should contain keyboard, controller and mouse bindings, mouse sensitivity and etc.
    Controls = 5,
    /// This namespace should contain various localization and language options
    Localization = 6,
    /// This namespace should contain gameplay settings common to all playthroughs (e.g., should interactive objects be highlighted
    /// or not). Things that depend on a specific playthough (e.g., difficulty) should go into savegames.
    Gameplay = 7,
    /// This namespace should contain project settings
    Project = 8,
    /// This namespace should contain configuration values that do not belong to any other namespace
    Other = 9,
    COUNT
};

/// A key used for lookups in the iyf::ConfigurationValueMap 
struct ConfigurationValueHandle {
    constexpr ConfigurationValueHandle(StringHash nameHash, StringHash namespaceHash) : nameHash(nameHash), namespaceHash(namespaceHash) {}
    ConfigurationValueHandle(StringHash nameHash, ConfigurationValueNamespace namespaceID);
    
    const StringHash nameHash;
    const StringHash namespaceHash;
    
    friend bool operator==(const ConfigurationValueHandle& a, const ConfigurationValueHandle& b) {
        return (a.nameHash == b.nameHash) && (a.namespaceHash == b.namespaceHash);
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
            iyf::util::HashCombine(seed, k.namespaceHash.value());
            return seed;
        }
    };
}

namespace iyf {

namespace con {
const std::string& GetConfigurationValueNamespaceName(ConfigurationValueNamespace namespaceID);
const StringHash GetConfigurationValueNamespaceNameHash(ConfigurationValueNamespace namespaceID);
}

/// \warning the order must match iyf::ConfigurationVariant
enum class ConfigurationValueType : std::uint8_t {
    Double  = 0,
    Int64   = 1,
    Boolean = 2,
    String  = 3
};

/// \warning the order must match iyf::ConfigurationValueType
using ConfigurationVariant = std::variant<double, std::int64_t, bool, std::string>;

class Configuration;
class Configurable;
class ConfigurationManifest;

class ConfigurationValue {
public:
    ConfigurationValue() = default;
    
    template <typename T>
    inline ConfigurationValue(T value, std::string name, std::string namespaceName)
        : variant(value), name(std::move(name)), namespaceName(std::move(namespaceName)) { }
        
    template <typename T>
    inline ConfigurationValue(T value, std::string_view name, std::string_view namespaceName)
        : variant(value), name(name.data(), name.length()), namespaceName(namespaceName.data(), namespaceName.length()) { }
    
    
    /// \return Real, non-hashed name of this value
    inline const std::string& getName() const {
        return name;
    }
    
    inline const std::string& getNamespaceName() const {
        return namespaceName;
    }
    
    inline ConfigurationValueType getType() const {
        return static_cast<ConfigurationValueType>(variant.index());
    }
    
    // We don't need any additional type checks since variant performs static_asserts.
    template <typename T>
    inline operator T() const {
        return std::get<T>(variant);
    }
    
    inline const ConfigurationVariant& getValue() const {
        return variant;
    }
    
    friend bool operator==(const ConfigurationValue& a, const ConfigurationValue& b);
    friend bool operator!=(const ConfigurationValue& a, const ConfigurationValue& b);
private:
    ConfigurationVariant variant;
    std::string name;
    std::string namespaceName;
};

inline bool operator==(const ConfigurationValue& a, const ConfigurationValue& b) {
    return (a.name == b.name) && (a.namespaceName == b.namespaceName) && (a.variant == b.variant);
}

inline bool operator!=(const ConfigurationValue& a, const ConfigurationValue& b) {
    return (a.name != b.name) && (a.namespaceName != b.namespaceName) && (a.variant != b.variant);
}

template <>
inline ConfigurationValue::operator std::int32_t() const {
    std::int64_t value = std::get<std::int64_t>(variant);
    return static_cast<std::int32_t>(value);
}

template <>
inline ConfigurationValue::operator std::int64_t() const {
    std::int64_t value = std::get<std::int64_t>(variant);
    return static_cast<std::int64_t>(value);
}

struct ConfigurationValueMap {
    std::unordered_map<ConfigurationValueHandle, ConfigurationValue> data;
};

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

class NonConfigLine {
public:
    NonConfigLine() {}
    NonConfigLine(std::string line) : line(std::move(line)) {}
    
    inline const std::string& getLine() const {
        return line;
    }
private:
    std::string line;
};

enum class ConfigurationFileError {
    Correct,
    NonUnixLineEndings,
    InvalidLineSyntax,
    NoIdentifier,
    NoName,
    NamespaceNotAlphanumericASCII,
    NameNotAlphanumericASCII,
    NoValue,
    InvalidStringParameter,
    InvalidNumericParameter,
    UnknownError
};

class ConfigurationFile {
public:
    class ParseResult {
    public:
        ParseResult() : lineCount(0) {}
        
        std::size_t lineCount;
        std::vector<std::pair<std::size_t, ConfigurationFileError>> linesWithErrors;
        
        inline bool hasErrors() const {
            return !linesWithErrors.empty();
        }
        
        std::string printErrors() const;
    };
    
    /// Creates a new empty ConfigurationFile
    ConfigurationFile() {}
    
    /// Parses a configuration file that was loaded to a string
    ParseResult parse(const std::string& fileContents);
    
    using ConfigurationFileLine = std::variant<NonConfigLine, ConfigurationValue>;
    
    const std::vector<ConfigurationFileLine>& getLines() const {
        return lines;
    }
private:
    struct ParsedLine {
        ParsedLine(ConfigurationFileError error) : ParsedLine(NonConfigLine(), error) {}
        ParsedLine(ConfigurationFileLine line, ConfigurationFileError error)
            : line(line), error(error) {}
        
        ConfigurationFileLine line;
        ConfigurationFileError error;
    };
    
    enum class NumberParseResult {
        Int,
        Double,
        ParseFailed
    };
        
    ParsedLine processLine(const std::string_view& fileContents);
    NumberParseResult parseNumber(const std::string_view& str, std::int64_t& intVal, double& doubleVal);
    std::vector<ConfigurationFileLine> lines;
};

class Configuration : private NonCopyable {
public:
    enum class Mode {
        Editable,
        ReadOnly
    };
    
    /// Creates a new iyf::Configuration instance by reading all specified configuration files. Their order is very
    /// important because configuration values specified later will automatically override the earlier ones.
    ///
    /// If the mode is Mode::Editable, the last path in the list is assumed to point to the user's configuration file 
    /// that will be updated whenever Configuration::serialize() is called. Therefore, ensure that it is writable.
    /// An empty configuration file will be created automatically if it does not exist.
    ///
    /// \throws ConfigurationLoadError if the vector of paths was empty, contained non-existing files or the config 
    /// files had errors.
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
    /// \throws ConfigurationValueReadError If a value wasn't found.
    ///
    /// \param[in] handle A pre-built ConfigurationValueHandle
    inline ConfigurationValue getValue(const ConfigurationValueHandle& handle) const {
        std::lock_guard<std::mutex> lock(configurationValueMutex);
        
        auto result = resolvedConfigurationValues.data.find(handle);
        
        if (result == resolvedConfigurationValues.data.end()) {
            throw ConfigurationValueReadError("Unknown configuration value with hash: " + std::to_string(handle.nameHash));
        }
        
        return result->second;
    }

    /// Get the iyf::ConfigurationValue using a pre-hashed name and namespace.
    ///
    /// \remark This function is thread safe.
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and namespace wasn't found.
    ///
    /// \param[in] nameHash A pre-hashed name
    /// \param[in] namespaceNameHash A pre-hashed namespace name
    inline ConfigurationValue getValue(StringHash nameHash, StringHash namespaceNameHash) const {
        ConfigurationValueHandle handle(nameHash, namespaceNameHash);
        return getValue(handle);
    }
    
    /// Get the iyf::ConfigurationValue using an std::string name name and namespace.
    ///
    /// \remark This function is thread safe.
    ///
    /// \throws ConfigurationValueReadError If a value with specified name and namespace wasn't found.
    ///
    /// \param[in] name Name of the parameter
    /// \param[in] namespace The name of the configuration file namespace
    inline ConfigurationValue getValue(const std::string& name, const std::string& namespaceName) const {
        return getValue(HS(name.c_str()), HS(namespaceName.c_str()));
    }
    
    inline ConfigurationValue getValue(const std::string& name, ConfigurationValueNamespace namespaceID) const {
        return getValue(HS(name.c_str()), con::GetConfigurationValueNamespaceNameHash(namespaceID));
    }
    
    inline ConfigurationValue getValue(StringHash nameHash, ConfigurationValueNamespace namespaceID) const {
        return getValue(ConfigurationValueHandle(nameHash, con::GetConfigurationValueNamespaceNameHash(namespaceID)));
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
    
    static std::string loadFile(const ConfigurationFilePath& path);
    static void fillConfigurationMaps(const std::vector<ConfigurationFilePath>& paths, ConfigurationValueMap& resolvedValueMap,
                                      ConfigurationValueMap* systemValueMap, ConfigurationValueMap* userValueMap, ConfigurationFile* userConfigFile,
                                      FileSystem* filesystem, Mode mode);
    static void fillMapsFromFiles(const std::vector<ConfigurationFile>& files, const std::vector<ConfigurationValueMap*>& maps);

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
    ConfigurationFile userConfigFile;
    
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
    /// \param [in] namespaceName The name of the namespace
    /// \param [in] value A value to insert
    template <typename T>
    inline void setValue(const std::string& name, const std::string& namespaceName, T value) {
        pendingUpdate = true;
        const ConfigurationValueHandle handle(HS(name.c_str()), HS(namespaceName.c_str()));
        updatedValues.data[handle] = ConfigurationValue(value, name, namespaceName);
    }
    
    
    template <typename T>
    inline void setValue(const std::string& name, ConfigurationValueNamespace namespaceID, T value) {
        pendingUpdate = true;
        const ConfigurationValueHandle handle(HS(name.c_str()), con::GetConfigurationValueNamespaceNameHash(namespaceID));
        updatedValues.data[handle] = ConfigurationValue(value, name, con::GetConfigurationValueNamespaceName(namespaceID));
    }
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value wasn't found in the maps
    ///
    /// \param [in] handle A pre-built ConfigurationValueHandle
    ConfigurationValue getValue(const ConfigurationValueHandle& handle) const;
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value wasn't found in the maps
    ///
    /// \param [in] nameHash A pre-hashed name
    /// \param [in] namespaceName A pre-hashed namespace name
    inline ConfigurationValue getValue(StringHash nameHash, StringHash namespaceName) const {
        return getValue(ConfigurationValueHandle(nameHash, namespaceName));
    }
    
    inline ConfigurationValue getValue(StringHash nameHash, ConfigurationValueNamespace namespaceID) const {
        return getValue(ConfigurationValueHandle(nameHash, con::GetConfigurationValueNamespaceNameHash(namespaceID)));
    }
    
    inline ConfigurationValue getValue(const std::string& name, ConfigurationValueNamespace namespaceID) const {
        return getValue(HS(name.c_str()), con::GetConfigurationValueNamespaceNameHash(namespaceID));
    }
    
    /// This function will check the internal map for updated but not yet commited values. If no values are found there,
    /// it will call Configuration::getValue().
    ///
    /// \remark This method is NOT thread safe
    ///
    /// \throws ConfigurationValueReadError If a value wasn't found in the maps
    ///
    /// \param [in] name Name of the parameter
    /// \param [in] namespaceName Name of the namespace
    inline ConfigurationValue getValue(const std::string& name, const std::string& namespaceName) const {
        return getValue(HS(name.c_str()), HS(namespaceName.c_str()));
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
#endif // IYF_CONFIGURATION_HPP
