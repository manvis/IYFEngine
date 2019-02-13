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

#include "graphics/GraphicsAPI.hpp"

#include "utilities/ConstantMapper.hpp"
#include "assets/loaders/TextureLoader.hpp"
#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "core/configuration/Configuration.hpp"
// Needed to fetch the localized window name
#include "localization/TextLocalization.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <SDL_syswm.h>

#include <glm/glm.hpp>

namespace iyf {

ConstantMapper<Format, std::string, static_cast<size_t>(Format::COUNT)> formatName = {
    "Undefined",
    "R4_G4_uNorm_pack8",
    "R4_G4_B4_A4_uNorm_pack16",
    "B4_G4_R4_A4_uNorm_pack16",
    "R5_G6_B5_uNorm_pack16",
    "B5_G6_R5_uNorm_pack16",
    "R5_G5_B5_A1_uNorm_pack16",
    "B5_G5_R5_A1_uNorm_pack16",
    "A1_R5_G5_B5_uNorm_pack16",
    "R8_uNorm",
    "R8_sNorm",
    "R8_uScaled",
    "R8_sScaled",
    "R8_uInt",
    "R8_sInt",
    "R8_sRGB",
    "R8_G8_uNorm",
    "R8_G8_sNorm",
    "R8_G8_uScaled",
    "R8_G8_sScaled",
    "R8_G8_uInt",
    "R8_G8_sInt",
    "R8_G8_sRGB",
    "R8_G8_B8_uNorm",
    "R8_G8_B8_sNorm",
    "R8_G8_B8_uScaled",
    "R8_G8_B8_sScaled",
    "R8_G8_B8_uInt",
    "R8_G8_B8_sInt",
    "R8_G8_B8_sRGB",
    "B8_G8_R8_uNorm",
    "B8_G8_R8_sNorm",
    "B8_G8_R8_uScaled",
    "B8_G8_R8_sScaled",
    "B8_G8_R8_uInt",
    "B8_G8_R8_sInt",
    "B8_G8_R8_sRGB",
    "R8_G8_B8_A8_uNorm",
    "R8_G8_B8_A8_sNorm",
    "R8_G8_B8_A8_uScaled",
    "R8_G8_B8_A8_sScaled",
    "R8_G8_B8_A8_uInt",
    "R8_G8_B8_A8_sInt",
    "R8_G8_B8_A8_sRGB",
    "B8_G8_R8_A8_uNorm",
    "B8_G8_R8_A8_sNorm",
    "B8_G8_R8_A8_uScaled",
    "B8_G8_R8_A8_sScaled",
    "B8_G8_R8_A8_uInt",
    "B8_G8_R8_A8_sInt",
    "B8_G8_R8_A8_sRGB",
    "A8_B8_G8_R8_uNorm_pack32",
    "A8_B8_G8_R8_sNorm_pack32",
    "A8_B8_G8_R8_uScaled_pack32",
    "A8_B8_G8_R8_sScaled_pack32",
    "A8_B8_G8_R8_uInt_pack32",
    "A8_B8_G8_R8_sInt_pack32",
    "A8_B8_G8_R8_sRGB_pack32",
    "A2_R10_G10_B10_uNorm_pack32",
    "A2_R10_G10_B10_sNorm_pack32",
    "A2_R10_G10_B10_uScaled_pack32",
    "A2_R10_G10_B10_sScaled_pack32",
    "A2_R10_G10_B10_uInt_pack32",
    "A2_R10_G10_B10_sInt_pack32",
    "A2_B10_G10_R10_uNorm_pack32",
    "A2_B10_G10_R10_sNorm_pack32",
    "A2_B10_G10_R10_uScaled_pack32",
    "A2_B10_G10_R10_sScaled_pack32",
    "A2_B10_G10_R10_uInt_pack32",
    "A2_B10_G10_R10_sInt_pack32",
    "R16_uNorm",
    "R16_sNorm",
    "R16_uScaled",
    "R16_sScaled",
    "R16_uInt",
    "R16_sInt",
    "R16_sFloat",
    "R16_G16_uNorm",
    "R16_G16_sNorm",
    "R16_G16_uScaled",
    "R16_G16_sScaled",
    "R16_G16_uInt",
    "R16_G16_sInt",
    "R16_G16_sFloat",
    "R16_G16_B16_uNorm",
    "R16_G16_B16_sNorm",
    "R16_G16_B16_uScaled",
    "R16_G16_B16_sScaled",
    "R16_G16_B16_uInt",
    "R16_G16_B16_sInt",
    "R16_G16_B16_sFloat",
    "R16_G16_B16_A16_uNorm",
    "R16_G16_B16_A16_sNorm",
    "R16_G16_B16_A16_uScaled",
    "R16_G16_B16_A16_sScaled",
    "R16_G16_B16_A16_uInt",
    "R16_G16_B16_A16_sInt",
    "R16_G16_B16_A16_sFloat",
    "R32_uInt",
    "R32_sInt",
    "R32_sFloat",
    "R32_G32_uInt",
    "R32_G32_sInt",
    "R32_G32_sFloat",
    "R32_G32_B32_uInt",
    "R32_G32_B32_sInt",
    "R32_G32_B32_sFloat",
    "R32_G32_B32_A32_uInt",
    "R32_G32_B32_A32_sInt",
    "R32_G32_B32_A32_sFloat",
    "R64_uInt",
    "R64_sInt",
    "R64_sFloat",
    "R64_G64_uInt",
    "R64_G64_sInt",
    "R64_G64_sFloat",
    "R64_G64_B64_uInt",
    "R64_G64_B64_sInt",
    "R64_G64_B64_sFloat",
    "R64_G64_B64_A64_uInt",
    "R64_G64_B64_A64_sInt",
    "R64_G64_B64_A64_sFloat",
    "B10_G11_R11_uFloat_pack32",
    "E5_B9_G9_R9_uFloat_pack32",
    "D16_uNorm",
    "X8_D24_uNorm_pack32",
    "D32_sFloat",
    "S8_uInt",
    "D16_uNorm_S8_uInt",
    "D24_uNorm_S8_uInt",
    "D32_sFloat_S8_uInt",
    "BC1_RGB_uNorm_block",
    "BC1_RGB_sRGB_block",
    "BC1_RGBA_uNorm_block",
    "BC1_RGBA_sRGB_block",
    "BC2_uNorm_block",
    "BC2_sRGB_block",
    "BC3_uNorm_block",
    "BC3_sRGB_block",
    "BC4_uNorm_block",
    "BC4_sNorm_block",
    "BC5_uNorm_block",
    "BC5_sNorm_block",
    "BC6H_uFloat_block",
    "BC6H_sFloat_block",
    "BC7_uNorm_block",
    "BC7_sRGB_block",
    "ETC2_R8_G8_B8_uNorm_block",
    "ETC2_R8_G8_B8_sRGB_block",
    "ETC2_R8_G8_B8_A1_uNorm_block",
    "ETC2_R8_G8_B8_A1_sRGB_block",
    "ETC2_R8_G8_B8_A8_uNorm_block",
    "ETC2_R8_G8_B8_A8_sRGB_block",
    "EAC_R11_uNorm_block",
    "EAC_R11_sNorm_block",
    "EAC_R11_G11_uNorm_block",
    "EAC_R11_G11_sNorm_block",
    "ASTC_4x4_uNorm_block",
    "ASTC_4x4_sRGB_block",
    "ASTC_5x4_uNorm_block",
    "ASTC_5x4_sRGB_block",
    "ASTC_5x5_uNorm_block",
    "ASTC_5x5_sRGB_block",
    "ASTC_6x5_uNorm_block",
    "ASTC_6x5_sRGB_block",
    "ASTC_6x6_uNorm_block",
    "ASTC_6x6_sRGB_block",
    "ASTC_8x5_uNorm_block",
    "ASTC_8x5_sRGB_block",
    "ASTC_8x6_uNorm_block",
    "ASTC_8x6_sRGB_block",
    "ASTC_8x8_uNorm_block",
    "ASTC_8x8_sRGB_block",
    "ASTC_10x5_uNorm_block",
    "ASTC_10x5_sRGB_block",
    "ASTC_10x6_uNorm_block",
    "ASTC_10x6_sRGB_block",
    "ASTC_10x8_uNorm_block",
    "ASTC_10x8_sRGB_block",
    "ASTC_10x10_uNorm_block",
    "ASTC_10x10_sRGB_block",
    "ASTC_12x10_uNorm_block",
    "ASTC_12x10_sRGB_block",
    "ASTC_12x12_uNorm_block",
    "ASTC_12x12_sRGB_block",
    "PVRTC1_2BPP_uNorm_block",
    "PVRTC1_4BPP_uNorm_block",
    "PVRTC2_2BPP_uNorm_block",
    "PVRTC2_4BPP_uNorm_block",
    "PVRTC1_2BPP_sRGB_block",
    "PVRTC1_4BPP_sRGB_block",
    "PVRTC2_2BPP_sRGB_block",
    "PVRTC2_4BPP_sRGB_block",
};

void GraphicsAPI::openWindow() {
    SDL_DisplayMode mode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
    int numVideoDisplays = SDL_GetNumVideoDisplays();

    LOG_V("Found {} screen(s)", numVideoDisplays);
    
    bool dumpScreenResolutions = config->getValue(ConfigurationValueHandle(HS("dumpScreenResolutions"), ConfigurationValueNamespace::Engine));
    if (dumpScreenResolutions) {
        std::stringstream ss;
        
        for (int i = 0; i < numVideoDisplays; ++i) {
            ss << "SCREEN: " << i << "\n\t";
            for (int j = 0; j < SDL_GetNumDisplayModes(i); ++j) {
                SDL_GetDisplayMode(i, j, &mode);
                ss << "\tWidth: " << mode.w << "\n"
                   << "\tHeight: " << mode.h << "\n"
                   << "\tRefresh rate: " << mode.refresh_rate << "\n"
                   << "\tPixel mode: " << SDL_BITSPERPIXEL(mode.format) << " " << SDL_GetPixelFormatName(mode.format);
            }
        }
        
        LOG_V("{}", ss.str());
    }
    
    int activeDisplayID = 0;
    bool borderlessMode;
    
    if (numVideoDisplays > 1) { // Jei keli, randam mažiausią iš dydžių ir atidarom borderless
        SDL_GetDisplayMode(0, 0, &mode); // Gaunam pagrindinio ekrano
        
        SDL_DisplayMode tempMode = { SDL_PIXELFORMAT_UNKNOWN, 0, 0, 0, 0 };
        
        for (int i = 1; i < numVideoDisplays; ++i) {
            SDL_GetDisplayMode(i, 0, &tempMode);
            
            if (tempMode.w < mode.w && tempMode.h < mode.h) {
                mode = tempMode;
            }
        }
        
        borderlessMode = true;
    } else { // Jei ekranas vienas, imam didžiausią
        SDL_GetDisplayMode(activeDisplayID, 0, &mode);
        
        borderlessMode = false;
    }
    
    if (engine->isEditorMode() && !borderlessMode) {
        borderlessMode = true;
        
        windowSize.x = mode.w - 50;
        windowSize.y = mode.h - 150;
//        screenWidth = mode.w;
//        screenHeight = mode.h;
    } else {
        windowSize.x = mode.w;
        windowSize.y = mode.h;
    }
    
    // TODO this only applies if full screen mode is chosen. Figure out what to log otherwise
    LOG_D("Chosen display mode: "
          "\n\tWidth: {}"
          "\n\tHeight: {}"
          "\n\tRefresh rate: {}"
          "\n\tPixel mode: {} {}", mode.w, mode.h, mode.refresh_rate, SDL_BITSPERPIXEL(mode.format), SDL_GetPixelFormatName(mode.format));

    std::uint32_t flags = SDL_WINDOW_SHOWN;
    
    std::string windowName = engine->isEditorMode() ? LOC_SYS(LH("gameName")) : LOC(LH("gameName"));
    
    window = SDL_CreateWindow 
        (windowName.c_str(),
         SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED,
         windowSize.x,
         windowSize.y,
         flags);
         //SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
         //SDL_WINDOW_BORDERLESS SDL_WINDOW_ALLOW_HIGHDPI
    if (!window) {
        throw std::runtime_error(SDL_GetError());
    }
    
    
    if (borderlessMode) {
//        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) {
//        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) { // borderless window?
//        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) { // fullscreen
//            throw std::runtime_error(SDL_GetError());
//        }
//        SDL_SetWindowBordered(window, SDL_FALSE);
//        SDL_SetWindowResizable(window, SDL_TRUE);
    } else {
        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_BORDERLESS) != 0) {
        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) { // full screen at current desktop resolution
        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) { // fullscreen
            throw std::runtime_error(SDL_GetError());
        }

