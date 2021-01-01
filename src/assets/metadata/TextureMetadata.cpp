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

#include "assets/metadata/TextureMetadata.hpp"
#include "io/File.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include <stdexcept>

namespace iyf {
static const char* COMPRESSION_FORMAT_FIELD_NAME = "compressionFormat";
static const char* IS_SRGB_VALUE_FIELD_NAME = "issRGBVal";
static const char* FACES_FIELD_NAME = "faces";
static const char* CHANNELS_FIELD_NAME = "channels";
static const char* LEVELS_FIELD_NAME = "levels";
static const char* FILTER_FIELD_NAME = "filter";
static const char* TILE_X_FIELD_NAME = "tileX";
static const char* TILE_Y_FIELD_NAME = "tileY";
static const char* WIDTH_FIELD_NAME = "width";
static const char* HEIGHT_FIELD_NAME = "height";
static const char* DEPTH_FIELD_NAME = "depth";
static const char* LAYERS_FIELD_NAME = "layers";
static const char* ANISOTROPY_FIELD_NAME = "anisotropy";
static const char* SIZE_FIELD_NAME = "size";


std::uint16_t TextureMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

void TextureMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt16(static_cast<std::uint16_t>(compressionFormat));
    fw.writeUInt8(issRGBVal);
    fw.writeUInt8(faces);
    fw.writeUInt8(channels);
    fw.writeUInt8(levels);
    fw.writeUInt8(static_cast<std::uint8_t>(filter));
    fw.writeUInt8(static_cast<std::uint8_t>(tileX));
    fw.writeUInt8(static_cast<std::uint8_t>(tileY));
    fw.writeUInt32(width);
    fw.writeUInt32(height);
    fw.writeUInt32(depth);
    fw.writeUInt32(layers);
    fw.writeUInt8(anisotropy);
    fw.writeUInt64(size);
}

void TextureMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    compressionFormat = static_cast<TextureCompressionFormat>(fr.readUInt16());
    issRGBVal = fr.readUInt8();
    faces = fr.readUInt8();
    channels = fr.readUInt8();
    levels = fr.readUInt8();
    filter = static_cast<TextureFilteringMethod>(fr.readUInt8());
    tileX = static_cast<TextureTilingMethod>(fr.readUInt8());
    tileY = static_cast<TextureTilingMethod>(fr.readUInt8());
    width = fr.readUInt32();
    height = fr.readUInt32();
    depth = fr.readUInt32();
    layers = fr.readUInt32();
    anisotropy = fr.readUInt8();
    size = fr.readUInt64();
}

void TextureMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.Key(COMPRESSION_FORMAT_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(compressionFormat));
    
    pw.Key(IS_SRGB_VALUE_FIELD_NAME);
    pw.Bool(issRGBVal);
    
    pw.Key(FACES_FIELD_NAME);
    pw.Uint(faces);
    
    pw.Key(CHANNELS_FIELD_NAME);
    pw.Uint(channels);
    
    pw.Key(LEVELS_FIELD_NAME);
    pw.Uint(levels);
    
    pw.Key(FILTER_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(filter));
    
    pw.Key(TILE_X_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(tileX));
    
    pw.Key(TILE_Y_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(tileY));
    
    pw.Key(WIDTH_FIELD_NAME);
    pw.Uint(width);
    
    pw.Key(HEIGHT_FIELD_NAME);
    pw.Uint(height);
    
    pw.Key(DEPTH_FIELD_NAME);
    pw.Uint(depth);
    
    pw.Key(LAYERS_FIELD_NAME);
    pw.Uint(layers);
    
    pw.Key(ANISOTROPY_FIELD_NAME);
    pw.Uint(anisotropy);
    
    pw.Key(SIZE_FIELD_NAME);
    pw.Uint64(size);
}

void TextureMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    compressionFormat = static_cast<TextureCompressionFormat>(jo[COMPRESSION_FORMAT_FIELD_NAME].GetUint());
    issRGBVal = jo[IS_SRGB_VALUE_FIELD_NAME].GetBool();
    faces = jo[FACES_FIELD_NAME].GetUint();
    channels = jo[CHANNELS_FIELD_NAME].GetUint();
    levels = jo[LEVELS_FIELD_NAME].GetUint();
    filter = static_cast<TextureFilteringMethod>(jo[FILTER_FIELD_NAME].GetUint());
    tileX = static_cast<TextureTilingMethod>(jo[TILE_X_FIELD_NAME].GetUint());
    tileY = static_cast<TextureTilingMethod>(jo[TILE_Y_FIELD_NAME].GetUint());
    width = jo[WIDTH_FIELD_NAME].GetUint();
    height = jo[HEIGHT_FIELD_NAME].GetUint();
    depth = jo[DEPTH_FIELD_NAME].GetUint();
    layers = jo[LAYERS_FIELD_NAME].GetUint();
    anisotropy = jo[ANISOTROPY_FIELD_NAME].GetUint();
    size = jo[SIZE_FIELD_NAME].GetUint64();
}

void TextureMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
