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

#include "core/Constants.hpp"

#include <algorithm>

namespace iyf {
namespace con {
inline std::size_t CalculateMipmapLevelSizeHalfBytePerPixel(std::size_t width, std::size_t height) {
    const std::size_t blockSize = 8;
    const std::size_t size = (width * height) / 2;
    
    // Even if only a single pixel is required (e.g., the last level of the mipmap), an entire
    // block has to be written.
    return std::max(size, blockSize);
}

inline std::size_t CalculateMipmapLevelSizeBytePerPixel(std::size_t width, std::size_t height) {
    const std::size_t blockSize = 16;
    const std::size_t size = width * height;
    
    // Even if only a single pixel is required (e.g., the last level of the mipmap), an entire
    // block has to be written.
    return std::max(size, blockSize);
}

std::size_t CompressedTextureMipmapLevelSize(TextureCompressionFormat format, std::size_t width, std::size_t height) {
    // Based on Compressonator/Source/Codec CalcBufferSize
    switch (format) {
        case TextureCompressionFormat::NotCompressed:
            return 0;
        case TextureCompressionFormat::BC1:
        case TextureCompressionFormat::BC4:
        case TextureCompressionFormat::ETC1:
        case TextureCompressionFormat::ETC2:
            return CalculateMipmapLevelSizeHalfBytePerPixel(width, height);
        case TextureCompressionFormat::BC2:
        case TextureCompressionFormat::BC3:
        case TextureCompressionFormat::BC5:
        case TextureCompressionFormat::BC6:
        case TextureCompressionFormat::BC7:
            return CalculateMipmapLevelSizeBytePerPixel(width, height);
        case TextureCompressionFormat::COUNT:
            throw std::runtime_error("COUNT is not a valid TextureCompressionFormat value");
    }
}

}
}