        if (SDL_SetWindowDisplayMode(window, &mode) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
    }
    
    SDL_SetWindowResizable(window, SDL_TRUE);
    

//    if (borderlessMode) {
//        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) { // fullscreen
//        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) { // borderless window
//            throw std::runtime_error(SDL_GetError());
//        }
//    } else {
//        //if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) { // borderless window
//        if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) { // fullscreen
//            throw std::runtime_error(SDL_GetError());
//        }
//    }
}

void GraphicsAPI::printWMInfo() {
    SDL_SysWMinfo info;
    
    SDL_VERSION(&info.version)
    
    if (!SDL_GetWindowWMInfo(window, &info)) {
        throw std::runtime_error("Failed to obtain SDL window manager information.");
    }
    
    std::stringstream ss;
    ss << "SDL is using";
    switch (info.subsystem) {
        case SDL_SYSWM_UNKNOWN:
            ss << " an unknown subsystem";
            break;
        case SDL_SYSWM_WINDOWS:
            ss << " the Windows subsystem";
            break;
        case SDL_SYSWM_X11:
            ss << " the X11 subsystem"; 
            break;
        case SDL_SYSWM_DIRECTFB:
            ss << " the DirectFB subsystem";
            break;
        case SDL_SYSWM_COCOA:
            ss << " the Cocoa subsystem";
            break;
        case SDL_SYSWM_UIKIT:
            ss << " the UiKit subsystem";
            break;
        case SDL_SYSWM_WAYLAND:
            ss << " the wayland subsystem";
            break;
        case SDL_SYSWM_MIR:
            ss << " the MIR subsystem";
            break;
        case SDL_SYSWM_WINRT:
            ss << " the WinRT subsystem";
            break;
        case SDL_SYSWM_ANDROID:
            ss << " the Android subsystem";
            break;
        default:
            ss << " a subsystem with id " << info.subsystem;
    };
    
    LOG_V("{}", ss.str());
}

