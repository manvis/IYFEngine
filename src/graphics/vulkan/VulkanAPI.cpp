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

#include <iostream>

#include <fstream>
#include <cstring>
#include <SDL_syswm.h>

#undef Always // X11 Macro
#undef None // X11 Macro
#undef Bool

#include "core/Engine.hpp"
#include "core/Logger.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/vulkan/VulkanUtilities.hpp"
#include "threading/ThreadProfiler.hpp"

#include "libshaderc/shaderc.hpp"
#include "stb_image.h"

namespace iyf {
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
    deviceMemoryManager = nullptr;
    
    freePresentationBarrierCommandBuffers();
    physicalDevice.queueFamilyProperties.clear();
    
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &mainCommandBuffer);
    vkFreeCommandBuffers(logicalDevice.handle, commandPool, 1, &imageUploadCommandBuffer);
    
    for (std::uint32_t i = 0; i < swapchain.images.size(); ++i) {
        vkDestroyImageView(logicalDevice.handle, swapchain.imageViews[i], nullptr);
    }
    
    vkDestroySwapchain(logicalDevice.handle, swapchain.handle, nullptr);
    vkDestroyFence(logicalDevice.handle, swapchain.frameCompleteFence, nullptr);
    
    vkDestroySurfaceKHR(instance, surface, nullptr);
    
    vkDestroyPipelineCache(logicalDevice.handle, pipelineCache, nullptr);
    vkDestroyCommandPool(logicalDevice.handle, commandPool, nullptr);
    
    vkDestroySemaphore(logicalDevice.handle, presentationComplete, nullptr);
    vkDestroySemaphore(logicalDevice.handle, renderingComplete, nullptr);
    
    vmaDestroyBuffer(allocator, imageTransferSource, imageTransferSourceAllocation);
    vmaDestroyAllocator(allocator);
    
    vkDestroyDevice(logicalDevice.handle, nullptr);
    
    if (isDebug) {
        destroyDebugReportCallback(instance, debugReportCallback, nullptr);
    }
    
    vkDestroyInstance(instance, nullptr);
    SDL_DestroyWindow(window);
}

bool VulkanAPI::startFrame() {
    IYFT_PROFILE(startFrame, iyft::ProfilerTag::Graphics);
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
    IYFT_PROFILE(endFrame, iyft::ProfilerTag::Graphics);
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
    
    result = vkQueueSubmit(logicalDevice.mainQueue, 1, &si, swapchain.frameCompleteFence);
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
    
    
    checkResult(vkWaitForFences(logicalDevice.handle, 1, &swapchain.frameCompleteFence, VK_TRUE, 50000000000), "Failed to wait on a fence."); //TODO good timeout value?
    checkResult(vkResetFences(logicalDevice.handle, 1, &swapchain.frameCompleteFence), "Failed to reset a fence");
    
//     result = vkQueueWaitIdle(logicalDevice.mainQueue);
//     checkResult(result, "Failed to wait on a queue.");
//     //vkDeviceWaitIdle(logicalDevice.handle);
    
    return true;
}

