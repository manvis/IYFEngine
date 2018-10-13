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

#ifndef IYF_VULKAN_UTILITIES_HPP
#define IYF_VULKAN_UTILITIES_HPP

#include "core/Constants.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/vulkan/VulkanConstantMappings.hpp"

namespace iyf {
namespace vkutil {
enum class SupportedTiling {
    Linear,
    Optimal,
    NotSupported
};

inline SupportedTiling getSupportedTiling(const VkFormatProperties& formatProperties, VkFormatFeatureFlagBits bitsToCheck) {
    if (formatProperties.optimalTilingFeatures & bitsToCheck) {
        return SupportedTiling::Optimal;
    } else if (formatProperties.linearTilingFeatures & bitsToCheck) {
        return SupportedTiling::Linear;
    } else {
        return SupportedTiling::NotSupported;
    }
}
inline std::vector<VkDynamicState> mapDynamicState(const std::vector<DynamicState>& states) {
    std::vector<VkDynamicState> vkStates;
    vkStates.reserve(states.size());
    
    for (auto &s : states) {
        VkDynamicState temp = vk::dynamicState(s);
        vkStates.push_back(temp);
    }
    
    return vkStates;
}

inline VkColorComponentFlags mapColorWriteMask(ColorWriteMaskFlags mask) {
    VkColorComponentFlags temp = 0;
    
    if (static_cast<bool>(mask & ColorWriteMaskFlagBits::Red)) {
        temp |= VK_COLOR_COMPONENT_R_BIT;
    }
    
    if (static_cast<bool>(mask & ColorWriteMaskFlagBits::Green)) {
        temp |= VK_COLOR_COMPONENT_G_BIT;
    }
    
    if (static_cast<bool>(mask & ColorWriteMaskFlagBits::Blue)) {
        temp |= VK_COLOR_COMPONENT_B_BIT;
    }
    
    if (static_cast<bool>(mask & ColorWriteMaskFlagBits::Alpha)) {
        temp |= VK_COLOR_COMPONENT_A_BIT;
    }
    
    return temp;
}

inline VkPipelineBindPoint mapPipelineBindPoint(PipelineBindPoint point) {
    if (point == PipelineBindPoint::Graphics) {
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    } else {
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

inline VkVertexInputRate mapInputRate(VertexInputRate rate) {
    switch (rate) {
    case VertexInputRate::Vertex:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case VertexInputRate::Instance:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    default:
        throw std::runtime_error("Invalid input rate");
    }
}

inline bool isDepthStencilFormat(Format format) {
    switch (format) {
        case Format::D16_uNorm_S8_uInt:
        case Format::D24_uNorm_S8_uInt:
        case Format::D32_sFloat_S8_uInt:
            return true;
        default:
            return false;
    }
}

inline std::vector<VkPipelineColorBlendAttachmentState> mapAttachments(const std::vector<ColorBlendAttachmentState>& states) {
    std::vector<VkPipelineColorBlendAttachmentState> vkStates;
    vkStates.reserve(states.size());
    
    for (auto &s : states) {
        VkPipelineColorBlendAttachmentState temp;
        temp.blendEnable         = s.blendEnable ? VK_TRUE : VK_FALSE;
        temp.srcColorBlendFactor = vk::blendFactor(s.srcColorBlendFactor);
        temp.dstColorBlendFactor = vk::blendFactor(s.dstColorBlendFactor);
        temp.colorBlendOp        = vk::blendOp(s.colorBlendOp);
        temp.srcAlphaBlendFactor = vk::blendFactor(s.srcAlphaBlendFactor);
        temp.dstAlphaBlendFactor = vk::blendFactor(s.dstAlphaBlendFactor);
        temp.alphaBlendOp        = vk::blendOp(s.alphaBlendOp);
        temp.colorWriteMask      = mapColorWriteMask(s.colorWriteMask);
        
        vkStates.push_back(temp);
    }
    
    return vkStates;
}

inline VkIndexType mapIndexType(IndexType type) {
    switch (type) {
    case IndexType::UInt16:
        return VK_INDEX_TYPE_UINT16;
    case IndexType::UInt32:
        return VK_INDEX_TYPE_UINT32;
    default:
        throw std::invalid_argument("Invalid index type");
    }
}

inline VkComponentMapping mapComponents(ComponentMapping mapping) {
    VkComponentMapping cm;
    cm.r = vk::componentSwizzle(mapping.r);
    cm.g = vk::componentSwizzle(mapping.g);
    cm.b = vk::componentSwizzle(mapping.b);
    cm.a = vk::componentSwizzle(mapping.a);
    
    return cm;
}

inline VkCommandBufferUsageFlags mapBufferUsageFlags(CommandBufferUsageFlags flags) {
    VkCommandBufferUsageFlags temp = 0;
    
    if (static_cast<bool>(flags & CommandBufferUsageFlagBits::OneTimeSubmit)) {
        temp |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }
    
    if (static_cast<bool>(flags & CommandBufferUsageFlagBits::RenderPassContinue)) {
        temp |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    }
    
    if (static_cast<bool>(flags & CommandBufferUsageFlagBits::SimultaneousUse)) {
        temp |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    }
    
    return temp;
}

inline VkCommandBufferInheritanceInfo mapInheritanceInfo(CommandBufferInheritanceInfo cbii) {
    VkCommandBufferInheritanceInfo vkcbii;
    vkcbii.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkcbii.pNext = nullptr;
    vkcbii.renderPass = cbii.renderPass.toNative<VkRenderPass>();
    vkcbii.subpass = cbii.subpass;
    vkcbii.framebuffer = cbii.framebuffer.toNative<VkFramebuffer>();
    // TODO
    vkcbii.occlusionQueryEnable = false;
    vkcbii.queryFlags = 0;
    vkcbii.pipelineStatistics = 0;
    return vkcbii;
}

inline std::vector<VkClearValue> mapClearValues(const std::vector<ClearValue> clearValues) {
    std::vector<VkClearValue> mapped;
    mapped.reserve(clearValues.size());
    
    for (auto& val : clearValues) {
        VkClearValue vkcv;
        
        if (val.getType() == ClearValue::Type::Color) {
            ClearColorValue ccv = val.getValue().color;
            
            if (ccv.getType() == ClearColorValue::Type::Float) {
                vkcv.color.float32[0] = ccv.getValue().float32[0];
                vkcv.color.float32[1] = ccv.getValue().float32[1];
                vkcv.color.float32[2] = ccv.getValue().float32[2];
                vkcv.color.float32[3] = ccv.getValue().float32[3];
            } else if (ccv.getType() == ClearColorValue::Type::Int32) {
                vkcv.color.int32[0] = ccv.getValue().int32[0];
                vkcv.color.int32[1] = ccv.getValue().int32[1];
                vkcv.color.int32[2] = ccv.getValue().int32[2];
                vkcv.color.int32[3] = ccv.getValue().int32[3];
            } else {
                vkcv.color.uint32[0] = ccv.getValue().uint32[0];
                vkcv.color.uint32[1] = ccv.getValue().uint32[1];
                vkcv.color.uint32[2] = ccv.getValue().uint32[2];
                vkcv.color.uint32[3] = ccv.getValue().uint32[3];
            }
            
        } else {
            vkcv.depthStencil.depth = val.getValue().depthStencil.depth;
            vkcv.depthStencil.stencil = val.getValue().depthStencil.stencil;
        }
        
        mapped.push_back(vkcv);
    }
    
    return mapped;
}

inline VkImageAspectFlags mapAspectMask(ImageAspectFlags flags) {
    VkImageAspectFlags temp = 0;
    
    if (static_cast<bool>(flags & ImageAspectFlagBits::Color)) {
        temp |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    if (static_cast<bool>(flags & ImageAspectFlagBits::Depth)) {
        temp |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    
    if (static_cast<bool>(flags & ImageAspectFlagBits::Stencil)) {
        temp |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    
    if (static_cast<bool>(flags & ImageAspectFlagBits::Metadata)) {
        temp |= VK_IMAGE_ASPECT_METADATA_BIT;
    }
    
    return temp;
}

inline VkImageSubresourceRange mapSubresourceRange(ImageSubresourceRange range) {
    VkImageSubresourceRange isr;
    isr.aspectMask     = mapAspectMask(range.aspectMask);
    isr.baseMipLevel   = range.baseMipLevel;
    isr.levelCount     = range.levelCount;
    isr.baseArrayLayer = range.baseArrayLayer;
    isr.layerCount     = range.layerCount;
    
    return isr;
}

inline VkCommandBufferLevel mapBufferLevel(BufferLevel level) {
    if (level == BufferLevel::Primary) {
        return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    } else {
        return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    }
}
    
inline std::vector<VkDescriptorSetLayout> mapSetLayouts(const std::vector<DescriptorSetLayoutHnd>& layouts) {
    std::vector<VkDescriptorSetLayout> result;
    
    for (auto& l : layouts) {
        result.push_back(l.toNative<VkDescriptorSetLayout>());
    }
    
    return result;
}


inline std::vector<VkPushConstantRange> mapPushConstantRanges(const std::vector<PushConstantRange>& ranges) {
    std::vector<VkPushConstantRange> result;
    
    for (auto& r : ranges) {
        VkPushConstantRange pcr;
        pcr.stageFlags = vk::shaderStage(r.stageFlags);
        pcr.offset = r.offset;
        pcr.size = r.size;
        
        result.push_back(pcr);
    }
    
    return result;
}

inline VkDescriptorType mapDescriptorType(DescriptorType descriptor) {
    switch (descriptor) {
    case DescriptorType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DescriptorType::CombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::SampledImage:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorType::StorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::UniformTexelBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case DescriptorType::StorageTexelBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case DescriptorType::UniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::StorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::UniformBufferDynamic:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case DescriptorType::StorageBufferDynamic:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case DescriptorType::InputAttachment:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    default:
        throw std::invalid_argument("Invalid descriptor type");
    }
}
    
inline std::vector<VkDescriptorSetLayoutBinding> mapBindings(const std::vector<DescriptorSetLayoutBinding>& bindings) {
    std::vector<VkDescriptorSetLayoutBinding> result;
    
    for (auto& b : bindings) {
        VkDescriptorSetLayoutBinding dslb;
        dslb.binding            = b.binding;
        dslb.descriptorType     = mapDescriptorType(b.descriptorType);
        dslb.descriptorCount    = b.descriptorCount;
        dslb.stageFlags         = vk::shaderStage(b.stageFlags);
        dslb.pImmutableSamplers = reinterpret_cast<VkSampler*>(const_cast<SamplerHnd*>(b.immutableSamplers.data()));
        
        result.push_back(dslb);
    }
    
    return result;
}

inline std::vector<VkDescriptorPoolSize> mapPoolSizes(const std::vector<DescriptorPoolSize>& size) {
    std::vector<VkDescriptorPoolSize> result;
    
    for (auto& s : size) {
        VkDescriptorPoolSize dps;
        dps.type            = mapDescriptorType(s.type);
        dps.descriptorCount = s.descriptorCount;
        
        result.push_back(dps);
    }
    
    return result;
}

inline std::vector<VkWriteDescriptorSet> mapWriteDescriptorSet(const std::vector<WriteDescriptorSet>& set) {
    std::vector<VkWriteDescriptorSet> result;
    
    for (auto& s : set) {
        VkWriteDescriptorSet wds;
        wds.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds.pNext            = nullptr;
        wds.dstSet           = s.dstSet.toNative<VkDescriptorSet>();
        wds.dstBinding       = s.dstBinding;
        wds.dstArrayElement  = s.dstArrayElement;
        wds.descriptorCount  = s.descriptorCount;
        wds.descriptorType   = mapDescriptorType(s.descriptorType);
        wds.pImageInfo       = VK_NULL_HANDLE;
        wds.pBufferInfo      = VK_NULL_HANDLE;
        wds.pTexelBufferView = VK_NULL_HANDLE;
        
        result.push_back(wds);
    }
    
    return result;
}

inline VkFilter mapFilter(Filter filter) {
    switch (filter) {
    case Filter::Nearest:
        return VK_FILTER_NEAREST;
    case Filter::Linear:
        return VK_FILTER_LINEAR;
    default:
        throw std::invalid_argument("Invalid filter type");
    }
}

inline VkSamplerMipmapMode mapMipmapMode(SamplerMipmapMode mode) {
    switch (mode) {
    case SamplerMipmapMode::Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case SamplerMipmapMode::Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        throw std::invalid_argument("Invalid sampler mipmap mode");
    }
}

inline VkSamplerAddressMode mapAddressMode(SamplerAddressMode mode) {
    switch (mode) {
    case SamplerAddressMode::Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case SamplerAddressMode::MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case SamplerAddressMode::ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SamplerAddressMode::ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case SamplerAddressMode::MirrorClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        throw std::invalid_argument("Invalid sampler address mode");
    }
}
}
}

#endif // IYF_VULKAN_UTILITIES_HPP
