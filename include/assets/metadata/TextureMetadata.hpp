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

#ifndef TEXTUREMETADATA_HPP
#define TEXTUREMETADATA_HPP

#include "assets/metadata/MetadataBase.hpp"

namespace iyf {
class TextureMetadata : public MetadataBase {
public:
    inline TextureMetadata() : MetadataBase(AssetType::Texture) {};
    
    inline TextureMetadata(hash64_t fileHash,
                           const fs::path& sourceAsset,
                           hash64_t sourceFileHash,
                           bool systemAsset,
                           const std::vector<std::string>& tags,
                           std::uint32_t width,
                           std::uint32_t height,
                           std::uint32_t depth,
                           std::uint8_t faces,
                           std::uint32_t layers,
                           std::uint8_t levels,
                           std::uint8_t channels,
                           TextureFilteringMethod filter,
                           TextureTilingMethod tileX,
                           TextureTilingMethod tileY,
                           std::uint8_t anisotropy,
                           TextureCompressionFormat compressionFormat,
                           bool issRGBVal) 
        : MetadataBase(AssetType::Texture, fileHash, sourceAsset, sourceFileHash, systemAsset, tags, true), 
          compressionFormat(compressionFormat), issRGBVal(issRGBVal), faces(faces), channels(channels),
          levels(levels), filter(filter), tileX(tileX), tileY(tileY), width(width), height(height),
          depth(depth), layers(layers), anisotropy(anisotropy) {}
    
    inline TextureCompressionFormat getCompressionFormat() const {
        return compressionFormat;
    }
    
    inline bool isCubemap() const {
        return faces == 6;
    }
    
    inline bool isArray() const {
        return layers > 1;
    }
    
    inline bool isVolume() const {
        return depth > 1;
    }
    
    /// Is the imported texture in sRGB color space (true) or linear (false)
    inline bool issRGB() const {
        return issRGBVal;
    }
    
    /// Is the imported texture in linear (true) or sRGB color space (false)
    inline bool isLinear() const {
        return !issRGBVal;
    }
    
    /// The width of the texture (in pixels)
    inline std::uint32_t getWidth() const {
        return width;
    }
    
    /// The height of the texture (in pixels)
    inline std::uint32_t getHeight() const {
        return height;
    }
    
    /// The depth of a volume texture (in pixels)
    inline std::uint32_t getDepth() const {
        return depth;
    }
    
    /// The number of layers in an array texture
    inline std::uint32_t getLayers() const {
        return layers;
    }
    
    /// The number of faces in the texture
    inline std::uint8_t getFaces() const {
        return faces;
    }
    
    /// Get the number of channels stored in the image
    inline std::uint8_t getChannels() const {
        return channels;
    }
    
    /// Get the number of mipmap levels stored in the texture
    inline std::uint8_t getLevels() const {
        return levels;
    }
    
    /// Returns the filtering method that should be used for this texture
    inline TextureFilteringMethod getFilter() const {
        return filter;
    }
    
    /// Return the method that should be used to tile this texture in the x (width) direction
    inline TextureTilingMethod getTileX() const {
        return tileX;
    }
    
    /// Return the method that should be used to tile this texture in the y (height) direction
    inline TextureTilingMethod getTileY() const {
        return tileY;
    }
    
    /// Return the preferred level of anisotropy for this texture
    ///
    /// \warning Normally, the level of anisotropic filtering is determined by the settings.
    /// If a non-zero value is returned by this metadata instance, the engine will be **forced**
    /// to use it. Therefore, setting high values in metadata may negatively impact performance
    /// on less powerful hardware. Likewise, forcing low values will reduce the quality of the
    /// texture when it's displayed on high-end systems.
    inline std::uint8_t getPreferredAnisotropy() const {
        return anisotropy;
    }
    
    virtual std::uint16_t getLatestSerializedDataVersion() const final override;
private:
    virtual void serializeImpl(Serializer& fw, std::uint16_t version) const final override;
    virtual void deserializeImpl(Serializer& fr, std::uint16_t version) final override;
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const final override;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint16_t version) final override;
    
    TextureCompressionFormat compressionFormat;
    bool issRGBVal;
    std::uint8_t faces;
    std::uint8_t channels;
    
    std::uint8_t levels;
    TextureFilteringMethod filter;
    TextureTilingMethod tileX;
    TextureTilingMethod tileY;
    
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
    std::uint32_t layers;
    
    std::uint8_t anisotropy;
};
}

#endif /* TEXTUREMETADATA_HPP */