ShaderHnd VulkanAPI::createShader(ShaderStageFlags, const void* data, std::size_t byteCount) {
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

ShaderHnd VulkanAPI::createShaderFromSource(ShaderStageFlags shaderStage, const std::string& code) {
    shaderc_shader_kind shaderKind;
    switch (static_cast<ShaderStageFlagBits>(shaderStage.uint64())) {
        case ShaderStageFlagBits::Vertex:
            shaderKind = shaderc_vertex_shader;
            break;
        case ShaderStageFlagBits::TessControl:
            shaderKind = shaderc_tess_control_shader;
            break;
        case ShaderStageFlagBits::TessEvaluation:
            shaderKind = shaderc_tess_evaluation_shader;
            break;
        case ShaderStageFlagBits::Geometry:
            shaderKind = shaderc_geometry_shader;
            break;
        case ShaderStageFlagBits::Fragment:
            shaderKind = shaderc_fragment_shader;
            break;
        case ShaderStageFlagBits::Compute:
            shaderKind = shaderc_vertex_shader;
            break;
        default:
            throw std::invalid_argument("An unknown shader stage has been specified.");
    }
    
    shaderc::Compiler compiler;
    shaderc::CompileOptions compilerOptions;
    compilerOptions.SetOptimizationLevel(shaderc_optimization_level_size);
    
    shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(code, shaderKind, "shaderFromSource", compilerOptions);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        LOG_E("Failed to compile a shader from source. Error: " << result.GetErrorMessage());
        throw std::runtime_error("Failed to compile a shader from source. Check log for more info.");
    }
    
    std::vector<std::uint32_t> content(result.begin(), result.end());
    
    VkShaderModuleCreateInfo mci;
    mci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    mci.pNext    = nullptr;
    mci.codeSize = content.size() * sizeof(std::uint32_t);
    mci.pCode    = content.data();
    mci.flags    = 0;

    VkShaderModule module;
    checkResult(vkCreateShaderModule(logicalDevice.handle, &mci, nullptr, &module), "Failed to create a shader module.");

    return ShaderHnd(module);
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
        assert(!engine->getRenderer()->isRenderSurfaceSizeDynamic());
        const glm::uvec2 renderSurfaceSize = engine->getRenderer()->getRenderSurfaceSize();
        
        VkViewport vp;
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = renderSurfaceSize.x;
        vp.height = renderSurfaceSize.y;
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
        assert(!engine->getRenderer()->isRenderSurfaceSizeDynamic());
        const glm::uvec2 renderSurfaceSize = engine->getRenderer()->getRenderSurfaceSize();
        
        VkRect2D sc;
        sc.offset.x = 0;
        sc.offset.y = 0;
        sc.extent.width = renderSurfaceSize.x;
        sc.extent.height = renderSurfaceSize.y;
               
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

Image VulkanAPI::createImage(const ImageCreateInfo& info) {
    VkImageCreateInfo ici;
    ici.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.pNext                 = nullptr;
    ici.flags                 = (info.isCube) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; //TODO CUBEMAP flags
    ici.imageType             = VK_IMAGE_TYPE_2D;
    ici.format                = vk::format(info.format);
    ici.extent                = {info.extent.x, info.extent.y, info.extent.z};
    ici.mipLevels             = info.mipLevels;
    ici.arrayLayers           = info.arrayLayers;
    ici.samples               = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling                = VK_IMAGE_TILING_OPTIMAL; // TODO determine if it can be optimally tiled
    ici.usage                 = vk::imageUsage(info.usage);
    ici.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ici.queueFamilyIndexCount = 0;
    ici.pQueueFamilyIndices   = nullptr;
    ici.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    
    VmaAllocationCreateInfo aci = {};
    aci.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    if (!info.debugName.empty()) {
        std::string temp = info.debugName;
        aci.pUserData = &temp[0];
    }
    
    VkImage image;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    checkResult(vmaCreateImage(allocator, &ici, &aci, &image, &allocation, &allocationInfo), "Failed to create an image");
    
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, allocationInfo.memoryType, &flags);
    
    auto iter = imageToMemory.emplace(std::make_pair(image, AllocationAndInfo(allocation, allocationInfo, flags)));
    return Image(ImageHnd(image), info.extent, info.mipLevels, info.arrayLayers, info.usage, info.format, info.isCube ? ImageViewType::ImCube : ImageViewType::Im2D, &(iter.first->second));
}

