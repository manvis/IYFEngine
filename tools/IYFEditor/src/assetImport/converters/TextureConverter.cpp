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

#include "assetImport/converters/TextureConverter.hpp"
#include "assetImport/converterStates/TextureConverterState.hpp"

#include "assets/metadata/TextureMetadata.hpp"

#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/filesystem/FileSystem.hpp"
#include "core/filesystem/File.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "core/serialization/MemorySerializer.hpp"
#include "core/serialization/VirtualFileSystemSerializer.hpp"
#include "utilities/BitFunctions.hpp"

#include "assetImport/ConverterManager.hpp"

#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "Compressonator.h"
#include "tools/Compressonator_Test_Helpers.h"
#include "glm/gtc/packing.hpp"

/// Same as the one used in stb_image_resize. Used to prevent alpha premultiplication from removing the color data in transparent pixels
#define IYF_ALPHA_EPSILON ((float)1 / (1 << 20) / (1 << 20) / (1 << 20) / (1 << 20))

// Write conversion results (converted back into regular images) next to the executable
#define IYF_WRITE_DEBUG_CONVERSION_IMAGES

// #define IYF_DEBUG_IMAGE_OUTPUT_HDR // HDR does not work with alpha

namespace iyf {
namespace editor {
// This is based on stb_image
inline std::uint8_t ColorToLDR(float hdr) {
    const float invGamma = 1.0f / 2.2f;
    float result = std::pow(hdr, invGamma) * 255.0f + 0.5f;
    result = std::clamp(result, 0.0f, 255.0f);
    
    return result;
}

// This is based on stb_image
inline std::uint8_t AlphaToLDR(float hdr) {
    float result = hdr * 255.0f + 0.5f;
    result = std::clamp(result, 0.0f, 255.0f);
    
    return result;
}

inline TextureCompressionFormat CompressonatorToEngineFormat(CMP_FORMAT format) {
    switch (format) {
        case CMP_FORMAT_BC1:
            return TextureCompressionFormat::BC1;
        case CMP_FORMAT_BC2:
            return TextureCompressionFormat::BC2;
        case CMP_FORMAT_BC3:
            return TextureCompressionFormat::BC3;
        case CMP_FORMAT_BC4:
            return TextureCompressionFormat::BC4;
        case CMP_FORMAT_BC5:
            return TextureCompressionFormat::BC5;
        case CMP_FORMAT_BC6H:
        case CMP_FORMAT_BC6H_SF:
            return TextureCompressionFormat::BC6;
        case CMP_FORMAT_BC7:
            return TextureCompressionFormat::BC7;
        default:
            throw std::runtime_error("The compressonator is not supposed to decompress this format");
    }
}

[[maybe_unused]] void debugWriteImageHDR(const fs::path& path, const float* source, std::size_t width, std::size_t height, std::size_t channels, bool premultiplied) {
    const std::size_t size = width * height * channels;
    float* destination = new float[size];
    
//     LOG_D("WHS " << width << " " << height << " " << channels << "; size " << size);
    
    if (channels == 4 && premultiplied) {
        for (std::size_t i = 0; i < size; i += 4) {
            const float alpha = source[i + 3];
            
            destination[i    ] = (source[i    ] / alpha);
            destination[i + 1] = (source[i + 1] / alpha);
            destination[i + 2] = (source[i + 2] / alpha);
            destination[i + 3] = alpha;
        }
    } else {
        for (std::size_t i = 0; i < size; ++i) {
            destination[i] = source[i];
        }
    }
    
    int result = stbi_write_hdr(path.c_str(), width, height, channels, destination);
    if (!result) {
        throw std::runtime_error("Failed to write a debug image");
    }
    
    delete[] destination;
}

[[maybe_unused]] void debugWriteImage(const fs::path& path, const float* source, std::size_t width, std::size_t height, std::size_t channels, bool premultiplied, bool unpremultiply, bool sRGBSource) {
    const std::size_t pixelCount = width * height;
    std::uint8_t* destination = new std::uint8_t[pixelCount * channels];
    
//     LOG_D("WHS " << width << " " << height << " " << channels << "; size " << (pixelCount * channels));
    if (channels == 4 && premultiplied && unpremultiply) {
        if (sRGBSource) {
            for (std::size_t i = 0; i < pixelCount; ++i) {
                const float alpha = source[i * 4 + 3];
                
                destination[i * 4 + 0] = ColorToLDR((source[i * 4 + 0] / alpha));
                destination[i * 4 + 1] = ColorToLDR((source[i * 4 + 1] / alpha));
                destination[i * 4 + 2] = ColorToLDR((source[i * 4 + 2] / alpha));
                destination[i * 4 + 3] = AlphaToLDR(alpha);
            }
        } else {
            for (std::size_t i = 0; i < pixelCount; ++i) {
                const float alpha = source[i * 4 + 3];
                
                destination[i * 4 + 0] = AlphaToLDR((source[i * 4 + 0] / alpha));
                destination[i * 4 + 1] = AlphaToLDR((source[i * 4 + 1] / alpha));
                destination[i * 4 + 2] = AlphaToLDR((source[i * 4 + 2] / alpha));
                destination[i * 4 + 3] = AlphaToLDR(alpha);
            }
        }
    } else if (channels == 4) {
        if (sRGBSource) {
            for (std::size_t i = 0; i < pixelCount; ++i) {
                destination[i * 4 + 0] = ColorToLDR(source[i * 4 + 0]);
                destination[i * 4 + 1] = ColorToLDR(source[i * 4 + 1]);
                destination[i * 4 + 2] = ColorToLDR(source[i * 4 + 2]);
                destination[i * 4 + 3] = AlphaToLDR(source[i * 4 + 3]);
            }
        } else {
            
            for (std::size_t i = 0; i < pixelCount; ++i) {
                destination[i * 4 + 0] = AlphaToLDR(source[i * 4 + 0]);
                destination[i * 4 + 1] = AlphaToLDR(source[i * 4 + 1]);
                destination[i * 4 + 2] = AlphaToLDR(source[i * 4 + 2]);
                destination[i * 4 + 3] = AlphaToLDR(source[i * 4 + 3]);
            }
        }
    } else {
        if (sRGBSource) {
            for (std::size_t i = 0; i < pixelCount * channels; ++i) {
                destination[i] = ColorToLDR(source[i]);
            }
        } else {
            for (std::size_t i = 0; i < pixelCount * channels; ++i) {
                destination[i] = AlphaToLDR(source[i]);
            }
        }
    }
    
    int result = stbi_write_png(path.c_str(), width, height, channels, destination, 0);
    if (!result) {
        throw std::runtime_error("Failed to write a debug image");
    }
    
    delete[] destination;
}

void saveCompressedImage([[maybe_unused]]const CMP_Texture& source, CMP_Texture& destination, const fs::path& path, std::size_t f, std::size_t l, [[maybe_unused]]bool needToSwizzleRB) {
    fs::path debugPath = path.stem();
    debugPath += "_";
    debugPath += std::to_string(f);
    debugPath += "_";
    debugPath += std::to_string(l);
    debugPath += "_conv";
    debugPath += ".dds";
    SaveDDSFile(debugPath.c_str(), destination);
    
    // This has some corner cases as well (e.g. empty images when decompressing BC6H). It's better to save a dds and open
    // it in external tool.
    // WARNING !!! DO NOT ERASE THE COMMENTED CODE BELOW !!! I may eventually decide to compute PSNR or SSIM like the
    // Compressonator GUI does and this code will serve as a nice starting point.
    
//     CMP_Texture newSource;
//     memset(&newSource, 0, sizeof(newSource));
//     newSource.dwSize = sizeof(newSource);
//     newSource.dwWidth = destination.dwWidth;
//     newSource.dwHeight = destination.dwHeight;
//     newSource.dwPitch = 0;
//     newSource.format = destination.format;
//     newSource.dwDataSize = destination.dwDataSize;
//     newSource.pData = destination.pData;
//     
//     CMP_Texture newDestination;
//     memset(&newDestination, 0, sizeof(newDestination));
//     newDestination.dwSize = sizeof(newDestination);
//     newDestination.dwWidth = source.dwWidth;
//     newDestination.dwHeight = source.dwHeight;
//     newDestination.dwPitch = 0;
//     newDestination.format = CMP_FORMAT_RGBA_8888;
//     newDestination.dwDataSize = CMP_CalculateBufferSize(&newDestination);
//     newDestination.pData = (CMP_BYTE*)malloc(newDestination.dwDataSize);
//     
// //     LOG_D("sf" << newSource.format << "; df" << newDestination.format << "\n\t" <<
// //           "ds" << newDestination.dwDataSize << "; ss" << newSource.dwDataSize << "\n\t" <<
// //           "sw" << newDestination.dwWidth << "; sh" << newSource.dwHeight);
//     
//     CMP_CompressOptions options;
//     memset(&options, 0, sizeof(options));
//     options.dwSize = sizeof(options);
// 
//     const float quality = 1.0f;
//     const std::string qualityParam = std::to_string(quality);
// 
//     snprintf(options.CmdSet[0].strCommand, sizeof(AMD_CMD_SET::strCommand), "Quality");
//     snprintf(options.CmdSet[0].strParameter, sizeof(AMD_CMD_SET::strParameter), "%s", qualityParam.c_str());
//     snprintf(options.CmdSet[1].strCommand, sizeof(AMD_CMD_SET::strCommand), "ModeMask");
//     snprintf(options.CmdSet[1].strParameter, sizeof(AMD_CMD_SET::strParameter), "255");
//     snprintf(options.CmdSet[2].strCommand, sizeof(AMD_CMD_SET::strCommand), "NumThreads");
//     snprintf(options.CmdSet[2].strParameter, sizeof(AMD_CMD_SET::strParameter), "8");
//     options.NumCmds = 3;
// 
//     CMP_ERROR result;
//     result = CMP_ConvertTexture(&newSource, &newDestination, &options, nullptr, 0, 0);
//     if (result != CMP_OK) {
//         throw std::runtime_error("Debug conversion failed");
//     }
//     
//     fs::path debugPath2 = path.stem();
//     debugPath2 += "_";
//     debugPath2 += std::to_string(f);
//     debugPath2 += "_";
//     debugPath2 += std::to_string(l);
//     debugPath2 += "_conv";
//     debugPath2 += ".png";
//     
//     int writeResult = stbi_write_png(debugPath2.c_str(), newDestination.dwWidth, newDestination.dwHeight, 4, newDestination.pData, 0);
//     if (!writeResult) {
//         throw std::runtime_error("Failed to write a debug image");
//     }
//     
//     free(newDestination.pData);
    // WARNING !!! DO NOT ERASE THE COMMENTED CODE ABOVE !!! I may eventually decide to compute PSNR or SSIM like the
    // Compressonator GUI does and this code will serve as a nice starting point.
}

inline void writeMipMapImage(std::vector<TextureConverter::MipmapLevelData>& mipMapLevelData, const TextureConverterState& textureState, std::size_t face, std::size_t level, bool sRGBSource) {
    fs::path debugPath = textureState.getSourceFilePath().stem();
    debugPath += "_";
    debugPath += std::to_string(face);
    debugPath += "_";
    debugPath += std::to_string(level);
    debugPath += "_scaled_processed";
#ifdef IYF_DEBUG_IMAGE_OUTPUT_HDR
    debugPath += ".hdr";
    debugWriteImageHDR(debugPath, mipMapLevelData[level].data.get(), mipMapLevelData[level].width, mipMapLevelData[level].height, textureState.getChannels(), textureState.premultiplyAlpha);
#else // IYF_DEBUG_IMAGE_OUTPUT_HDR
    debugPath += ".png";
    debugWriteImage(debugPath, mipMapLevelData[level].data.get(), mipMapLevelData[level].width, mipMapLevelData[level].height, textureState.getChannels(), textureState.premultiplyAlpha, true, sRGBSource);
#endif // IYF_DEBUG_IMAGE_OUTPUT_HDR
}

class TextureConverterInternalState : public InternalConverterState {
public:
    TextureConverterInternalState(const Converter* converter) : InternalConverterState(converter), size(0) {}
    
