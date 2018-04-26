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

#ifdef BUILD_WITH_VULKAN
#include "graphics/vulkan/VulkanAPI.hpp"
#include "graphics/vulkan/VulkanConstantMappings.hpp"

#include "core/Logger.hpp"

#include <iostream>

#include <fstream>
#include <cstring>
#include <SDL_syswm.h>

#undef Always // X11 Macro
#undef None // X11 Macro

#include "stb_image.h"
#include "assets/loaders/TextureLoader.hpp"

namespace iyf {

namespace vkutil {
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

inline std::pair<VkFormat, Format> compressionFormatToEngineFormat(TextureCompressionFormat tcf, bool sRGB) {
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
    
    return {vk::format(engineFormat), engineFormat};
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

// TODO move to memory cpp
std::uint32_t VulkanAPI::getMemoryType(std::uint32_t typeBits, VkFlags propertyFlags) {
    for (std::uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; ++i) {
        if ((typeBits & 1) == 1) {
            if ((physicalDevice.memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
                return i;
            }
        }
        
        typeBits >>= 1;
    }
    
    throw std::runtime_error("Couldn't get required memory type " + std::to_string(typeBits) + " " + std::to_string(propertyFlags));
}

void VulkanAPI::setImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange imageSubresourceRange, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags destStageFlags) {
    // Pagal https://github.com/SaschaWillems/Vulkan/blob/master/base/vulkantools.cpp ir Vulkan SDK cube.c pavyzdį
    VkImageMemoryBarrier imb;
    imb.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imb.pNext               = nullptr;
    imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imb.oldLayout           = oldImageLayout;
    imb.newLayout           = newImageLayout;
    imb.image               = image;
    imb.subresourceRange    = imageSubresourceRange;
    imb.srcAccessMask       = 0;
    imb.dstAccessMask       = 0;

    switch (oldImageLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imb.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imb.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imb.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
        //TODO VK_IMAGE_LAYOUT_GENERAL?
    }

    switch (newImageLayout) {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        //imb.srcAccessMask = imb.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
        imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        //imb.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imb.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imb.dstAccessMask = imb.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (imb.srcAccessMask == 0) {
            imb.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
        //TODO Kiti?
    }

    vkCmdPipelineBarrier(cmd, srcStageFlags, destStageFlags, 0, 0, nullptr, 0, nullptr, 1, &imb);
}

void VulkanAPI::setupPresentationBarrierCommandBuffers() {
    prePresentationBarrierCommands.resize(swapchain.images.size());
    postPresentationBarrierCommands.resize(swapchain.images.size());

    VkCommandBufferAllocateInfo cbai;
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext              = nullptr;
    cbai.commandPool        = commandPool;
    cbai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = swapchain.images.size();

    VkResult result;
    result = vkAllocateCommandBuffers(logicalDevice.handle, &cbai, prePresentationBarrierCommands.data());
    checkResult(result, "Buffer allocation failed.");
    result = vkAllocateCommandBuffers(logicalDevice.handle, &cbai, postPresentationBarrierCommands.data());
    checkResult(result, "Buffer allocation failed.");
    
    VkCommandBufferBeginInfo cbbi;
    cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext            = nullptr;
    cbbi.flags            = 0;//TODO?
    cbbi.pInheritanceInfo = nullptr;
    //TODO usage flags and inherritance info

    for (std::uint32_t i = 0; i < swapchain.images.size(); ++i) {
        result = vkBeginCommandBuffer(prePresentationBarrierCommands[i], &cbbi);
        checkResult(result, "Failed to begin a command buffer.");
        
        VkImageSubresourceRange isr;
        isr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        isr.baseMipLevel   = 0;
        isr.levelCount     = 1;
        isr.baseArrayLayer = 0;
        isr.layerCount     = 1;

        VkImageMemoryBarrier imbPrePresentation;
        imbPrePresentation.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imbPrePresentation.pNext               = nullptr;
        imbPrePresentation.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imbPrePresentation.dstAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        imbPrePresentation.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imbPrePresentation.newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imbPrePresentation.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imbPrePresentation.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imbPrePresentation.subresourceRange    = isr;
        imbPrePresentation.image               = swapchain.images[i];

        vkCmdPipelineBarrier(prePresentationBarrierCommands[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imbPrePresentation);

        result = vkEndCommandBuffer(prePresentationBarrierCommands[i]);
        checkResult(result, "Failed to end a command buffer.");

        result = vkBeginCommandBuffer(postPresentationBarrierCommands[i], &cbbi);
        checkResult(result, "Failed to begin a command buffer.");

        VkImageMemoryBarrier imbPostPresentation;
        imbPostPresentation.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imbPostPresentation.pNext               = nullptr;
        imbPostPresentation.srcAccessMask       = 0;
        imbPostPresentation.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        imbPostPresentation.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        imbPostPresentation.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        imbPostPresentation.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imbPostPresentation.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imbPostPresentation.subresourceRange    = isr;
        imbPostPresentation.image               = swapchain.images[i];

        // TODO this should be replaced with pass transitions
        vkCmdPipelineBarrier(postPresentationBarrierCommands[i], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imbPostPresentation);

        result = vkEndCommandBuffer(postPresentationBarrierCommands[i]);
        checkResult(result, "Failed to end a command buffer.");
    }
}

void VulkanAPI::freePresentationBarrierCommandBuffers() {
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, swapchain.images.size(), prePresentationBarrierCommands.data());
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, swapchain.images.size(), postPresentationBarrierCommands.data());
}

VkCommandBuffer VulkanAPI::allocateCommandBuffer(VkCommandBufferLevel bufferLevel, std::uint32_t bufferCount, bool beginBuffer) {
    VkResult result;
    VkCommandBuffer buffer;

    VkCommandBufferAllocateInfo cbai;
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext              = nullptr;
    cbai.commandPool        = commandPool;
    cbai.level              = bufferLevel;
    cbai.commandBufferCount = bufferCount;

    result = vkAllocateCommandBuffers(logicalDevice.handle, &cbai, &buffer);
    checkResult(result, "Failed to allocate command buffers.");

    if (beginBuffer) {
        VkCommandBufferBeginInfo cbbi;
        cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.pNext            = nullptr;
        cbbi.flags            = 0;//TODO?
        cbbi.pInheritanceInfo = nullptr;
        
        result = vkBeginCommandBuffer(buffer, &cbbi);
        checkResult(result, "Failed to begin a command buffer.");
        if (result != VK_SUCCESS) {
            throw std::runtime_error("");
        }
    }

    return buffer;
}

void VulkanAPI::freeCommandBuffer(const VkCommandBuffer& commandBuffer) {
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &commandBuffer);
}

void VulkanAPI::dispose() {
    destroyHelperMeshesAndObjectsaryMeshes();
    
    freePresentationBarrierCommandBuffers();
    physicalDevice.queueFamilyProperties.clear();
    
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &mainCommandBuffer);
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &imageUploadCommandBuffer);
    
    for (auto& fb : framebuffers) {
        vkDestroyFramebuffer(logicalDevice.handle, fb, nullptr);
    }
    
    for (std::uint32_t i = 0; i < swapchain.images.size(); ++i) {
        vkDestroyImageView(logicalDevice.handle, swapchain.imageViews[i], nullptr);
    }
    
    vkDestroyRenderPass(logicalDevice.handle, renderPass, nullptr);
    vkDestroySwapchain(logicalDevice.handle, swapchain.handle, nullptr);
    
    vkDestroyImageView(logicalDevice.handle, depthStencilView, nullptr);
    vkDestroyImage(logicalDevice.handle, depthStencilImage, nullptr);
    vkFreeMemory(logicalDevice.handle, depthStencilMemory, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    vkDestroyPipelineCache(logicalDevice.handle, pipelineCache, nullptr);
    vkDestroyCommandPool(logicalDevice.handle, commandPool, nullptr);
    
    vkDestroySemaphore(logicalDevice.handle, presentationComplete, nullptr);
    vkDestroySemaphore(logicalDevice.handle, renderingComplete, nullptr);
    
    vmaDestroyAllocator(allocator);
    
    vkDestroyDevice(logicalDevice.handle, nullptr);
    
    if (isDebug) {
        destroyDebugReportCallback(instance, debugReportCallback, nullptr);
    }
    
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
}

bool VulkanAPI::startFrame() {
    VkResult result = vkAcquireNextImage(logicalDevice.handle, swapchain.handle, UINT64_MAX, presentationComplete, (VkFence)nullptr, &currentSwapBuffer);
    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        LOG_W("vkAcquireNextImage returned VK_SUBOPTIMAL_KHR");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        //TODO window size change
        LOG_W("vkAcquireNextImage returned VK_ERROR_OUT_OF_DATE_KHR");
        break;
    default:
        checkResult(result, "Failed to acquire a new image.");
        return false;
    }
    
    VkSubmitInfo si;
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 0;
    si.pWaitSemaphores      = nullptr;
    si.pWaitDstStageMask    = nullptr;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &postPresentationBarrierCommands[currentSwapBuffer];
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores    = nullptr;
    
    result = vkQueueSubmit(logicalDevice.mainQueue, 1, &si, VK_NULL_HANDLE);
    checkResult(result, "Failed to submmit a queue.");
    
//-----------------------------------------------------------------------------
/*    const VkClearColorValue clearColor = {{0.75f, 0.95f, 0.98f, 1.0f}};
    
    VkCommandBufferBeginInfo cbbi;
    cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext            = nullptr;
    cbbi.flags            = 0;//TODO?
    cbbi.pInheritanceInfo = nullptr;
    
    vkBeginCommandBuffer(mainCommandBuffer, &cbbi);
    
    VkClearValue clearValues[2];
    clearValues[0].color = clearColor;
    clearValues[1].depthStencil.depth = 1.0f;
    clearValues[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rpbi;
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.pNext = nullptr;
    rpbi.renderPass = renderPass;
    rpbi.framebuffer = framebuffers[currentSwapBuffer];
    rpbi.renderArea.offset.x = 0;
    rpbi.renderArea.offset.y = 0;
    rpbi.renderArea.extent.width = getRenderSurfaceWidth();
    rpbi.renderArea.extent.height = getRenderSurfaceHeight();
    rpbi.clearValueCount = 2;
    rpbi.pClearValues = clearValues;

    vkCmdBeginRenderPass(mainCommandBuffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);*/
    
    return true;
}