Image VulkanAPI::createUncompressedImage(const UncompressedImageCreateInfo& info) {
    const ImageMemoryType type = info.type;
    const glm::uvec2& dimensions = info.dimensions;
    const bool isWritable = info.isWritable;
    const bool usedAsColorOrDepthAttachment = info.usedAsColorOrDepthAttachment;
    const bool usedAsInputAttachment = info.usedAsInputAttachment;
    const bool usedAsTransferSource = info.usedAsTransferSource;
    const void* data = info.data;
    
    std::uint64_t size;
    VkFormat format;
    Format engineFormat;
    
    bool needsMemoryUpload = (data != nullptr);
    bool isDepthStencil = (type == ImageMemoryType::DepthStencilFloat);
    
    if (type == ImageMemoryType::R32) {
        size = 4 * dimensions.x * dimensions.y;
        format = VK_FORMAT_R32_UINT;
        engineFormat = Format::R32_uInt;
    } else if (type == ImageMemoryType::RGB) {
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
    } else if (type == ImageMemoryType::DepthStencilFloat) {
        assert(Format::D32_sFloat_S8_uInt == depthStencilFormatEngine);
        
        size = (4 + 1) * (dimensions.x * dimensions.y); // 4 bytes for the depth part, 1 for stencil
        format = depthStencilFormat;
        engineFormat = depthStencilFormatEngine;
    } else {
        throw std::runtime_error("Invalid texture type.");
    }
    
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice.handle, format, &formatProperties);
    
    VkImageUsageFlags iuf = VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageUsageFlags iufEng = ImageUsageFlagBits::Sampled;
    
    bool linearTilingRequired = false;
    
    // TODO what if some features are only supported in linear mode and some in optimal mode?
    if (usedAsColorOrDepthAttachment) {
        if (isDepthStencil) {
            iuf |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            iufEng |= ImageUsageFlagBits::DepthStencilAttachment;
            
            vkutil::SupportedTiling st = vkutil::getSupportedTiling(formatProperties, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
            if (st == vkutil::SupportedTiling::NotSupported) {
                throw std::runtime_error("The required image format cannot be used as a depth stencil attachment");
            } else if (st == vkutil::SupportedTiling::Linear) {
                LOG_W("The image that is currently being created cannot use optimal tiling.")
                linearTilingRequired = true;
            }
        } else {
            iuf |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            iufEng |= ImageUsageFlagBits::ColorAtachment;
            
            vkutil::SupportedTiling st = vkutil::getSupportedTiling(formatProperties, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
            if (st == vkutil::SupportedTiling::NotSupported) {
                throw std::runtime_error("The required image format cannot be used as a color attachment");
            } else if (st == vkutil::SupportedTiling::Linear) {
                LOG_W("The image that is currently being created cannot use optimal tiling.")
                linearTilingRequired = true;
            }
        }
    }
    
    if (usedAsTransferSource) {
        iuf |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        iufEng |= ImageUsageFlagBits::TransferSource;
        
        vkutil::SupportedTiling st = vkutil::getSupportedTiling(formatProperties, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
        if (st == vkutil::SupportedTiling::NotSupported) {
            throw std::runtime_error("The required image format cannot be used as a transfer source");
        } else if (st == vkutil::SupportedTiling::Linear) {
            LOG_W("The image that is currently being created cannot use optimal tiling.")
            linearTilingRequired = true;
        }
    }
    
    if (usedAsInputAttachment) {
        iuf |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        iufEng |= ImageUsageFlagBits::InputAttachment;
        
        // TODO do I need a check here?
    }
    
    if (needsMemoryUpload) {
        iuf |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        iufEng |= ImageUsageFlagBits::TransferDestination;
    }
    
    if (isWritable) {
        iuf |= VK_IMAGE_USAGE_STORAGE_BIT;
        iufEng |= ImageUsageFlagBits::Storage;
    }
    
    ImageCreateInfo ici(glm::uvec3(dimensions.x, dimensions.y, 1), 1, 1, iufEng, engineFormat, false, "UncompressedImage");
    Image image = createImage(ici);
    
    if (!needsMemoryUpload) {
        VkCommandBufferBeginInfo cbbi;
        cbbi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cbbi.pNext            = nullptr;
        cbbi.flags            = 0;//TODO?
        cbbi.pInheritanceInfo = nullptr;

        checkResult(vkBeginCommandBuffer(imageUploadCommandBuffer, &cbbi), "Failed to begin a command buffer.");
        
        VkImageSubresourceRange sr;
        sr.aspectMask     = isDepthStencil ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;
        sr.baseMipLevel   = 0;
        sr.levelCount     = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount     = 1;
        
        setImageLayout(imageUploadCommandBuffer, image.getHandle().toNative<VkImage>(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, sr);

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

        bic.imageSubresource.aspectMask     = isDepthStencil ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;;
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
        sr.aspectMask     = isDepthStencil ? (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT) : VK_IMAGE_ASPECT_COLOR_BIT;;
        sr.baseMipLevel   = 0;
        sr.levelCount     = 1;
        sr.baseArrayLayer = 0;
        sr.layerCount     = 1;

        setImageLayout(imageUploadCommandBuffer, image.getHandle().toNative<VkImage>(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, sr);
        vkCmdCopyBufferToImage(imageUploadCommandBuffer, stagingData.first, image.getHandle().toNative<VkImage>(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bics.size(), bics.data());
        setImageLayout(imageUploadCommandBuffer, image.getHandle().toNative<VkImage>(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sr);

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
    
    LOG_V("Created an image:" << 
          "\n\t\tDimensions: " << dimensions.x << "x" << dimensions.y <<
          "\n\t\tFormat: " << getFormatName(engineFormat) <<
          "\n\t\tOptimal tiling: " << (linearTilingRequired ? "false" : "true"));
    
    return image;
}

bool VulkanAPI::destroyImage(const Image& image) {
    VkImage handle = image.getHandle().toNative<VkImage>();
    
    auto iter = imageToMemory.find(handle);
    
    if (iter == imageToMemory.end()) {
        throw std::runtime_error("Can't destoy a non existing image");
    }
    
    vmaDestroyImage(allocator, image.getHandle().toNative<VkImage>(), iter->second.allocation);
    
    imageToMemory.erase(iter);
    
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

Buffer VulkanAPI::createBuffer(const BufferCreateInfo& info) {
    VkBufferCreateInfo bci;
    bci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext                 = nullptr;
    bci.flags                 = 0;
    bci.size                  = info.size.count();
    bci.usage                 = vk::bufferUsage(info.flags);
    bci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE; // TODO use a transfer queue (if available)
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices   = nullptr;
    
    VmaAllocationCreateInfo aci = {};
    
    // TODO start using  info.frequentHostAccess;
    
    // VMA_ALLOCATION_CREATE_MAPPED_BIT will be ignored if the device memory cannot be mapped. Do not disable. The VulkanDeviceMemoryManager
    // checks the allocationInfo.pMappedData to determine if a staging buffer is needed or not. If VMA_ALLOCATION_CREATE_MAPPED_BIT flag isn't
    // present, pMappedData will always be null and that will confuse the VulkanDeviceMemoryManager.
    aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    
    // TODO for now, it's either CPU (visible and coherent) or GPU. Only enable the rest once proper flushing and invalidation
    // during memory uploads or reads is implemented.
    switch (info.memoryUsage) {
        case MemoryUsage::GPUOnly:
            aci.usage = VMA_MEMORY_USAGE_GPU_ONLY; // May end up being HOST_VISIBLE in some cases.
            break;
        case MemoryUsage::CPUToGPU:
            aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU; // Guarantees host visible. Usually uncached.
            break;
        case MemoryUsage::GPUToCPU:
            aci.usage = VMA_MEMORY_USAGE_GPU_TO_CPU; // Guarantees host visible and cached.
            break;
        case MemoryUsage::CPUOnly:
            aci.usage = VMA_MEMORY_USAGE_CPU_ONLY; // Guarantees HOST_VISIBLE and HOST_COHERENT
            break;
    }
    
    if (!info.debugName.empty()) {
        std::string temp = info.debugName;
        aci.pUserData = &temp[0];
    }
    
    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    checkResult(vmaCreateBuffer(allocator, &bci, &aci, &handle, &allocation, &allocationInfo), "Failed to create a buffer");
    
    if (aci.usage != VMA_MEMORY_USAGE_GPU_ONLY && allocationInfo.pMappedData == nullptr) {
        throw std::runtime_error("Was expecting a mapped buffer.");
    }
    
    const Bytes finalSize = Bytes(allocationInfo.size);
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, allocationInfo.memoryType, &flags);
    
    auto iter = bufferToMemory.emplace(std::make_pair(handle, AllocationAndInfo(allocation, allocationInfo, flags)));
    
    return Buffer(BufferHnd(handle), info.flags, info.memoryUsage, finalSize, &(iter.first->second));
}

bool VulkanAPI::destroyBuffer(const Buffer& buffer) {
    VkBuffer handle = buffer.handle().toNative<VkBuffer>();
    auto result = bufferToMemory.find(handle);
    
    if (result == bufferToMemory.end()) {
        throw std::runtime_error("Can't destoy a non existing buffer");
    }
    
    vmaDestroyBuffer(allocator, handle, result->second.allocation);
    
    bufferToMemory.erase(result);
    
    return true;
}

bool VulkanAPI::readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) {
    assert(buffer.memoryUsage() == MemoryUsage::CPUOnly);
    
    auto allocationAndInfo = bufferToMemory.find(buffer.handle().toNative<VkBuffer>());
    assert(allocationAndInfo != bufferToMemory.end());
    
    void* p = allocationAndInfo->second.info.pMappedData;
    // TODO non-coherent
    
    for (const auto& c : copies) {
        const char* source = static_cast<const char*>(p);
        source += c.srcOffset;
        
        char* destination = static_cast<char*>(data);
        destination += c.dstOffset;
        
        std::memcpy(destination, source, c.size);
    }
    
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

inline static VkAttachmentReference* ProcessAttachmentReferences(std::size_t count, const AttachmentReference* firstAttachment, std::vector<VkAttachmentReference>& convertedReferences) {
    if (count == 0) {
        return nullptr;
    }
    
    assert(firstAttachment != nullptr);
    
    const std::size_t currentSize = convertedReferences.size();
    if ((count + currentSize) >= convertedReferences.capacity()) {
        throw std::runtime_error("Too many VkAttachmentReference objects");
    }
    
    VkAttachmentReference* firstConvertedElement = convertedReferences.data() + currentSize;
    
    for (std::size_t i = 0; i < count; ++i) {
        const AttachmentReference* currentAttachment = firstAttachment + i;
        
        VkAttachmentReference ar;
        ar.attachment = currentAttachment->attachment;
        ar.layout = vk::imageLayout(currentAttachment->layout);
        
        convertedReferences.push_back(ar);
    }
    
    return firstConvertedElement;
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
        VkAttachmentDescription adv;
        adv.flags = static_cast<VkAttachmentDescriptionFlags>(d.flags);
        adv.samples = static_cast<VkSampleCountFlagBits>(vk::sampleCount(d.samples));
        adv.format = vk::format(d.format);
        adv.loadOp = vk::attachmentLoadOp(d.loadOp);
        adv.storeOp = vk::attachmentStoreOp(d.storeOp);
        adv.stencilLoadOp = vk::attachmentLoadOp(d.stencilLoadOp);
        adv.stencilStoreOp = vk::attachmentStoreOp(d.stencilStoreOp);
        adv.initialLayout = vk::imageLayout(d.initialLayout);
        adv.finalLayout = vk::imageLayout(d.finalLayout);
        
        ad.emplace_back(std::move(adv));
    }
    rpci.pAttachments = (info.attachments.size() == 0) ? nullptr : ad.data();
    
    // Subpass contents
    rpci.subpassCount = info.subpasses.size();
    std::vector<VkSubpassDescription> sd;
    std::vector<VkAttachmentReference> ar;
    ar.reserve(40);
    
    sd.reserve(info.subpasses.size());
    for (const auto &d : info.subpasses) {
        VkSubpassDescription sdv;
        sdv.flags = 0;
        sdv.pipelineBindPoint = vkutil::mapPipelineBindPoint(d.pipelineBindPoint);
        sdv.inputAttachmentCount = static_cast<std::uint32_t>(d.inputAttachments.size());
        sdv.pInputAttachments = ProcessAttachmentReferences(d.inputAttachments.size(), d.inputAttachments.data(), ar);
        sdv.colorAttachmentCount = static_cast<std::uint32_t>(d.colorAttachments.size());
        sdv.pColorAttachments = ProcessAttachmentReferences(d.colorAttachments.size(), d.colorAttachments.data(), ar);
        sdv.pResolveAttachments = ProcessAttachmentReferences(d.resolveAttachments.size(), d.resolveAttachments.data(), ar);
        sdv.pDepthStencilAttachment = ProcessAttachmentReferences(1, &d.depthStencilAttachment, ar);
        sdv.preserveAttachmentCount = static_cast<std::uint32_t>(d.preserveAttachments.size());
        sdv.pPreserveAttachments = d.preserveAttachments.data();
        
        sd.emplace_back(std::move(sdv));
    }
    rpci.pSubpasses      = (info.subpasses.size() == 0) ? nullptr : sd.data();
    
    // Subpass dependencies
    rpci.dependencyCount = info.dependencies.size();
    std::vector<VkSubpassDependency> spd;// = vkutil::mapSubpassDependencies(info.dependencies);
    spd.reserve(info.dependencies.size());
    for (const auto &d : info.dependencies) {
        VkSubpassDependency spdv;
        spdv.srcSubpass = d.srcSubpass;
        spdv.dstSubpass = d.dstSubpass;
        spdv.srcStageMask = static_cast<VkPipelineStageFlags>(d.srcStageMask);
        spdv.dstStageMask = static_cast<VkPipelineStageFlags>(d.dstStageMask);
        spdv.srcAccessMask = static_cast<VkAccessFlags>(d.srcAccessMask);
        spdv.dstAccessMask = static_cast<VkAccessFlags>(d.dstAccessMask);
        spdv.dependencyFlags = static_cast<VkDependencyFlags>(d.dependencyFlags);

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

            bool isDepthStencil = vkutil::isDepthStencilFormat(img.getFormat());
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
            ivci.image            = img.getHandle().toNative<VkImage>();
            ivci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format           = vk::format(img.getFormat());
            ivci.components       = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
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
            ImageUsageFlags iuf = i.isAttachment ? ImageUsageFlagBits::InputAttachment : ImageUsageFlagBits::Sampled;
            
            VkImageAspectFlags af;

            bool isDepthStencil = vkutil::isDepthStencilFormat(i.format);
            if (isDepthStencil) {
                iuf |= ImageUsageFlagBits::DepthStencilAttachment;
                af = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            } else {
                iuf |= ImageUsageFlagBits::ColorAtachment;
                af = VK_IMAGE_ASPECT_COLOR_BIT;
            }

            ImageCreateInfo ici(glm::uvec3(extent, 1), 1, 1, iuf, i.format, false, "FramebufferImage");
            Image image = createImage(ici);

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
            ivci.image            = image.getHandle().toNative<VkImage>();
            ivci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format           = vk::format(i.format);
            ivci.components       = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            ivci.subresourceRange = isr;

            VkImageView imageView;
            checkResult(vkCreateImageView(logicalDevice.handle, &ivci, nullptr, &imageView), "Failed to create an image view.");

            fbo.images.push_back(std::move(image));
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
        
        if (framebuffer.isImageOwned[j]) {
            destroyImage(i);
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

// std::uint64_t bufferOffset;
// std::uint32_t bufferRowLength;
// std::uint32_t bufferImageHeight;
// ImageSubresourceLayers imageSubresource;
// glm::ivec3 imageOffset;
// glm::uvec3 imageExtent;
void VulkanCommandBuffer::copyImageToBuffer(const Image& srcImage, ImageLayout layout, const Buffer& dstBuffer, const std::vector<BufferImageCopy>& regions) {
    std::vector<VkBufferImageCopy> convertedRegions;
    convertedRegions.reserve(regions.size());
    
    for (const auto& r : regions) {
        VkBufferImageCopy bic;
        bic.bufferOffset      = r.bufferOffset;
        bic.bufferRowLength   = r.bufferRowLength;
        bic.bufferImageHeight = r.bufferImageHeight;
        
        bic.imageSubresource.aspectMask     = vkutil::mapAspectMask(r.imageSubresource.aspectMask);
        bic.imageSubresource.mipLevel       = r.imageSubresource.mipLevel;
        bic.imageSubresource.layerCount     = r.imageSubresource.layerCount;
        bic.imageSubresource.baseArrayLayer = r.imageSubresource.baseArrayLayer;
        
        bic.imageOffset.x = r.imageOffset.x;
        bic.imageOffset.y = r.imageOffset.y;
        bic.imageOffset.z = r.imageOffset.z;
        
        bic.imageExtent.width = r.imageExtent.x;
        bic.imageExtent.height = r.imageExtent.y;
        bic.imageExtent.depth = r.imageExtent.z;
        
        convertedRegions.push_back(std::move(bic));
    }
    
    vkCmdCopyImageToBuffer(cmdBuff, srcImage.getHandle().toNative<VkImage>(), vk::imageLayout(layout), dstBuffer.handle().toNative<VkBuffer>(), regions.size(), convertedRegions.data());
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
