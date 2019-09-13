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

#ifndef IYF_TEXTURE_LOADER_HPP
#define IYF_TEXTURE_LOADER_HPP

#include "core/Constants.hpp"
#include "glm/vec3.hpp"

namespace iyf {
/// \brief Must always be filled by TextureLoader::Load();
class TextureData {
public:
    TextureData() : loaded(false) {}
    
    std::uint32_t version;
    std::uint32_t faceCount;
    std::uint32_t channelCount;
    std::uint32_t mipmapLevelCount;
    
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
    std::uint32_t layers;
    
    TextureCompressionFormat format;
    bool sRGB;
private:
    bool loaded;
public:
    const char* data;
    std::size_t size;
    
    /// Computes the size (in bytes) of the specified mipmap level.
    std::size_t getLevelSize(std::size_t level) const;
    std::size_t getLevelOffset(std::size_t level) const;
    std::size_t getMipmapChainSize() const;
    
    const glm::uvec3& getLevelExtents(std::size_t level) const;
    
    const void* getData(std::size_t layer, std::size_t face, std::size_t level) const;
    
    inline bool isLoaded() const {
        return loaded;
    }
    
private:
    friend class TextureLoader;

    struct SizeAndOffset {
        std::size_t size;
        std::size_t offset;
    };
    
    std::array<SizeAndOffset, 16> sizesAndOffsets;
    std::array<glm::uvec3, 16> extents;
};

class TextureLoader {
public:
    enum class Result {
        InvalidMagicNumber,
        InvalidVersionNumber,
        LoadSuccessful
    };
    
    Result load(const void* inputData, std::size_t byteCount, TextureData& data) const;
};
}

#endif // IYF_TEXTURE_LOADER_HPP