bool VulkanAPI::endFrame() {
    VkResult result;
    VkSubmitInfo si;
 /*   vkCmdEndRenderPass(mainCommandBuffer);
    
    vkEndCommandBuffer(mainCommandBuffer);
    
    VkPipelineStageFlags mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    
    
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &presentationComplete;
    si.pWaitDstStageMask    = &mask;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &mainCommandBuffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &renderingComplete;
    
    result = vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
    checkResult(result, "queue_submit_fail");*/
    
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 0;
    si.pWaitSemaphores      = nullptr;
    si.pWaitDstStageMask    = nullptr;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &prePresentationBarrierCommands[currentSwapBuffer];
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores    = nullptr;
    
    result = vkQueueSubmit(logicalDevice.mainQueue, 1, &si, VK_NULL_HANDLE);
    checkResult(result, "Failed to submit a queue");
    
    // TODO fence?
    //VkResult fenceRes;
    //do {
    //    fenceRes = vkWaitForFences(logicalDevice.handle, 1, &renderFence, VK_TRUE, 100000000);
    //} while (fenceRes == VK_TIMEOUT);
    //VK_CHECK_RESULT(fenceRes);
    //vkResetFences(logicalDevice.handle, 1, &renderFence);

    VkPresentInfoKHR pi;
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.pNext              = nullptr;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &swapchain.handle;
    pi.pImageIndices      = &currentSwapBuffer;
    pi.pWaitSemaphores    = &renderingComplete;
    pi.waitSemaphoreCount = 1;
    pi.pResults           = nullptr;
    
    result = vkQueuePresentKHR(logicalDevice.mainQueue, &pi);

    switch (result) {
    case VK_SUCCESS:
        break;
    case VK_SUBOPTIMAL_KHR:
        LOG_W("vkQueuePresentKHR returned VK_SUBOPTIMAL_KHR");
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        //TODO window size change
        LOG_W("vkQueuePresentKHR returned VK_ERROR_OUT_OF_DATE_KHR");
        break;
    default:
        checkResult(result, "Failed to present an image.");
        return false;
    }
    
    // TODO no longer explicitly wait. Look for examples
    result = vkQueueWaitIdle(logicalDevice.mainQueue);
    checkResult(result, "Failed to wait on a queue.");
    //vkDeviceWaitIdle(logicalDevice.handle);
    
    return true;
}

ShaderHnd VulkanAPI::createShader(ShaderStageFlags shaderStageFlag, const void* data, std::size_t byteCount) {
    VkShaderModuleCreateInfo mci;
    mci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    mci.pNext    = nullptr;
    mci.codeSize = byteCount;
    mci.pCode    = static_cast<const std::uint32_t*>(data);
    mci.flags    = 0;

    VkShaderModule module;
    checkResult(vkCreateShaderModule(logicalDevice.handle, &mci, nullptr, &module), "Failed to create a shader module.");

    return ShaderHnd(module);
}

ShaderHnd VulkanAPI::createShaderFromSource(ShaderStageFlags, const std::string&) {
    throw std::runtime_error("Method not supported");
    return ShaderHnd();
}

bool VulkanAPI::destroyShader(ShaderHnd handle) {
    vkDestroyShaderModule(logicalDevice.handle, handle.toNative<VkShaderModule>(), nullptr);
    return true;
}

Pipeline VulkanAPI::createPipeline(const PipelineCreateInfo& info) {
    const auto& visci = info.vertexInputState;
    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.reserve(visci.vertexBindingDescriptions.size());
    
    for (auto &b : visci.vertexBindingDescriptions) {
        VkVertexInputBindingDescription temp;
        temp.binding   = static_cast<std::uint32_t>(b.binding);
        temp.stride    = b.stride;
        temp.inputRate = vkutil::mapInputRate(b.inputRate);
        
        bindings.push_back(temp);
    }
    
    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(visci.vertexAttributeDescriptions.size());
    
    for (auto &b : visci.vertexAttributeDescriptions) {
        VkVertexInputAttributeDescription temp;
        temp.location = b.location;
        temp.binding  = static_cast<std::uint32_t>(b.binding);
        temp.format   = vk::format(b.format);
        temp.offset   = b.offset;
        
        attributes.push_back(temp);
    }
    
    VkPipelineVertexInputStateCreateInfo pvisci;
    pvisci.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pvisci.pNext                           = nullptr;
    pvisci.flags                           = 0;
    pvisci.vertexBindingDescriptionCount   = bindings.size();
    pvisci.pVertexBindingDescriptions      = bindings.data();
    pvisci.vertexAttributeDescriptionCount = attributes.size();
    pvisci.pVertexAttributeDescriptions    = attributes.data();


    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(info.shaders.size());
    
    for (auto &s : info.shaders) {
        VkPipelineShaderStageCreateInfo pssci;
        pssci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pssci.pNext               = nullptr;
        pssci.flags               = 0;
        pssci.stage               = static_cast<VkShaderStageFlagBits>(vk::shaderStage(s.first));
        pssci.module              = s.second.toNative<VkShaderModule>();
        pssci.pName               = "main";
        pssci.pSpecializationInfo = nullptr;
        
        shaderStages.push_back(pssci);
    }

    
    const auto& tsci = info.tessellationState;
    VkPipelineTessellationStateCreateInfo ptsci;
    ptsci.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    ptsci.pNext              = nullptr;
    ptsci.flags              = 0;
    ptsci.patchControlPoints = tsci.patchControlPoints;
    
    
    const auto& vsci = info.viewportState;
    std::vector<VkViewport> viewports;
    
    if (vsci.viewports.size() != 0) { //TODO if not dynamic?
        viewports.reserve(vsci.viewports.size());
        for (auto &v : vsci.viewports) {
            VkViewport vp;
            vp.x        = v.x;
            vp.y        = v.y;
            vp.width    = v.width;
            vp.height   = v.height;
            vp.minDepth = v.minDepth;
            vp.maxDepth = v.maxDepth;
            
            viewports.push_back(vp);
        }
    } else {
        VkViewport vp;
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = getRenderSurfaceWidth();
        vp.height = getRenderSurfaceHeight();
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;

        viewports.push_back(vp);
    }
    
    std::vector<VkRect2D> scissors;
    
    if (vsci.scissors.size() != 0) {
        scissors.reserve(vsci.scissors.size());
        for (auto &s : vsci.scissors) {
            VkRect2D sc;
            sc.offset.x      = s.offset.x;
            sc.offset.y      = s.offset.y;
            sc.extent.width  = s.extent.x;
            sc.extent.height = s.extent.y;
            
            scissors.push_back(sc);
        }
    } else {
        VkRect2D sc;
        sc.offset.x = 0;
        sc.offset.y = 0;
        sc.extent.width = getRenderSurfaceWidth();
        sc.extent.height = getRenderSurfaceHeight();
               
        scissors.push_back(sc);
    }
    
    
    VkPipelineViewportStateCreateInfo pvsci;
    pvsci.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pvsci.pNext         = nullptr;
    pvsci.flags         = 0;
    pvsci.viewportCount = static_cast<std::uint32_t>(viewports.size());
    pvsci.pViewports    = viewports.data();
    pvsci.scissorCount  = static_cast<std::uint32_t>(scissors.size());
    pvsci.pScissors     = scissors.data();
    
    
    const auto& rsci = info.rasterizationState;
    VkPipelineRasterizationStateCreateInfo prsci;
    prsci.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    prsci.pNext                   = nullptr;
    prsci.flags                   = 0;
    prsci.depthClampEnable        = rsci.depthClampEnable ? VK_TRUE : VK_FALSE;
    prsci.rasterizerDiscardEnable = rsci.rasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
    prsci.polygonMode             = vk::polygonMode(rsci.polygonMode);
    prsci.cullMode                = vk::cullMode(rsci.cullMode);
    prsci.frontFace               = vk::frontFace(rsci.frontFace);
    prsci.depthBiasEnable         = rsci.depthBiasEnable ? VK_TRUE : VK_FALSE;
    prsci.depthBiasConstantFactor = rsci.depthBiasConstantFactor;
    prsci.depthBiasClamp          = rsci.depthBiasClamp;
    prsci.depthBiasSlopeFactor    = rsci.depthBiasSlopeFactor;
    prsci.lineWidth               = rsci.lineWidth;
    
    
    const auto& dssci = info.depthStencilState;
    VkPipelineDepthStencilStateCreateInfo pdssci;
    pdssci.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pdssci.pNext                 = nullptr;
    pdssci.flags                 = 0;
    pdssci.depthTestEnable       = dssci.depthTestEnable ? VK_TRUE : VK_FALSE;
    pdssci.depthWriteEnable      = dssci.depthWriteEnable ? VK_TRUE : VK_FALSE;
    pdssci.depthCompareOp        = vk::compareOp(dssci.depthCompareOp);
    pdssci.depthBoundsTestEnable = dssci.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
    pdssci.stencilTestEnable     = dssci.stencilTestEnable ? VK_TRUE : VK_FALSE;
    pdssci.minDepthBounds        = dssci.minDepthBounds;
    pdssci.maxDepthBounds        = dssci.maxDepthBounds;
    
    pdssci.front.failOp      = vk::stencilOp(dssci.front.failOp);
    pdssci.front.passOp      = vk::stencilOp(dssci.front.passOp);
    pdssci.front.depthFailOp = vk::stencilOp(dssci.front.depthFailOp);
    pdssci.front.compareOp   = vk::compareOp(dssci.front.compareOp);
    pdssci.front.compareMask = dssci.front.compareMask;
    pdssci.front.writeMask   = dssci.front.writeMask;
    pdssci.front.reference   = dssci.front.reference;
    
    pdssci.back.failOp      = vk::stencilOp(dssci.back.failOp);
    pdssci.back.passOp      = vk::stencilOp(dssci.back.passOp);
    pdssci.back.depthFailOp = vk::stencilOp(dssci.back.depthFailOp);
    pdssci.back.compareOp   = vk::compareOp(dssci.back.compareOp);
    pdssci.back.compareMask = dssci.back.compareMask;
    pdssci.back.writeMask   = dssci.back.writeMask;
    pdssci.back.reference   = dssci.back.reference;
    
    
    const auto& msci = info.multisampleState;
    VkPipelineMultisampleStateCreateInfo pmsci;
    pmsci.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pmsci.pNext                 = nullptr;
    pmsci.flags                 = 0;
    pmsci.rasterizationSamples  = static_cast<VkSampleCountFlagBits>(vk::sampleCount(msci.rasterizationSamples));
    pmsci.sampleShadingEnable   = msci.sampleShadingEnable ? VK_TRUE : VK_FALSE;
    pmsci.minSampleShading      = msci.minSampleShading;
    pmsci.pSampleMask           = msci.sampleMask.size() > 0 ? msci.sampleMask.data() : nullptr;
    pmsci.alphaToCoverageEnable = msci.alphaToCoverageEnable ? VK_TRUE : VK_FALSE;
    pmsci.alphaToOneEnable      = msci.alphaToOneEnable ? VK_TRUE : VK_FALSE;

    const auto& dsci = info.dynamicState;
    auto mappedState = vkutil::mapDynamicState(dsci.dynamicStates);
    VkPipelineDynamicStateCreateInfo pdsci;
    pdsci.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pdsci.pNext             = nullptr;
    pdsci.flags             = 0;
    pdsci.dynamicStateCount = dsci.dynamicStates.size();
    pdsci.pDynamicStates    = dsci.dynamicStates.size() > 0 ? mappedState.data() : nullptr;
    
    const auto& cbsci = info.colorBlendState;
    VkPipelineColorBlendStateCreateInfo pcbsci;
    pcbsci.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pcbsci.pNext             = nullptr;
    pcbsci.flags             = 0;
    pcbsci.logicOpEnable     = cbsci.logicOpEnable ? VK_TRUE : VK_FALSE;
    pcbsci.logicOp           = vk::logicOp(cbsci.logicOp);
    pcbsci.attachmentCount   = cbsci.attachments.size();
    auto attachments         = vkutil::mapAttachments(cbsci.attachments);
    pcbsci.pAttachments      = attachments.data();
    pcbsci.blendConstants[0] = cbsci.blendConstants.x;
    pcbsci.blendConstants[1] = cbsci.blendConstants.y;
    pcbsci.blendConstants[2] = cbsci.blendConstants.z;
    pcbsci.blendConstants[3] = cbsci.blendConstants.w;
    
    
    const auto& iasci = info.inputAssemblyState;
    VkPipelineInputAssemblyStateCreateInfo piasci;
    piasci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    piasci.pNext                  = nullptr;
    piasci.flags                  = 0;
    piasci.topology               = vk::primitiveTopology(iasci.topology);
    piasci.primitiveRestartEnable = iasci.primitiveRestartEnable;
    
    VkGraphicsPipelineCreateInfo pci;
    pci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.pNext               = nullptr;
    pci.flags               = 0; //TODO ? VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT/VK_PIPELINE_CREATE_DERIVATIVE_BIT ?
    pci.stageCount          = shaderStages.size();
    pci.pStages             = shaderStages.data();
    pci.pVertexInputState   = &pvisci;
    pci.pInputAssemblyState = &piasci;
    pci.pTessellationState  = &ptsci;
    pci.pViewportState      = &pvsci;
    pci.pRasterizationState = &prsci;
    pci.pMultisampleState   = &pmsci;
    pci.pDepthStencilState  = &pdssci;
    pci.pColorBlendState    = &pcbsci;
    pci.pDynamicState       = (dsci.dynamicStates.size() > 0) ? &pdsci : nullptr;
    pci.layout              = info.layout.toNative<VkPipelineLayout>();
    pci.renderPass          = info.renderPass.toNative<VkRenderPass>();
    pci.subpass             = info.subpass;
    pci.basePipelineHandle  = nullptr;
    pci.basePipelineIndex   = -1;

    VkPipeline pipeline;
    vkCreateGraphicsPipelines(logicalDevice.handle, pipelineCache, 1, &pci, nullptr, &pipeline);
    
    Pipeline pipelineObj;
    pipelineObj.handle = PipelineHnd(pipeline);
    pipelineObj.bindPoint = PipelineBindPoint::Graphics;
    
    return pipelineObj;
}

