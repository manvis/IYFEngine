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

#ifndef IYF_ASSET_CONSTANTS_HPP
#define IYF_ASSET_CONSTANTS_HPP

#include "core/filesystem/cppFilesystem.hpp"

namespace iyf {
enum class AssetsToEnableResult {
    HasAssetsToEnable, /// < The TypeManager has assets that are ready to be enabled
    NoAssetsToEnable, /// < The TypeManager doesn't have any assets that need to be enabled
    Busy /// < The TypeManager has assets that need to be enabled, but can't do it. E.g., a GPU upload buffer for this frame has been filled.
};

/// \brief Identifiers for different asset types.
///
/// \warning AssetType::ANY and AssetType::COUNT MUST NEVER BE USED IN THE ASSET DATABASE. Even if you add a custom asset type and then
/// decide to stop using it, keep it in the enum to avoid moving AssetType::ANY and breaking your Project
///
/// \warning When updating or changing this:
///   - update AssetTypeToPath;
///   - update AssetTypeToTranslationString;
///   - update the names of asset specific directories;
///   - update the extensions
///   - update the importers.
///   - update the Metadata variant
///
/// \warning Updating these values may break existing projects.
enum class AssetType : std::uint8_t {
    Animation = 0,
    Mesh = 1,
    Texture = 2,
    Font = 3,
    Audio = 4,
    Video = 5,
    Script = 6,
    Shader = 7,
    Strings = 8,
    Custom = 9,
    MaterialTemplate = 10,
    COUNT,
    /// Any is equal to COUNT
    ANY = COUNT
};

namespace con {
// -----------------------------------------------------------------------------
// Directories
// -----------------------------------------------------------------------------
/// \brief Base path of all assets used by the game
const fs::path& BaseAssetPath();

/// \brief Path to system strings used by the editor and tools
const fs::path& SystemStringPath();

/// \brief Path to imports
const fs::path& ImportPath();

/// \brief Default paths for each AssetType. AssetType::Any and AssetType::COUNT return empty strings they are helper values.
const fs::path& AssetTypeToPath(AssetType type);

/// \brief Default translation strings for each AssetType. Usage: submit these to LOC macro and get the human readable string in current language
const std::string& AssetTypeToTranslationString(AssetType type);

// -----------------------------------------------------------------------------
// File extensions
// -----------------------------------------------------------------------------
/// \brief Extension to use for all material files
const std::string& MaterialInstanceFormatExtension();

/// \brief Extension to use for all material template files
const std::string& MaterialTemplateFormatExtension();

/// \brief Extension to use for engine's project files
const std::string& ProjectFileExtension();

/// \brief Extension to use for engine's binary metadata files
const std::string& MetadataExtension();

/// \brief Extension to use for engine's text (JSON) metadata files
const std::string& TextMetadataExtension();

/// \brief Extension to use for asset import settings
const std::string& ImportSettingsExtension();

/// \brief The extension used by asset file packages
const std::string& PackFileExtension();

/// \brief The extension used by files that store the world data
const std::string& WorldFileExtension();

// -----------------------------------------------------------------------------
// Special files 
// -----------------------------------------------------------------------------
/// \brief Default font file to use for ImGui rendering. This file should exist in FontPath
const std::string& ImGuiFont();

/// \brief Name of the database file containing localization data. This file should exist in StringPath
const std::string& LocalizationDatabase();

/// \brief Default texture to use when one is missing.
const fs::path& MissingTexture();

/// \brief Default mesh to use when one is missing.
const fs::path& MissingMesh();

const std::string& EngineBaseConfigFile();

/// \brief the name of a file that contains core project settings. This file should exist in the root dir 
/// of all projects.
const std::string& ProjectFile();

/// \brief The name of the default world file that is stored in the system assets.
const std::string& DefaultWorldFile();

const std::string& SystemAssetPackName();
const std::string& DefaultReleasePackName();

}

}

#endif // IYF_ASSET_CONSTANTS_HPP
