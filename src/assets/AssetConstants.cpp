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

namespace iyf {
namespace con {
// -----------------------------------------------------------------------------
// Directories
// -----------------------------------------------------------------------------
// -------------------------------- WARNING ------------------------------------
// ---   CHANGING THE NAMES OF DIRECTORIES WILL BREAK ALL CURRENT PROJECTS   ---
// -------------------------------- WARNING ------------------------------------

const fs::path& BaseAssetPath() {
    static const fs::path path = u8"assets";
    return path;
}

const fs::path& SystemStringPath() {
    static const fs::path path = BaseAssetPath() / u8"systemStrings";
    return path;
}

/// \brief Path to imports
const fs::path& ImportPath() {
    static const fs::path path = u8"imports";
    return path;
}

const fs::path& AssetTypeToPath(AssetType type) {
    static const fs::path AnimationPath = BaseAssetPath() / u8"animations";
    static const fs::path MeshPath = BaseAssetPath() / u8"meshes";
    static const fs::path TexturePath = BaseAssetPath() / u8"textures";
    static const fs::path FontPath = BaseAssetPath() / u8"fonts";
    static const fs::path AudioPath = BaseAssetPath() / u8"audio";
    static const fs::path VideoPath = BaseAssetPath() / u8"video";
    static const fs::path ScriptPath = BaseAssetPath() / u8"scripts";
    static const fs::path ShaderPath = BaseAssetPath() / u8"shaders";
    static const fs::path StringPath = BaseAssetPath() / u8"strings";
    static const fs::path CustomPath = BaseAssetPath() / u8"custom";
    static const fs::path MaterialTemplatePath = BaseAssetPath() / u8"material_templates";
    static const fs::path MaterialInstancePath = BaseAssetPath() / u8"materials";
    
//     static const fs::path WorldPath = BaseAssetPath() / u8"worlds";

    static const std::array<fs::path, static_cast<std::size_t>(AssetType::COUNT)> AssetTypeToPathMap = {
        AnimationPath,        // 0
        MeshPath,             // 1
        TexturePath,          // 2
        FontPath,             // 3
        AudioPath,            // 4
        VideoPath,            // 5
        ScriptPath,           // 6
        ShaderPath,           // 7
        StringPath,           // 8
        CustomPath,           // 9
        MaterialTemplatePath, // 10
        MaterialInstancePath,  // 11
    };

    return AssetTypeToPathMap[static_cast<std::size_t>(type)];
}

static const std::array<std::string, static_cast<std::size_t>(AssetType::COUNT) + 1> AssetTypeToTranslationStringMap = {
    "animation",        // 0
    "mesh",             // 1
    "texture",          // 2
    "font",             // 3
    "audio",            // 4
    "video",            // 5
    "script",           // 6
    "shader",           // 7
    "string",           // 8
    "custom",           // 9
    "materialTemplate", // 10
    "materialInstance", // 11
    "any",       // COUNT/ANY
};

const std::string& AssetTypeToTranslationString(AssetType type) {
    return AssetTypeToTranslationStringMap[static_cast<std::size_t>(type)];
}

// -----------------------------------------------------------------------------
// File extensions
// -----------------------------------------------------------------------------
const std::string& MaterialInstanceFormatExtension() {
    static const std::string ext = u8".iyfm";
    return ext;
}

const std::string& MaterialTemplateFormatExtension() {
    static const std::string ext = u8".iyfmt";
    return ext;
}

const std::string& ProjectFileExtension() {
    static const std::string ext = u8".iyfp";
    return ext;
}

const std::string& MetadataExtension() {
    static const std::string ext = u8".iyfd";
    return ext;
}

const std::string& TextMetadataExtension() {
    static const std::string ext = u8".iyfjd";
    return ext;
}

const std::string& ImportSettingsExtension() {
    static const std::string ext = u8".iyfi";
    return ext;
}

const std::string& PackFileExtension() {
    static const std::string ext = u8".iyfpak";
    return ext;
}

const std::string& WorldFileExtension() {
    static const std::string ext = u8".iyfw";
    return ext;
}

// -----------------------------------------------------------------------------
// Special files 
// -----------------------------------------------------------------------------
const std::string& ImGuiFont() {
    static const std::string file = u8"Roboto-Regular.ttf";
    return file;
}

const std::string& LocalizationDatabase() {
    static const std::string file = u8"strings.db";
    return file;
}

const fs::path& SystemAssetSource() {
    static const fs::path path = u8"raw/system";
    return path;
}

const fs::path& MissingTexture() {
    static const fs::path file = SystemAssetSource() / u8"missingTexture.png";
    return file;
}

const fs::path& MissingMesh() {
    static const fs::path file = SystemAssetSource() / u8"missing.dae";
    return file;
}

const std::string& EngineBaseConfigFile() {
    static const std::string file = u8"EngineBaseConfig.cfg";
    return file;
}

const std::string& ProjectFile() {
    static const std::string file = u8"Project" + ProjectFileExtension();
    return file;
}

/// \brief The name of the default world file that is stored in the system assets.
const std::string& DefaultWorldFile() {
    static const std::string file = u8"DefaultEmptyWorld" + WorldFileExtension();
    return file;
}

const std::string& SystemAssetPackName() {
    static const std::string file = u8"system" + PackFileExtension();
    return file;
}

const std::string& DefaultReleasePackName() {
    static const std::string file = u8"assets" + PackFileExtension();
    return file;
}

}
}