bool VulkanAPI::destroyPipeline(const Pipeline& pipeline) {
    vkDestroyPipeline(logicalDevice.handle, pipeline.handle.toNative<VkPipeline>(), nullptr);
    return true;
}

Pipeline VulkanAPI::createPipeline(const ComputePipelineCreateInfo& info) {
    VkPipelineShaderStageCreateInfo pssci;
    pssci.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pssci.pNext               = nullptr;
    pssci.flags               = 0;
    pssci.stage               = static_cast<VkShaderStageFlagBits>(vk::shaderStage(info.shader.first));
    pssci.module              = info.shader.second.toNative<VkShaderModule>();
    pssci.pName               = "main";
    pssci.pSpecializationInfo = nullptr;

    VkComputePipelineCreateInfo pci;
    pci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pci.pNext              = nullptr;
    pci.flags              = 0;
    pci.stage              = pssci;
    pci.layout             = info.layout.toNative<VkPipelineLayout>();
    pci.basePipelineHandle = nullptr;
    pci.basePipelineIndex  = -1;
    
    VkPipeline pipeline;
    vkCreateComputePipelines(logicalDevice.handle, pipelineCache, 1, &pci, nullptr, &pipeline);
//    vkCreateGraphicsPipelines(logicalDevice.handle, pipelineCache, 1, &pci, nullptr, &pipeline);
    
    Pipeline pipelineObj;
    pipelineObj.handle = PipelineHnd(pipeline);
    pipelineObj.bindPoint = PipelineBindPoint::Compute;
    
    return pipelineObj;
}

PipelineLayoutHnd VulkanAPI::createPipelineLayout(const PipelineLayoutCreateInfo& info) {
    VkPipelineLayoutCreateInfo plci;
    plci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pNext                  = nullptr;
    plci.flags                  = 0;
    
    const auto setLayouts = vkutil::mapSetLayouts(info.setLayouts);
    plci.setLayoutCount         = setLayouts.size();
    plci.pSetLayouts            = setLayouts.size() > 0 ? setLayouts.data() : nullptr;
    
    const auto constantRanges = vkutil::mapPushConstantRanges(info.pushConstantRanges);
    plci.pushConstantRangeCount = constantRanges.size();
    plci.pPushConstantRanges    = constantRanges.size() > 0 ? constantRanges.data() : nullptr;
    
    VkPipelineLayout pl;
    vkCreatePipelineLayout(logicalDevice.handle, &plci, nullptr, &pl);
    
    return PipelineLayoutHnd(pl);
}

bool VulkanAPI::destroyPipelineLayout(PipelineLayoutHnd handle) {
    vkDestroyPipelineLayout(logicalDevice.handle, handle.toNative<VkPipelineLayout>(), nullptr);
    return true;
}

DescriptorSetLayoutHnd VulkanAPI::createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) {
    VkDescriptorSetLayoutCreateInfo dslci;
    dslci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslci.pNext        = nullptr;
    dslci.flags        = 0;
    
    const auto bindings = vkutil::mapBindings(info.bindings);
    dslci.bindingCount = bindings.size();
    dslci.pBindings    = bindings.size() > 0 ? bindings.data() : nullptr;
    
    VkDescriptorSetLayout dsl;
    
    vkCreateDescriptorSetLayout(logicalDevice.handle, &dslci, nullptr, &dsl);
    return DescriptorSetLayoutHnd(dsl);
}

bool VulkanAPI::destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) {
    vkDestroyDescriptorSetLayout(logicalDevice.handle, handle.toNative<VkDescriptorSetLayout>(), nullptr);
    return true;
}

std::vector<DescriptorSetHnd> VulkanAPI::allocateDescriptorSets(const DescriptorSetAllocateInfo& info) {
    //VkDescriptorSet ds;
    std::vector<DescriptorSetHnd> ds;
    ds.resize(info.setLayouts.size());
    
    VkDescriptorSetAllocateInfo dsai;
    dsai.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsai.pNext              = nullptr;
    dsai.descriptorPool     = info.descriptorPool.toNative<VkDescriptorPool>();
    dsai.descriptorSetCount = info.setLayouts.size();
    auto setLayouts = vkutil::mapSetLayouts(info.setLayouts);
    dsai.pSetLayouts        = setLayouts.data();
    
    checkResult(vkAllocateDescriptorSets(logicalDevice.handle, &dsai, reinterpret_cast<VkDescriptorSet*>(ds.data())), "descriptor_set_allocate_fail");
    
    return ds;
}

bool VulkanAPI::updateDescriptorSets(const std::vector<WriteDescriptorSet>& set) {
    std::vector<VkWriteDescriptorSet> mappedSet = vkutil::mapWriteDescriptorSet(set);
    
    std::vector<VkDescriptorImageInfo> dii;
    std::vector<VkDescriptorBufferInfo> dbi;
    std::vector<VkBufferView> bv;
    
    std::size_t diiTotal = 0;
    std::size_t dbiTotal = 0;
    std::size_t bvTotal = 0;
    for (auto& s : set) {
        diiTotal += s.imageInfos.size();
        dbiTotal += s.bufferInfos.size();
        bvTotal += s.texelBufferViews.size();
    }
    
    dii.reserve(diiTotal);
    dbi.reserve(dbiTotal);
    bv.reserve(bvTotal);
    
    for (size_t i = 0; i < mappedSet.size(); ++i) {
        if (mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER || 
            mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            
            for (auto& tbv : set[i].texelBufferViews) {
                bv.push_back(tbv.toNative<VkBufferView>());
            }
            
            mappedSet[i].pTexelBufferView = &bv[bv.size() - set[i].texelBufferViews.size()];
        } else if (mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                   mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                   mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                   mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            
            for (auto& bi : set[i].bufferInfos) {
                VkDescriptorBufferInfo info;
                info.buffer = bi.buffer.toNative<VkBuffer>();
                info.offset = bi.offset;
                info.range  = bi.range;
                
                dbi.push_back(info);
            }
            
            mappedSet[i].pBufferInfo = &dbi[dbi.size() - set[i].bufferInfos.size()];
        } else {
            //mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || 
            //mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            //mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            //mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
            //mappedSet[i].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
            for (auto& ii : set[i].imageInfos) {
                VkDescriptorImageInfo info;
                info.sampler     = ii.sampler.toNative<VkSampler>();
                info.imageView   = ii.imageView.toNative<VkImageView>();
                info.imageLayout = vk::imageLayout(ii.imageLayout);
                
                dii.push_back(info);
            }
            
            mappedSet[i].pImageInfo = &dii[dii.size() - set[i].imageInfos.size()];
        }
    }
        
    vkUpdateDescriptorSets(logicalDevice.handle, mappedSet.size(), mappedSet.data(), 0, nullptr);
    return true;
}

DescriptorPoolHnd VulkanAPI::createDescriptorPool(const DescriptorPoolCreateInfo& info) {
    VkDescriptorPoolCreateInfo dpci;
    dpci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.pNext         = nullptr;
    dpci.flags         = 0;
    dpci.maxSets       = info.maxSets; // TODO Reasonable limits?
    dpci.poolSizeCount = info.poolSizes.size();
    auto poolSizes = vkutil::mapPoolSizes(info.poolSizes);
    dpci.pPoolSizes    = poolSizes.data();
    
    VkDescriptorPool dp;
    checkResult(vkCreateDescriptorPool(logicalDevice.handle, &dpci, nullptr, &dp), "Failed to create a descriptor pool.");
    
    return DescriptorPoolHnd(dp);
}

bool VulkanAPI::destroyDescriptorPool(DescriptorPoolHnd handle) {
    vkDestroyDescriptorPool(logicalDevice.handle, handle.toNative<VkDescriptorPool>(), nullptr);
    
    return true;
}

