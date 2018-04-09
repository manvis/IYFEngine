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

#ifndef TEXTURECONVERTER_HPP
#define TEXTURECONVERTER_HPP

#include "Converter.hpp"

namespace iyf {

namespace editor {
class TextureConverter : public Converter {
public:
    struct MipmapLevelData {
        MipmapLevelData() : width(0), height(0), data(nullptr) {}
        MipmapLevelData(std::size_t width, std::size_t height, std::unique_ptr<float[]> data) : width(width), height(height), data(std::move(data)) {}
        
        std::size_t width;
        std::size_t height;
        std::unique_ptr<float[]> data;
    };
    
    TextureConverter(const ConverterManager* manager);
    ~TextureConverter();
    
    virtual std::unique_ptr<ConverterState> initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const final override;
    
    /// \brief Reads a texture file and converts it to a format optimized for this engine.
    virtual bool convert(ConverterState& state) const final override;
protected:

    bool importCompressed(const fs::path& inPath, std::vector<ImportedAssetData>& importedAssets) const;
    TextureCompressionFormat compressonatorDetermineFormat(const TextureConverterState& textureState) const;
    bool compressonatorCompress(Serializer& serializer, std::vector<MipmapLevelData>& mipMapLevelData, const TextureConverterState& textureState,
                                std::size_t face, std::size_t level) const;
    bool writeMipmapLevel(Serializer& serializer, std::uint8_t* bytes, std::size_t count) const;
};
}
}

#endif /* TEXTURECONVERTER_HPP */

