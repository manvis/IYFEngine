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

#ifndef IYF_CONSTANTS_HPP
#define IYF_CONSTANTS_HPP

#include <string>
#include <array>

#include "core/filesystem/cppFilesystem.hpp"

#undef None

/// \file
/// This file stores constants commonly used by many of the engine's classes
namespace iyf {
/// \warning Updating these values may break existing projects.
enum class ShaderPurpose : std::uint8_t {
    System = 0,
    UI = 1,
    Skybox = 2,
    Particles = 3,
    Surface = 4,
    Compute = 5,
    COUNT
};

/// \warning Updating these values may break existing projects.
enum class TextureFilteringMethod : std::uint8_t {
    None = 0,
    Bilinear = 1,
    Trilinear = 2
};

/// \warning Updating these values may break existing projects.
enum class TextureTilingMethod : std::uint8_t {
    Clamp = 0,
    Repeat = 1,
    MirroredRepeat = 2
};

/// \warning Updating these values may break existing projects.
enum class TextureCompressionFormat : std::uint16_t {
    NotCompressed = 0,
    // Desktop, BC
    BC1 = 1,
    BC2 = 2,
    BC3 = 3,
    BC4 = 4,
    BC5 = 5,
    BC6 = 6,
    BC7 = 7,
    // Android, ETC
    ETC1 = 1000,
    ETC2 = 1001,
    // ...
    COUNT
};

enum class CompressionFormatFamily : std::uint8_t {
    BC = 0,
    ETC = 1,
};

/// \warning Updating these values may break existing projects.
enum class TextureImportMode : std::uint8_t {
    Regular = 0,
    NormalMap = 1,
    HighQuality = 2,
    HDR = 3,
    SingleChannel = 4,
    COUNT
};

namespace con {
// -----------------------------------------------------------------------------
// Default file extensions 
// -----------------------------------------------------------------------------

/// \brief Extension to use for all material files
extern const std::string MaterialFormatExtension;

/// \brief Extension to use for engine's project files
extern const std::string ProjectFileExtension;

/// \brief Extension to use for engine's binary metadata files
extern const std::string MetadataExtension;

/// \brief Extension to use for engine's text (JSON) metadata files
extern const std::string TextMetadataExtension;

/// \brief Extension to use for asset import settings
extern const std::string ImportSettingsExtension;

/// \brief The extension used by asset file packages
extern const std::string PackFileExtension;

/// \brief The extension used by files that store the world data
extern const std::string WorldFileExtension;

// -----------------------------------------------------------------------------
// Default files 
// -----------------------------------------------------------------------------

/// \brief Default font file to use for ImGui rendering. This file should exist in FontPath
const std::string ImGuiFont = u8"Roboto-Regular.ttf";

/// \brief Name of the database file containing localization data. This file should exist in StringPath
const std::string LocalizationDatabase = u8"strings.db";

/// \brief Default texture to use when one is missing. This file should exist in TexturePath
const std::string MissingTexture = u8"MissingTexture.sys.ktx";

/// \brief Default mesh to use when one is missing. This file should exist in MeshPath
const std::string MissingMesh = u8"MissingMesh.sys.iyfm";

extern const std::string EngineBaseConfigFile;

/// \brief the name of a file that contains core project settings. This file should exist in the root dir 
/// of all projects.
extern const std::string ProjectFile;

/// \brief The name of the default world file that is stored in the system assets.
extern const std::string DefaultWorldFile;

extern const std::string SystemAssetPackName;
extern const std::string DefaultReleasePackName;

// -----------------------------------------------------------------------------
// Compressed texture data
// -----------------------------------------------------------------------------

/// \return the size of a mipmap level in bytes or 0 if TextureCompressionFormat::NotCompressed is provided
std::size_t CompressedTextureMipmapLevelSize(TextureCompressionFormat format, std::size_t width, std::size_t height);

// -----------------------------------------------------------------------------
// Editor and DB specific constants. DO NOT EDIT
// -----------------------------------------------------------------------------

const std::size_t MaxLevelNameLength = 32;
const std::size_t MaxEntityNameLength = 32;
const std::size_t MaxLevelDescriptionLength = 512;

enum class ColorDataTypeFlagBits : std::uint8_t {
    Color        = 0x1,
    Texture2D    = 0x2,
    TextureCube  = 0x4,
    TextureArray = 0x8,
};

enum class ColorChannelCountFlagBits : std::uint8_t {
    One   = 0x1,
    Two   = 0x2,
    Three = 0x4,
    Four  = 0x8
};

// -----------------------------------------------------------------------------
// Rendering constants
//
// These are high level and are used by World instances, mesh and texture loaders (and converters), etc.
// For Graphics API constants check "graphics/GraphicsAPIConstants.hpp"
// -----------------------------------------------------------------------------

/// \brief How big can a single vertex be.
/// \warning If you want to make this bigger, you will need to adjust the type of the Mesh::padding variable in Mesh class as well
const std::size_t MaxVertexSize = 255;

/// \brief Maximum number of sub-meshes that a single mesh can have. Attachments are a separate thing. This represents mesh splits based on different materials.
/// \warning You should use attachments instead of submeshes. Moreover, simply inreasing this won't be enough, unless the value is less or equal to 255. A lot of places  
/// in the engine (MeshMetadata, MeshLoader, MeshConverter, etc.), use std::uint8_t to store submesh counts.
const std::size_t MaxSubMeshes = 32;

/// \brief Maximum number of animations that a single mesh can have.
const std::size_t MaxAnimations = 64;

/// \brief Maximum number of vertices that a single mesh (all of its sub-meshes) can have.
/// \warning Simply increasing this won't be enough. A lot of places in this engine use std::uint16_t for vertex counts. You'll still need to create a custom
/// binary data format capable of storing meshes with more than 65535 vertices, update the loader, change the data types in
/// MeshData and modify Material editing classes. It's possible that even more changes may be required.
const std::size_t MaxVerticesPerMesh = 65535;

/// \brief The maximum number of floating point inputs a single Material (or MaterialDefinition) can have.
///
/// E.g., if this is 16, the material may have up to 16 float inputs or four vec4 inputs or almost any other combination, 
/// for as long as the total number floats is less than MaxMaterialComponents.
///
/// \warning Some combinations may introduce padding and prevent some floats from being used.
const std::size_t MaxMaterialComponents = 16;

static_assert(MaxMaterialComponents % 4 == 0, "The number of components must be a multiple of 4");

/// The maximum number of texture inputs a material can have
const std::size_t MaxMaterialTextures = 8;

/// A 64bit bitset is used to track which floats are loaded from texture and which ones are loaded from the
/// material data buffers.
static_assert(MaxMaterialComponents <= 64, "Can't use more than 64 material components.");

// -----------------------------------------------------------------------------
// Configuration that only programmers should be able to edit
// -----------------------------------------------------------------------------

/// \brief How frequently (in milliseconds) the logic is updated. Graphics, on the other hand, are updated as frequently as possible
const unsigned int TicksMs = 32; //64; //16;

/// \brief Font size to use for ImGui rendering
const float ImGuiFontSize = 14.0f;

}
}


#endif // IYF_CONSTANTS_HPP