Image VulkanAPI::createCompressedImage(const void* inputData, std::size_t byteCount) {
    //TODO START USING THE ALLOCATOR HERE. USE A MUTEX ON THE IMAGE TRANSFER BUFFER OR A FENCE?
    TextureLoader loader;
    TextureLoader::Data textureData;
    TextureLoader::Result result = loader.load(inputData, byteCount, textureData);
    
    if (result != TextureLoader::Result::LoadSuccessful) {
        throw std::runtime_error("Failed to load a texture");
    }
    
    auto formatMapping = vkutil::compressionFormatToEngineFormat(textureData.format, textureData.sRGB);
    VkFormat format = formatMapping.first;
    Format engineFormat = formatMapping.second;
    
    ImageViewType imViewType;
    VkImageType imageType;
    
    // For now, I only support a subset of image types
    if (textureData.faceCount == 1) {
        imageType = VK_IMAGE_TYPE_2D;
        imViewType = ImageViewType::Im2D;
    } else if (textureData.faceCount == 6) {
        imageType = VK_IMAGE_TYPE_2D;
        imViewType = ImageViewType::ImCube;
    } else {
        throw std::runtime_error("A texture must have 1 or 6 faces.");
    }
    
    auto stagingData = createTemporaryBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, textureData.size, textureData.data);
    
    std::vector<VkBufferImageCopy> bics;
    size_t layerId = 0;
    size_t offset = 0;
    
    for (std::size_t face = 0; face < textureData.faceCount; ++face) {
        for (std::size_t level = 0; level < textureData.mipmapLevelCount; ++level) {
            const glm::uvec3& extents = textureData.getLevelExtents(level);
            
            VkBufferImageCopy bic;
            bic.bufferOffset      = offset;
            bic.bufferRowLength   = 0;
            bic.bufferImageHeight = 0;
            
            bic.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            bic.imageSubresource.mipLevel       = level;
            bic.imageSubresource.baseArrayLayer = layerId;
            bic.imageSubresource.layerCount     = 1;
            
            bic.imageOffset.x = 0;
            bic.imageOffset.y = 0;
            bic.imageOffset.z = 0;
            
            bic.imageExtent.width  = extents.x;
            bic.imageExtent.height = extents.y;
            bic.imageExtent.depth  = extents.z;
            
            bics.push_back(bic);
            
            offset += textureData.getLevelSize(level);
        }
        
        layerId++;
    }
    
    VkImageCreateInfo ici;
    ici.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.pNext                 = nullptr;
    ici.flags                 = (imViewType == ImageViewType::ImCube) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; //TODO CUBEMAP flags
    ici.imageType             = imageType;
    ici.format                = format;
    ici.extent                = {textureData.width, textureData.height, textureData.depth};
    ici.mipLevels             = textureData.mipmapLevelCount;
    ici.arrayLayers           = (textureData.faceCount == 6) ? 6 : textureData.layers;
    ici.samples               = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ici.usage                 = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ici.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ici.queueFamilyIndexCount = 0;
    ici.pQueueFamilyIndices   = nullptr;
    ici.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    checkResult(vkCreateImage(logicalDevice.handle, &ici, nullptr, &image), "Failed to create an image.");

    VkMemoryRequirements mrDev;
    vkGetImageMemoryRequirements(logicalDevice.handle, image, &mrDev);
    
    VkMemoryAllocateInfo maiDev;
    maiDev.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    maiDev.pNext           = nullptr;
    maiDev.allocationSize  = mrDev.size;
    maiDev.memoryTypeIndex = getMemoryType(mrDev.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory imageMemory;
    checkResult(vkAllocateMemory(logicalDevice.handle, &maiDev, nullptr, &imageMemory), "Failed to allocate memory.");
    
    checkResult(vkBindImageMemory(logicalDevice.handle, image, imageMemory, 0), "Failed to bind memory.");
    
    imageToMemory[image] = imageMemory;

    VkCommandBufferBeginInfo cbbi;
    cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cbbi.pNext            = nullptr;
    cbbi.flags            = 0;//TODO?
    cbbi.pInheritanceInfo = nullptr;
    
    checkResult(vkBeginCommandBuffer(imageUploadCommandBuffer, &cbbi), "Failed to begin a command buffer.");

    VkImageSubresourceRange sr;
    sr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    sr.baseMipLevel   = 0;
    sr.levelCount     = textureData.mipmapLevelCount;
    sr.baseArrayLayer = 0;
    sr.layerCount     = (textureData.faceCount == 6) ? 6 : textureData.layers;

    setImageLayout(imageUploadCommandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, sr);
    vkCmdCopyBufferToImage(imageUploadCommandBuffer, stagingData.first, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bics.size(), bics.data());
    setImageLayout(imageUploadCommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sr);

    checkResult(vkEndCommandBuffer(imageUploadCommandBuffer), "Failed to end a command buffer.");

    VkFenceCreateInfo fci;
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = nullptr;
    fci.flags = 0;
    
    VkFence imageCopyFence;
    checkResult(vkCreateFence(logicalDevice.handle, &fci, nullptr, &imageCopyFence), "Failed to create a fence."); //TODO reset?

    VkSubmitInfo si;
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 0;
    si.pWaitSemaphores      = nullptr;
    si.pWaitDstStageMask    = nullptr;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &imageUploadCommandBuffer;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores    = nullptr;

    checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, imageCopyFence), "Failed to submmit a queue.");
    checkResult(vkWaitForFences(logicalDevice.handle, 1, &imageCopyFence, VK_TRUE, 50000000000), "Failed to wait on a fence."); //TODO good timeout value?

    vkDestroyFence(logicalDevice.handle, imageCopyFence, nullptr);
    
    vkFreeMemory(logicalDevice.handle, stagingData.second, nullptr);
    vkDestroyBuffer(logicalDevice.handle, stagingData.first, nullptr);
    return {ImageHnd(image), textureData.width, textureData.height, textureData.mipmapLevelCount, textureData.layers, imViewType, engineFormat};
}

Image VulkanAPI::createUncompressedImage(ImageMemoryType type, const glm::uvec2& dimensions, bool isWritable, bool usedAsColorAttachment, const void* data) {
    std::uint64_t size;
    VkFormat format;
    Format engineFormat;
    
    bool needsMemoryUpload = (data != nullptr);
    
    if (type == ImageMemoryType::RGB) {
        size = 3 * dimensions.x * dimensions.y;
        format = VK_FORMAT_R8G8B8_UNORM;
        engineFormat = Format::R8_G8_B8_uNorm;
    } else if (type == ImageMemoryType::RGBA) {
        size = 4 * dimensions.x * dimensions.y;
        format = VK_FORMAT_R8G8B8A8_UNORM;
        engineFormat = Format::R8_G8_B8_A8_uNorm;
    } else if (type == ImageMemoryType::RGBAHalf) {
        size = 2 * 4 * dimensions.x * dimensions.y;
        format = VK_FORMAT_R16G16B16A16_SFLOAT;
        engineFormat = Format::R16_G16_B16_A16_sFloat;
    } else {
        throw std::runtime_error("Invalid texture type.");
    }
    
    VkImageUsageFlags iuf = VK_IMAGE_USAGE_SAMPLED_BIT;
    
    if (usedAsColorAttachment) {
        iuf |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    
    if (needsMemoryUpload) {
        iuf |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    
    if (isWritable) {
        iuf |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    
    // Create an image and allocate memory for it
    VkImageCreateInfo ici;
    ici.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.pNext                 = nullptr;
    ici.flags                 = 0;
    ici.imageType             = VK_IMAGE_TYPE_2D;
    ici.format                = format;
    ici.extent                = {static_cast<std::uint32_t>(dimensions.x), static_cast<std::uint32_t>(dimensions.y), 1};
    ici.mipLevels             = 1;
    ici.arrayLayers           = 1;
    ici.samples               = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling                = VK_IMAGE_TILING_OPTIMAL;
    ici.usage                 = iuf;
    ici.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ici.queueFamilyIndexCount = 0;
    ici.pQueueFamilyIndices   = nullptr;
    ici.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage image;
    checkResult(vkCreateImage(logicalDevice.handle, &ici, nullptr, &image), "Failed to create an image.");

    VkMemoryRequirements mrDev;
    vkGetImageMemoryRequirements(logicalDevice.handle, image, &mrDev);
    
    VkMemoryAllocateInfo maiDev;
    maiDev.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    maiDev.pNext           = nullptr;
    maiDev.allocationSize  = mrDev.size;
    maiDev.memoryTypeIndex = getMemoryType(mrDev.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkDeviceMemory imageMemory;
    checkResult(vkAllocateMemory(logicalDevice.handle, &maiDev, nullptr, &imageMemory), "Failed to allocate memory.");
    
    checkResult(vkBindImageMemory(logicalDevice.handle, image, imageMemory, 0), "Failed to bind memory.");
    
    imageToMemory[image] = imageMemory;
    
    if (!needsMemoryUpload) {
        VkCommandBufferBeginInfo cbbi;
        cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.pNext            = nullptr;
        cbbi.flags            = 0;//TODO?
        cbbi.pInheritanceInfo = nullptr;

        checkResult(vkBeginCommandBuffer(imageUploadCommandBuffer, &cbbi), "Failed to begin a command buffer.");
        
        VkImageSubresourceRange sr;
        sr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        sr.baseMipLevel   = 0;
        sr.levelCount     = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount     = 1;
        
        setImageLayout(imageUploadCommandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, sr);

        checkResult(vkEndCommandBuffer(imageUploadCommandBuffer), "Failed to end a command buffer.");
        
        VkSubmitInfo si;
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.pNext                = nullptr;
        si.waitSemaphoreCount   = 0;
        si.pWaitSemaphores      = nullptr;
        si.pWaitDstStageMask    = nullptr;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &imageUploadCommandBuffer;
        si.signalSemaphoreCount = 0;
        si.pSignalSemaphores    = nullptr;

        checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, VK_NULL_HANDLE), "Failed to submit a queue");
        checkResult(vkQueueWaitIdle(logicalDevice.mainQueue), "Failed to wait on a queue.");
    } else {
        auto stagingData = createTemporaryBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size, data);

        VkBufferImageCopy bic;
        bic.bufferOffset      = 0;
        bic.bufferRowLength   = 0;
        bic.bufferImageHeight = 0;

        bic.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        bic.imageSubresource.mipLevel       = 0;
        bic.imageSubresource.baseArrayLayer = 0;
        bic.imageSubresource.layerCount     = 1;

        bic.imageOffset.x = 0;
        bic.imageOffset.y = 0;
        bic.imageOffset.z = 0;

        bic.imageExtent.width  = static_cast<std::uint32_t>(dimensions.x);
        bic.imageExtent.height = static_cast<std::uint32_t>(dimensions.y);
        bic.imageExtent.depth  = 1;

        std::vector<VkBufferImageCopy> bics{bic};

        VkCommandBufferBeginInfo cbbi;
        cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.pNext            = nullptr;
        cbbi.flags            = 0;//TODO?
        cbbi.pInheritanceInfo = nullptr;

        checkResult(vkBeginCommandBuffer(imageUploadCommandBuffer, &cbbi), "Failed to begin a command buffer.");

        VkImageSubresourceRange sr;
        sr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        sr.baseMipLevel   = 0;
        sr.levelCount     = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount     = 1;

        setImageLayout(imageUploadCommandBuffer, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, sr);
        vkCmdCopyBufferToImage(imageUploadCommandBuffer, stagingData.first, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bics.size(), bics.data());
        setImageLayout(imageUploadCommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sr);

        checkResult(vkEndCommandBuffer(imageUploadCommandBuffer), "Failed to end a command buffer.");

        VkFenceCreateInfo fci;
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.pNext = nullptr;
        fci.flags = 0;

        VkFence imageCopyFence;
        checkResult(vkCreateFence(logicalDevice.handle, &fci, nullptr, &imageCopyFence), "Failed to create a fence."); //TODO reset?

        VkSubmitInfo si;
        si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.pNext                = nullptr;
        si.waitSemaphoreCount   = 0;
        si.pWaitSemaphores      = nullptr;
        si.pWaitDstStageMask    = nullptr;
        si.commandBufferCount   = 1;
        si.pCommandBuffers      = &imageUploadCommandBuffer;
        si.signalSemaphoreCount = 0;
        si.pSignalSemaphores    = nullptr;

        checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, imageCopyFence), "Failed to submit a queue.");
        checkResult(vkWaitForFences(logicalDevice.handle, 1, &imageCopyFence, VK_TRUE, 50000000000), "Failed to wait on a fence."); //TODO good timeout value?

        vkDestroyFence(logicalDevice.handle, imageCopyFence, nullptr);

        vkFreeMemory(logicalDevice.handle, stagingData.second, nullptr);
        vkDestroyBuffer(logicalDevice.handle, stagingData.first, nullptr);
    }
    
    
    return {ImageHnd(image), static_cast<std::uint64_t>(dimensions.x), static_cast<std::uint64_t>(dimensions.y), 1, 1, ImageViewType::Im2D, engineFormat};
}

bool VulkanAPI::destroyImage(const Image& image) {
    VkImage handle = image.handle.toNative<VkImage>();
    
    VkDeviceMemory memory = imageToMemory[handle];
    
    vkDestroyImage(logicalDevice.handle, handle, nullptr);
    vkFreeMemory(logicalDevice.handle, memory, nullptr);
    
    imageToMemory.erase(handle);
    
    return true;
}

