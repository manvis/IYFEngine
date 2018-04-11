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

#ifndef IYF_PROJECT_HPP
#define IYF_PROJECT_HPP

#include "core/configuration/Configuration.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "core/interfaces/TextSerializable.hpp"
#include "../VERSION.hpp"

#include <memory>
#include <functional>

namespace iyf {

/// \warning This class requires game name and company name strings. 
class Project : public TextSerializable {
public:
    enum class CreationResult {
        EmptyPath,
        EmptyName,
        NotADirectory,
        NonEmptyDirectory,
        FolderCreationFailed,
        ProjectFileCreationFailed,
        CreatedSuccessfully
    };
    
    /// Creates a new project directory in newProjectPath/projectName, all required sub-directories and all files required
    /// for the project to function 
    ///
    /// \param[in] newProjectPath this is the directory where the project folder will be created
    /// \param[in] projectName this is the name of the project and the folder that will be created in the newProjectPath
    /// \param[in] baseLocale the default project locale. Use the language_COUNTRY format, e.g., "en_US", "lt_LT",
    /// etc. If the game is available in multiple languages, the language file corresponding to the base locale will be
    /// the one that defines all strings that need to be translated into other languages.
    /// \param[in] createDirectories should this function create the dirctory tree that the project expects, or not. If not, only
    /// the project file will be created
    /// \return new project object that can be used by EditorState or classed deriving from it
    static CreationResult Create(const fs::path& newProjectPath, const std::string& projectName, std::function<void(const std::string&)> callback, 
                                 const std::string& baseLocale = "en_US", bool createDirectories = true);
    
    static bool CreateImportedAssetDirectories(const fs::path& path);
    static bool CreateImportsDirectory(const fs::path& path);
    
    /// Sets the project root to the specified path and automatically deserializes (by calling deserialize()) the project configuration
    /// file from there. This must be a real file system path.
    Project(fs::path root);
    
    ~Project();
    
    /// Serializes the Project data to the project file that resides in the root path.
    bool serialize() const;
    
    /// deserialize the Project data from the project file that resides in the root path.
    bool deserialize();
    
    /// Serializes the Project data to a PrettyStringWriter. You're probably looking for serialize() that will
    /// automatically write the contents of this object to the project file that resides in the root path.
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    
    /// Deserializes the Project data from a JSONObject. You're probably looking for deserialize() that will
    /// automatically deserialize the project file that resides in the root path.
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    const fs::path& getRootPath() const {
        return root;
    }
    
    /// Sets the name of the first World that will be loaded when the game loads for the first time.
    ///
    /// \warning This function does not save the value to the configuration file. Use serializeJSON() to do so.
    void setFirstWorldName(std::string name);
    
    /// Returns the name of the first World that will be loaded when the game loads for the first time.
    std::string getFirstWorldName() const;

    /// Sets the name of the company that made this game.
    ///
    /// \remark This string is used internally (e.g. to create savegame and preference directories, such as C:/Users/Username/Documents/SomeCompany/SomeGame) 
    /// and it is never shown directly to the user. User facing company name strings are stored in the localization database.
    ///
    /// \warning This string MUST be UTF-8 encoded, preferably short and SHOULD only contain letters, numbers and spaces. Avoid punctuation because some
    /// operating systems may think it has a special meaning and mess up the savegame folder creation.
    ///
    /// \warning Changing this string between game updates will make the end users LOSE all previously saved games and preferences
    ///
    /// \warning This function does not save the value to the configuration file. Use serializeJSON() to do so.
    void setCompanyName(std::string name);
    
    /// Returns the name of the company that made this game.
    ///
    /// \remark This string is used internally (e.g. to create savegame and preference directories, such as C:/Users/Username/Documents/SomeCompany/SomeGame) 
    /// and it is never shown directly to the user. User facing company name strings are stored in the localization database.
    std::string getCompanyName() const;
    
    /// Sets the name of the game.
    ///
    /// \remark This string is used internally (e.g. to create savegame and preference directories, such as C:/Users/Username/Documents/SomeCompany/SomeGame) 
    /// and it is never shown directly to the user. User facing company name strings are stored in the localization database.
    ///
    /// \warning This string MUST be UTF-8 encoded, preferably short and SHOULD only contain letters, numbers and spaces. Avoid punctuation because some
    /// operating systems may think it has a special meaning and mess up the savegame folder creation.
    ///
    /// \warning Changing this string between game updates will make the end users LOSE all previously saved games and preferences
    ///
    /// \warning This function does not save the value to the configuration file. Use serializeJSON() to do so.
    void setGameName(std::string name);
    
    /// Returns the name of the game.
    ///
    /// \remark This string is used internally (e.g. to create savegame and preference directories, such as C:/Users/Username/Documents/SomeCompany/SomeGame) 
    /// and it is never shown directly to the user. User facing company name strings are stored in the localization database.
    std::string getGameName() const;
    
    /// Sets the current version.
    ///
    /// Used when packing assets for release, saving games, etc.
    ///
    /// \warning This function does not save the value to the configuration file. Use serialize() to do so.
    void setVersion(const Version& version);
    
    /// Returns the current version
    ///
    /// Used when packing assets for release, saving games, etc.
    Version getVersion() const;
    
    /// Returns the base locale in the language_COUNTRY format (e.g., "en_US", "lt_LT", etc.).
    ///
    /// Used in the localization system to determine the string file that contains the base list of strings that can
    /// be translated.
    std::string getBaseLocale() const;
    
    /// Returns the base locale. Must be in the language_COUNTRY format (e.g., "en_US", "lt_LT", etc.).
    ///
    /// Used in the localization system to determine the string file that contains the base list of strings that can
    /// be translated.
    void setBaseLocale(std::string locale);
private:
    fs::path root;
    
    std::string companyName;
    std::string gameName;
    std::string firstWorldName;
    std::string baseLocale;
    Version version;
};
}

#endif // IYF_PROJECT_HPP
