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
    COUNT,
    /// Any is equal to COUNT
    ANY = COUNT
};

namespace con {
// -----------------------------------------------------------------------------
// Directories
// -----------------------------------------------------------------------------

// -------------------------------- WARNING ------------------------------------
// ---   CHANGING THE NAMES OF DIRECTORIES WILL BREAK ALL CURRENT PROJECTS   ---
// -------------------------------- WARNING ------------------------------------
    
/// The base asset directory.
extern const fs::path AssetPath;

/// Directory where all system assets, common to all games, are stored
extern const fs::path SystemAssetPath;

/// Location where the Engine expects to find all files for import.
extern const fs::path ImportPath;

/// Location where the Engine expects to find all 3D mesh animations.
/// Corresponds to AssetType::Animation
extern const fs::path AnimationPath;

/// Location where the Engine expects to find all 3D meshes.
/// Corresponds to AssetType::Mesh
extern const fs::path MeshPath;

/// Location where the Engine expects to find all textures, skyboxes, etc.
/// Corresponds to AssetType::Texture
extern const fs::path TexturePath;

/// Location where the Engine expects to find all font files.
/// Corresponds to AssetType::Font
extern const fs::path FontPath;

/// Location where the Engine expects to find all music and sound effect files.
/// Corresponds to AssetType::Audio
extern const fs::path AudioPath;

/// Location where the Engine expects to find all video files.
/// Corresponds to AssetType::Video
extern const fs::path VideoPath;

/// Location where the Engine expects to find all worlds
/// Corresponds to AssetType::World
extern const fs::path WorldPath;

/// Location where the Engine expects to find all scripts
/// Corresponds to AssetType::Script
extern const fs::path ScriptPath;

/// Location where the Engine expects to find all shaders.
/// Corresponds to AssetType::Shader
extern const fs::path ShaderPath;

/// Location where the Engine expects to find all pipeline definitions.
/// Corresponds to AssetType::Pipeline
extern const fs::path PipelinePath;

/// Location where the Engine expects to find localized strings for the game.
/// Corresponds to AssetType::Strings
extern const fs::path StringPath;

/// Location where the Engine expects to find all files that don't fit any other category 
/// (e.g., custom assets for custom game specific systems should go there).
/// Corresponds to AssetType::Custom
extern const fs::path CustomPath;

/// Location where the Engine expects to find material deifinitions
/// Corresponds to AssetType::Material
extern const fs::path MaterialPath;

/// Location where the Engine expects to find localized strings for the editor, tools
/// and internal systems
extern const fs::path SystemStringPath;

// -----------------------------------------------------------------------------
// Asset data mappings
// -----------------------------------------------------------------------------
/// \brief Default paths for each AssetType. AssetType::Any and AssetType::COUNT return empty strings they are helper values.
const fs::path& AssetTypeToPath(AssetType type);

/// \brief Default translation strings for each AssetType. Usage: submit these to LOC macro and get the human readable string in current language
const std::string& AssetTypeToTranslationString(AssetType type);
}

}

#endif // IYF_ASSET_CONSTANTS_HPP
