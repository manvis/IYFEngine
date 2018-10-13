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

#include "graphics/GraphicsAPIConstants.hpp"
#include "core/Constants.hpp"

#include <string>
#include <unordered_map>

namespace iyf {
namespace con {
static const std::unordered_map<std::string, ShaderStageFlags> ExtensionMap = {
    {".vert", ShaderStageFlagBits::Vertex},
    {".tesc", ShaderStageFlagBits::TessControl},
    {".tese", ShaderStageFlagBits::TessEvaluation},
    {".geom", ShaderStageFlagBits::Geometry},
    {".frag", ShaderStageFlagBits::Fragment},
    {".comp", ShaderStageFlagBits::Compute},
};

ShaderStageFlags ExtensionToShaderStage(const std::string& extension) {
    const auto result = ExtensionMap.find(extension);
    
    if (result != ExtensionMap.end()) {
        return result->second;
    } else {
        return ShaderStageFlags();
    }
}

Format CompressionFormatToEngineFormat(const TextureCompressionFormat& tcf, bool sRGB) {
    Format engineFormat = Format::Undefined;
    switch (tcf) {
        case TextureCompressionFormat::NotCompressed:
            throw std::runtime_error("This function only supports compressed formats.");
        case TextureCompressionFormat::BC1:
            engineFormat = sRGB ? Format::BC1_RGB_sRGB_block : Format::BC1_RGB_uNorm_block;
            break;
        case TextureCompressionFormat::BC2:
            engineFormat = sRGB ? Format::BC2_sRGB_block : Format::BC2_uNorm_block;
            break;
        case TextureCompressionFormat::BC3:
            engineFormat = sRGB ? Format::BC3_sRGB_block : Format::BC3_uNorm_block;
            break;
        case TextureCompressionFormat::BC4:
            engineFormat = Format::BC4_uNorm_block;
            break;
        case TextureCompressionFormat::BC5:
            engineFormat = Format::BC5_uNorm_block;
            break;
        case TextureCompressionFormat::BC6:
            engineFormat = Format::BC6H_uFloat_block;
            break;
        case TextureCompressionFormat::BC7:
            engineFormat = sRGB ? Format::BC7_sRGB_block : Format::BC7_uNorm_block;
            break;
        case TextureCompressionFormat::ETC1:
            throw std::runtime_error("Not implemented yet");
            break;
        case TextureCompressionFormat::ETC2:
            throw std::runtime_error("Not implemented yet");
            break;
        case TextureCompressionFormat::COUNT:
            throw std::runtime_error("COUNT is not a format.");
    }
    
    if (engineFormat == Format::Undefined) {
        throw std::runtime_error("An invalid TextureCompressionFormat was provided.");
    }
    
    return engineFormat;
}
}
}