//TODO cache?
SamplerHnd GraphicsAPI::createPresetSampler(SamplerPreset preset, float maxLod) {
    SamplerCreateInfo sci;
    
    switch (preset) {
    case SamplerPreset::ImguiTexture:
        sci.minFilter = Filter::Linear;
        sci.magFilter = Filter::Linear;
        break;
    case SamplerPreset::Default3DModel:
        sci.minFilter = Filter::Linear;
        sci.magFilter = Filter::Linear;
        sci.mipmapMode = SamplerMipmapMode::Linear;
        sci.anisotropyEnable = true;
        sci.maxAnisotropy = 8.0f;//TODO nustatymas
        sci.maxLod = maxLod;
        break;
    case SamplerPreset::SkyBox:
        sci.minFilter = Filter::Linear;
        sci.magFilter = Filter::Linear;
        sci.mipmapMode = SamplerMipmapMode::Linear;
        sci.anisotropyEnable = true;
        sci.maxAnisotropy = 8.0f;//TODO nustatymas
        sci.maxLod = maxLod;
        break;
    }
    
    return createSampler(sci);
}

ImageViewHnd GraphicsAPI::createDefaultImageView(const Image& image) {
    ImageViewCreateInfo ivci;
    ivci.image = image.getHandle();
    ivci.viewType = image.getType();
    ivci.format = image.getFormat();
    ivci.subresourceRange.levelCount = image.getMipLevels();
    ivci.subresourceRange.layerCount = (image.getType() == ImageViewType::ImCube) ? 6 : image.getArrayLayers();

    return createImageView(ivci);
}

