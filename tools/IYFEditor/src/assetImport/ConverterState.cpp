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

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

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
    
    pw.Key(CONTENTS_FIELD_NAME);
    pw.String(CONTENTS_FIELD_VALUE);
    
    pw.Key(TYPE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(getType()));
    
    pw.Key(VERSION_FIELD_NAME);
    pw.Uint64(version);
    
    pw.Key(SOURCE_HASH_FIELD_NAME);
    pw.Uint64(sourceFileHash.value());
    
    if (systemAsset) {
        pw.Key(IS_SYSTEM_ASSET_FIELD_NAME);
        pw.Bool(systemAsset);
    }
    
    pw.Key(TAG_FIELD_NAME);
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
}
