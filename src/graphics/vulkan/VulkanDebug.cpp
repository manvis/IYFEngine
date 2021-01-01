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
#include "logging/Logger.hpp"
#include "configuration/Configuration.hpp"
#include "utilities/StringUtilities.hpp"

namespace iyf {
inline const char* ObjectTypeToName(VkObjectType type) {
    switch (type) {
        case VK_OBJECT_TYPE_UNKNOWN:
            return "UNKNOWN";
        case VK_OBJECT_TYPE_INSTANCE:
            return "Instance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "Physical Device";
        case VK_OBJECT_TYPE_DEVICE:
            return "Device";
        case VK_OBJECT_TYPE_QUEUE:
            return "Queue";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "Semaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "Command Buffer";
        case VK_OBJECT_TYPE_FENCE:
            return "Fence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "Device Memory";
        case VK_OBJECT_TYPE_BUFFER:
            return "Buffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "Image";
        case VK_OBJECT_TYPE_EVENT:
            return "Event";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "Query Pool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "Buffer View";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "Image View";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "Shader Module";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "Pipeline Cache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "Pipeline Layout";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "Render Pass";
        case VK_OBJECT_TYPE_PIPELINE:
            return "Pipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "Descriptor Set Layout";
        case VK_OBJECT_TYPE_SAMPLER:
            return "Sampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "Descriptor Pool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "Descriptor Set";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "Framebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "Command Pool";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
            return "Sampler YCBCR Conversion";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
            return "Descriptor Update Template";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "Surface";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "Swapchain";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "Display";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "Display Mode";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "Debug Report Callback";
        case VK_OBJECT_TYPE_OBJECT_TABLE_NVX:
            return "Object Table";
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX:
            return "Indirect Commands Layout";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return "Debug Utils Messenger";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return "Validation Cache";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
            return "Acceleration Structure";
        case VK_OBJECT_TYPE_RANGE_SIZE:
        case VK_OBJECT_TYPE_MAX_ENUM:
            throw std::runtime_error("Invalid Object Type");
    }
    
    return "UNHANDLED";
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    VulkanDebugUserData* debugUserData = static_cast<VulkanDebugUserData*>(userData);
    
    const char* message = callbackData->pMessage;
    const char* messageName = callbackData->pMessageIdName;
    
    std::stringstream ss;
    ss << "Vulkan ";
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        ss << "GENERAL";
    }
    
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        ss << "VALIDATION";
    }
    
    if (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        ss << "PERFORMANCE";
    }
    
    ss << " message\n\t\tNAME: " << messageName << "\n\t\tMSG:  " << message;
    
    if (callbackData->objectCount > 0) {
        ss << "\n\t\tOBJS: ";
        
        for (std::size_t i = 0; i < callbackData->objectCount; ++i) {
            const VkDebugUtilsObjectNameInfoEXT& info = callbackData->pObjects[i];
            
            // nullptr shouldn't happen, but better safe than sorry
            const bool unnamed = (info.pObjectName == nullptr) || (std::strlen(info.pObjectName) == 0);
            
            ss << "\n\t\t\tID:   " << i;
            ss << "\n\t\t\tHND:  0x" << std::hex << info.objectHandle << std::dec;
            ss << "\n\t\t\tNAME: " << (unnamed ? "UNNAMED" : info.pObjectName);
            ss << "\n\t\t\tTYPE: " << ObjectTypeToName(info.objectType);
        }
    }
    
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOG_E("{}", ss.str());
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOG_W("{}", ss.str());
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        LOG_I("{}", ss.str());
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        LOG_V("{}", ss.str()); 
    }

    if (debugUserData->abortOnError &&
        ((severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) || 
         (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT && !(type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)))
       ) {
        throw std::runtime_error("Validation layer reported an error or a non-performance warning");
    }

    return VK_FALSE;
}

void VulkanAPI::createDebugCallback() {
    if (isDebug) {
        createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        destroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        setDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
        
        if (createDebugUtilsMessenger == nullptr || destroyDebugUtilsMessenger == nullptr || setDebugUtilsObjectName == nullptr) {
            LOG_W("Vulkan debug utils messenger is not available on this system. You will not receive Vulkan validation messages.");
            return;
        }
        
        debugUserData.abortOnError = config->getValue("vulkanAbortOnValidationError", ConfigurationValueNamespace::Engine);
        
        LOG_V("Preparing Vulkan debug and validation");
        
        VkDebugUtilsMessengerCreateFlagsEXT severity = 0;
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugVerboseSeverity"), ConfigurationValueNamespace::Engine))) {
            severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugInformationSeverity"), ConfigurationValueNamespace::Engine))) {
            severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugWarningSeverity"), ConfigurationValueNamespace::Engine))) {
            severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugErrorSeverity"), ConfigurationValueNamespace::Engine))) {
            severity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        }
        
        VkDebugUtilsMessageTypeFlagsEXT messageType = 0;
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugGeneralMessages"), ConfigurationValueNamespace::Engine))) {
            messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugValidationMessages"), ConfigurationValueNamespace::Engine))) {
            messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        }
        
        if (config->getValue(ConfigurationValueHandle(HS("vulkanDebugPerformanceMessages"), ConfigurationValueNamespace::Engine))) {
            messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        }
    
        VkDebugUtilsMessengerCreateInfoEXT ci;
        ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        ci.flags = 0;
        ci.pNext = nullptr;
        ci.pUserData = &debugUserData;
        ci.messageSeverity = severity;
        ci.messageType = messageType;
        ci.pfnUserCallback = &vulkanDebugCallback;
        
        createDebugUtilsMessenger(instance, &ci, nullptr, &debugMessenger);
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

}
