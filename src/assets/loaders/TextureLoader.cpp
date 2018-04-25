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

#include "assets/loaders/TextureLoader.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "core/Constants.hpp"

namespace iyf {
static const std::array<char, 4> MAGIC_NUMBER = {'I', 'Y', 'F', 'T'};

TextureLoader::Result TextureLoader::load(const void* inputData, std::size_t byteCount) const {
    MemorySerializer ms(static_cast<const char*>(inputData), byteCount);
    
    std::array<char, 4> readMagicNumber;
    ms.readBytes(readMagicNumber.data(), readMagicNumber.size());
    
    if (readMagicNumber != MAGIC_NUMBER) {
        return Result::InvalidMagicNumber;
    }
    

    const std::uint8_t version = ms.readUInt8();
    if (version != 1) {
        return Result::InvalidVersionNumber;
    }

    const std::uint8_t faceCount = ms.readUInt8();
    const std::uint8_t channelCount = ms.readUInt8();
    const std::uint8_t mipmapLevelCount = ms.readUInt8();
    
    const std::uint32_t width = ms.readUInt32();
    const std::uint32_t height = ms.readUInt32();
    const std::uint32_t depth = ms.readUInt32();
    const std::uint32_t layers = ms.readUInt32();
    
    assert(depth == 1 && layers == 1);

    const TextureCompressionFormat format = static_cast<TextureCompressionFormat>(ms.readUInt16());
    const bool sRGB = ms.readUInt8();
    
    const std::uint8_t reserved = ms.readUInt8(); // Reads the reserved value;
    assert(reserved == 0);

    
    return Result::LoadSuccessful;
}

}