void GraphicsAPI::handleConfigChange(const ConfigurationValueMap& changedValues) {
    // TODO react to actual config changes
}

std::string GraphicsAPI::getFormatName(Format format) const {
    return formatName(format);
}

std::vector<Buffer> GraphicsAPI::createBuffers(const std::vector<BufferCreateInfo>& infos) {
    std::vector<Buffer> outBuffers;
    outBuffers.reserve(infos.size());
    
    for (std::size_t i = 0; i < infos.size(); ++i) {
        outBuffers.push_back(createBuffer(infos[i]));
    }
    
    return outBuffers;
}

bool GraphicsAPI::destroyBuffers(const std::vector<Buffer>& buffers) {
    bool result = true;
    for (const Buffer& b : buffers) {
        result &= destroyBuffer(b);
    }
    
    return result;
}

ImageCreateInfo GraphicsAPI::buildImageCreateInfo(const TextureData& textureData) {
    ImageCreateInfo ici;
    ici.extent = {textureData.width, textureData.height, textureData.depth};
    ici.format = con::CompressionFormatToEngineFormat(textureData.format, textureData.sRGB);
    ici.mipLevels = textureData.mipmapLevelCount;
    ici.arrayLayers = (textureData.faceCount == 6) ? 6 : textureData.layers;
    ici.usage = ImageUsageFlagBits::TransferDestination | ImageUsageFlagBits::Sampled;
    
    if (textureData.faceCount == 1) {
        ici.isCube = false;
    } else if (textureData.faceCount == 6) {
        ici.isCube = true;
    } else {
        throw std::runtime_error("A texture must have 1 or 6 faces.");
    }
    
    return ici;
}


}
