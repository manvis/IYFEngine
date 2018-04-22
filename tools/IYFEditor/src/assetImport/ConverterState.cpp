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

#include "assetImport/ConverterState.hpp"

namespace iyf::editor {
static const char* CONTENTS_FIELD_NAME = "fileContents";
static const char* CONTENTS_FIELD_VALUE = "ImporterSettingsJSON";
static const char* VERSION_FIELD_NAME = "importerSettingsVersion";
static const char* TYPE_FIELD_NAME = "assetType";
static const char* SOURCE_HASH_FIELD_NAME = "sourceFileHash";
static const char* IS_SYSTEM_ASSET_FIELD_NAME = "isSystemAsset";
static const char* TAG_FIELD_NAME = "tags";

void ConverterState::serializeJSON(PrettyStringWriter& pw) const {
    if (!isConversionComplete()) {
        throw std::logic_error("Cannot serialize an incomplete ConverterState instance");
    }
    
    const std::uint64_t version = getLatestSerializedDataVersion();
    
    pw.String(CONTENTS_FIELD_NAME);
    pw.String(CONTENTS_FIELD_VALUE);
    
    pw.String(TYPE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(getType()));
    
    pw.String(VERSION_FIELD_NAME);
    pw.Uint64(version);
    
    pw.String(SOURCE_HASH_FIELD_NAME);
    pw.Uint64(sourceFileHash.value());
    
    if (systemAsset) {
        pw.String(IS_SYSTEM_ASSET_FIELD_NAME);
        pw.Bool(systemAsset);
    }
    
    pw.String(TAG_FIELD_NAME);
    pw.StartArray();
    for (const auto& tag : tags) {
        pw.String(tag.data(), tag.size(), true);
    }
    pw.EndArray();
    
    serializeJSONImpl(pw, version);
}

void ConverterState::deserializeJSON(JSONObject& jo) {
    assert(jo.HasMember(CONTENTS_FIELD_NAME) && std::strcmp(jo[CONTENTS_FIELD_NAME].GetString(), CONTENTS_FIELD_VALUE) == 0);
    
    if (!jo.HasMember(TYPE_FIELD_NAME)) {
        throw std::runtime_error("The provided JSON did not contain the asset type field");
    }
    
    if (!jo.HasMember(VERSION_FIELD_NAME)) {
        throw std::runtime_error("The provided JSON did not contain the version number field");
    }
    
    const std::uint64_t version = jo[VERSION_FIELD_NAME].GetInt64();
    const AssetType type = static_cast<AssetType>(jo[TYPE_FIELD_NAME].GetUint());
    
    if (type != getType()) {
        throw std::runtime_error("Tried to deserialize conversion settings of the wrong type");
    }
    
    if (jo.HasMember(IS_SYSTEM_ASSET_FIELD_NAME)) {
        systemAsset = jo[IS_SYSTEM_ASSET_FIELD_NAME].GetBool();
    }
    
    tags.clear();
    if (jo.HasMember(TAG_FIELD_NAME)) {
        for (const auto& tag : jo[TAG_FIELD_NAME].GetArray()) {
            tags.push_back(tag.GetString());
        }
    }
    
    deserializeJSONImpl(jo, version);
}

std::uint64_t TextureConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

static const char* PREMULTIPLY_ALPHA_FIELD_NAME = "premultiplyAlpha";
static const char* IS_SRGB_VALUE_FIELD_NAME = "sRGB";
static const char* NO_MIP_MAPS_FIELD_NAME = "noMipMaps";
static const char* IMPORT_MODE_FIELD_NAME = "importMode";
static const char* FILTER_FIELD_NAME = "filteringMethod";
static const char* TILE_X_FIELD_NAME = "xTiling";
static const char* TILE_Y_FIELD_NAME = "yTiling";
static const char* QUALITY_FIELD_NAME = "quality";
static const char* ANISOTROPY_FIELD_NAME = "preferredAnisotropy";

void TextureConverterState::serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const {
    assert(version == 1);
    
    pw.String(PREMULTIPLY_ALPHA_FIELD_NAME);
    pw.Bool(premultiplyAlpha);
    
    pw.String(IS_SRGB_VALUE_FIELD_NAME);
    pw.Bool(sRGBSource);
    
    pw.String(NO_MIP_MAPS_FIELD_NAME);
    pw.Bool(noMipMaps);
    
    pw.String(IMPORT_MODE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(importMode));
    
    pw.String(FILTER_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(filteringMethod));
    
    pw.String(TILE_X_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(xTiling));
    
    pw.String(TILE_Y_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(yTiling));
    
    pw.String(QUALITY_FIELD_NAME);
    pw.Double(quality);
    
    pw.String(ANISOTROPY_FIELD_NAME);
    pw.Uint(preferredAnisotropy);
}

void TextureConverterState::deserializeJSONImpl(JSONObject& jo, std::uint64_t version) {
    assert(version == 1);
    
    premultiplyAlpha = jo[PREMULTIPLY_ALPHA_FIELD_NAME].GetBool();
    sRGBSource = jo[IS_SRGB_VALUE_FIELD_NAME].GetBool();
    noMipMaps = jo[NO_MIP_MAPS_FIELD_NAME].GetBool();
    importMode = static_cast<TextureImportMode>(jo[IMPORT_MODE_FIELD_NAME].GetUint());
    filteringMethod = static_cast<TextureFilteringMethod>(jo[FILTER_FIELD_NAME].GetUint());
    xTiling = static_cast<TextureTilingMethod>(jo[TILE_X_FIELD_NAME].GetUint());
    yTiling = static_cast<TextureTilingMethod>(jo[TILE_Y_FIELD_NAME].GetUint());
    quality = jo[QUALITY_FIELD_NAME].GetFloat();
    preferredAnisotropy = jo[ANISOTROPY_FIELD_NAME].GetUint();
}

static const char* USE_32_BIT_INDICES_FIELD_NAME = "use32BitIndices";
static const char* CONVERT_ANIMATIONS_FIELD_NAME = "exportAnimations";
static const char* MESH_SCALE_FIELD_NAME = "scale";

std::uint64_t MeshConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

void MeshConverterState::serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const {
    pw.String(USE_32_BIT_INDICES_FIELD_NAME);
    pw.Bool(use32bitIndices);
    
    pw.String(CONVERT_ANIMATIONS_FIELD_NAME);
    pw.Bool(convertAnimations);
    
    pw.String(MESH_SCALE_FIELD_NAME);
    pw.Double(scale);
    
    // TODO serialize the not yet created fields
    // if (version == 2) {}
}

void MeshConverterState::deserializeJSONImpl(JSONObject& jo, std::uint64_t version) {
    use32bitIndices = jo[USE_32_BIT_INDICES_FIELD_NAME].GetBool();
    convertAnimations = jo[CONVERT_ANIMATIONS_FIELD_NAME].GetBool();
    scale = jo[MESH_SCALE_FIELD_NAME].GetFloat();
    
    // TODO deserialize the not yet created fields
    // if (version == 2) {}
}

std::uint64_t FontConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

void FontConverterState::serializeJSONImpl(PrettyStringWriter&, std::uint64_t version) const {
    assert(version == 1);
    // not needed, for now
}

void FontConverterState::deserializeJSONImpl(JSONObject&, std::uint64_t version) {
    assert(version == 1);
    // not needed, for now
}

static const char* PURPOSE_FIELD_NAME = "purpose";
static const char* STAGAE_FIELD_NAME = "stage";

std::uint64_t ShaderConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

void ShaderConverterState::serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const {
    assert(version == 1);
    
    pw.String(PURPOSE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(purpose));
    
    pw.String(STAGAE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(stage));
}

void ShaderConverterState::deserializeJSONImpl(JSONObject& jo, std::uint64_t version) {
    assert(version == 1);
    
    purpose = static_cast<ShaderPurpose>(jo[PURPOSE_FIELD_NAME].GetUint());
    stage = static_cast<ShaderStageFlagBits>(jo[STAGAE_FIELD_NAME].GetUint());
}

static const char* PRIORITY_FIELD_NAME = "priority";

std::uint64_t LocalizationStringConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

void LocalizationStringConverterState::serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const {
    assert(version == 1);
    
    pw.String(PRIORITY_FIELD_NAME);
    pw.Uint(priority);
}

void LocalizationStringConverterState::deserializeJSONImpl(JSONObject& jo, std::uint64_t version) {
    assert(version == 1);
    
    priority = jo[PRIORITY_FIELD_NAME].GetUint();
}

}
