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

#include "assets/metadata/MeshMetadata.hpp"
#include "core/filesystem/File.hpp"

#include "utilities/hashing/Hashing.hpp"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include <stdexcept>

namespace iyf {
static const char* SUB_MESH_COUNT_FIELD_NAME = "subMesheCount";
static const char* HAS_BONES_FIELD_NAME = "hasBones";
static const char* INDICES_32_BIT_FIELD_NAME = "indices32Bit";
static const char* COLOR_CHANNEL_COUNT_FIELD_NAME = "colorChannelCount";
static const char* VERTEX_COUNT_FIELD_NAME = "vertexCount";
static const char* INDEX_COUNT_FIELD_NAME = "indexCount";
static const char* SKELETON_KEY_FIELD_NAME = "skeletonKey";
static const char* BONE_COUNT_FIELD_NAME = "boneCount";
static const char* VERSION_FIELD_NAME = "version";
static const char* UV_SET_COUNT_FIELD_NAME = "UVSetCount";

std::uint16_t MeshMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

void MeshMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt8(numSubMeshes);
    fw.writeUInt8(hasBonesVal);
    fw.writeUInt8(indices32Bit);
    fw.writeUInt8(numColorChannels);
    fw.writeUInt32(vertexCount);
    fw.writeUInt32(indexCount);
    fw.writeUInt32(skeletonKey);
    fw.writeUInt8(boneCount);
    fw.writeUInt16(meshFormatVersion);
    fw.writeUInt8(numUVSets);
}

void MeshMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    numSubMeshes = fr.readUInt8();
    hasBonesVal = fr.readUInt8();
    indices32Bit = fr.readUInt8();
    numColorChannels = fr.readUInt8();
    vertexCount = fr.readUInt32();
    indexCount = fr.readUInt32();
    skeletonKey = fr.readUInt32();
    boneCount = fr.readUInt8();
    meshFormatVersion = fr.readUInt16();
    numUVSets = fr.readUInt8();
}

void MeshMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.Key(SUB_MESH_COUNT_FIELD_NAME);
    pw.Uint(numSubMeshes);
    
    pw.Key(HAS_BONES_FIELD_NAME);
    pw.Bool(hasBonesVal);
    
    pw.Key(INDICES_32_BIT_FIELD_NAME);
    pw.Bool(indices32Bit);
    
    pw.Key(COLOR_CHANNEL_COUNT_FIELD_NAME);
    pw.Uint(numColorChannels);
    
    pw.Key(VERTEX_COUNT_FIELD_NAME);
    pw.Uint(vertexCount);
    
    pw.Key(INDEX_COUNT_FIELD_NAME);
    pw.Uint(indexCount);
    
    pw.Key(SKELETON_KEY_FIELD_NAME);
    pw.Uint(skeletonKey);
    
    pw.Key(BONE_COUNT_FIELD_NAME);
    pw.Uint(boneCount);
    
    pw.Key(VERSION_FIELD_NAME);
    pw.Uint(meshFormatVersion);
    
    pw.Key(UV_SET_COUNT_FIELD_NAME);
    pw.Uint(numUVSets);
}

void MeshMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    numSubMeshes = jo[SUB_MESH_COUNT_FIELD_NAME].GetUint();
    hasBonesVal = jo[HAS_BONES_FIELD_NAME].GetBool();
    indices32Bit = jo[INDICES_32_BIT_FIELD_NAME].GetBool();
    numColorChannels = jo[COLOR_CHANNEL_COUNT_FIELD_NAME].GetUint();
    vertexCount = jo[VERTEX_COUNT_FIELD_NAME].GetUint();
    indexCount = jo[INDEX_COUNT_FIELD_NAME].GetUint();
    skeletonKey = jo[SKELETON_KEY_FIELD_NAME].GetUint();
    boneCount = jo[BONE_COUNT_FIELD_NAME].GetUint();
    meshFormatVersion = jo[VERSION_FIELD_NAME].GetUint();
    numUVSets = jo[UV_SET_COUNT_FIELD_NAME].GetUint();
}

void MeshMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