SamplerHnd VulkanAPI::createSampler(const SamplerCreateInfo& info) {
    VkSamplerCreateInfo sci;
    sci.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.pNext                   = nullptr;
    sci.flags                   = 0;
    sci.magFilter               = vkutil::mapFilter(info.magFilter);
    sci.minFilter               = vkutil::mapFilter(info.minFilter);
    sci.mipmapMode              = vkutil::mapMipmapMode(info.mipmapMode);
    sci.addressModeU            = vkutil::mapAddressMode(info.addressModeU);
    sci.addressModeV            = vkutil::mapAddressMode(info.addressModeV);
    sci.addressModeW            = vkutil::mapAddressMode(info.addressModeW);
    sci.mipLodBias              = info.mipLodBias;
    sci.anisotropyEnable        = info.anisotropyEnable ? VK_TRUE : VK_FALSE;
    sci.maxAnisotropy           = info.maxAnisotropy;
    sci.compareEnable           = info.compareEnable ? VK_TRUE : VK_FALSE;
    sci.compareOp               = vk::compareOp(info.compareOp);
    sci.minLod                  = info.minLod;
    sci.maxLod                  = info.maxLod;
    sci.borderColor             = vk::borderColor(info.borderColor);
    sci.unnormalizedCoordinates = info.unnormalizedCoordinates ? VK_TRUE : VK_FALSE;
    
    VkSampler sampler;
    vkCreateSampler(logicalDevice.handle, &sci, nullptr, &sampler);
    return SamplerHnd(sampler);
}

bool VulkanAPI::destroySampler(SamplerHnd handle) {
    vkDestroySampler(logicalDevice.handle, handle.toNative<VkSampler>(), nullptr);
    return true;
}

ImageViewHnd VulkanAPI::createImageView(const ImageViewCreateInfo& info) {
    VkImageViewCreateInfo ivci;
    ivci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ivci.pNext            = nullptr;
    ivci.flags            = 0;
    ivci.image            = info.image.toNative<VkImage>();
    ivci.viewType         = vk::imageViewType(info.viewType);
    ivci.format           = vk::format(info.format);
    ivci.components       = vkutil::mapComponents(info.components);
    ivci.subresourceRange = vkutil::mapSubresourceRange(info.subresourceRange);
    
    VkImageView iv;
    vkCreateImageView(logicalDevice.handle, &ivci, nullptr, &iv);

    return ImageViewHnd(iv);
}

bool VulkanAPI::destroyImageView(ImageViewHnd handle) {
    vkDestroyImageView(logicalDevice.handle, handle.toNative<VkImageView>(), nullptr);
    return true;
}

// UniformBufferSlice VulkanAPI::createUniformBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
//     auto result = createBuffer(BufferType::Uniform, size, flag, data);
//     bufferToMemory[result.first] = result.second;
//     
//     return UniformBufferSlice(reinterpret_cast<UniformBufferHnd>(result.first), 0, size, flag);
// }
// 
// bool VulkanAPI::setUniformBufferData(const UniformBufferSlice& slice, const void* data) {
//     return updateBuffer(slice.handle(), slice.offset(), slice.size(), data);
// }
// 
// bool VulkanAPI::updateUniformBufferData(const UniformBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
//     return partialUpdateBuffer(slice.handle(), slice.offset(), slice.size(), subSlice.offset(), subSlice.size(), data);
// }
// 
// bool VulkanAPI::destroyUniformBuffer(const UniformBufferSlice& slice) {
//     return destroyBuffer(reinterpret_cast<VkBuffer>(slice.handle()));
// }
// 
// StorageBufferSlice VulkanAPI::createStorageBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
//     auto result = createBuffer(BufferType::Storage, size, flag, data);
//     bufferToMemory[result.first] = result.second;
//     
//     return StorageBufferSlice(reinterpret_cast<StorageBufferHnd>(result.first), 0, size, flag);
// }
// 
// bool VulkanAPI::setStorageBufferData(const StorageBufferSlice& slice, const void* data) {
//     return updateBuffer(slice.handle(), slice.offset(), slice.size(), data);
// }
// 
// bool VulkanAPI::updateStorageBufferData(const StorageBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
//     return partialUpdateBuffer(slice.handle(), slice.offset(), slice.size(), subSlice.offset(), subSlice.size(), data);
// }
// 
// bool VulkanAPI::destroyStorageBuffer(const StorageBufferSlice& slice) {
//     return destroyBuffer(reinterpret_cast<VkBuffer>(slice.handle()));
// }
// 
// 
// VertexBufferSlice VulkanAPI::createVertexBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
//     auto result = createBuffer(BufferType::Uniform, size, flag, data);
//     bufferToMemory[result.first] = result.second;
//     
//     return VertexBufferSlice(reinterpret_cast<UniformBufferHnd>(result.first), 0, size, flag);
// }
// 
// bool VulkanAPI::setVertexBufferData(const VertexBufferSlice& slice, const void* data) {
//     return updateBuffer(slice.handle(), slice.offset(), slice.size(), data);
// }
// 
// bool VulkanAPI::updateVertexBufferData(const VertexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
//     return partialUpdateBuffer(slice.handle(), slice.offset(), slice.size(), subSlice.offset(), subSlice.size(), data);
// }
// 
// bool VulkanAPI::destroyVertexBuffer(const VertexBufferSlice& slice) {
//     return destroyBuffer(reinterpret_cast<VkBuffer>(slice.handle()));
// }
// 
// 
// IndexBufferSlice VulkanAPI::createIndexBuffer(std::uint64_t size, IndexType type, BufferUpdateFrequency flag, const void* data) {
//     auto result = createBuffer(BufferType::Uniform, size, flag, data);
//     bufferToMemory[result.first] = result.second;
//     
//     return IndexBufferSlice(reinterpret_cast<UniformBufferHnd>(result.first), 0, size, flag, type);
// }
// 
// bool VulkanAPI::setIndexBufferData(const IndexBufferSlice& slice, const void* data) {
//     return updateBuffer(slice.handle(), slice.offset(), slice.size(), data);
// }
// 
// bool VulkanAPI::updateIndexBufferData(const IndexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) {
//     return partialUpdateBuffer(slice.handle(), slice.offset(), slice.size(), subSlice.offset(), subSlice.size(), data);
// }
// 
// bool VulkanAPI::destroyIndexBuffer(const IndexBufferSlice& slice) {
//     return destroyBuffer(reinterpret_cast<VkBuffer>(slice.handle()));
// }

VkBufferUsageFlagBits VulkanAPI::mapBufferType(BufferType bufferType) const {
    switch (bufferType) {
    case BufferType::Vertex:
        return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    case BufferType::Index:
        return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    case BufferType::Uniform:
        return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    case BufferType::Storage:
        return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    default:
        throw std::runtime_error("Invalid buffer type");
    }
}

// std::pair<VkBuffer, VkDeviceMemory> VulkanAPI::createBuffer(BufferType bufferType, std::uint64_t size, BufferUpdateFrequency flag, const void* data) {
//     std::pair<VkBuffer, VkDeviceMemory> result;
//     bool deviceLocal = (BufferUpdateFrequency::Never == flag);
//     
//     //TODO sparse? su flags
//     if (deviceLocal) {
//         const auto stagingResult = createTemporaryBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size, data);
//         result = createTemporaryBuffer(mapBufferType(bufferType) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size, nullptr);
//         
//         VkCommandBuffer copyBuff = allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, true);
//         VkBufferCopy bc;
//         bc.srcOffset = 0;
//         bc.dstOffset = 0;
//         bc.size = size;
//         
//         vkCmdCopyBuffer(copyBuff, stagingResult.first, result.first, 1, &bc);
//         checkResult(vkEndCommandBuffer(copyBuff), "com_buffer_end_fail");
// 
//         VkSubmitInfo si;
//         si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//         si.pNext                = nullptr;
//         si.waitSemaphoreCount   = 0;
//         si.pWaitSemaphores      = nullptr;
//         si.pWaitDstStageMask    = nullptr;
//         si.commandBufferCount   = 1;
//         si.pCommandBuffers      = &copyBuff;
//         si.signalSemaphoreCount = 0;
//         si.pSignalSemaphores    = nullptr;
// 
//         checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, VK_NULL_HANDLE), "queue_submit_fail");
//         checkResult(vkQueueWaitIdle(logicalDevice.mainQueue), "queue_wait_fail");
// 
//         vkDestroyBuffer(logicalDevice.handle, stagingResult.first, nullptr);
//         vkFreeMemory(logicalDevice.handle, stagingResult.second, nullptr);
//         
//         vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &copyBuff);
//     } else {
//         result = createTemporaryBuffer(mapBufferType(bufferType), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, size, data);
//     }
//     
//     return result;
// }