    /// The contents of the file loaded to memory
    std::unique_ptr<char[]> rawData;
    /// The size of the loaded file
    std::size_t size;
};

CMP_BYTE* addAlphaAndPackToHalf(const float* data, std::size_t numberOfPixels) {
    const std::size_t newSize = numberOfPixels * 4;
//     float* newData = new float[newSize];
//     
//     for (std::size_t i = 0; i < numberOfPixels; ++i) {
//         newData[i * 4 + 0] = data[i * 3 + 0];
//         newData[i * 4 + 1] = data[i * 3 + 1];
//         newData[i * 4 + 2] = data[i * 3 + 2];
//         newData[i * 4 + 3] = 1.0f;
//         
// //         newData[i * 4 + 0] = 1.0f;
// //         newData[i * 4 + 1] = data[i * 3 + 0];
// //         newData[i * 4 + 2] = data[i * 3 + 1];
// //         newData[i * 4 + 3] = data[i * 3 + 2];
//         
// //         newData[i * 4 + 1] = 1.0f;
// //         newData[i * 4 + 2] = 1.0f;
// //         newData[i * 4 + 3] = 1.0f;
//     }
    std::uint16_t* newData = new std::uint16_t[newSize];
    
    for (std::size_t i = 0; i < numberOfPixels; ++i) {
        newData[i * 4 + 0] = glm::packHalf1x16(data[i * 3 + 0]);
        newData[i * 4 + 1] = glm::packHalf1x16(data[i * 3 + 1]);
        newData[i * 4 + 2] = glm::packHalf1x16(data[i * 3 + 2]);
        newData[i * 4 + 3] = 1.0f;
    }
    
    return reinterpret_cast<CMP_BYTE*>(newData);
}

// static CMP_BYTE* swizzzleComponents(const float* data, std::size_t numberOfPixels) {
//     const std::size_t newSize = numberOfPixels * 4;
//     float* newData = new float[newSize];
//     
//     /// RGBA data to ARGB newData
//     for (std::size_t i = 0; i < numberOfPixels; ++i) {
//         newData[i * 4 + 0] = data[i * 4 + 3];
//         newData[i * 4 + 1] = data[i * 4 + 0];
//         newData[i * 4 + 2] = data[i * 4 + 1];
//         newData[i * 4 + 3] = data[i * 4 + 2];
//     }
//     
//     return reinterpret_cast<CMP_BYTE*>(newData);
// }

enum class CompressorName {
    Compressonator
};

CompressorName determineCompressorForPlatform(PlatformIdentifier platformID) {
    switch (platformID) {
        case PlatformIdentifier::Windows_Desktop_x86_64:
        case PlatformIdentifier::Linux_Desktop_x86_64:
            return CompressorName::Compressonator;
        case PlatformIdentifier::Android_Handheld_ARM64_v8a:
        case PlatformIdentifier::Android_Handheld_ARM_v7a:
        case PlatformIdentifier::Android_Handheld_x86:
        case PlatformIdentifier::Android_Handheld_x86_64:
            // TODO add support for ETC textures and Android
            throw std::runtime_error("Android texture compression hasn't been implemented yet.");
        case PlatformIdentifier::COUNT:
            throw std::runtime_error("COUNT is not a valid platformID");
    }
    
    throw std::runtime_error("Failed to determine the compressor.");
}

CMP_FORMAT determineCompressonatorFormat(const TextureConverterState& textureState) {
    // Best summary of BC formats: http://www.reedbeta.com/blog/understanding-bcn-texture-compression-formats/
    // TLDR:
    // BC1 - Color and cutout maps (1 bit alpha). I don't ever use the 1 bit alpha in this engine.
    // BC2 - Unused.
    // BC3 - Color + full alpha. Packing of color and alpha maps together.
    // BC4 - Single Channel (grayscale)
    // BC5 - 2 grayscale channels. Used for normal maps.
    // BC6 - half float HDR images
    // BC7 - high quality color + alpha
    const TextureImportMode importMode = textureState.importMode;
    switch (importMode) {
        case TextureImportMode::Regular:
            return (textureState.getChannels() == 4) ? CMP_FORMAT::CMP_FORMAT_BC3 : CMP_FORMAT::CMP_FORMAT_BC1;
        case TextureImportMode::NormalMap:
            return CMP_FORMAT::CMP_FORMAT_BC5;
        case TextureImportMode::HighQuality:
            return CMP_FORMAT::CMP_FORMAT_BC7; // Unlike with Regular, no check because BC7 has optional alpha support
        case TextureImportMode::HDR:
            return CMP_FORMAT::CMP_FORMAT_BC6H;
        case TextureImportMode::SingleChannel:
            return CMP_FORMAT::CMP_FORMAT_BC4;
        case TextureImportMode::COUNT:
            throw std::runtime_error("Invalid TextureImportMode");
    }
    
    throw std::runtime_error("Failed to determine an appropriate compression format.");
}

std::size_t computeMipMapLevelCount(TextureConverterState& state) {
    if (state.noMipMaps) {
        return 1;
    } else {
        if (state.isCubemap()) {
            const std::size_t logVal = std::log2(state.getHeight());
            return logVal + 1;
        } else {
            const std::size_t minDimension = std::min(state.getWidth(), state.getHeight());
            const std::size_t logVal = std::log2(minDimension);
            return logVal + 1;
        }
    }
}

TextureConverter::MipmapLevelData extractFace(const float* source, std::size_t width, std::size_t height, std::size_t channels, std::size_t faceCount, std::size_t face) {
    const std::size_t newWidth = width / faceCount;
    const std::size_t newSize = newWidth * height * channels;
    const std::size_t offset = newWidth * channels;
    
//     LOG_D(newWidth << " " << height << " " << newSize << " " << channels << " " << offset);
    
    std::unique_ptr<float[]> data(new float[newSize]);
    
//     std::stringstream ss;
    for (std::size_t i = 0; i < height; ++i) {
        const std::size_t sourceOffset = (i * width * channels) + (offset * face);
        const std::size_t destinationOffset = i * newWidth * channels;
        
//         ss << "\n\t" << i << " " << sourceOffset << " " << destinationOffset << " cc " << newWidth * channels * sizeof(float);
        std::memcpy(data.get() + destinationOffset, source + sourceOffset, newWidth * channels * sizeof(float));
    }
//     LOG_D("Offsets " << ss.str() << "\n\t" << newSize);
    
    TextureConverter::MipmapLevelData result;
    result.width = newWidth;
    result.height = height;
    result.data = std::move(data);
    return result;
}

void throwCompressonatorError(CMP_ERROR result) {
    std::stringstream ss;
    ss << "Compressonator reported an error: ";
    
    switch (result) {
        case CMP_ABORTED:
            ss << "The conversion was aborted.";
            break;
        case CMP_ERR_INVALID_SOURCE_TEXTURE:
            ss << "The source texture is invalid.";
            break;
        case CMP_ERR_INVALID_DEST_TEXTURE:
            ss << "The destination texture is invalid.";
            break;
        case CMP_ERR_UNSUPPORTED_SOURCE_FORMAT:
            ss << "The source format is not a supported format.";
            break;
        case CMP_ERR_UNSUPPORTED_DEST_FORMAT:
            ss << "The destination format is not a supported format.";
            break;
        case CMP_ERR_UNSUPPORTED_GPU_ASTC_DECODE:
            ss << "The gpu hardware is not supported.";
            break;
        case CMP_ERR_SIZE_MISMATCH:
            ss << "The source and destination texture sizes do not match.";
            break;
        case CMP_ERR_UNABLE_TO_INIT_CODEC:
            ss << "Compressonator was unable to initialize the codec needed for conversion.";
            break;
        case CMP_ERR_UNABLE_TO_INIT_DECOMPRESSLIB:
            ss << "GPU_Decode Lib was unable to initialize the codec needed for decompression.";
            break;
        case CMP_ERR_UNABLE_TO_INIT_COMPUTELIB:
            ss << "Compute Lib was unable to initialize the codec needed for compression.";
            break;
        case CMP_ERR_GENERIC:
            ss << "An unknown error occurred.";
            break;
        case CMP_OK:
            // Just to silence the warning. Don't use default here. Compressonator devs may add more codes
            // and having the warnings appear in that case would be useful.
            ss << "The universe has just imploded";
            break;
    }
    
    LOG_E(ss.str());
    throw std::runtime_error(ss.str());
}

bool TextureConverter::writeMipmapLevel(Serializer& serializer, std::uint8_t* bytes, std::size_t count) const {
    return (serializer.writeBytes(bytes, count) == static_cast<std::int64_t>(count));
}


TextureCompressionFormat TextureConverter::compressonatorDetermineFormat(const TextureConverterState& textureState) const {
    return CompressonatorToEngineFormat(determineCompressonatorFormat(textureState));
}

bool TextureConverter::compressonatorCompress(Serializer& serializer, std::vector<MipmapLevelData>& mipMapLevelData, const TextureConverterState& textureState,
                                              std::size_t face, std::size_t level) const {
    const CMP_FORMAT format = determineCompressonatorFormat(textureState);//CMP_FORMAT_BC6H;//CMP_FORMAT_BC7;//
    const bool sourceDataHDR = (format == CMP_FORMAT_BC6H || format == CMP_FORMAT_BC6H_SF);
    const bool needToSwizzleRB = !(format == CMP_FORMAT_BC7 || format == CMP_FORMAT_BC6H || format == CMP_FORMAT_BC6H_SF);
    
    if (sourceDataHDR && textureState.getChannels() != 3) {
        LOG_W("Cannot convert " << textureState.getSourceFilePath() << ". HDR images must have exactly 3 channels");
        return false;
    }
    
    // Compress the current mipmap level
    CMP_Texture sourceTexture;
    memset(&sourceTexture, 0, sizeof(sourceTexture));
    sourceTexture.dwSize = sizeof(sourceTexture);
    sourceTexture.dwWidth = mipMapLevelData[level].width;
    sourceTexture.dwHeight = mipMapLevelData[level].height;
    sourceTexture.dwPitch = 0;
    
    //float result = hdr * 255.0f + 0.5f;
    //result = std::clamp(result, 0.0f, 255.0f);
    
    if (!sourceDataHDR) {
        // We turn floats into uint8s ourselves. The Compressonator SDK can remap HDR data to LDR, however, it expects actual HDR images
        // and messes up the colors.
        // By the way, Compressonator's BC7 compressor seems to assert when compressing small mipmap levels (2x2) if it is provided RGB data.
        // This is why we make a 4 channel buffer.
        std::size_t channelCount = (textureState.getChannels() > 2) ? 4 : textureState.getChannels();
        sourceTexture.dwDataSize = mipMapLevelData[level].width * mipMapLevelData[level].height * channelCount * sizeof(std::uint8_t);
        
        const float* source = mipMapLevelData[level].data.get();
        std::uint8_t* destination = new std::uint8_t[sourceTexture.dwDataSize];
        std::size_t numberOfPixels = mipMapLevelData[level].width * mipMapLevelData[level].height;
        
        // If the image has to be linear (e.g. a normal map), don't turn it into sRGB before compression
        const bool sRGB = textureState.sRGBSource;
        switch (textureState.getChannels()) {
            case 1:
                sourceTexture.format = CMP_FORMAT::CMP_FORMAT_R_8;
                for (std::size_t i = 0; i < numberOfPixels; ++i) {
                    destination[i] = sRGB ? ColorToLDR(source[i]) : AlphaToLDR(source[i]);
                }
                break;
            case 2:
                sourceTexture.format = CMP_FORMAT::CMP_FORMAT_RG_8;
                for (std::size_t i = 0; i < numberOfPixels; ++i) {
                    destination[i * 2 + 0] = sRGB ? ColorToLDR(source[i * 2 + 0]) : AlphaToLDR(source[i * 2 + 0]);
                    destination[i * 2 + 1] = sRGB ? ColorToLDR(source[i * 2 + 1]) : AlphaToLDR(source[i * 2 + 1]);
                }
                break;
            case 3:
                if (needToSwizzleRB) {
                    sourceTexture.format = CMP_FORMAT::CMP_FORMAT_BGRA_8888;
                    for (std::size_t i = 0; i < numberOfPixels; ++i) {
                        destination[i * 4 + 0] = sRGB ? ColorToLDR(source[i * 3 + 2]) : AlphaToLDR(source[i * 3 + 2]);
                        destination[i * 4 + 1] = sRGB ? ColorToLDR(source[i * 3 + 1]) : AlphaToLDR(source[i * 3 + 1]);
                        destination[i * 4 + 2] = sRGB ? ColorToLDR(source[i * 3 + 0]) : AlphaToLDR(source[i * 3 + 0]);
                        destination[i * 4 + 3] = 255;
                    }
                } else {
                    sourceTexture.format = CMP_FORMAT::CMP_FORMAT_RGBA_8888;
                    for (std::size_t i = 0; i < numberOfPixels; ++i) {
                        destination[i * 4 + 0] = sRGB ? ColorToLDR(source[i * 3 + 0]) : AlphaToLDR(source[i * 3 + 0]);
                        destination[i * 4 + 1] = sRGB ? ColorToLDR(source[i * 3 + 1]) : AlphaToLDR(source[i * 3 + 1]);
                        destination[i * 4 + 2] = sRGB ? ColorToLDR(source[i * 3 + 2]) : AlphaToLDR(source[i * 3 + 2]);
                        destination[i * 4 + 3] = 255;
                    }
                }
                break;
            case 4:
                if (needToSwizzleRB) {sourceTexture.format = CMP_FORMAT::CMP_FORMAT_RGBA_8888;
                    for (std::size_t i = 0; i < numberOfPixels; ++i) {
                        destination[i * 4 + 0] = sRGB ? ColorToLDR(source[i * 4 + 2]) : AlphaToLDR(source[i * 4 + 2]);
                        destination[i * 4 + 1] = sRGB ? ColorToLDR(source[i * 4 + 1]) : AlphaToLDR(source[i * 4 + 1]);
                        destination[i * 4 + 2] = sRGB ? ColorToLDR(source[i * 4 + 0]) : AlphaToLDR(source[i * 4 + 0]);
                        destination[i * 4 + 3] = AlphaToLDR(source[i * 4 + 3]);
                    }
                } else {
                    sourceTexture.format = CMP_FORMAT::CMP_FORMAT_RGBA_8888;
                    for (std::size_t i = 0; i < numberOfPixels; ++i) {
                        destination[i * 4 + 0] = sRGB ? ColorToLDR(source[i * 4 + 0]) : AlphaToLDR(source[i * 4 + 0]);
                        destination[i * 4 + 1] = sRGB ? ColorToLDR(source[i * 4 + 1]) : AlphaToLDR(source[i * 4 + 1]);
                        destination[i * 4 + 2] = sRGB ? ColorToLDR(source[i * 4 + 2]) : AlphaToLDR(source[i * 4 + 2]);
                        destination[i * 4 + 3] = AlphaToLDR(source[i * 4 + 3]);
                    }
                }
                break;
            default:
                throw std::runtime_error("Unknown number of channels in image");
        }
        
        sourceTexture.pData = destination;
    } else {
        // Compressonator asserts on some formats, that's why we manually convert to half float an append an alpha channel (CMP_FORMAT_ARGB_16F)
        sourceTexture.pData = addAlphaAndPackToHalf(mipMapLevelData[level].data.get(), mipMapLevelData[level].width * mipMapLevelData[level].height);
        sourceTexture.dwDataSize = mipMapLevelData[level].width * mipMapLevelData[level].height * 4 * sizeof(float); // We added an extra channel
        sourceTexture.format = CMP_FORMAT::CMP_FORMAT_ARGB_16F;
    }
    
    CMP_Texture destinationTexture;
    memset(&destinationTexture, 0, sizeof(destinationTexture));
    destinationTexture.dwSize = sizeof(destinationTexture);
    destinationTexture.dwWidth = mipMapLevelData[level].width;
    destinationTexture.dwHeight = mipMapLevelData[level].height;
    destinationTexture.dwPitch = 0;
    destinationTexture.format = format;
    destinationTexture.dwDataSize = CMP_CalculateBufferSize(&destinationTexture);
    destinationTexture.pData = (CMP_BYTE*)malloc(destinationTexture.dwDataSize);
    
    if (textureState.isDebugOutputRequested()) {
        LOG_V("Compressing a texture: " << textureState.getSourceFilePath() <<
              "\n\tFace: " << face <<
              "\n\tLevel: " << level <<
              "\n\tDimensions: " << destinationTexture.dwWidth << "x" << destinationTexture.dwHeight <<
              "\n\tFormat ID (is sRGB): " << static_cast<std::size_t>(CompressonatorToEngineFormat(format)) << " (" << (textureState.sRGBSource ? "true)" : "false)") << 
              "\n\tDetermined size (expected size): " << destinationTexture.dwDataSize << 
              " (" << con::CompressedTextureMipmapLevelSize(CompressonatorToEngineFormat(format), destinationTexture.dwWidth, destinationTexture.dwHeight) << ")");
    }
    assert(destinationTexture.dwDataSize == con::CompressedTextureMipmapLevelSize(CompressonatorToEngineFormat(format), destinationTexture.dwWidth, destinationTexture.dwHeight));
    
    CMP_CompressOptions options;
    memset(&options, 0, sizeof(options));
    options.dwSize = sizeof(options);

    const float quality = textureState.quality;
    const std::string qualityParam = std::to_string(quality);
    
    snprintf(options.CmdSet[0].strCommand, sizeof(AMD_CMD_SET::strCommand), "Quality");
    snprintf(options.CmdSet[0].strParameter, sizeof(AMD_CMD_SET::strParameter), "%s", qualityParam.c_str());
    snprintf(options.CmdSet[1].strCommand, sizeof(AMD_CMD_SET::strCommand), "ModeMask");
    snprintf(options.CmdSet[1].strParameter, sizeof(AMD_CMD_SET::strParameter), (textureState.getChannels() == 4 ? "255" : "207"));
    snprintf(options.CmdSet[2].strCommand, sizeof(AMD_CMD_SET::strCommand), "NumThreads");
    snprintf(options.CmdSet[2].strParameter, sizeof(AMD_CMD_SET::strParameter), "8");
    options.NumCmds = 3;
    
    CMP_ERROR result = CMP_ConvertTexture(&sourceTexture, &destinationTexture, &options, nullptr, 0, 0);
    if (result != CMP_OK) {
        throwCompressonatorError(result);
    }

    if (textureState.isDebugOutputRequested()) {
        saveCompressedImage(sourceTexture, destinationTexture, textureState.getSourceFilePath(), face, level, needToSwizzleRB);
    }
    
    // Serialize the current converted mipmap layer
    bool writeResult = writeMipmapLevel(serializer, destinationTexture.pData, destinationTexture.dwDataSize);
    
    // Free the intermediate data buffer and the final compressed result
    delete[] sourceTexture.pData;
    free(destinationTexture.pData);
    return writeResult;
}

TextureConverter::TextureConverter(const ConverterManager* manager) : Converter(manager) {
    if (CMP_InitializeBCLibrary() != BC_ERROR_NONE) {
        throw std::runtime_error("Failed to initialize the block compression library");
    }
}

TextureConverter::~TextureConverter() {
    CMP_ShutdownBCLibrary();
}

std::unique_ptr<ConverterState> TextureConverter::initializeConverter(const fs::path& inPath, PlatformIdentifier platformID) const {
     std::unique_ptr<TextureConverterInternalState> internalState = std::make_unique<TextureConverterInternalState>(this);
    
    File inFile(inPath, File::OpenMode::Read);
    auto data = inFile.readWholeFile();
    
    internalState->rawData = std::move(data.first);
    internalState->size = data.second;
    
    stbi_uc* ucData = reinterpret_cast<stbi_uc*>(internalState->rawData.get());
    const FileHash sourceFileHash = HF(internalState->rawData.get(), internalState->size);
    
    int x = 0;
    int y = 0;
    int channels = 0;
    
    if (!stbi_info_from_memory(ucData, internalState->size, &x, &y, &channels)) {
        LOG_W("stb_image failed to extract information from " << inPath.generic_string());
        return nullptr;
    }
    const bool sourceDataHDR = stbi_is_hdr_from_memory(ucData, internalState->size);
    
    if (x <= 0 || y <= 0) {
        LOG_W("Cannot import a invalid texture from " << inPath.generic_string());
        return nullptr;
    }
    
    auto state = std::unique_ptr<TextureConverterState>(new TextureConverterState(platformID, std::move(internalState), inPath, sourceFileHash));
    state->sourceDataHDR = sourceDataHDR;
    state->width = x;
    state->height = y;
    state->channels = channels;
    
    const std::size_t width = x;
    const std::size_t height = y;
    if (isPowerOfTwo(width) && isPowerOfTwo(height)) {
        state->cubemap = false;
        LOG_V("Importing a regular power of two texture: " << width << "x" << height << "; channels: " << channels);
    } else if (isPowerOfTwo(width / 6) && isPowerOfTwo(height) && (width / 6 == height)) {
        state->cubemap = true;
        LOG_V("Importing a cubemap texture: " << width << "x" << height << "; channels: " << channels);
    } else {
        LOG_W("Can only import textures with power of two dimensions or cubemaps where (width / 6 == height) && isPowerOfTwo(width / 6) && isPowerOfTwo(height)");
        return nullptr;
    }
    
    if (channels == 1) {
        state->importMode = TextureImportMode::SingleChannel;
    } else if (sourceDataHDR) {
        state->importMode = TextureImportMode::HDR;
    } else {
        state->importMode = TextureImportMode::Regular;
    }
    
    //state->premultiplyAlpha = false;
    
    return state;
}

using ImageDataPtr = std::unique_ptr<float[], std::function<void(float*)>>;
bool TextureConverter::convert(ConverterState& state) const {
    TextureConverterState& textureState = dynamic_cast<TextureConverterState&>(state);
    TextureConverterInternalState* internalState = dynamic_cast<TextureConverterInternalState*>(state.getInternalState());
    assert(internalState != nullptr);
    
    int x = 0;
    int y = 0;
    int c = 0;
    stbi_uc* ucData = reinterpret_cast<stbi_uc*>(internalState->rawData.get());
    
    ImageDataPtr imageData = nullptr;
    if (textureState.sRGBSource || textureState.sourceDataHDR) {
        // We will need to process the textures further (alpha premultiply, resize for mip-maps, etc.). To avoid
        // precision issues and to perform the operations in linear space instead of sRGB (as is required), we need
        // to load all images - LDR or HDR - as linear floating point values.
        imageData = ImageDataPtr(stbi_loadf_from_memory(ucData, internalState->size, &x, &y, &c, 0), [](float* p){stbi_image_free(p);});
        
        if (imageData == nullptr) {
            LOG_W("stb_image failed to load the image from " << state.getSourceFilePath().generic_string());
            return false;
        }
        
        assert(x == textureState.width);
        assert(y == textureState.height);
        assert(c == textureState.channels);
    } else {
        // Some sRGB images may store linear data (e.g., a normal map saved as a PNG file). We cannot use
        // stbi_loadf_from_memory beacause it would try to linearize the data that's already linear. stb_image
        // has the stbi_ldr_to_hdr_gamma function, that, in theory, can be passed 1.0f. Unfortunately, it sets
        // a static global variable and (as of this writing) is not thread safe. Therefore, to avoid conversion,
        // we have to read the file using the regular LRD interface and promote the data to floating point ourselves.
        stbi_uc* temp = stbi_load_from_memory(ucData, internalState->size, &x, &y, &c, 0);
        
        if (temp == nullptr) {
            LOG_W("stb_image failed to load the image from " << state.getSourceFilePath().generic_string());
            return false;
        }
        
        assert(x == textureState.width);
        assert(y == textureState.height);
        assert(c == textureState.channels);
        
        const std::size_t elementCount = x * y * c;
        
        imageData = ImageDataPtr(new float[elementCount], [](float* p){delete[] p;});
        
        for (std::size_t i = 0; i < elementCount; ++i) {
            imageData[i] = temp[i] / 255.0f;
        }
        
        stbi_image_free(temp);
    }
    
    const std::size_t faceCount = textureState.isCubemap() ? 6 : 1;
    const std::size_t mipmapLevels = computeMipMapLevelCount(textureState);
    const CompressorName compressor = determineCompressorForPlatform(state.getPlatformIdentifier());
    TextureCompressionFormat compressionFormat = TextureCompressionFormat::NotCompressed;
    switch (compressor) {
        case CompressorName::Compressonator:
            compressionFormat = compressonatorDetermineFormat(textureState);
            break;
    }
    
    assert(compressionFormat != TextureCompressionFormat::NotCompressed);
    
    const std::size_t topLevelWidth = textureState.width / faceCount;
    const std::size_t topLevelHeight = textureState.height;
    
    LOG_D("TLWH: " << topLevelWidth << " " << topLevelHeight);
    
    // Used to store the data received from the compressor
    MemorySerializer fw(1024 * 1024 * 8);
    
    // Magic number
    fw.writeUInt8('I');
    fw.writeUInt8('Y');
    fw.writeUInt8('F');
    fw.writeUInt8('T');
    
    fw.writeUInt8(1); // Version
    fw.writeUInt8(faceCount);
    fw.writeUInt8(textureState.channels);
    fw.writeUInt8(mipmapLevels);
    
    fw.writeUInt32(topLevelWidth);
    fw.writeUInt32(topLevelHeight);
    fw.writeUInt32(1); // Depth
    fw.writeUInt32(1); // Layers
    
    fw.writeUInt16(static_cast<std::uint16_t>(compressionFormat));
    fw.writeUInt8(textureState.sRGBSource);
    fw.writeUInt8(0); // Reserved
    
    std::uint64_t headerSize = fw.size();
    
    // For each face. Unless we're processing a cubemap, this will always run once.
    for (std::size_t face = 0; face < faceCount; ++face) {
        std::vector<MipmapLevelData> mipMapLevelData;
        
        // First of all, we "extract the face". Usually, this makes an unnecessary copy (I should optimize it away), however, when the 
        // image is a cubemap, this does exactly what the function's name says.
        mipMapLevelData.push_back(extractFace(imageData.get(), textureState.width, textureState.height, textureState.channels, faceCount, face));
        
        // Next, we premultiply alpha (if the settings tell us to).
        if (textureState.channels == 4 && textureState.premultiplyAlpha) {
            const std::size_t pixelCount = mipMapLevelData[0].width * mipMapLevelData[0].height;
            
            for (std::size_t i = 0; i < pixelCount; i += 4) {
                // IYF_ALPHA_EPSILON is based on STBIR_ALPHA_EPSILON. It is used to prevent the zeroing out of color values
                const float alpha = mipMapLevelData[0].data[i * 4 + 3] + IYF_ALPHA_EPSILON;
                
                mipMapLevelData[0].data[i * 4 + 0] *= alpha;
                mipMapLevelData[0].data[i * 4 + 1] *= alpha;
                mipMapLevelData[0].data[i * 4 + 2] *= alpha;
            }
        }
        
        // Next, we build the mipmap chain. If mipmaps have been disabled, mipmapLevels will be set to 1 and only the main image will
        // be processed.
        for (std::size_t level = 0; level < mipmapLevels; ++level) {
            // Compute the next mipmap level (if any remain)
            if (level != mipmapLevels - 1) {
                const std::size_t newWidth = mipMapLevelData[level].width / 2;
                const std::size_t newHeight = mipMapLevelData[level].height / 2;
                
                std::unique_ptr<float[]> buffer(new float[newWidth * newHeight * textureState.channels]);
                mipMapLevelData.emplace_back(newWidth, newHeight, std::move(buffer));
                
                const std::size_t alphaChannelID = (textureState.channels == 4) ? 3 : STBIR_ALPHA_CHANNEL_NONE;
                const int alphaFlags = (textureState.channels == 4 && textureState.premultiplyAlpha) ? STBIR_FLAG_ALPHA_PREMULTIPLIED : 0;
                
                // TODO expose the choice of the filter
                const int result = stbir_resize_float_generic(mipMapLevelData[level].data.get(), mipMapLevelData[level].width, mipMapLevelData[level].height, 0,
                                                              mipMapLevelData[level + 1].data.get(), mipMapLevelData[level + 1].width, mipMapLevelData[level + 1].height, 0,
                                                              textureState.channels, alphaChannelID, alphaFlags,
                                                              STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR, nullptr);
                
                if (!result) {
                    throw std::runtime_error("MipMap generation critical failure.");
                }
            }
            
            // ---------- Perform additional adjustments in linear space here (if needed) ----------
            
            // --------------------------- End of additional adjustments ---------------------------
            
            if (textureState.isDebugOutputRequested()) {
                writeMipMapImage(mipMapLevelData, textureState, face, level, (textureState.sRGBSource || textureState.sourceDataHDR));
            }
            
            // Use the appropriate compressor to compress the face/level for the chosen platform
            bool compressionResult = true;
            
            switch (compressor) {
                case CompressorName::Compressonator:
                    compressionResult = compressonatorCompress(fw, mipMapLevelData, textureState, face, level);
                    break;
            }
            
            if (!compressionResult) {
                return false;
            }
            
        }
    }
    
    const fs::path outputPath = manager->makeFinalPathForAsset(state.getSourceFilePath(), state.getType(), state.getPlatformIdentifier());
    
    std::uint64_t fullSize = fw.size();
    std::uint64_t dataSize = fullSize - headerSize;
    
    File textureFile(outputPath, File::OpenMode::Write);
    textureFile.writeBytes(fw.data(), fw.size());
    textureFile.close();
    
    FileHash fileHash = HF(fw.data(), fw.size());
    TextureMetadata textureMetadata(fileHash, textureState.getSourceFilePath(), textureState.getSourceFileHash(), state.isSystemAsset(), state.getTags(), topLevelWidth, topLevelHeight, 1, faceCount, 1,
                                    mipmapLevels, textureState.channels, textureState.filteringMethod, textureState.xTiling, textureState.yTiling, textureState.preferredAnisotropy,
                                    compressionFormat, textureState.sRGBSource, dataSize);
    ImportedAssetData iadTexture(AssetType::Texture, textureMetadata, outputPath);
    state.getImportedAssets().push_back(iadTexture);
    
    return true;
}
}
}
