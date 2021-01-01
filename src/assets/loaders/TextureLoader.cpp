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
#include "io/serialization/MemorySerializer.hpp"
#include "logging/Logger.hpp"

namespace iyf {
static const std::array<char, 4> MAGIC_NUMBER = {'I', 'Y', 'F', 'T'};

TextureLoader::Result TextureLoader::load(const void* inputData, std::size_t byteCount, TextureData& data) const {
    MemorySerializer ms(static_cast<const char*>(inputData), byteCount);
    
    std::array<char, 4> readMagicNumber;
    ms.readBytes(readMagicNumber.data(), readMagicNumber.size());
    
    if (readMagicNumber != MAGIC_NUMBER) {
        return Result::InvalidMagicNumber;
    }

    TextureData tempData = {};
    tempData.version = ms.readUInt8();
    if (tempData.version != 1) {
        return Result::InvalidVersionNumber;
    }

    tempData.faceCount = ms.readUInt8();
    tempData.channelCount = ms.readUInt8();
    tempData.mipmapLevelCount = ms.readUInt8();
    
    assert(tempData.faceCount == 1 || tempData.faceCount == 6);
    
    // Just to make sure everything fits into the array. Yes, -1 is here for a reason
    // we save the total offset into the array as well
    assert(tempData.mipmapLevelCount < (tempData.sizesAndOffsets.size() - 1));
    
    tempData.width = ms.readUInt32();
    tempData.height = ms.readUInt32();
    tempData.depth = ms.readUInt32();
    tempData.layers = ms.readUInt32();
    
    assert(tempData.depth == 1 && tempData.layers == 1);

    tempData.format = static_cast<TextureCompressionFormat>(ms.readUInt16());
    tempData.sRGB = ms.readUInt8();
    
    const std::uint8_t reserved = ms.readUInt8(); // Reads the reserved value;
    assert(reserved == 0);
    
    std::size_t position = ms.tell();
    tempData.data = ms.data() + position;
    tempData.size = ms.size() - position;
    
    glm::uvec3 currentExtents(tempData.width, tempData.height, 1);
    std::size_t currentOffset = 0;
    for (std::size_t i = 0; i < tempData.mipmapLevelCount; ++i) {
        tempData.extents[i] = currentExtents;
        const std::size_t size = con::CompressedTextureMipmapLevelSize(tempData.format, currentExtents.x, currentExtents.y);
        
        currentExtents.x /= 2;
        currentExtents.y /= 2;
        
        tempData.sizesAndOffsets[i].offset = currentOffset;
        tempData.sizesAndOffsets[i].size = size;
        
        currentOffset += size;
    }
    
    // Save the total offset
    tempData.sizesAndOffsets[tempData.mipmapLevelCount].offset = currentOffset;
    tempData.sizesAndOffsets[tempData.mipmapLevelCount].size = 0;
    
    tempData.loaded = true;
    
    assert((tempData.sizesAndOffsets[tempData.mipmapLevelCount].offset * tempData.faceCount) == tempData.size);

    data = std::move(tempData);
    
    return Result::LoadSuccessful;
}

std::size_t TextureData::getLevelSize(std::size_t level) const {
    assert(level < mipmapLevelCount);
    return sizesAndOffsets[level].size;
}

std::size_t TextureData::getLevelOffset(std::size_t level) const {
    assert(level < mipmapLevelCount);
    return sizesAndOffsets[level].offset;
}

std::size_t TextureData::getMipmapChainSize() const {
    return sizesAndOffsets[mipmapLevelCount].offset;
}

const glm::uvec3& TextureData::getLevelExtents(std::size_t level) const {
    assert(level < mipmapLevelCount);
    return extents[level];
}

const void* TextureData::getData(std::size_t layer, std::size_t face, std::size_t level) const {
    // TODO this isn't ready for layers or depth, however, I don't use textures like that for the time being
    assert(version == 1);
    assert(layer == 0);
    assert(level < mipmapLevelCount);
    
    const std::size_t offset = getMipmapChainSize() * face + getLevelOffset(level);
    return data + offset;
}

}
