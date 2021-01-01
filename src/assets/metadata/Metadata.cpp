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

#include "assets/metadata/Metadata.hpp"
#include "assets/metadata/AnimationMetadata.hpp"
#include "assets/metadata/MeshMetadata.hpp"
#include "assets/metadata/TextureMetadata.hpp"
#include "assets/metadata/FontMetadata.hpp"
#include "assets/metadata/AudioMetadata.hpp"
#include "assets/metadata/VideoMetadata.hpp"
#include "assets/metadata/ScriptMetadata.hpp"
#include "assets/metadata/ShaderMetadata.hpp"
#include "assets/metadata/StringMetadata.hpp"
#include "assets/metadata/CustomMetadata.hpp"
#include "assets/metadata/MaterialTemplateMetadata.hpp"
#include "assets/metadata/MaterialInstanceMetadata.hpp"

#include "logging/Logger.hpp"

#include <new>

namespace iyf {
// WARNING: When adding new metadata types, simply add a new COMMAND line to the IYF_METADATA_SWITCH macro
    
#define IYF_METADATA_SWITCH(SWITCH_ON, COMMAND) \
    switch (SWITCH_ON) { \
        COMMAND(Animation, AnimationMetadata)\
        COMMAND(Mesh, MeshMetadata)\
        COMMAND(Texture, TextureMetadata)\
        COMMAND(Font, FontMetadata)\
        COMMAND(Audio, AudioMetadata)\
        COMMAND(Video, VideoMetadata)\
        COMMAND(Script, ScriptMetadata)\
        COMMAND(Shader, ShaderMetadata)\
        COMMAND(Strings, StringMetadata)\
        COMMAND(Custom, CustomMetadata)\
        COMMAND(MaterialTemplate, MaterialTemplateMetadata)\
        COMMAND(MaterialInstance, MaterialInstanceMetadata)\
        case AssetType::COUNT:\
            break;\
    }

#define IYF_METADATA_OBJECT_CHECK(objType) \
    static_assert(alignof(objType) == Metadata::Alignment, "Incorret alignment of object " #objType); \
    static_assert(sizeof(objType) <= Metadata::Size, "Incorret size of object " #objType);\
    static_assert(std::is_copy_constructible_v<objType>, "#objType is not copy constructible");\
    static_assert(std::is_copy_assignable_v<objType>, "#objType is not copy assignable");\
    static_assert(std::is_move_constructible_v<objType>, "#objType is not move constructible");\
    static_assert(std::is_move_constructible_v<objType>, "#objType is not move assignable");\

#define IYF_DETERMINE_SIZE_CASE(EnumType, MetaType) \
    case AssetType::EnumType: { \
            IYF_METADATA_OBJECT_CHECK(MetaType) \
            typeSize = sizeof(MetaType); \
        } \
        break;

#define IYF_CONSTRUCT_METADATA_COPY_CASE(EnumType, MetaType) \
    case AssetType::EnumType: { \
            const MetaType& otherData = other.get<MetaType>();\
            MetaType* obj = new(data) MetaType(otherData); \
            type = obj->getAssetType(); \
        } \
        break;

#define IYF_CONSTRUCT_METADATA_MOVE_CASE(EnumType, MetaType) \
    case AssetType::EnumType: { \
            MetaType& otherData = other.get<MetaType>();\
            MetaType* obj = new(data) MetaType(std::move(otherData)); \
            type = obj->getAssetType(); \
        } \
        break;

#define IYF_PERFORM_COMPARISON_CASE(EnumType, MetaType) \
    case AssetType::EnumType: { \
            const MetaType& realA = get<MetaType>();\
            const MetaType& realB = other.get<MetaType>();\
            return realA == realB;\
        }
    
#define IYF_CONSTRUCT_METADATA_COPY(SWITCH_ON) \
    IYF_METADATA_SWITCH(SWITCH_ON, IYF_CONSTRUCT_METADATA_COPY_CASE)
    
#define IYF_CONSTRUCT_METADATA_MOVE(SWITCH_ON) \
    IYF_METADATA_SWITCH(SWITCH_ON, IYF_CONSTRUCT_METADATA_MOVE_CASE)

#define IYF_GET_METADATA_SIZE(SWITCH_ON) \
    IYF_METADATA_SWITCH(SWITCH_ON, IYF_DETERMINE_SIZE_CASE)

#define IYF_PERFORM_COMPARISON(SWITCH_ON) \
    IYF_METADATA_SWITCH(SWITCH_ON, IYF_PERFORM_COMPARISON_CASE)

Metadata::Metadata(const Metadata& other) {
    if (other.hasValidValue()) {
        IYF_CONSTRUCT_METADATA_COPY(other.getAssetType());
    } else {
        type = AssetType::COUNT;
    }
}

Metadata& Metadata::operator=(const Metadata& other) {
    if (&other != this && other.hasValidValue()) {
        destroyContents();
        IYF_CONSTRUCT_METADATA_COPY(other.getAssetType());
    } else {
        type = AssetType::COUNT;
    }
    
    return *this;
}

Metadata::Metadata(Metadata&& other) {
    if (other.hasValidValue()) {
        IYF_CONSTRUCT_METADATA_MOVE(other.getAssetType());
        other.destroyContents();
    } else {
        type = AssetType::COUNT;
    }
}

Metadata& Metadata::operator=(Metadata&& other) {
    if (&other != this && other.hasValidValue()) {
        destroyContents();
        IYF_CONSTRUCT_METADATA_MOVE(other.getAssetType());
        other.destroyContents();
    } else {
        type = AssetType::COUNT;
    }
    
    return *this;
}

void Metadata::destroyContents() {
    if (hasValidValue()) {
        MetadataBase* base = std::launder(reinterpret_cast<MetadataBase*>(&data[0]));
        
        assert(&data[0] != nullptr);
        assert(base != nullptr);
        base->~MetadataBase();
        type = AssetType::ANY;
    }
}

Metadata::~Metadata() {
    destroyContents();
}

std::size_t Metadata::GetAssetMetadataSize(AssetType type) {
    std::size_t typeSize = 0;
    IYF_GET_METADATA_SIZE(type);
    return typeSize;
}

bool Metadata::equals(const Metadata& other) const {
    if ((type == AssetType::ANY) || (other.type == AssetType::ANY)) {
        return false;
    }
    
    if (type != other.type) {
        return false;
    }
    
    IYF_PERFORM_COMPARISON(type);
    return false;
}
}
