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

#include "assets/metadata/MetadataBase.hpp"
#include "utilities/hashing/Hashing.hpp"

#include <stdexcept>

namespace iyf {
static const char* ASSET_TYPE_FIELD_NAME = "assetType";
static const char* FILE_HASH_FIELD_NAME = "fileHash";
static const char* SOURCE_ASSET_FIELD_NAME = "sourceAsset";
static const char* SERIALIZED_DATA_VERSION_FIELD_NAME = "serializedDataVersion";

//static const std::array<char, 4> MAGIC_NUMBER = {'I', 'Y', 'F', 'D'};

void MetadataBase::serialize(Serializer& fw) const {
    if (!complete) {
        throw std::logic_error("Cannot serialize incomplete metadata");
    }
    
    //fw.writeBytes(MAGIC_NUMBER.data(), 4);
    fw.writeUInt8(static_cast<std::uint8_t>(getAssetType()));
    fw.writeUInt16(getLatestSerializedDataVersion());
    fw.writeUInt64(fileHash.value());
    fw.writeString(sourceAsset.generic_string(), StringLengthIndicator::UInt16);
    
    serializeImpl(fw, getLatestSerializedDataVersion());
}

void MetadataBase::deserialize(Serializer& fr)  {
    //std::array<char, 4> temp;
    //fr.readBytes(temp.data(), 4);
    
    AssetType type = static_cast<AssetType>(fr.readUInt8());
    
    if (type != assetType) {
        throw std::runtime_error("Tried to deserialize a metadata file of a different AssetType");
    }
    
    std::uint16_t version = fr.readUInt16();
    
    fileHash = hash64_t(fr.readUInt64());
    
    std::string temp;
    fr.readString(temp, StringLengthIndicator::UInt16, 0);
    sourceAsset = std::move(temp);
    
    deserializeImpl(fr, version);
    
    complete = true;
}

void MetadataBase::serializeJSON(PrettyStringWriter& pw) const  {
    if (!complete) {
        throw std::logic_error("Cannot serialize incomplete metadata");
    }
    
    pw.StartObject();
    
    pw.String(ASSET_TYPE_FIELD_NAME);
    pw.Uint(static_cast<unsigned int>(getAssetType()));
    
    pw.String(FILE_HASH_FIELD_NAME);
    pw.Uint64(fileHash.value());
    
    pw.String(SOURCE_ASSET_FIELD_NAME);
    pw.String(sourceAsset.generic_string().c_str(), sourceAsset.generic_string().length(), true);
    
    pw.String(SERIALIZED_DATA_VERSION_FIELD_NAME);
    pw.Uint(getLatestSerializedDataVersion());
    
    serializeJSONImpl(pw, getLatestSerializedDataVersion());
    pw.EndObject();
}

void MetadataBase::deserializeJSON(JSONObject& jo) {
    if (!jo.HasMember(ASSET_TYPE_FIELD_NAME) || !jo[ASSET_TYPE_FIELD_NAME].IsUint()) {
        throw std::runtime_error("A deserialized JSON did not have an asset type field");
    }
    
    if (static_cast<AssetType>(jo[ASSET_TYPE_FIELD_NAME].GetUint()) != getAssetType()) {
        throw std::runtime_error("Tried to deserialize a metadata file of a different AssetType");
    }
    
    fileHash = hash64_t(jo[FILE_HASH_FIELD_NAME].GetUint64());
    sourceAsset = fs::path(jo[SOURCE_ASSET_FIELD_NAME].GetString());
    
    std::uint16_t version = jo[SERIALIZED_DATA_VERSION_FIELD_NAME].GetInt();
    
    deserializeJSONImpl(jo, version);
    
    complete = true;
}
}