std::pair<VkBuffer, VkDeviceMemory> VulkanAPI::createTemporaryBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, std::uint64_t size, const void* data) {
    VkBuffer handle;
    VkDeviceMemory memory;
    
    VkBufferCreateInfo bci;
    bci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext                 = nullptr;
    bci.flags                 = 0;
    bci.size                  = size;
    bci.usage                 = usageFlags;
    bci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices   = nullptr;

    checkResult(vkCreateBuffer(logicalDevice.handle, &bci, nullptr, &handle), "Failed to create a buffer.");

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(logicalDevice.handle, handle, &mr);
    
    VkMemoryAllocateInfo mai;
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.pNext           = nullptr;
    mai.allocationSize  = mr.size;
    mai.memoryTypeIndex = getMemoryType(mr.memoryTypeBits, memoryFlags);
    
    checkResult(vkAllocateMemory(logicalDevice.handle, &mai, nullptr, &memory), "Failed to allocate memory");
    
    if (data != nullptr) {
        void* p;
        checkResult(vkMapMemory(logicalDevice.handle, memory, 0, size, 0, &p), "Failed to map memory.");
        std::memcpy(p, data, size);
        
        if (!(memoryFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            VkMappedMemoryRange fr;
            fr.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            fr.pNext  = nullptr;
            fr.memory = memory;
            fr.offset = 0;
            fr.size   = VK_WHOLE_SIZE;
            vkFlushMappedMemoryRanges(logicalDevice.handle, 1, &fr);
        }

        vkUnmapMemory(logicalDevice.handle, memory);
    }

    checkResult(vkBindBufferMemory(logicalDevice.handle, handle, memory, 0), "Failed to bind memory.");
    
    return {handle, memory};
}

bool VulkanAPI::createBuffers(const std::vector<BufferCreateInfo>& info, MemoryType memoryType, std::vector<Buffer>& outBuffers) {
    outBuffers.clear();
    outBuffers.reserve(info.size());
    
    VkMemoryRequirements mr;
    std::uint64_t totalAllocationSize = 0;
    
    for (std::size_t j = 0; j < info.size(); ++j) {
        const auto& bciv = info[j];
        
        VkBuffer handle;
        
        VkBufferCreateInfo bci;
        bci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.pNext                 = nullptr;
        bci.flags                 = 0;
        bci.size                  = bciv.size.count();
        bci.usage                 = vk::bufferUsage(bciv.flags);
        bci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        bci.queueFamilyIndexCount = 0;
        bci.pQueueFamilyIndices   = nullptr;
        
        checkResult(vkCreateBuffer(logicalDevice.handle, &bci, nullptr, &handle), "Failed to create a buffer.");
        
        vkGetBufferMemoryRequirements(logicalDevice.handle, handle, &mr);
        
        outBuffers.emplace_back(BufferHnd(handle), bciv.flags, memoryType, Bytes(mr.size), Bytes(totalAllocationSize));
        
        totalAllocationSize += mr.size;
    }
    
    VkFlags memoryFlags = (memoryType == MemoryType::DeviceLocal) ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    
    // According to the spec. (11.6. Resource Memory Association):
    // • The memoryTypeBits member always contains at least one bit set corresponding to a VkMemoryType
    //   with a propertyFlags that has the VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT bit set.
    // and
    // • If buffer is a VkBuffer not created with the VK_BUFFER_CREATE_SPARSE_BINDING_BIT bit set, or if
    //  image is a VkImage that was created with a VK_IMAGE_TILING_LINEAR value in the tiling member of
    //  the VkImageCreateInfo structure passed to vkCreateImage, then the memoryTypeBits member
    //  always contains at least one bit set corresponding to a VkMemoryType with a propertyFlags that
    //  has both the VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT bit and the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //  bit set. In other words, mappable coherent memory can always be attached to these objects.
    //
    // Since we ALWAYS use either VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT or VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    // for all the buffers we're creating and we do not create sparse buffers, it is safe to create a single allocation with a memoryTypeIndex
    // retrieved based on the last mr value
            
    VkMemoryAllocateInfo mai;
    mai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.pNext           = nullptr;
    mai.allocationSize  = totalAllocationSize;
    mai.memoryTypeIndex = getMemoryType(mr.memoryTypeBits, memoryFlags);
    
    VkDeviceMemory memory;
    checkResult(vkAllocateMemory(logicalDevice.handle, &mai, nullptr, &memory), "Failed to allocate memory.");
    
    std::size_t offset = 0;
    for (std::size_t i = 0; i < info.size(); ++i) {
        VkBuffer handle = outBuffers[i].handle().toNative<VkBuffer>();
        checkResult(vkBindBufferMemory(logicalDevice.handle, handle, memory, offset), "Failed to bind memory.");
        
        bufferToMemory[handle] = memory;
        
        offset += outBuffers[i].size();
    }
    
    memoryToBufferCount[memory] = outBuffers.size();
    
    return true;
}

bool VulkanAPI::destroyBuffers(const std::vector<Buffer>& buffers) {
    for (const auto& b : buffers) {
        VkBuffer handle = b.handle().toNative<VkBuffer>();
        vkDestroyBuffer(logicalDevice.handle, handle, nullptr);
        
        VkDeviceMemory memory = bufferToMemory[handle];
        std::uint64_t count = memoryToBufferCount[memory] - 1;
        
        bufferToMemory.erase(handle);
        
        if (count == 0) {
            vkFreeMemory(logicalDevice.handle, memory, nullptr);
            
            memoryToBufferCount.erase(memory);
        } else {
            memoryToBufferCount[memory] = count;
        }
    }
    
    return true;
}

bool VulkanAPI::updateHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, const void* data) {
    assert(buffer.memoryType() == MemoryType::HostVisible);
    
    VkDeviceMemory memory = bufferToMemory[buffer.handle().toNative<VkBuffer>()];
    void* p;
    // TODO stop unmapping buffers
    checkResult(vkMapMemory(logicalDevice.handle, memory, buffer.offset(), buffer.size(), 0, &p), "Failed to map memory.");
    
    for (const auto& c : copies) {
        const char* source = static_cast<const char*>(data);
        source += c.srcOffset;
        
        char* destination = static_cast<char*>(p);
        destination += c.dstOffset;
        
        std::memcpy(destination, source, c.size);
    }
    
    // If not coherent
//    VkMappedMemoryRange fr;
//    fr.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//    fr.pNext  = nullptr;
//    fr.memory = memory;
//    fr.offset = buffer.offset();
//    fr.size   = buffer.size();
//    vkFlushMappedMemoryRanges(logicalDevice.handle, 1, &fr);

    vkUnmapMemory(logicalDevice.handle, memory);
    
    return true;
}

void VulkanAPI::updateDeviceVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, const void* data) {
    assert(buffer.memoryType() == MemoryType::DeviceLocal);
    assert(buffer.usageFlags() & BufferUsageFlagBits::TransferDestination);
    
    // Creating a huge temp buffer with gaps would make no sense. Instead, we compute the total required size
    // and allocate a smaller buffer.
    std::uint64_t size = 0;
    for (const auto& c : copies) {
        size = c.size;
    }
    
    // Because of our "optimization", we have to pass a nullptr and copy the data ourselves
    const auto stagingResult = createTemporaryBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, size, nullptr);
    
    void* p;
    checkResult(vkMapMemory(logicalDevice.handle, stagingResult.second, 0, size, 0, &p), "Failed to map memory.");
    
    std::uint64_t offset = 0;
    for (const auto& c : copies) {
        const char* source = static_cast<const char*>(data);
        source += c.srcOffset;
        
        char* destination = static_cast<char*>(p);
        destination += offset;
        offset += c.size;
        
        std::memcpy(destination, source, c.size);
    }

    vkUnmapMemory(logicalDevice.handle, stagingResult.second);
    
    VkCommandBuffer copyBuff = allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, true);
    
    std::vector<VkBufferCopy> stagingCopies;
    stagingCopies.reserve(copies.size());
    
    offset = 0;
    for (const auto& c : copies) {
        VkBufferCopy bc;
        bc.srcOffset = offset;
        offset += c.size;
        
        bc.dstOffset = c.dstOffset;
        bc.size = c.size;
        
        stagingCopies.push_back(std::move(bc));
    }
    
    vkCmdCopyBuffer(copyBuff, stagingResult.first, buffer.handle().toNative<VkBuffer>(), stagingCopies.size(), stagingCopies.data());
    checkResult(vkEndCommandBuffer(copyBuff), "Failed to end a command buffer.");

    VkSubmitInfo si;
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = 0;
    si.pWaitSemaphores      = nullptr;
    si.pWaitDstStageMask    = nullptr;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &copyBuff;
    si.signalSemaphoreCount = 0;
    si.pSignalSemaphores    = nullptr;

    checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, VK_NULL_HANDLE), "Failed to submit a queue.");
    checkResult(vkQueueWaitIdle(logicalDevice.mainQueue), "Failed to wait on a queue");

    vkDestroyBuffer(logicalDevice.handle, stagingResult.first, nullptr);
    vkFreeMemory(logicalDevice.handle, stagingResult.second, nullptr);
    
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &copyBuff);
}

bool VulkanAPI::updateBuffer(std::uint64_t handle, std::uint64_t offset, std::uint64_t size, const void* data) {
    return partialUpdateBuffer(handle, offset, size, 0, size, data);
}

bool VulkanAPI::destroyBuffer(VkBuffer buffer) {
    VkDeviceMemory memory = bufferToMemory[buffer];
    
    vkDestroyBuffer(logicalDevice.handle, buffer, nullptr);
    vkFreeMemory(logicalDevice.handle, memory, nullptr);
    
    bufferToMemory.erase(buffer);
    
    return true;
}

bool VulkanAPI::partialUpdateBuffer(std::uint64_t handle, std::uint64_t offset, std::uint64_t size, std::uint64_t subOffset, std::uint64_t subSize, const void* data) {
    VkDeviceMemory memory = bufferToMemory[reinterpret_cast<VkBuffer>(handle)];
    void* p;
    checkResult(vkMapMemory(logicalDevice.handle, memory, offset + subOffset, subSize, 0, &p), "Failed to map memory.");
    std::memcpy(p, data, subSize);
    
    VkMappedMemoryRange fr;
    fr.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    fr.pNext  = nullptr;
    fr.memory = memory;
    fr.offset = offset + subOffset;
    fr.size   = subSize;
    vkFlushMappedMemoryRanges(logicalDevice.handle, 1, &fr);

    vkUnmapMemory(logicalDevice.handle, memory);
    
    return true;
}

CommandPool* VulkanAPI::createCommandPool(QueueType type, std::uint32_t queueId) {
    VkCommandPool pool;
    VkResult result;
    
    VkCommandPoolCreateInfo cpci;
    cpci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext            = nullptr;
    cpci.queueFamilyIndex = physicalDevice.chosenMainQueueFamilyId; //TODO make this depend on type and id
    cpci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO transient, reset?
    
    result = vkCreateCommandPool(logicalDevice.handle, &cpci, nullptr, &pool);
    checkResult(result, "Failed to create a command pool.");
    
    return new VulkanCommandPool(this, pool);
}

bool VulkanAPI::destroyCommandPool(CommandPool* pool) {
    if (pool == nullptr) {
        return false;
    }
    
    VulkanCommandPool* commandPool = static_cast<VulkanCommandPool*>(pool);
    vkDestroyCommandPool(logicalDevice.handle, commandPool->getVulkanCommandPool(), nullptr);
    delete commandPool;
    
    return true;
}

RenderPassHnd VulkanAPI::createRenderPass(const RenderPassCreateInfo& info) {
    VkRenderPassCreateInfo rpci;
    rpci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.pNext           = nullptr;
    rpci.flags           = 0;
    
    // Attachments
    rpci.attachmentCount = info.attachments.size();
    std::vector<VkAttachmentDescription> ad;
    ad.reserve(info.attachments.size());
    for (const auto &d : info.attachments) {
        VkAttachmentDescription adv = {static_cast<VkAttachmentDescriptionFlags>(d.flags),
                         vk::format(d.format),
                         static_cast<VkSampleCountFlagBits>(vk::sampleCount(d.samples)),
                         vk::attachmentLoadOp(d.loadOp),
                         vk::attachmentStoreOp(d.storeOp),
                         vk::attachmentLoadOp(d.stencilLoadOp),
                         vk::attachmentStoreOp(d.stencilStoreOp),
                         vk::imageLayout(d.initialLayout),
                         vk::imageLayout(d.finalLayout)}; 
        ad.emplace_back(std::move(adv));
    }
    rpci.pAttachments = (info.attachments.size() == 0) ? nullptr : ad.data();
    
    // Subpass contents
    rpci.subpassCount = info.subpasses.size();
    std::vector<VkSubpassDescription> sd;
    sd.reserve(info.subpasses.size());
    for (const auto &d : info.subpasses) {
        VkSubpassDescription sdv = {0,
                                    vkutil::mapPipelineBindPoint(d.pipelineBindPoint),
                                    static_cast<std::uint32_t>(d.inputAttachments.size()),
                                    (d.inputAttachments.size() == 0) ? nullptr : reinterpret_cast<const VkAttachmentReference*>(d.inputAttachments.data()),
                                    static_cast<std::uint32_t>(d.colorAttachments.size()),
                                    (d.colorAttachments.size() == 0) ? nullptr : reinterpret_cast<const VkAttachmentReference*>(d.colorAttachments.data()),
                                    (d.resolveAttachments.size() == 0) ? nullptr : reinterpret_cast<const VkAttachmentReference*>(d.resolveAttachments.data()),
//                                    d.usesDepth ? reinterpret_cast<const VkAttachmentReference*>(&d.depthStencilAttachment) : nullptr,
                                    reinterpret_cast<const VkAttachmentReference*>(&d.depthStencilAttachment),
                                    static_cast<std::uint32_t>(d.preserveAttachments.size()),
                                    d.preserveAttachments.data()
        };
        
        sd.emplace_back(std::move(sdv));
    }
    rpci.pSubpasses      = (info.subpasses.size() == 0) ? nullptr : sd.data();
    
    // Subpass dependencies
    rpci.dependencyCount = info.dependencies.size();
    std::vector<VkSubpassDependency> spd;// = vkutil::mapSubpassDependencies(info.dependencies);
    spd.reserve(info.dependencies.size());
    for (const auto &d : info.dependencies) {
        VkSubpassDependency spdv = {
            d.srcSubpass,
            d.dstSubpass,
            static_cast<VkPipelineStageFlags>(d.srcStageMask),
            static_cast<VkPipelineStageFlags>(d.dstStageMask),
            static_cast<VkAccessFlags>(d.srcAccessMask),
            static_cast<VkAccessFlags>(d.dstAccessMask),
            static_cast<VkDependencyFlags>(d.dependencyFlags)
        };

        spd.emplace_back(std::move(spdv));
    }
    rpci.pDependencies   = (info.dependencies.size() == 0) ? nullptr : spd.data();
    
    VkRenderPass pass;
    checkResult(vkCreateRenderPass(logicalDevice.handle, &rpci, nullptr, &pass), "Failed to create a render pass.");
    
    return RenderPassHnd(pass);
}

