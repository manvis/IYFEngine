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

#include "graphics/vulkan/VulkanAPI.hpp"

#include "utilities/hashing/Hashing.hpp"
#include "core/Logger.hpp"
#include "utilities/StringUtilities.hpp"

namespace iyf {
void VulkanAPI::createDebugCallback() {
    if (isDebug) {
        LOG_V("Preparing Vulkan debug and validation");
        
        createDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
        destroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        debugReportMessage = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT");
        
        if (createDebugReportCallback == VK_NULL_HANDLE || destroyDebugReportCallback == VK_NULL_HANDLE || debugReportMessage == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to load debug function addresses");
        }

        VkDebugReportFlagsEXT flags = 0;
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkan_debug_information_flag"), ConfigurationValueFamily::Engine))) {
            flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkan_debug_warning_flag"), ConfigurationValueFamily::Engine))) {
            flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkan_debug_performance_warning_flag"), ConfigurationValueFamily::Engine))) {
            flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkan_debug_error_flag"), ConfigurationValueFamily::Engine))) {
            flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkan_debug_debug_flag"), ConfigurationValueFamily::Engine))) {
            flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        }

        VkDebugReportCallbackCreateInfoEXT drcci;
        drcci.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        drcci.pNext       = nullptr;
        drcci.pfnCallback = (PFN_vkDebugReportCallbackEXT)vulkanDebugCallback;
        drcci.pUserData   = nullptr;
//        drcci.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        drcci.flags = flags;
        
        checkResult(createDebugReportCallback(instance, &drcci, nullptr, &debugReportCallback), "debug_callback_failed");
        
        LOG_V("Debug ");
    }
}

bool VulkanAPI::checkResult(VkResult result, const std::string& whatFailed, bool throwIfFailed) {
    if (result == VK_SUCCESS) {
        return true;
    }
    
    std::string resultCode;
    switch (result) {
    case VK_NOT_READY:
        resultCode = "VK_NOT_READY";
        break;
    case VK_TIMEOUT:
        resultCode = "VK_TIMEOUT";
        break;
    case VK_EVENT_SET:
        resultCode = "VK_EVENT_SET";
        break;
    case VK_EVENT_RESET:
        resultCode = "VK_EVENT_RESET";
        break;
    case VK_INCOMPLETE:
        resultCode = "VK_INCOMPLETE";
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        resultCode = "VK_ERROR_OUT_OF_HOST_MEMORY";
        break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        resultCode = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        break;
    case VK_ERROR_INITIALIZATION_FAILED:
        resultCode = "VK_ERROR_INITIALIZATION_FAILED";
        break;
    case VK_ERROR_DEVICE_LOST:
        resultCode = "VK_ERROR_DEVICE_LOST";
        break;
    case VK_ERROR_MEMORY_MAP_FAILED:
        resultCode = "VK_ERROR_MEMORY_MAP_FAILED";
        break;
    case VK_ERROR_LAYER_NOT_PRESENT:
        resultCode = "VK_ERROR_LAYER_NOT_PRESENT";
        break;
    case VK_ERROR_TOO_MANY_OBJECTS:
        resultCode = "VK_ERROR_TOO_MANY_OBJECTS";
        break;
    case VK_ERROR_FRAGMENTED_POOL:
        resultCode = "VK_ERROR_FRAGMENTED_POOL";
        break;
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        resultCode = "VK_ERROR_FORMAT_NOT_SUPPORTED";
        break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        resultCode = "VK_ERROR_EXTENSION_NOT_PRESENT";
        break;
    case VK_ERROR_FEATURE_NOT_PRESENT:
        resultCode = "VK_ERROR_FEATURE_NOT_PRESENT";
        break;
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        resultCode = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        resultCode = "VK_ERROR_SURFACE_LOST_KHR";
        break;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        resultCode = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        break;
    case VK_SUBOPTIMAL_KHR:
        resultCode = "VK_SUBOPTIMAL_KHR";
        break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        resultCode = "VK_ERROR_OUT_OF_DATE_KHR";
        break;
    case VK_ERROR_VALIDATION_FAILED_EXT:
        resultCode = "VK_ERROR_VALIDATION_FAILED_EXT";
        break;
    case VK_ERROR_OUT_OF_POOL_MEMORY_KHR:
        resultCode = "VK_ERROR_OUT_OF_POOL_MEMORY_KHR";
        break;
    case VK_ERROR_INVALID_SHADER_NV:
        resultCode = "VK_ERROR_INVALID_SHADER_NV";
        break;
    default:
        resultCode = "UNKNOWN";
    }

    LOG_E("{}; Result code : {}", whatFailed, resultCode)
    
    if (throwIfFailed) {
        throw std::runtime_error(whatFailed);
    }
    
    return false;
}

