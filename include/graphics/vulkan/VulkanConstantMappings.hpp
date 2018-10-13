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

#ifndef VULKANCONSTANTMAPPINGS_HPP
#define VULKANCONSTANTMAPPINGS_HPP

#include <vulkan/vulkan.h>

#include "graphics/GraphicsAPI.hpp"
#include "utilities/ConstantMapper.hpp"
#include "utilities/FlagMapper.hpp"

namespace iyf {
namespace vk {
// WARNING: try to keep the ConstantMapper objects alphabetical
    
inline ConstantMapper<AttachmentLoadOp, VkAttachmentLoadOp, static_cast<size_t>(AttachmentLoadOp::COUNT)> attachmentLoadOp = {
    VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_LOAD_OP_CLEAR,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE
};

inline ConstantMapper<AttachmentStoreOp, VkAttachmentStoreOp, static_cast<size_t>(AttachmentStoreOp::COUNT)> attachmentStoreOp = {
    VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_STORE_OP_DONT_CARE
};
            
inline ConstantMapper<BlendFactor, VkBlendFactor, static_cast<size_t>(BlendFactor::COUNT)> blendFactor = {
    VK_BLEND_FACTOR_ZERO,                     //BlendFactor::Zero
    VK_BLEND_FACTOR_ONE,                      //BlendFactor::One
    VK_BLEND_FACTOR_SRC_COLOR,                //BlendFactor::SrcColor
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,      //BlendFactor::OneMinusSrcColor
    VK_BLEND_FACTOR_DST_COLOR,                //BlendFactor::DstColor
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,      //BlendFactor::OneMinusDstColor
    VK_BLEND_FACTOR_SRC_ALPHA,                //BlendFactor::SrcAlpha
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,      //BlendFactor::OneMinusSrcAlpha
    VK_BLEND_FACTOR_DST_ALPHA,                //BlendFactor::DstAlpha
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,      //BlendFactor::OneMinusDstAlpha
    VK_BLEND_FACTOR_CONSTANT_COLOR,           //BlendFactor::ConstantColor
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, //BlendFactor::OneMinusConstantColor
    VK_BLEND_FACTOR_CONSTANT_ALPHA,           //BlendFactor::ConstantAlpha
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, //BlendFactor::OneMinusConstantAlpha
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,       //BlendFactor::SrcAlphaSaturate
    VK_BLEND_FACTOR_SRC1_COLOR,               //BlendFactor::Src1Color
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,     //BlendFactor::OneMinusSrc1Color
    VK_BLEND_FACTOR_SRC1_ALPHA,               //BlendFactor::Src1Alpha
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,      //BlendFactor::OneMinusSrc1Alpha
};

inline ConstantMapper<BlendOp, VkBlendOp, static_cast<size_t>(BlendOp::COUNT)> blendOp = {
    VK_BLEND_OP_ADD,              //BlendOp::Add
    VK_BLEND_OP_SUBTRACT,         //BlendOp::Subtract
    VK_BLEND_OP_REVERSE_SUBTRACT, //BlendOp::ReverseSubtract
    VK_BLEND_OP_MIN,              //BlendOp::Min
    VK_BLEND_OP_MAX,              //BlendOp::Max
};

inline ConstantMapper<BorderColor, VkBorderColor, static_cast<size_t>(BorderColor::COUNT)> borderColor = {
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, //BorderColor::FloatTransparentBlack
    VK_BORDER_COLOR_INT_TRANSPARENT_BLACK,   //BorderColor::IntTransparentBlack
    VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,      //BorderColor::FloatOpaqueBlack
    VK_BORDER_COLOR_INT_OPAQUE_BLACK,        //BorderColor::IntOpaqueBlack
    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,      //BorderColor::FloatOpaqueWhite
    VK_BORDER_COLOR_INT_OPAQUE_WHITE,        //BorderColor::IntOpaqueWhite
};

inline IdentityFlagMapper<BufferUsageFlags, VkBufferUsageFlags> bufferUsage;
inline IdentityFlagMapper<ImageUsageFlags, VkImageUsageFlags> imageUsage;

inline ConstantMapper<CompareOp, VkCompareOp, static_cast<size_t>(CompareOp::COUNT)> compareOp = {
    VK_COMPARE_OP_NEVER,            //CompareOp::Never
    VK_COMPARE_OP_LESS,             //CompareOp::Less
    VK_COMPARE_OP_LESS_OR_EQUAL,    //CompareOp::LessEqual
    VK_COMPARE_OP_EQUAL,            //CompareOp::Equal
    VK_COMPARE_OP_GREATER,          //CompareOp::Greater
    VK_COMPARE_OP_GREATER_OR_EQUAL, //CompareOp::GreaterEqual
    VK_COMPARE_OP_ALWAYS,           //CompareOp::Always
    VK_COMPARE_OP_NOT_EQUAL,        //CompareOp::NotEqual
};

inline ConstantMapper<ComponentSwizzle, VkComponentSwizzle, static_cast<size_t>(ComponentSwizzle::COUNT)> componentSwizzle = {
    VK_COMPONENT_SWIZZLE_IDENTITY, //ComponentSwizzle::Identity
    VK_COMPONENT_SWIZZLE_ZERO,     //ComponentSwizzle::Zero
    VK_COMPONENT_SWIZZLE_ONE,      //ComponentSwizzle::One
    VK_COMPONENT_SWIZZLE_R,        //ComponentSwizzle::R
    VK_COMPONENT_SWIZZLE_G,        //ComponentSwizzle::G
    VK_COMPONENT_SWIZZLE_B,        //ComponentSwizzle::B
    VK_COMPONENT_SWIZZLE_A,        //ComponentSwizzle::A
};

inline FlagMapper<CullModeFlags, VkCullModeFlags, static_cast<size_t>(CullModeFlagBits::COUNT)> cullMode = {
    VK_CULL_MODE_NONE,      //CullModeFlagBits::None
    VK_CULL_MODE_FRONT_BIT, //CullModeFlagBits::Front
    VK_CULL_MODE_BACK_BIT   //CullModeFlagBits::Back
};

inline ConstantMapper<DynamicState, VkDynamicState, static_cast<size_t>(DynamicState::COUNT)> dynamicState = {
    VK_DYNAMIC_STATE_VIEWPORT,             //DynamicState::Viewport
    VK_DYNAMIC_STATE_SCISSOR,              //DynamicState::Scissor
    VK_DYNAMIC_STATE_LINE_WIDTH,           //DynamicState::LineWidth
    VK_DYNAMIC_STATE_DEPTH_BIAS,           //DynamicState::DepthBias
    VK_DYNAMIC_STATE_BLEND_CONSTANTS,      //DynamicState::BlendConstants
    VK_DYNAMIC_STATE_DEPTH_BOUNDS,         //DynamicState::DepthBounds
    VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK, //DynamicState::StencilCompareMask
    VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,   //DynamicState::StencilWriteMask
    VK_DYNAMIC_STATE_STENCIL_REFERENCE,    //DynamicState::StencilReference
};

inline ConstantMapper<Format, VkFormat, static_cast<size_t>(Format::COUNT)> format = {
    VK_FORMAT_UNDEFINED,                   //Format::Undefined
    VK_FORMAT_R4G4_UNORM_PACK8,            //Format::R4_G4_uNorm_pack8
    VK_FORMAT_R4G4B4A4_UNORM_PACK16,       //Format::R4_G4_B4_A4_uNorm_pack16
    VK_FORMAT_B4G4R4A4_UNORM_PACK16,       //Format::B4_G4_R4_A4_uNorm_pack16
    VK_FORMAT_R5G6B5_UNORM_PACK16,         //Format::R5_G6_B5_uNorm_pack16
    VK_FORMAT_B5G6R5_UNORM_PACK16,         //Format::B5_G6_R5_uNorm_pack16
    VK_FORMAT_R5G5B5A1_UNORM_PACK16,       //Format::R5_G5_B5_A1_uNorm_pack16
    VK_FORMAT_B5G5R5A1_UNORM_PACK16,       //Format::B5_G5_R5_A1_uNorm_pack16
    VK_FORMAT_A1R5G5B5_UNORM_PACK16,       //Format::A1_R5_G5_B5_uNorm_pack16
    VK_FORMAT_R8_UNORM,                    //Format::R8_uNorm
    VK_FORMAT_R8_SNORM,                    //Format::R8_sNorm
    VK_FORMAT_R8_USCALED,                  //Format::R8_uScaled
    VK_FORMAT_R8_SSCALED,                  //Format::R8_sScaled
    VK_FORMAT_R8_UINT,                     //Format::R8_uInt
    VK_FORMAT_R8_SINT,                     //Format::R8_sInt
    VK_FORMAT_R8_SRGB,                     //Format::R8_sRGB
    VK_FORMAT_R8G8_UNORM,                  //Format::R8_G8_uNorm
    VK_FORMAT_R8G8_SNORM,                  //Format::R8_G8_sNorm
    VK_FORMAT_R8G8_USCALED,                //Format::R8_G8_uScaled
    VK_FORMAT_R8G8_SSCALED,                //Format::R8_G8_sScaled
    VK_FORMAT_R8G8_UINT,                   //Format::R8_G8_uInt
    VK_FORMAT_R8G8_SINT,                   //Format::R8_G8_sInt
    VK_FORMAT_R8G8_SRGB,                   //Format::R8_G8_sRGB
    VK_FORMAT_R8G8B8_UNORM,                //Format::R8_G8_B8_uNorm
    VK_FORMAT_R8G8B8_SNORM,                //Format::R8_G8_B8_sNorm
    VK_FORMAT_R8G8B8_USCALED,              //Format::R8_G8_B8_uScaled
    VK_FORMAT_R8G8B8_SSCALED,              //Format::R8_G8_B8_sScaled
    VK_FORMAT_R8G8B8_UINT,                 //Format::R8_G8_B8_uInt
    VK_FORMAT_R8G8B8_SINT,                 //Format::R8_G8_B8_sInt
    VK_FORMAT_R8G8B8_SRGB,                 //Format::R8_G8_B8_sRGB
    VK_FORMAT_B8G8R8_UNORM,                //Format::B8_G8_R8_uNorm
    VK_FORMAT_B8G8R8_SNORM,                //Format::B8_G8_R8_sNorm
    VK_FORMAT_B8G8R8_USCALED,              //Format::B8_G8_R8_uScaled
    VK_FORMAT_B8G8R8_SSCALED,              //Format::B8_G8_R8_sScaled
    VK_FORMAT_B8G8R8_UINT,                 //Format::B8_G8_R8_uInt
    VK_FORMAT_B8G8R8_SINT,                 //Format::B8_G8_R8_sInt
    VK_FORMAT_B8G8R8_SRGB,                 //Format::B8_G8_R8_sRGB
    VK_FORMAT_R8G8B8A8_UNORM,              //Format::R8_G8_B8_A8_uNorm
    VK_FORMAT_R8G8B8A8_SNORM,              //Format::R8_G8_B8_A8_sNorm
    VK_FORMAT_R8G8B8A8_USCALED,            //Format::R8_G8_B8_A8_uScaled
    VK_FORMAT_R8G8B8A8_SSCALED,            //Format::R8_G8_B8_A8_sScaled
    VK_FORMAT_R8G8B8A8_UINT,               //Format::R8_G8_B8_A8_uInt
    VK_FORMAT_R8G8B8A8_SINT,               //Format::R8_G8_B8_A8_sInt
    VK_FORMAT_R8G8B8A8_SRGB,               //Format::R8_G8_B8_A8_sRGB
    VK_FORMAT_B8G8R8A8_UNORM,              //Format::B8_G8_R8_A8_uNorm
    VK_FORMAT_B8G8R8A8_SNORM,              //Format::B8_G8_R8_A8_sNorm
    VK_FORMAT_B8G8R8A8_USCALED,            //Format::B8_G8_R8_A8_uScaled
    VK_FORMAT_B8G8R8A8_SSCALED,            //Format::B8_G8_R8_A8_sScaled
    VK_FORMAT_B8G8R8A8_UINT,               //Format::B8_G8_R8_A8_uInt
    VK_FORMAT_B8G8R8A8_SINT,               //Format::B8_G8_R8_A8_sInt
    VK_FORMAT_B8G8R8A8_SRGB,               //Format::B8_G8_R8_A8_sRGB
    VK_FORMAT_A8B8G8R8_UNORM_PACK32,       //Format::A8_B8_G8_R8_uNorm_pack32
    VK_FORMAT_A8B8G8R8_SNORM_PACK32,       //Format::A8_B8_G8_R8_sNorm_pack32
    VK_FORMAT_A8B8G8R8_USCALED_PACK32,     //Format::A8_B8_G8_R8_uScaled_pack32
    VK_FORMAT_A8B8G8R8_SSCALED_PACK32,     //Format::A8_B8_G8_R8_sScaled_pack32
    VK_FORMAT_A8B8G8R8_UINT_PACK32,        //Format::A8_B8_G8_R8_uInt_pack32
    VK_FORMAT_A8B8G8R8_SINT_PACK32,        //Format::A8_B8_G8_R8_sInt_pack32
    VK_FORMAT_A8B8G8R8_SRGB_PACK32,        //Format::A8_B8_G8_R8_sRGB_pack32
    VK_FORMAT_A2R10G10B10_UNORM_PACK32,    //Format::A2_R10_G10_B10_uNorm_pack32
    VK_FORMAT_A2R10G10B10_SNORM_PACK32,    //Format::A2_R10_G10_B10_sNorm_pack32
    VK_FORMAT_A2R10G10B10_USCALED_PACK32,  //Format::A2_R10_G10_B10_uScaled_pack32
    VK_FORMAT_A2R10G10B10_SSCALED_PACK32,  //Format::A2_R10_G10_B10_sScaled_pack32
    VK_FORMAT_A2R10G10B10_UINT_PACK32,     //Format::A2_R10_G10_B10_uInt_pack32
    VK_FORMAT_A2R10G10B10_SINT_PACK32,     //Format::A2_R10_G10_B10_sInt_pack32
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,    //Format::A2_B10_G10_R10_uNorm_pack32
    VK_FORMAT_A2B10G10R10_SNORM_PACK32,    //Format::A2_B10_G10_R10_sNorm_pack32
    VK_FORMAT_A2B10G10R10_USCALED_PACK32,  //Format::A2_B10_G10_R10_uScaled_pack32
    VK_FORMAT_A2B10G10R10_SSCALED_PACK32,  //Format::A2_B10_G10_R10_sScaled_pack32
    VK_FORMAT_A2B10G10R10_UINT_PACK32,     //Format::A2_B10_G10_R10_uInt_pack32
    VK_FORMAT_A2B10G10R10_SINT_PACK32,     //Format::A2_B10_G10_R10_sInt_pack32
    VK_FORMAT_R16_UNORM,                   //Format::R16_uNorm
    VK_FORMAT_R16_SNORM,                   //Format::R16_sNorm
    VK_FORMAT_R16_USCALED,                 //Format::R16_uScaled
    VK_FORMAT_R16_SSCALED,                 //Format::R16_sScaled
    VK_FORMAT_R16_UINT,                    //Format::R16_uInt
    VK_FORMAT_R16_SINT,                    //Format::R16_sInt
    VK_FORMAT_R16_SFLOAT,                  //Format::R16_sFloat
    VK_FORMAT_R16G16_UNORM,                //Format::R16_G16_uNorm
    VK_FORMAT_R16G16_SNORM,                //Format::R16_G16_sNorm
    VK_FORMAT_R16G16_USCALED,              //Format::R16_G16_uScaled
    VK_FORMAT_R16G16_SSCALED,              //Format::R16_G16_sScaled
    VK_FORMAT_R16G16_UINT,                 //Format::R16_G16_uInt
    VK_FORMAT_R16G16_SINT,                 //Format::R16_G16_sInt
    VK_FORMAT_R16G16_SFLOAT,               //Format::R16_G16_sFloat
    VK_FORMAT_R16G16B16_UNORM,             //Format::R16_G16_B16_uNorm
    VK_FORMAT_R16G16B16_SNORM,             //Format::R16_G16_B16_sNorm
    VK_FORMAT_R16G16B16_USCALED,           //Format::R16_G16_B16_uScaled
    VK_FORMAT_R16G16B16_SSCALED,           //Format::R16_G16_B16_sScaled
    VK_FORMAT_R16G16B16_UINT,              //Format::R16_G16_B16_uInt
    VK_FORMAT_R16G16B16_SINT,              //Format::R16_G16_B16_sInt
    VK_FORMAT_R16G16B16_SFLOAT,            //Format::R16_G16_B16_sFloat
    VK_FORMAT_R16G16B16A16_UNORM,          //Format::R16_G16_B16_A16_uNorm
    VK_FORMAT_R16G16B16A16_SNORM,          //Format::R16_G16_B16_A16_sNorm
    VK_FORMAT_R16G16B16A16_USCALED,        //Format::R16_G16_B16_A16_uScaled
    VK_FORMAT_R16G16B16A16_SSCALED,        //Format::R16_G16_B16_A16_sScaled
    VK_FORMAT_R16G16B16A16_UINT,           //Format::R16_G16_B16_A16_uInt
    VK_FORMAT_R16G16B16A16_SINT,           //Format::R16_G16_B16_A16_sInt
    VK_FORMAT_R16G16B16A16_SFLOAT,         //Format::R16_G16_B16_A16_sFloat
    VK_FORMAT_R32_UINT,                    //Format::R32_uInt
    VK_FORMAT_R32_SINT,                    //Format::R32_sInt
    VK_FORMAT_R32_SFLOAT,                  //Format::R32_sFloat
    VK_FORMAT_R32G32_UINT,                 //Format::R32_G32_uInt
    VK_FORMAT_R32G32_SINT,                 //Format::R32_G32_sInt
    VK_FORMAT_R32G32_SFLOAT,               //Format::R32_G32_sFloat
    VK_FORMAT_R32G32B32_UINT,              //Format::R32_G32_B32_uInt
    VK_FORMAT_R32G32B32_SINT,              //Format::R32_G32_B32_sInt
    VK_FORMAT_R32G32B32_SFLOAT,            //Format::R32_G32_B32_sFloat
    VK_FORMAT_R32G32B32A32_UINT,           //Format::R32_G32_B32_A32_uInt
    VK_FORMAT_R32G32B32A32_SINT,           //Format::R32_G32_B32_A32_sInt
    VK_FORMAT_R32G32B32A32_SFLOAT,         //Format::R32_G32_B32_A32_sFloat
    VK_FORMAT_R64_UINT,                    //Format::R64_uInt
    VK_FORMAT_R64_SINT,                    //Format::R64_sInt
    VK_FORMAT_R64_SFLOAT,                  //Format::R64_sFloat
    VK_FORMAT_R64G64_UINT,                 //Format::R64_G64_uInt
    VK_FORMAT_R64G64_SINT,                 //Format::R64_G64_sInt
    VK_FORMAT_R64G64_SFLOAT,               //Format::R64_G64_sFloat
    VK_FORMAT_R64G64B64_UINT,              //Format::R64_G64_B64_uInt
    VK_FORMAT_R64G64B64_SINT,              //Format::R64_G64_B64_sInt
    VK_FORMAT_R64G64B64_SFLOAT,            //Format::R64_G64_B64_sFloat
    VK_FORMAT_R64G64B64A64_UINT,           //Format::R64_G64_B64_A64_uInt
    VK_FORMAT_R64G64B64A64_SINT,           //Format::R64_G64_B64_A64_sInt
    VK_FORMAT_R64G64B64A64_SFLOAT,         //Format::R64_G64_B64_A64_sFloat
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,     //Format::B10_G11_R11_uFloat_pack32
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,      //Format::E5_B9_G9_R9_uFloat_pack32
    VK_FORMAT_D16_UNORM,                   //Format::D16_uNorm
    VK_FORMAT_X8_D24_UNORM_PACK32,         //Format::X8_D24_uNorm_pack32
    VK_FORMAT_D32_SFLOAT,                  //Format::D32_sFloat
    VK_FORMAT_S8_UINT,                     //Format::S8_uInt
    VK_FORMAT_D16_UNORM_S8_UINT,           //Format::D16_uNorm_S8_uInt
    VK_FORMAT_D24_UNORM_S8_UINT,           //Format::D24_uNorm_S8_uInt
    VK_FORMAT_D32_SFLOAT_S8_UINT,          //Format::D32_sFloat_S8_uInt
    VK_FORMAT_BC1_RGB_UNORM_BLOCK,         //Format::BC1_RGB_uNorm_block
    VK_FORMAT_BC1_RGB_SRGB_BLOCK,          //Format::BC1_RGB_sRGB_block
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,        //Format::BC1_RGBA_uNorm_block
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK,         //Format::BC1_RGBA_sRGB_block
    VK_FORMAT_BC2_UNORM_BLOCK,             //Format::BC2_uNorm_block
    VK_FORMAT_BC2_SRGB_BLOCK,              //Format::BC2_sRGB_block
    VK_FORMAT_BC3_UNORM_BLOCK,             //Format::BC3_uNorm_block
    VK_FORMAT_BC3_SRGB_BLOCK,              //Format::BC3_sRGB_block
    VK_FORMAT_BC4_UNORM_BLOCK,             //Format::BC4_uNorm_block
    VK_FORMAT_BC4_SNORM_BLOCK,             //Format::BC4_sNorm_block
    VK_FORMAT_BC5_UNORM_BLOCK,             //Format::BC5_uNorm_block
    VK_FORMAT_BC5_SNORM_BLOCK,             //Format::BC5_sNorm_block
    VK_FORMAT_BC6H_UFLOAT_BLOCK,           //Format::BC6H_uFloat_block
    VK_FORMAT_BC6H_SFLOAT_BLOCK,           //Format::BC6H_sFloat_block
    VK_FORMAT_BC7_UNORM_BLOCK,             //Format::BC7_uNorm_block
    VK_FORMAT_BC7_SRGB_BLOCK,              //Format::BC7_sRGB_block
    VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     //Format::ETC2_R8_G8_B8_uNorm_block
    VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,      //Format::ETC2_R8_G8_B8_sRGB_block
    VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,   //Format::ETC2_R8_G8_B8_A1_uNorm_block
    VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,    //Format::ETC2_R8_G8_B8_A1_sRGB_block
    VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   //Format::ETC2_R8_G8_B8_A8_uNorm_block
    VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,    //Format::ETC2_R8_G8_B8_A8_sRGB_block
    VK_FORMAT_EAC_R11_UNORM_BLOCK,         //Format::EAC_R11_uNorm_block
    VK_FORMAT_EAC_R11_SNORM_BLOCK,         //Format::EAC_R11_sNorm_block
    VK_FORMAT_EAC_R11G11_UNORM_BLOCK,      //Format::EAC_R11_G11_uNorm_block
    VK_FORMAT_EAC_R11G11_SNORM_BLOCK,      //Format::EAC_R11_G11_sNorm_block
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK,        //Format::ASTC_4x4_uNorm_block
    VK_FORMAT_ASTC_4x4_SRGB_BLOCK,         //Format::ASTC_4x4_sRGB_block
    VK_FORMAT_ASTC_5x4_UNORM_BLOCK,        //Format::ASTC_5x4_uNorm_block
    VK_FORMAT_ASTC_5x4_SRGB_BLOCK,         //Format::ASTC_5x4_sRGB_block
    VK_FORMAT_ASTC_5x5_UNORM_BLOCK,        //Format::ASTC_5x5_uNorm_block
    VK_FORMAT_ASTC_5x5_SRGB_BLOCK,         //Format::ASTC_5x5_sRGB_block
    VK_FORMAT_ASTC_6x5_UNORM_BLOCK,        //Format::ASTC_6x5_uNorm_block
    VK_FORMAT_ASTC_6x5_SRGB_BLOCK,         //Format::ASTC_6x5_sRGB_block
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK,        //Format::ASTC_6x6_uNorm_block
    VK_FORMAT_ASTC_6x6_SRGB_BLOCK,         //Format::ASTC_6x6_sRGB_block
    VK_FORMAT_ASTC_8x5_UNORM_BLOCK,        //Format::ASTC_8x5_uNorm_block
    VK_FORMAT_ASTC_8x5_SRGB_BLOCK,         //Format::ASTC_8x5_sRGB_block
    VK_FORMAT_ASTC_8x6_UNORM_BLOCK,        //Format::ASTC_8x6_uNorm_block
    VK_FORMAT_ASTC_8x6_SRGB_BLOCK,         //Format::ASTC_8x6_sRGB_block
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK,        //Format::ASTC_8x8_uNorm_block
    VK_FORMAT_ASTC_8x8_SRGB_BLOCK,         //Format::ASTC_8x8_sRGB_block
    VK_FORMAT_ASTC_10x5_UNORM_BLOCK,       //Format::ASTC_10x5_uNorm_block
    VK_FORMAT_ASTC_10x5_SRGB_BLOCK,        //Format::ASTC_10x5_sRGB_block
    VK_FORMAT_ASTC_10x6_UNORM_BLOCK,       //Format::ASTC_10x6_uNorm_block
    VK_FORMAT_ASTC_10x6_SRGB_BLOCK,        //Format::ASTC_10x6_sRGB_block
    VK_FORMAT_ASTC_10x8_UNORM_BLOCK,       //Format::ASTC_10x8_uNorm_block
    VK_FORMAT_ASTC_10x8_SRGB_BLOCK,        //Format::ASTC_10x8_sRGB_block
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,      //Format::ASTC_10x10_uNorm_block
    VK_FORMAT_ASTC_10x10_SRGB_BLOCK,       //Format::ASTC_10x10_sRGB_block
    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,      //Format::ASTC_12x10_uNorm_block
    VK_FORMAT_ASTC_12x10_SRGB_BLOCK,       //Format::ASTC_12x10_sRGB_block
    VK_FORMAT_ASTC_12x12_UNORM_BLOCK,      //Format::ASTC_12x12_uNorm_block
    VK_FORMAT_ASTC_12x12_SRGB_BLOCK,       //Format::ASTC_12x12_sRGB_block
    VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG, //Format::PVRTC1_2BPP_uNorm_block
    VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG, //Format::PVRTC1_4BPP_uNorm_block
    VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG, //Format::PVRTC2_2BPP_uNorm_block
    VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG, //Format::PVRTC2_4BPP_uNorm_block
    VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,  //Format::PVRTC1_2BPP_sRGB_block
    VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,  //Format::PVRTC1_4BPP_sRGB_block
    VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,  //Format::PVRTC2_2BPP_sRGB_block
    VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,  //Format::PVRTC2_4BPP_sRGB_block
};

inline ConstantMapper<FrontFace, VkFrontFace, static_cast<size_t>(FrontFace::COUNT)> frontFace = {
    VK_FRONT_FACE_CLOCKWISE,         //FrontFace::Clockwise
    VK_FRONT_FACE_COUNTER_CLOCKWISE, //FrontFace::CounterClockwise
};

inline ConstantMapper<ImageLayout, VkImageLayout, static_cast<size_t>(ImageLayout::COUNT)> imageLayout = {
    VK_IMAGE_LAYOUT_UNDEFINED,                        //ImageLayout::Undefined
    VK_IMAGE_LAYOUT_GENERAL,                          //ImageLayout::General
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,         //ImageLayout::ColorAttachmentOptimal
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, //ImageLayout::DepthStencilAttachmentOptimal
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,  //ImageLayout::DepthStencilReadOnlyOptimal
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,         //ImageLayout::ShaderReadOnlyOptimal
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,             //ImageLayout::TransferSrcOptimal
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,             //ImageLayout::TransferDstOptimal
    VK_IMAGE_LAYOUT_PREINITIALIZED,                   //ImageLayout::Preinitialized
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                  //ImageLayout::PresentSource
};

inline ConstantMapper<LogicOp, VkLogicOp, static_cast<size_t>(LogicOp::COUNT)> logicOp = {
    VK_LOGIC_OP_CLEAR,         //LogicOp::Clear
    VK_LOGIC_OP_AND,           //LogicOp::And
    VK_LOGIC_OP_AND_REVERSE,   //LogicOp::AndReverse
    VK_LOGIC_OP_COPY,          //LogicOp::Copy
    VK_LOGIC_OP_AND_INVERTED,  //LogicOp::AndInverted
    VK_LOGIC_OP_NO_OP,         //LogicOp::NoOp
    VK_LOGIC_OP_XOR,           //LogicOp::Xor
    VK_LOGIC_OP_OR,            //LogicOp::Or
    VK_LOGIC_OP_NOR,           //LogicOp::Nor
    VK_LOGIC_OP_EQUIVALENT,    //LogicOp::Equivalent
    VK_LOGIC_OP_INVERT,        //LogicOp::Invert
    VK_LOGIC_OP_OR_REVERSE,    //LogicOp::OrReverse
    VK_LOGIC_OP_COPY_INVERTED, //LogicOp::CopyInverted
    VK_LOGIC_OP_OR_INVERTED,   //LogicOp::OrInverted
    VK_LOGIC_OP_NAND,          //LogicOp::Nand
    VK_LOGIC_OP_SET,           //LogicOp::Set
};

inline ConstantMapper<PolygonMode, VkPolygonMode, static_cast<size_t>(PolygonMode::COUNT)> polygonMode = {
    VK_POLYGON_MODE_FILL,  //PolygonMode::Fill
    VK_POLYGON_MODE_LINE,  //PolygonMode::Line
    VK_POLYGON_MODE_POINT, //PolygonMode::Point
};

inline ConstantMapper<PrimitiveTopology, VkPrimitiveTopology, static_cast<size_t>(PrimitiveTopology::COUNT)> primitiveTopology = {
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,                    //PrimitiveTopology::PointList
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST,                     //PrimitiveTopology::LineList
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,                    //PrimitiveTopology::LineStrip
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                 //PrimitiveTopology::TriangleList
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                //PrimitiveTopology::TriangleStrip
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,                  //PrimitiveTopology::TriangleFan
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,      //PrimitiveTopology::LineListWithAdjacency
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,     //PrimitiveTopology::LineStripWithAdjacency
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,  //PrimitiveTopology::TriangleListWithAdjacency
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY, //PrimitiveTopology::TriangleStripWithAdjacency
    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,                    //PrimitiveTopology::PatchList
};

inline FlagMapper<SampleCountFlags, VkSampleCountFlags, static_cast<size_t>(SampleCountFlagBits::COUNT)> sampleCount = {
    0,                      //NONE
    VK_SAMPLE_COUNT_1_BIT,  //SampleCountFlagBits::X1
    VK_SAMPLE_COUNT_2_BIT,  //SampleCountFlagBits::X2
    VK_SAMPLE_COUNT_4_BIT,  //SampleCountFlagBits::X4
    VK_SAMPLE_COUNT_8_BIT,  //SampleCountFlagBits::X8
    VK_SAMPLE_COUNT_16_BIT, //SampleCountFlagBits::X16
    VK_SAMPLE_COUNT_32_BIT, //SampleCountFlagBits::X32
    VK_SAMPLE_COUNT_64_BIT, //SampleCountFlagBits::X64
};

inline IdentityFlagMapper<ShaderStageFlags, VkShaderStageFlags> shaderStage;

inline ConstantMapper<StencilOp, VkStencilOp, static_cast<size_t>(StencilOp::COUNT)> stencilOp = {
    VK_STENCIL_OP_KEEP,                //StencilOp::Keep
    VK_STENCIL_OP_ZERO,                //StencilOp::Zero
    VK_STENCIL_OP_REPLACE,             //StencilOp::Replace
    VK_STENCIL_OP_INCREMENT_AND_CLAMP, //StencilOp::IncrementAndClamp
    VK_STENCIL_OP_DECREMENT_AND_CLAMP, //StencilOp::DecrementAndClamp
    VK_STENCIL_OP_INVERT,              //StencilOp::Invert
    VK_STENCIL_OP_INCREMENT_AND_WRAP,  //StencilOp::IncrementAndWrap
    VK_STENCIL_OP_DECREMENT_AND_WRAP,  //StencilOp::DecrementAndWrap
};

inline ConstantMapper<ImageViewType, VkImageViewType, static_cast<size_t>(ImageViewType::COUNT)> imageViewType = {
    VK_IMAGE_VIEW_TYPE_1D,         //ImageViewType::Im1D
    VK_IMAGE_VIEW_TYPE_2D,         //ImageViewType::Im2D
    VK_IMAGE_VIEW_TYPE_3D,         //ImageViewType::Im3D
    VK_IMAGE_VIEW_TYPE_CUBE,       //ImageViewType::ImCube
    VK_IMAGE_VIEW_TYPE_1D_ARRAY,   //ImageViewType::Im1DArray
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,   //ImageViewType::Im2DArray
    VK_IMAGE_VIEW_TYPE_CUBE_ARRAY, //ImageViewType::ImCubeArray
};

}
}

#endif /* VULKANCONSTANTMAPPINGS_HPP */