bool VulkanAPI::destroyRenderPass(RenderPassHnd handle) {
    vkDestroyRenderPass(logicalDevice.handle, handle.toNative<VkRenderPass>(), nullptr);
    return true;
}


Format VulkanAPI::getSurfaceFormat() {
    return surfaceFormatEngine;
}

Format VulkanAPI::getDepthStencilFormat() {
    return depthStencilFormatEngine;
}

SemaphoreHnd VulkanAPI::createSemaphore() {
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo sci;
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = nullptr;
    sci.flags = 0;
    checkResult(vkCreateSemaphore(logicalDevice.handle, &sci, nullptr, &semaphore), "Failed to create a semaphore.");
    
    return SemaphoreHnd(semaphore);
}

void VulkanAPI::destroySemaphore(SemaphoreHnd hnd) {
    vkDestroySemaphore(logicalDevice.handle, hnd.toNative<VkSemaphore>(), nullptr);
}

FenceHnd VulkanAPI::createFence(bool createSignaled) {
    VkFence fence;
    VkFenceCreateInfo fci;
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = nullptr;
    fci.flags = createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
    
    checkResult(vkCreateFence(logicalDevice.handle, &fci, nullptr, &fence), "Failed to create a fence.");
    
    return FenceHnd(fence);
}

void VulkanAPI::destroyFence(FenceHnd fence) {
    vkDestroyFence(logicalDevice.handle, fence.toNative<VkFence>(), nullptr);
}

bool VulkanAPI::getFenceStatus(FenceHnd fence) {
    return vkGetFenceStatus(logicalDevice.handle, fence.toNative<VkFence>()) == VK_SUCCESS;
}

bool VulkanAPI::waitForFence(FenceHnd fence, std::uint64_t timeout) {
    VkResult res;
    
    res = vkWaitForFences(logicalDevice.handle, 1, reinterpret_cast<const VkFence*>(&fence), VK_TRUE, timeout);
    
    if (res == VK_SUCCESS) {
        return true;
    } else if (res == VK_TIMEOUT) {
        return false;
    } else {
        checkResult(res, "Failed to wait on a fence.");
        return false;
    }
}

bool VulkanAPI::waitForFences(const std::vector<FenceHnd>& fences, bool waitForAll, std::uint64_t timeout) {
    VkResult res;
    
    res = vkWaitForFences(logicalDevice.handle, fences.size(), reinterpret_cast<const VkFence*>(fences.data()), waitForAll ? VK_TRUE : VK_FALSE, timeout);
    
    if (res == VK_SUCCESS) {
        return true;
    } else if (res == VK_TIMEOUT) {
        return false;
    } else {
        checkResult(res, "Failed to wait on fences.");
        return false;
    }
}

void VulkanAPI::resetFence(FenceHnd fence) {
    checkResult(vkResetFences(logicalDevice.handle, 1, reinterpret_cast<const VkFence*>(&fence)), "Failed to reset a fence.");
}

void VulkanAPI::resetFences(const std::vector<FenceHnd>& fences) {
    checkResult(vkResetFences(logicalDevice.handle, fences.size(), reinterpret_cast<const VkFence*>(fences.data())), "Failed to reset fences.");
}

void VulkanAPI::submitQueue(const SubmitInfo& info, FenceHnd fence) {
    VkSubmitInfo si;
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.pNext                = nullptr;
    si.waitSemaphoreCount   = info.waitSemaphores.size();
    si.pWaitSemaphores      = info.waitSemaphores.size() == 0 ? nullptr : reinterpret_cast<const VkSemaphore*>(info.waitSemaphores.data());
    si.pWaitDstStageMask    = reinterpret_cast<const VkPipelineStageFlags*>(info.waitDstStageMask.data());
    si.commandBufferCount   = info.commandBuffers.size();
    si.pCommandBuffers      = info.commandBuffers.size() == 0 ? nullptr : reinterpret_cast<const VkCommandBuffer*>(info.commandBuffers.data());
    si.signalSemaphoreCount = info.signalSemaphores.size();
    si.pSignalSemaphores    = info.signalSemaphores.size() == 0 ? nullptr : reinterpret_cast<const VkSemaphore*>(info.signalSemaphores.data());
    
    VkFence f = (!fence.isValid()) ? VK_NULL_HANDLE : fence.toNative<VkFence>();

    checkResult(vkQueueSubmit(logicalDevice.mainQueue, 1, &si, f), "Failed to submit a queue.");
    //checkResult(vkQueueWaitIdle(queue), "queue_wait_fail");
}

