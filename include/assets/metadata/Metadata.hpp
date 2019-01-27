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

#ifndef IYF_METADATA_HPP
#define IYF_METADATA_HPP

#include "assets/AssetConstants.hpp"
#include "assets/metadata/MetadataBase.hpp"
#include "utilities/ForceInline.hpp"

namespace iyf {
// Forward declarations
class AnimationMetadata;
class MeshMetadata;
class TextureMetadata;
class FontMetadata;
class AudioMetadata;
class VideoMetadata;
class ScriptMetadata;
class ShaderMetadata;
class StringMetadata;
class CustomMetadata;
class MaterialTemplateMetadata;

///// We use a variant here because the Metadata is stored in a frequently accessed std::unordered_map. Using polymorphism would require pointers and, by
///// extension, an extra layer of indirection.
///// 
///// \warning The order of metadata elements must match con::AssetType
// using Metadata = std::variant<AnimationMetadata, MeshMetadata, TextureMetadata, FontMetadata, AudioMetadata, VideoMetadata, ScriptMetadata,
//                               ShaderMetadata, StringMetadata, CustomMetadata, MaterialTemplateMetadata>;

template <typename T>
using EnableIfDerivedFromMetadataBase = typename std::enable_if<std::is_base_of_v<MetadataBase, std::decay_t<T>>>::type;

/// A variant class of sorts, however, unlike std::variant it isn't aware of what it may contain. While it doesn't allow certain compile time error checks,
/// we no longer need to declare all metadata types in advance and this speeds up compilation signifficantly.
class Metadata {
public:
    static constexpr std::size_t Alignment = 8;
    static constexpr std::size_t Size = 240;
    
    static std::size_t GetAssetMetadataSize(AssetType type);
    
    Metadata() : type(AssetType::COUNT) {}
    
    Metadata(const Metadata& other);
    Metadata& operator=(const Metadata& other);
    Metadata(Metadata&& other);
    Metadata& operator=(Metadata&& other);
    
    template <typename T, typename EnableCondition = EnableIfDerivedFromMetadataBase<T>>
    Metadata(T&& value) : type(AssetType::COUNT) {
        using TDec = std::decay_t<T>;
        static_assert(std::is_base_of_v<MetadataBase, TDec>);
        TDec* obj = new(data) TDec(std::forward<T>(value));
        type = obj->getAssetType();
    }
    
    template <typename T, typename EnableCondition = EnableIfDerivedFromMetadataBase<T>>
    IYF_FORCE_INLINE T& get() {
        return *std::launder(reinterpret_cast<T*>(&data));
    }
    
    template <typename T, typename EnableCondition = EnableIfDerivedFromMetadataBase<T>>
    IYF_FORCE_INLINE const T& get() const {
        return *std::launder(reinterpret_cast<const T*>(&data));
    }
    
    MetadataBase& getBase() {
        return *std::launder(reinterpret_cast<MetadataBase*>(&data));
    }
    
    const MetadataBase& getBase() const {
        return *std::launder(reinterpret_cast<const MetadataBase*>(&data));
    }
    
    IYF_FORCE_INLINE AssetType getAssetType() const {
        return type;
    }
    
    IYF_FORCE_INLINE bool hasValidValue() const {
        return type != AssetType::ANY;
    }
    
    template <typename T, typename EnableCondition = EnableIfDerivedFromMetadataBase<T>>
    Metadata& operator=(T&& value) {
        destroyContents();
        
        using TDec = std::decay_t<T>;
        static_assert(std::is_base_of_v<MetadataBase, TDec>);
        TDec* obj = new(data) TDec(std::forward<T>(value));
        type = obj->getAssetType();
        
        return *this;
    }
    
    friend bool operator!=(const Metadata& a, const Metadata& b) {
        return !(a == b);
    }
    
    friend bool operator==(const Metadata& a, const Metadata& b) {
        return a.equals(b);
    }
    
    ~Metadata();
private:
    void destroyContents();
    bool equals(const Metadata& other) const;
    
    alignas(Alignment) unsigned char data[Size];
    AssetType type;
};
}

#endif // IYF_METADATA_HPP

