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

#include "core/Constants.hpp"

#include <algorithm>

namespace iyf {
namespace con {

const fs::path AssetPath = u8"assets";
const fs::path SystemAssetPath = AssetPath / u8"system";
const fs::path ImportPath = u8"imports";
const fs::path AnimationPath = AssetPath / u8"animations";
const fs::path MeshPath = AssetPath / u8"meshes";
const fs::path TexturePath = AssetPath / u8"textures";
const fs::path FontPath = AssetPath / u8"fonts";
const fs::path AudioPath = AssetPath / u8"audio";
const fs::path VideoPath = AssetPath / u8"video";
const fs::path WorldPath = AssetPath / u8"worlds";
const fs::path ScriptPath = AssetPath / u8"scripts";
const fs::path ShaderPath = AssetPath / u8"shaders";
const fs::path PipelinePath = AssetPath / u8"pipelines";
const fs::path StringPath = AssetPath / u8"strings";
const fs::path CustomPath = AssetPath / u8"custom";
const fs::path MaterialPath = AssetPath / u8"materials";

const std::string AnimationFormatExtension = u8".iyfa";
const std::string MeshFormatExtension = u8".iyfm";
const std::string ProjectFileExtension = u8".iyfp";
const std::string MetadataExtension = u8".iyfd";
const std::string TextMetadataExtension = u8".iyfjd";
const std::string ImportSettingsExtension = u8".iyfi";
const std::string PackFileExtension = u8".iyfpak";
const std::string WorldFileExtension = u8".iyfw";

const std::string SystemAssetPackName = u8"system" + PackFileExtension;
const std::string DefaultReleasePackName = u8"assets" + PackFileExtension;

const std::string DefaultWorldFile = u8"DefaultEmptyWorld" + WorldFileExtension;
const std::string ProjectFile = u8"Project" + ProjectFileExtension;
const std::string EngineBaseConfigFile = u8"EngineBaseConfig.lua";

static const std::array<fs::path, static_cast<std::size_t>(AssetType::COUNT)> AssetTypeToPathMap = {
    AnimationPath, // 0
    MeshPath,      // 1
    TexturePath,   // 2
    FontPath,      // 3
    AudioPath,     // 4
    VideoPath,     // 5
    WorldPath,     // 6
    ScriptPath,    // 7
    ShaderPath,    // 8
    PipelinePath,  // 9
    StringPath,    // 10
    CustomPath,    // 11
    MaterialPath,  // 12
};

const fs::path& AssetTypeToPath(AssetType type) {
    return AssetTypeToPathMap[static_cast<std::size_t>(type)];
}

static const std::array<std::string, static_cast<std::size_t>(AssetType::COUNT) + 1> AssetTypeToTranslationStringMap = {
    "animation", // 0
    "mesh",      // 1
    "texture",   // 2
    "font",      // 3
    "audio",     // 4
    "video",     // 5
    "world",     // 6
    "script",    // 7
    "shader",    // 8
    "pipeline",  // 9
    "string",    // 10
    "custom",    // 11
    "material",  // 12
    "any",       // COUNT/ANY
};

const std::string& AssetTypeToTranslationString(AssetType type) {
    return AssetTypeToTranslationStringMap[static_cast<std::size_t>(type)];
}

inline std::size_t CalculateMipmapLevelSizeHalfBytePerPixel(std::size_t width, std::size_t height) {
    const std::size_t blockSize = 8;
    const std::size_t size = (width * height) / 2;
    
    // Even if only a single pixel is required (e.g., the last level of the mipmap), an entire
    // block has to be written.
    return std::max(size, blockSize);
}

inline std::size_t CalculateMipmapLevelSizeBytePerPixel(std::size_t width, std::size_t height) {
    const std::size_t blockSize = 16;
    const std::size_t size = width * height;
    
    // Even if only a single pixel is required (e.g., the last level of the mipmap), an entire
    // block has to be written.
    return std::max(size, blockSize);
}

std::size_t CompressedTextureMipmapLevelSize(TextureCompressionFormat format, std::size_t width, std::size_t height) {
    // Based on Compressonator/Source/Codec CalcBufferSize
    switch (format) {
        case TextureCompressionFormat::NotCompressed:
            return 0;
        case TextureCompressionFormat::BC1:
        case TextureCompressionFormat::BC4:
        case TextureCompressionFormat::ETC1:
        case TextureCompressionFormat::ETC2:
            return CalculateMipmapLevelSizeHalfBytePerPixel(width, height);
        case TextureCompressionFormat::BC2:
        case TextureCompressionFormat::BC3:
        case TextureCompressionFormat::BC5:
        case TextureCompressionFormat::BC6:
        case TextureCompressionFormat::BC7:
            return CalculateMipmapLevelSizeBytePerPixel(width, height);
        case TextureCompressionFormat::COUNT:
            throw std::runtime_error("COUNT is not a valid TextureCompressionFormat value");
    }
}

}
}
