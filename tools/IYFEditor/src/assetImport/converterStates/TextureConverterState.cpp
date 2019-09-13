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

#include "assetImport/converterStates/TextureConverterState.hpp"

namespace iyf::editor {
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
    
    pw.Key(PREMULTIPLY_ALPHA_FIELD_NAME);
    pw.Bool(premultiplyAlpha);
    
    pw.Key(IS_SRGB_VALUE_FIELD_NAME);
    pw.Bool(sRGBSource);
    
    pw.Key(NO_MIP_MAPS_FIELD_NAME);
    pw.Bool(noMipMaps);
    
    pw.Key(IMPORT_MODE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(importMode));
    
    pw.Key(FILTER_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(filteringMethod));
    
    pw.Key(TILE_X_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(xTiling));
    
    pw.Key(TILE_Y_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(yTiling));
    
    pw.Key(QUALITY_FIELD_NAME);
    pw.Double(quality);
    
    pw.Key(ANISOTROPY_FIELD_NAME);
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
}