inline std::string buildReportString(const char* sentByLayer, const char* reportType, const char* objectType, std::uint64_t senderObject, size_t location, std::int32_t code, const char* message) {
    return fmt::format("Vulkan validation layer called \"{}\" reported {} in {}"
                       "\n\t\t Object: {}; Location: {}; Message code: {}"
                       "\n\t\t MESSAGE: \n\t\t{}", sentByLayer, reportType, objectType, senderObject, location, code, message);
}

VkBool32 vulkanDebugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT type, std::uint64_t senderObject, size_t location, std::int32_t code, const char* sentByLayer, const char* message, void*) {
    const std::string messageStart = " \"";
    
    const char* layerName;
    if (sentByLayer == nullptr || std::strlen(sentByLayer) == 0) {
        layerName = "UNKNOWN";
    } else {
        layerName = sentByLayer;
    }
    
    const char* objectType;
    switch (type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
        objectType = "an UNKNOWN OBJECT";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
        objectType = "the INSTANCE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
        objectType = "a PHYSICAL DEVICE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
        objectType = "a DEVICE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
        objectType = "a QUEUE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
        objectType = "a SEMAPHORE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
        objectType = "a COMMAND BUFFER";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
        objectType = "a FENCE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
        objectType = "DEVICE MEMORY";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        objectType = "a BUFFER";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        objectType = "an IMAGE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
        objectType = "an EVENT";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
        objectType = "a QUERY POOL";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
        objectType = "a BUFFER VIEW";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
        objectType = "an IMAGE VIEW";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
        objectType = "a SHADER MODULE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
        objectType = "a PIPELINE CACHE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
        objectType = "a PIPELINE LAYOUR";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        objectType = "a RENDER PASS";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
        objectType = "a PIPELINE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
        objectType = "a DESCRIPTOR SET LAYOUT";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
        objectType = "a SAMPLER";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
        objectType = "a DESCRIPTOR POOL";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
        objectType = "a DESCRIPTOR SET";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
        objectType = "a FRAMEBUFFER";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
        objectType = "a COMMAND POOL";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
        objectType = "the SURFACE";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        objectType = "the SWAPCHAIN";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
        objectType = "the DEBUG REPORT";
        break;
    default:
        objectType = "an UNKNOWN OBJECT";
    }
    
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        LOG_E("{}", buildReportString(layerName, "an ERROR", objectType, senderObject, location, code, message));
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        LOG_W("{}", buildReportString(layerName, "a WARNING", objectType, senderObject, location, code, message));
    } else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        LOG_W("{}", buildReportString(layerName, "a PERFORMANCE WARNING", objectType, senderObject, location, code, message));
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        LOG_I("{}", buildReportString(layerName, "INFORMATION", objectType, senderObject, location, code, message));
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        LOG_V("{}", buildReportString(layerName, "DEBUG INFORMATION", objectType, senderObject, location, code, message));// LOG_D will get disabled when compiling in release mode 
    }

//    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT || flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        throw std::runtime_error("Validation layer reported an error");
    }

    return VK_FALSE;
}

}