//Framebuffer VulkanAPI::createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<FramebufferAttachmentCreateInfo>& info) {
Framebuffer VulkanAPI::createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<std::variant<Image, FramebufferAttachmentCreateInfo>>& info) {
    Framebuffer fbo;
    fbo.images.reserve(info.size());
    fbo.imageViews.reserve(info.size());
    fbo.isImageOwned.reserve(info.size());
    
    for (auto& ci : info) {
        if (ci.index() == 0) { // an existing Image
            Image img = std::get<Image>(ci);
            
            VkImageAspectFlags af;

            bool isDepthStencil = vkutil::isDepthStencilFormat(img.format);
            if (isDepthStencil) {
                af = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            } else {
                af = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            
            VkImageSubresourceRange isr;
            isr.aspectMask     = af;
            isr.baseMipLevel   = 0;
            isr.levelCount     = 1;
            isr.baseArrayLayer = 0;
            isr.layerCount     = 1;

            VkImageViewCreateInfo ivci;
            ivci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ivci.pNext            = nullptr;
            ivci.flags            = 0;
            ivci.image            = img.handle.toNative<VkImage>();
            ivci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format           = vk::format(img.format);
            ivci.components       = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
            ivci.subresourceRange = isr;

            VkImageView imageView;
            checkResult(vkCreateImageView(logicalDevice.handle, &ivci, nullptr, &imageView), "Failed to create an image view.");
            
//            if (reinterpret_cast<std::uintptr_t>(imageView) == 0x94) {
//                throw std::runtime_error("err");
//            }
            
            fbo.images.push_back(std::move(img));
            fbo.imageViews.push_back(ImageViewHnd(imageView));
            fbo.isImageOwned.push_back(false);
        } else { // FramebufferAttachmentCreateInfo
            const FramebufferAttachmentCreateInfo& i = std::get<FramebufferAttachmentCreateInfo>(ci);
            //VkImageUsageFlags uf = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageUsageFlags uf = i.isAttachment ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
            VkImageAspectFlags af;

            bool isDepthStencil = vkutil::isDepthStencilFormat(i.format);
            if (isDepthStencil) {
                uf |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                af = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            } else {
                uf |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                af = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            VkImageCreateInfo ici;
            ici.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            ici.pNext                 = nullptr;
            ici.flags                 = 0;
            ici.imageType             = VK_IMAGE_TYPE_2D;
            ici.format                = vk::format(i.format);
            ici.extent                = {extent.x, extent.y, 1};
            ici.mipLevels             = 1;
            ici.arrayLayers           = 1;
            ici.samples               = VK_SAMPLE_COUNT_1_BIT;
            ici.tiling                = VK_IMAGE_TILING_OPTIMAL;
            ici.usage                 = uf;
            ici.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            ici.queueFamilyIndexCount = 0;
            ici.pQueueFamilyIndices   = nullptr;
            ici.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImage image;
            checkResult(vkCreateImage(logicalDevice.handle, &ici, nullptr, &image), "Failed to create an image.");

            VkMemoryRequirements mrDev;
            vkGetImageMemoryRequirements(logicalDevice.handle, image, &mrDev);

            VkMemoryAllocateInfo maiDev;
            maiDev.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            maiDev.pNext           = nullptr;
            maiDev.allocationSize  = mrDev.size;
            maiDev.memoryTypeIndex = getMemoryType(mrDev.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            VkDeviceMemory imageMemory;
            checkResult(vkAllocateMemory(logicalDevice.handle, &maiDev, nullptr, &imageMemory), "Failed to allocate memory.");

            checkResult(vkBindImageMemory(logicalDevice.handle, image, imageMemory, 0), "Failed to bind memory.");

            imageToMemory[image] = imageMemory;

            VkImageSubresourceRange isr;
            isr.aspectMask     = af;
            isr.baseMipLevel   = 0;
            isr.levelCount     = 1;
            isr.baseArrayLayer = 0;
            isr.layerCount     = 1;

            VkImageViewCreateInfo ivci;
            ivci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ivci.pNext            = nullptr;
            ivci.flags            = 0;
            ivci.image            = image;
            ivci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format           = vk::format(i.format);
            ivci.components       = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
            ivci.subresourceRange = isr;

            VkImageView imageView;
            checkResult(vkCreateImageView(logicalDevice.handle, &ivci, nullptr, &imageView), "Failed to create an image view.");

            Image img;
            img.handle = ImageHnd(image);
            img.format = i.format;
            img.width  = extent.x;
            img.height = extent.y;
            img.levels = 1;
            img.layers = 1;
            img.type   = ImageViewType::Im2D;

            fbo.images.push_back(std::move(img));
            fbo.imageViews.push_back(ImageViewHnd(imageView));
            fbo.isImageOwned.push_back(true);
        }
    }
    
    // empty submit------------
//    VkCommandBufferBeginInfo cbbi;
//    cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    cbbi.pNext            = nullptr;
//    cbbi.flags            = 0;//TODO?
//    cbbi.pInheritanceInfo = nullptr;
//    
//    checkResult(vkBeginCommandBuffer(imageUploadCommandBuffer, &cbbi), "com_buffer_begin_fail");
//
//    checkResult(vkEndCommandBuffer(imageUploadCommandBuffer), "com_buffer_end_fail");
//
//    VkSubmitInfo si;
//    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    si.pNext                = nullptr;
//    si.waitSemaphoreCount   = 0;
//    si.pWaitSemaphores      = nullptr;
//    si.pWaitDstStageMask    = nullptr;
//    si.commandBufferCount   = 1;
//    si.pCommandBuffers      = &imageUploadCommandBuffer;
//    si.signalSemaphoreCount = 0;
//    si.pSignalSemaphores    = nullptr;
//
//    checkResult(vkQueueSubmit(queue, 1, &si, nullptr), "queue_submit_fail");
//    checkResult(vkQueueWaitIdle(queue), "queue_wait_fail");
    // empty submit end------------
    
    VkFramebufferCreateInfo fbci;
    fbci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbci.pNext           = nullptr;
    fbci.flags           = 0;
    fbci.renderPass      = renderPass.toNative<VkRenderPass>();
    fbci.attachmentCount = static_cast<std::uint32_t>(fbo.imageViews.size());
    fbci.pAttachments    = reinterpret_cast<const VkImageView*>(fbo.imageViews.data());
    fbci.width           = extent.x;
    fbci.height          = extent.y;
    fbci.layers          = 1;
    
    VkFramebuffer fb;
    checkResult(vkCreateFramebuffer(logicalDevice.handle, &fbci, nullptr, &fb), "Failed to create a framebuffer.");
    
    fbo.handle = FramebufferHnd(fb);
    return fbo;
}

void VulkanAPI::destroyFramebufferWithAttachments(const Framebuffer& framebuffer) {
    assert(framebuffer.imageViews.size() == framebuffer.images.size());
    
    for (std::size_t j = 0; j < framebuffer.images.size(); ++j) {
        const auto& i = framebuffer.images[j];
        
        if (framebuffer.isImageOwned[j]) { // Jei ne - reikšia šitas view mums buvo perduotas ir už jo naikinimą atsakingas vartotojas
            VkDeviceMemory memory = imageToMemory[i.handle.toNative<VkImage>()];
    
            vkDestroyImage(logicalDevice.handle, i.handle.toNative<VkImage>(), nullptr);
            vkFreeMemory(logicalDevice.handle, memory, nullptr);

            imageToMemory.erase(i.handle.toNative<VkImage>());
        }
        
        ImageViewHnd v = framebuffer.imageViews[j];
        vkDestroyImageView(logicalDevice.handle, v.toNative<VkImageView>(), nullptr);
    }
    
    vkDestroyFramebuffer(logicalDevice.handle, framebuffer.handle.toNative<VkFramebuffer>(), nullptr);
}

// -------------------------------Command buffer
void VulkanCommandBuffer::setViewport(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) {
    tempViewports.clear();
    
    if (tempViewports.capacity() < count) { // TODO patikrinti, kas geriau šitas nuolatinis tikrinimas, ar fiksuotas ir lygus maksimaliam skaičiui vektorius TODO tas pats su scissors
        tempViewports.resize(count);
    }
    
    for (size_t i = 0; i < count; ++i) {
        auto& vp = tempViewports[i];//TODO scissors ir viewports konvertavimą į atskiras funkcijas, nes keliose vietose darom lygiai tą patį. Tas pats ir su OpenGL
        const auto& v = viewports[i];
        
        vp.x        = v.x;
        vp.y        = v.y;
        vp.width    = v.width;
        vp.height   = v.height;
        vp.minDepth = v.minDepth;
        vp.maxDepth = v.maxDepth;
    }
    
    vkCmdSetViewport(cmdBuff, first, count, tempViewports.data());
}

void VulkanCommandBuffer::setScissor(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) {
    tempScissors.clear();
    
    if (tempScissors.capacity() < count) {
        tempScissors.resize(count);
    }
    
    for (size_t i = 0; i < count; ++i) {
        auto& sc = tempScissors[i];
        const auto& s = rectangles[i];
        
        sc.offset.x      = s.offset.x;
        sc.offset.y      = s.offset.y;
        sc.extent.width  = s.extent.x;
        sc.extent.height = s.extent.y;
    }
    
    vkCmdSetScissor(cmdBuff, first, count, tempScissors.data());
}

void VulkanCommandBuffer::draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) {
    vkCmdDraw(cmdBuff, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance)  {
    vkCmdDrawIndexed(cmdBuff, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    vkCmdDispatch(cmdBuff, x, y, z);
}

void VulkanCommandBuffer::bindVertexBuffers(std::uint32_t firstBinding, const Buffer& buffer) {
    VkBuffer vkbuffer = buffer.handle().toNative<VkBuffer>();
    VkDeviceSize offset = static_cast<VkDeviceSize>(0);
    
    vkCmdBindVertexBuffers(cmdBuff, static_cast<std::uint32_t>(firstBinding), 1, &vkbuffer, &offset);
}

void VulkanCommandBuffer::bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<Buffer>& buffers) {
    //tempBuffers.clear();
    //tempOffsets.clear();
    
    //if (tempBuffers.capacity() < bindingCount) {
    //    tempBuffers.resize(bindingCount);
    //    tempOffsets.resize(bindingCount);
    //}
    
    for (size_t i = 0; i < bindingCount; ++i) {
        tempBuffers[i] = buffers[i].handle().toNative<VkBuffer>();
        tempOffsets[i] = static_cast<VkDeviceSize>(0);
    }
    
    vkCmdBindVertexBuffers(cmdBuff, static_cast<std::uint32_t>(firstBinding), bindingCount, tempBuffers.data(), tempOffsets.data());
}

void VulkanCommandBuffer::bindIndexBuffer(const Buffer& buffer, IndexType indexType) {
    vkCmdBindIndexBuffer(cmdBuff, buffer.handle().toNative<VkBuffer>(), static_cast<VkDeviceSize>(0), vkutil::mapIndexType(indexType));
}

bool VulkanCommandBuffer::bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) {
    const VkDescriptorSet* sets = reinterpret_cast<VkDescriptorSet*>(const_cast<DescriptorSetHnd*>(descriptorSets.data()));
    
    vkCmdBindDescriptorSets(cmdBuff, point == PipelineBindPoint::Graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE, layout.toNative<VkPipelineLayout>(), firstSet, descriptorSets.size(), sets, dynamicOffsets.size(), dynamicOffsets.data());
    
    return true;
}

void VulkanCommandBuffer::pushConstants(PipelineLayoutHnd handle, ShaderStageFlags flags, std::uint32_t offset, std::uint32_t size, const void* data) {
    vkCmdPushConstants(cmdBuff, handle.toNative<VkPipelineLayout>(), vk::shaderStage(flags), offset, size, data);
}

void VulkanCommandBuffer::bindPipeline(const Pipeline& pipeline) {
    vkCmdBindPipeline(cmdBuff, vkutil::mapPipelineBindPoint(pipeline.bindPoint), pipeline.handle.toNative<VkPipeline>());
}

void VulkanCommandBuffer::begin(const CommandBufferBeginInfo& cbbi) {
    VkCommandBufferBeginInfo vkcbbi;
    vkcbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkcbbi.pNext            = nullptr;
    vkcbbi.flags            = vkutil::mapBufferUsageFlags(cbbi.flags);
    VkCommandBufferInheritanceInfo cbii = vkutil::mapInheritanceInfo(cbbi.inheritanceInfo);
    vkcbbi.pInheritanceInfo = (level == BufferLevel::Secondary) ? &cbii : nullptr;

    VkResult result = vkBeginCommandBuffer(cmdBuff, &vkcbbi);
    backend->checkResult(result, "Failed to begin a command buffer.");
}

void VulkanCommandBuffer::end() {
    VkResult result = vkEndCommandBuffer(cmdBuff);
    backend->checkResult(result, "Failed to end a command buffer.");
}

void VulkanCommandBuffer::beginRenderPass(const RenderPassBeginInfo& rpbi, SubpassContents contents) {
    VkRenderPassBeginInfo vkrpbi;
    vkrpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    vkrpbi.pNext = nullptr;
    vkrpbi.renderPass = rpbi.renderPass.toNative<VkRenderPass>();//renderPass;
    vkrpbi.framebuffer = rpbi.framebuffer.toNative<VkFramebuffer>();//framebuffers[currentSwapBuffer];
    vkrpbi.renderArea.offset.x = rpbi.renderArea.offset.x;
    vkrpbi.renderArea.offset.y = rpbi.renderArea.offset.y;
    vkrpbi.renderArea.extent.width = rpbi.renderArea.extent.x;//getRenderSurfaceWidth();
    vkrpbi.renderArea.extent.height = rpbi.renderArea.extent.y;//getRenderSurfaceHeight();
    vkrpbi.clearValueCount = rpbi.clearValues.size();
    std::vector<VkClearValue> clearValues = vkutil::mapClearValues(rpbi.clearValues);
    vkrpbi.pClearValues = clearValues.data();
    
    vkCmdBeginRenderPass(cmdBuff, &vkrpbi, (contents == SubpassContents::Inline) ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void VulkanCommandBuffer::nextSubpass(SubpassContents contents) {
    vkCmdNextSubpass(cmdBuff, (contents == SubpassContents::Inline) ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
}

void VulkanCommandBuffer::endRenderPass() {
    vkCmdEndRenderPass(cmdBuff);
}

// --------------------------------- Command pool

CommandBuffer* VulkanCommandPool::allocateCommandBuffer(BufferLevel level, bool beginBuffer) {
    VkResult result;
    VkCommandBuffer buffer;

    VkCommandBufferAllocateInfo cbai;
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext              = nullptr;
    cbai.commandPool        = commandPool;
    cbai.level              = vkutil::mapBufferLevel(level);
    cbai.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(backend->logicalDevice.handle, &cbai, &buffer);
    backend->checkResult(result, "Failed to allocate a command buffer.");

    VulkanCommandBuffer* newCmdBuff = new VulkanCommandBuffer(level, backend, buffer, backend->physicalDevice.properties);
    if (beginBuffer) {
        newCmdBuff->begin();
    }
    
    return newCmdBuff;
}

std::vector<CommandBuffer*> VulkanCommandPool::allocateCommandBuffers(std::uint32_t count, BufferLevel level, bool beginBuffers) {
    VkResult result;
    std::vector<VkCommandBuffer> buffers;
    buffers.resize(count);

    VkCommandBufferAllocateInfo cbai;
    cbai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.pNext              = nullptr;
    cbai.commandPool        = commandPool;
    cbai.level              = vkutil::mapBufferLevel(level);
    cbai.commandBufferCount = count;

    result = vkAllocateCommandBuffers(backend->logicalDevice.handle, &cbai, buffers.data());
    backend->checkResult(result, "Failed to allocate command buffers.");

    std::vector<CommandBuffer*> newCmdBuffs;
    newCmdBuffs.reserve(count);
    
    for (auto b : buffers) {
        VulkanCommandBuffer* buff = new VulkanCommandBuffer(level, backend, b, backend->physicalDevice.properties);
        
        if (beginBuffers) {
            buff->begin();
        }
        
        newCmdBuffs.push_back(buff);
    }
    
    return newCmdBuffs;
}

void VulkanCommandPool::freeCommandBuffer(CommandBuffer* cmdBuff) {
    VulkanCommandBuffer* buffer = static_cast<VulkanCommandBuffer*>(cmdBuff);
    
    vkFreeCommandBuffers(backend->logicalDevice.handle, commandPool, 1, &(buffer->cmdBuff));
    
    delete buffer;
}

void VulkanCommandPool::freeCommandBuffers(const std::vector<CommandBuffer*>& cmdBuffs) {
    for (auto cmdBuff : cmdBuffs) {
        VulkanCommandBuffer* buffer = static_cast<VulkanCommandBuffer*>(cmdBuff);
        
        vkFreeCommandBuffers(backend->logicalDevice.handle, commandPool, 1, &(buffer->cmdBuff));
        
        delete buffer;
    }
}

}


#endif //BUILD_WITH_VULKAN
