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
#include "graphics/vulkan/VulkanDeviceMemoryManager.hpp"
#include "graphics/interfaces/SwapchainChangeListener.hpp"

#include "../VERSION.hpp"

#include "utilities/hashing/Hashing.hpp"
#include "utilities/DataSizes.hpp"
#include "logging/Logger.hpp"
#include "core/Engine.hpp"

#include "fmt/ostream.h"

// WARNING DO NOT TOUCH these undefs. Vulkan headers pull in X11, Project.hpp pulls in RapidJSON,
// names clash, things explode. This solves it.
#undef None
#undef Bool

#include "core/Project.hpp"
#include "utilities/StringUtilities.hpp"

#include <iomanip>

#include <SDL2/SDL_vulkan.h>

namespace iyf {
using namespace iyf::literals;

static const std::vector<const char*> REQUIRED_DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool VulkanAPI::initialize() {
    openWindow();
    printWMInfo();
    
    LOG_V("Starting Vulkan initialization");
    
    createInstance();
    createDebugCallback();
    createSurface();
    choosePhysicalDevice();
    createLogicalDevice();
    
    // TODO make configurable
    std::vector<Bytes> stagingBufferSizes = {
        Mebibytes(16), // MeshAssetData
        Mebibytes(16), // TextureAssetData
        Mebibytes(16), // PerFrameData
        Mebibytes(32),  // Instant
    };
    deviceMemoryManager = new VulkanDeviceMemoryManager(this, stagingBufferSizes);
    deviceMemoryManager->initializeAllocator();
    createVulkanMemoryAllocatorAndHelperBuffers();
    
    vkCreateSwapchain = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(logicalDevice.handle, "vkCreateSwapchainKHR");
    vkDestroySwapchain = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(logicalDevice.handle, "vkDestroySwapchainKHR");
    vkGetSwapchainImages = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(logicalDevice.handle, "vkGetSwapchainImagesKHR");
    vkAcquireNextImage = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(logicalDevice.handle, "vkAcquireNextImageKHR");
    vkQueuePresent = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(logicalDevice.handle, "vkQueuePresentKHR");
    
    if (vkCreateSwapchain == VK_NULL_HANDLE || vkDestroySwapchain == VK_NULL_HANDLE || vkGetSwapchainImages == VK_NULL_HANDLE || vkAcquireNextImage == VK_NULL_HANDLE || vkQueuePresent == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to obtain pointers to swapchain functions");
    }
    
    swapchain.handle = VK_NULL_HANDLE;
    createSwapchain();
    chooseDepthStencilFormat();
    setupCommandPool();
    setupPresentationBarrierCommandBuffers();
    
    frameCompleteFences.resize(getMaxFramesInFlight());
    presentationCompleteSemaphores.resize(getMaxFramesInFlight());
    renderingCompleteSemaphores.resize(getMaxFramesInFlight());
    
    VkFenceCreateInfo fci;
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.pNext = nullptr;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkSemaphoreCreateInfo sci;
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = nullptr;
    sci.flags = 0;
    
    for (std::size_t i = 0; i < getMaxFramesInFlight(); ++i) {
        vkCreateFence(logicalDevice.handle, &fci, nullptr, &frameCompleteFences[i]);
        std::string name = fmt::format("Frame completion fence #{}", i);
        setObjectName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<std::uint64_t>(frameCompleteFences[i]), name.c_str());
        
        vkCreateSemaphore(logicalDevice.handle, &sci, nullptr, &presentationCompleteSemaphores[i]);
        name = fmt::format("Presentaion complete semaphore #{}", i);
        setObjectName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<std::uint64_t>(presentationCompleteSemaphores[i]), name.c_str());
        
        vkCreateSemaphore(logicalDevice.handle, &sci, nullptr, &renderingCompleteSemaphores[i]);
        name = fmt::format("Rendering complete semaphore #{}", i);
        setObjectName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<std::uint64_t>(renderingCompleteSemaphores[i]), name.c_str());
    }
    
    VkPipelineCacheCreateInfo pcci;
    pcci.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pcci.pNext           = nullptr;
    pcci.flags           = 0;
    pcci.initialDataSize = 0;
    pcci.pInitialData    = nullptr;
    
    // TODO load from disk
    checkResult(vkCreatePipelineCache(logicalDevice.handle, &pcci, nullptr, &pipelineCache), "");
    setObjectName(VK_OBJECT_TYPE_PIPELINE_CACHE, reinterpret_cast<std::uint64_t>(pipelineCache), "Main pipeline cache");

    mainCommandBuffer = allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, false);//TODO allocate >1
    imageUploadCommandBuffer = allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1, false);
    
    
    isInit = true;
    
    deviceMemoryManager->initialize();
    
    return true;
}

void VulkanAPI::setupCommandPool() {
    VkResult result;
    
    // TODO Multithreaded with multiple pools?
    VkCommandPoolCreateInfo cpci;
    cpci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.pNext            = nullptr;
    cpci.queueFamilyIndex = physicalDevice.chosenMainQueueFamilyId;
    cpci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // TODO transient?
    
    result = vkCreateCommandPool(logicalDevice.handle, &cpci, nullptr, &commandPool);
    setObjectName(VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<std::uint64_t>(commandPool), "Main command pool");
    
    checkResult(result, "Failed to create a command pool.");
}

void VulkanAPI::findLayers(LayerType type, const std::vector<const char*>& expectedLayers) {
    std::string layerTypeName = (type == LayerType::Instance) ? "instance" : "device";
    
    std::uint32_t layerCount = 0;
    LOG_V("Searching for {} layers", layerTypeName);
    
    VkResult result = VK_SUCCESS;
    
    // Like with most things in Vulkan, we get the counts first...
    if (type == LayerType::Instance) {
        result = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    } else {
        result = vkEnumerateDeviceLayerProperties(physicalDevice.handle, &layerCount, nullptr);
    }
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to get the number of " + layerTypeName + " layers");
    }
    
    ///...then use them to allocate enough memory to copy over the required data
    if (layerCount > 0) {
        std::stringstream ss;
        ss << "Found " << layerCount << " " << layerTypeName << " layer(s)";
        
        std::vector<VkLayerProperties> layerProperties;
        layerProperties.resize(layerCount);
        
        if (type == LayerType::Instance) {
            result = vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());
        } else {
            result = vkEnumerateDeviceLayerProperties(physicalDevice.handle, &layerCount, layerProperties.data());
        }
        
        checkResult(result, "Failed to enumerate " + layerTypeName + " layers.");
        
        for (std::uint32_t i = 0; i < layerCount; ++i) {
            ss << "\n\t    " << std::right << std::setw(3) << i << "." << layerProperties[i].layerName;
        }
        
        LOG_V("{}", ss.str())
        
        ss = std::stringstream();
        ss << "Activated layers:";
        
        for (std::uint32_t i = 0; i < expectedLayers.size(); ++i) {
            bool found = false;
            
            for (std::uint32_t j = 0; j < layerCount; ++j) {
                if (!strcmp(expectedLayers[i], layerProperties[j].layerName)) {
                    ss << "\n\t    " << std::right << std::setw(3) << j << "." << layerProperties[j].layerName;
                    //ss << "\n\t" << layerProperties[j].description;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // Throw if we're missing any layers we need
                std::stringstream es;
                es << "Failed to find a mandatory " << layerTypeName << " layer: " << expectedLayers[i];
                LOG_E("{}", es.str())
                throw std::runtime_error(es.str());
            }
        }
        
        LOG_V("{}", ss.str())
    } else {
        // Throw if debug is enabled but no layers were found
        std::string resStr = "No " + layerTypeName + " layers were found.";
        LOG_E("{}", resStr)
        throw std::runtime_error(resStr);
    }
}

void VulkanAPI::createInstance() {
    // Simplest things first. We create a struct with data describing our engine and application
    std::string appName = config->getValue(ConfigurationValueHandle(HS("applicationName"), ConfigurationValueNamespace::Engine));
    std::string engineName = config->getValue(ConfigurationValueHandle(HS("engineName"), ConfigurationValueNamespace::Engine));
    
    VkApplicationInfo ai;
    ai.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pNext              = nullptr;
    ai.pApplicationName   = appName.c_str();
    ai.applicationVersion = engine->getProject()->getVersion().getPackedVersion();
    ai.pEngineName        = engineName.c_str();
    ai.engineVersion      = con::EngineVersion.getPackedVersion();
    ai.apiVersion         = VK_API_VERSION_1_1; // According to spec, patch number supplied here is ignored, so we should use VK_API_VERSION_ defines here
    
    std::vector<const char*> extensionsToEnable;
    // If we're in debug mode, we need to set up the debug and validation layers.
    // The names of the files we want are fetched from the config file.
    if (isDebug) {
        std::string layerNamesCfg = config->getValue(HS("validationLayers"), ConfigurationValueNamespace::Engine);

        const auto splitResult = util::splitString(layerNamesCfg, ",");

        layerNamesSplit.reserve(splitResult.size());
        for (const auto& name : splitResult)
        {
            layerNamesSplit.emplace_back(name);
        }
        
        validationLayerNames.reserve(layerNamesSplit.size());

        // Vulkan needs const char* 
        for (const auto& s : layerNamesSplit) {
            validationLayerNames.push_back(s.c_str());
        }

        findLayers(LayerType::Instance, validationLayerNames);
        extensionsToEnable.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    // Any extra extensions should go here
    
    unsigned int SDLInstanceExtensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &SDLInstanceExtensionCount, nullptr)) {
        throw std::runtime_error("SDL failed to enumerate Vulkan surface extensions.");
    }
    
    std::size_t extraExtensionCount = extensionsToEnable.size();
    extensionsToEnable.resize(extraExtensionCount + SDLInstanceExtensionCount);
    
    if (!SDL_Vulkan_GetInstanceExtensions(window, &SDLInstanceExtensionCount, extensionsToEnable.data() + extraExtensionCount)) {
        throw std::runtime_error("SDL failed to fetch the names of Vulkan surface extensions.");
    }
    
    if (isDebug) {
        std::stringstream ssNames;
        ssNames << "Enabling these Vulkan instance extensions: ";
        
        for (const char* name : extensionsToEnable) {
            ssNames << "\n\t\t" << name;
        }
        
        LOG_D("{}", ssNames.str());
    }
    
    // Assemble all the data into a single struct and use it to create the Vulkan instance object
    VkInstanceCreateInfo ici;
    ici.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ici.pNext                   = nullptr;
    ici.flags                   = 0;
    ici.pApplicationInfo        = &ai;
    if (!isDebug) {
        ici.enabledLayerCount   = 0;
        ici.ppEnabledLayerNames = nullptr;
    } else {
        ici.enabledLayerCount   = static_cast<std::uint32_t>(validationLayerNames.size());
        ici.ppEnabledLayerNames = validationLayerNames.data();
    }
    ici.enabledExtensionCount   = static_cast<std::uint32_t>(extensionsToEnable.size());
    ici.ppEnabledExtensionNames = extensionsToEnable.data();

    checkResult(vkCreateInstance(&ici, nullptr, &instance), "Failed to create the Vulkan instance.");
}

void VulkanAPI::choosePhysicalDevice() {
    LOG_V("Searching for physical devices");
    
    std::uint32_t gpuCount = 0;
    checkResult(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr), "Failed to obtain a physical device count.");

    if (gpuCount == 0) {
        throw std::runtime_error("No physical devices were found");
    } else {
        LOG_V("Found {} physical device(s).", gpuCount);
    }
    
    std::vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(gpuCount);
    
    checkResult(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()), "Failed to enumerate physical devices.");
    
    std::vector<PhysicalDevice> compatibleDevices;
    compatibleDevices.reserve(gpuCount);
    
    // Get capabilities of each available GPU
    for (std::size_t i = 0; i < gpuCount; ++i) {
        LOG_V("Physical device ", i);
        
        PhysicalDevice currentDevice;
        
        currentDevice.handle = physicalDevices[i];
        
        // Flag to indicate if this gpu is compatible and should be added to compatibleDevices 
        bool compatibleGPU = true;
        
        // Evaluate general compatibility requirements (e.g. Vulkan version) and limits
        if (!evaluatePhysicalDeviceProperties(currentDevice)) {
            compatibleGPU = false;
        }
        
        // Evaluate GPU memory compatibility requirements (e.g. minimum size of device memory required by the engine)
        if (!evaluatePhysicalDeviceMemoryProperties(currentDevice)) {
            compatibleGPU = false;
        }
        
        // Fetch and evaluate available gpu extensions
        if (!evaluatePhysicalDeviceExtensions(currentDevice)) {
            compatibleGPU = false;
        }
        
        // Evaluate feature availability
        if (!evaluatePhysicalDeviceFeatures(currentDevice)) {
            compatibleGPU = false;
        }
        
        // Evaluate queue families
        if (!evaluatePhysicalDeviceQueueFamilies(currentDevice)) {
            compatibleGPU = false;
        }
        
        // Evaluate surface capabilities
        if (!evaluatePhysicalDeviceSurfaceCapabilities(currentDevice)) {
            compatibleGPU = false;
        }
        
        if (compatibleGPU) {
            compatibleDevices.push_back(std::move(currentDevice));
        } else {
            LOG_V("Physical device {} is not compatible", i);
        }
    }
    
    // TODO allow the user to choose if more than one is compatible, OR(AND?) somehow determine which one is better
    if (compatibleDevices.size() == 0) {
        throw std::runtime_error("No compatible devices were found.");
    }
    
    physicalDevice = std::move(compatibleDevices[0]);
    
    LOG_V("Chose {} as the physical device", physicalDevice.properties.deviceName)
}

static void checkFeature(std::stringstream& ss, const char* name, VkBool32& available, bool required, bool& allAvailable) {
    ss << "\n\t\t" << std::left << std::setw(45) << name
                   << "available: " << ((available == VK_TRUE) ? "Y" : "N")
                   << " required: " << ((required == VK_TRUE) ? "Y" : "N");
    
    // Disable all features that we don't need
    if (available && !required) {
        available = false;
    } else if (!available && required) {
        allAvailable = false;
    }
}
bool VulkanAPI::evaluatePhysicalDeviceFeatures(PhysicalDevice& device) {
    vkGetPhysicalDeviceFeatures(device.handle, &device.features);
    VkPhysicalDeviceFeatures& features = device.features;
    
    bool allAvailable = true;
    
    std::stringstream ss;
    ss << "Available features:";
    
//    checkFeature("", features., false, allAvailable);
    checkFeature(ss, "robustBufferAccess",                      features.robustBufferAccess,                      false, allAvailable);
    checkFeature(ss, "fullDrawIndexUint32",                     features.fullDrawIndexUint32,                     false, allAvailable);
    checkFeature(ss, "imageCubeArray",                          features.imageCubeArray,                          false, allAvailable);
    checkFeature(ss, "independentBlend",                        features.independentBlend,                        false, allAvailable);
    checkFeature(ss, "geometryShader",                          features.geometryShader,                          false, allAvailable);
    checkFeature(ss, "tessellationShader",                      features.tessellationShader,                      false, allAvailable);
    checkFeature(ss, "sampleRateShading",                       features.sampleRateShading,                       false, allAvailable);
    checkFeature(ss, "dualSrcBlend",                            features.dualSrcBlend,                            false, allAvailable);
    checkFeature(ss, "logicOp",                                 features.logicOp,                                 false, allAvailable);
    checkFeature(ss, "multiDrawIndirect",                       features.multiDrawIndirect,                       false, allAvailable);
    checkFeature(ss, "drawIndirectFirstInstance",               features.drawIndirectFirstInstance,               false, allAvailable);
    checkFeature(ss, "depthClamp",                              features.depthClamp,                              false, allAvailable);
    checkFeature(ss, "depthBiasClamp",                          features.depthBiasClamp,                          false, allAvailable);
    // Non solid (wireframe) draw is used by debug features
    checkFeature(ss, "fillModeNonSolid",                        features.fillModeNonSolid,                        true,  allAvailable);
    checkFeature(ss, "depthBounds",                             features.depthBounds,                             false, allAvailable);
    // Wide lines are used in debug drawing
    checkFeature(ss, "wideLines",                               features.wideLines,                               true,  allAvailable);
    // Large points are used in debug drawing
    checkFeature(ss, "largePoints",                             features.largePoints,                             true,  allAvailable);
    checkFeature(ss, "alphaToOne",                              features.alphaToOne,                              false, allAvailable);
    checkFeature(ss, "multiViewport",                           features.multiViewport,                           false, allAvailable);
    // Anisotropy is important in making our textures look good
    checkFeature(ss, "samplerAnisotropy",                       features.samplerAnisotropy,                       true,  allAvailable);
    checkFeature(ss, "textureCompressionETC2",                  features.textureCompressionETC2,                  false, allAvailable);
    checkFeature(ss, "textureCompressionASTC_LDR",              features.textureCompressionASTC_LDR,              false, allAvailable);
    // We only accept block compressed textures for now
    checkFeature(ss, "textureCompressionBC",                    features.textureCompressionBC,                    true,  allAvailable);
    checkFeature(ss, "occlusionQueryPrecise",                   features.occlusionQueryPrecise,                   false, allAvailable);
    checkFeature(ss, "pipelineStatisticsQuery",                 features.pipelineStatisticsQuery,                 false, allAvailable);
    checkFeature(ss, "vertexPipelineStoresAndAtomics",          features.vertexPipelineStoresAndAtomics,          false, allAvailable);
    checkFeature(ss, "fragmentStoresAndAtomics",                features.fragmentStoresAndAtomics,                false, allAvailable);
    checkFeature(ss, "shaderTessellationAndGeometryPointSize",  features.shaderTessellationAndGeometryPointSize,  false, allAvailable);
    checkFeature(ss, "shaderImageGatherExtended",               features.shaderImageGatherExtended,               false, allAvailable);
    checkFeature(ss, "shaderStorageImageExtendedFormats",       features.shaderStorageImageExtendedFormats,       false, allAvailable);
    checkFeature(ss, "shaderStorageImageMultisample",           features.shaderStorageImageMultisample,           false, allAvailable);
    checkFeature(ss, "shaderStorageImageReadWithoutFormat",     features.shaderStorageImageReadWithoutFormat,     false, allAvailable);
    checkFeature(ss, "shaderStorageImageWriteWithoutFormat",    features.shaderStorageImageWriteWithoutFormat,    false, allAvailable);
    checkFeature(ss, "shaderUniformBufferArrayDynamicIndexing", features.shaderUniformBufferArrayDynamicIndexing, false, allAvailable);
    checkFeature(ss, "shaderSampledImageArrayDynamicIndexing",  features.shaderSampledImageArrayDynamicIndexing,  false, allAvailable);
    checkFeature(ss, "shaderStorageBufferArrayDynamicIndexing", features.shaderStorageBufferArrayDynamicIndexing, false, allAvailable);
    checkFeature(ss, "shaderStorageImageArrayDynamicIndexing",  features.shaderStorageImageArrayDynamicIndexing,  false, allAvailable);
    checkFeature(ss, "shaderClipDistance",                      features.shaderClipDistance,                      false, allAvailable);
    checkFeature(ss, "shaderCullDistance",                      features.shaderCullDistance,                      false, allAvailable);
    checkFeature(ss, "shaderFloat64",                           features.shaderFloat64,                           false, allAvailable);
    checkFeature(ss, "shaderInt64",                             features.shaderInt64,                             false, allAvailable);
    checkFeature(ss, "shaderInt16",                             features.shaderInt16,                             false, allAvailable);
    checkFeature(ss, "shaderResourceResidency",                 features.shaderResourceResidency,                 false, allAvailable);
    checkFeature(ss, "shaderResourceMinLod",                    features.shaderResourceMinLod,                    false, allAvailable);
    checkFeature(ss, "sparseBinding",                           features.sparseBinding,                           false, allAvailable);
    checkFeature(ss, "sparseResidencyBuffer",                   features.sparseResidencyBuffer,                   false, allAvailable);
    checkFeature(ss, "sparseResidencyImage2D",                  features.sparseResidencyImage2D,                  false, allAvailable);
    checkFeature(ss, "sparseResidencyImage3D",                  features.sparseResidencyImage3D,                  false, allAvailable);
    checkFeature(ss, "sparseResidency2Samples",                 features.sparseResidency2Samples,                 false, allAvailable);
    checkFeature(ss, "sparseResidency4Samples",                 features.sparseResidency4Samples,                 false, allAvailable);
    checkFeature(ss, "sparseResidency8Samples",                 features.sparseResidency8Samples,                 false, allAvailable);
    checkFeature(ss, "sparseResidency16Samples",                features.sparseResidency16Samples,                false, allAvailable);
    checkFeature(ss, "sparseResidencyAliased",                  features.sparseResidencyAliased,                  false, allAvailable);
    checkFeature(ss, "variableMultisampleRate",                 features.variableMultisampleRate,                 false, allAvailable);
    checkFeature(ss, "inheritedQueries",                        features.inheritedQueries,                        false, allAvailable);
    
    LOG_V("{}", ss.str())
    
    return allAvailable;
}

bool VulkanAPI::evaluatePhysicalDeviceMemoryProperties(PhysicalDevice& device) {
    vkGetPhysicalDeviceMemoryProperties(device.handle, &device.memoryProperties);
    
    std::uint32_t memoryHeaps = device.memoryProperties.memoryHeapCount;
    std::uint32_t memoryTypes = device.memoryProperties.memoryTypeCount;

    std::stringstream ss;
    ss << "Memory heaps: ";
    for (std::uint32_t i = 0; i < memoryHeaps; ++i) {
        const auto& heap = device.memoryProperties.memoryHeaps[i];

        ss << "\n\t\tHeap ID: " << i;
        ss << "\n\t\t\tHeap size: " << heap.size << " " << MiB(Bytes(heap.size)).count();
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            ss << "\n\t\t\tHeap in device memory";
        }
    }

    ss << "\n\tMemory properties:";

    for (std::uint32_t i = 0; i < memoryTypes; ++i) {
        const auto& type = device.memoryProperties.memoryTypes[i];

        ss << "\n\t\tMemory type: " << i;
        ss << "\n\t\t\tHeap id: " << type.heapIndex;
        ss << "\n\t\t\tMemory flags:";

        VkMemoryPropertyFlags flag = type.propertyFlags;

        if (flag == 0) {
            ss << "\n\t\t\t\tUnspeficied";
        } else {
            if (flag & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                ss << "\n\t\t\t\tDevice local";
            }

            if (flag & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                ss << "\n\t\t\t\tHost visible";
            }

            if (flag & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
                ss << "\n\t\t\t\tHost coherent";
            }

            if (flag & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
                ss << "\n\t\t\t\tHost cached";
            }

            if (flag & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                ss << "\n\t\t\t\tLazily allocated";
            }
        }
    }

    LOG_V("{}", ss.str())
    
    return true;
}

bool VulkanAPI::evaluatePhysicalDeviceProperties(PhysicalDevice& device) {
    vkGetPhysicalDeviceProperties(device.handle, &(device.properties));
    
    std::string typeName;
    switch (device.properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            typeName = "Integrated GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            typeName = "Discrete GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            typeName = "Virtual GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            typeName = "CPU";
            break;
        default:
            typeName = "Other";
    }

    std::uint32_t driverVersion = device.properties.driverVersion;
    std::uint32_t version = device.properties.apiVersion;

    std::stringstream ss;
    ss << "Physical device properties" << "\n\t\t" << "Vendor id: " << device.properties.vendorID 
                                       << "\n\t\t" << "Device name (id): " << device.properties.deviceName << " (" << device.properties.deviceID << ")"
                                       << "\n\t\t" << "Device type: " << typeName
                                       << "\n\t\t" << "Driver version: " << VK_VERSION_MAJOR(driverVersion) << "." << VK_VERSION_MINOR(driverVersion) << "." << VK_VERSION_PATCH(driverVersion)
                                       << "\n\t\t" << "API version: " << VK_VERSION_MAJOR(version) << "." << VK_VERSION_MINOR(version) << "." << VK_VERSION_PATCH(version);
    LOG_V("{}", ss.str());
    
    std::stringstream ss2;
    // TODO print and evaluate all limits based on the needs of the engine
    ss2 << "Physical device limits"
            << "\n\t\tMaxSamplerAllocationCount " << device.properties.limits.maxSamplerAllocationCount
            << "\n\t\tMaxVertexInputBindings " << device.properties.limits.maxVertexInputBindings
            << "\n\t\tMaxComputeSharedMemorySize " << device.properties.limits.maxComputeSharedMemorySize
            << "\n\t\tMaxComputeWorkGroupInvocations " << device.properties.limits.maxComputeWorkGroupInvocations
            << "\n\t\tMaxDescriptorSetStorageBuffers " << device.properties.limits.maxDescriptorSetStorageBuffers
            << "\n\t\tMaxMemoryAllocationCount " << device.properties.limits.maxMemoryAllocationCount;
    LOG_V("{}", ss2.str())
    
    return true;
}

bool VulkanAPI::evaluatePhysicalDeviceExtensions(PhysicalDevice& device) {
    std::uint32_t deviceExtensionCount = 0;
    checkResult(vkEnumerateDeviceExtensionProperties(device.handle, nullptr, &deviceExtensionCount, nullptr), "Failed to obtain physical device extension count.");

    if (deviceExtensionCount > 0) {
        device.extensionProperties.resize(deviceExtensionCount);

        checkResult(vkEnumerateDeviceExtensionProperties(device.handle, nullptr, &deviceExtensionCount, device.extensionProperties.data()), "Failed to enumerate physical device extensions.");
    } else {
        LOG_V("No physical device extensions were found.");

        return false;
    }

    // Listing all extensions that the device can provide
    std::stringstream ss;
    for (std::uint32_t e = 0; e < device.extensionProperties.size(); e++) {
        ss << "\n\t\t" << device.extensionProperties[e].extensionName;
    }

    LOG_V("Physical device extensions: {}", ss.str());

    // Checking to see if all required extensions are present. Unlike layers that can be changed via config, the names of these extensions
    // are hardcoded because their presence is mandatory for the engine to function
    for (std::uint32_t e = 0; e < REQUIRED_DEVICE_EXTENSIONS.size(); ++e) {
        bool found = false;

        for (std::uint32_t j = 0; j < device.extensionProperties.size(); ++j) {
            if (!strcmp(REQUIRED_DEVICE_EXTENSIONS[e], device.extensionProperties[j].extensionName)) {
                found = true;
                device.enabledExtensions.push_back(device.extensionProperties[j].extensionName);
                break;
            }
        }

        if (!found) {
            LOG_V("Physical device is missing a required extension: {}", REQUIRED_DEVICE_EXTENSIONS[e]);
            
            return false;
        }
    }
    
    device.dedicatedAllocationExtensionEnabled = false;
    device.getMemoryRequirements2ExtensionEnabled = false;
    
    bool dedicatedAllocationExtensionFound = false;
    bool getMemoryRequirements2ExtensionFound = false;
    
    // Find potentially useful optional extensions
    for (std::uint32_t e = 0; e < device.extensionProperties.size(); ++e) {
        if (!strcmp(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, device.extensionProperties[e].extensionName)) {
            dedicatedAllocationExtensionFound = true;
        }
        
        if (!strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, device.extensionProperties[e].extensionName)) {
            getMemoryRequirements2ExtensionFound = true;
        }
    }
    
    if (getMemoryRequirements2ExtensionFound) {
        device.getMemoryRequirements2ExtensionEnabled = true;
        device.enabledExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    }
    
    if (dedicatedAllocationExtensionFound && getMemoryRequirements2ExtensionFound) {
        device.dedicatedAllocationExtensionEnabled = true;
        device.enabledExtensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);
    }

    return true;
}

void VulkanAPI::createSurface() {
    if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) {
        throw std::runtime_error("SDL failed to create a Vulkan render surface");
    }
}

bool VulkanAPI::evaluatePhysicalDeviceQueueFamilies(PhysicalDevice& device) {
    LOG_V("Searching for a physical device queue family.");
    
    std::uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &queueFamilyCount, nullptr);
    
    if (queueFamilyCount == 0) {
        LOG_V("No suitable queue families were found");
        return false;
    }

    device.queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &queueFamilyCount, device.queueFamilyProperties.data());
    
    LOG_V("Found {} queue families.", queueFamilyCount);

    // When selecting queue families, we need to keep a few things from Vulkan spec (1.0.42) chapter 4.1 in mind:
    //     if graphics operations are supported, then at least one queue family must support both graphics and compute operations
    // AND
    //     if graphics or compute operations are supported by a queue family, then the support of transfer operations is implicit and
    // MAY NOT BE REPORTED
    //
    // What this means:
    // 1. There's always a generic queue (I call it "main") that supports graphics + compute + transfer
    // 2. Dedicated graphics (no compute bits set) or compute (no graphics bits set) queues may or may not have transfer bits set
    // 3. Dedicated transfer queues won't have graphics nor compute set
    //
    // Another thing to keep in mind is that a presentation capable queue may not match
    std::uint32_t mainQueueID = std::numeric_limits<std::uint32_t>::max(); // std::numeric_limits<>::max() is definitely safe as an "empty" value, since there can't possibly be that many queues
    std::uint32_t dedicatedComputeID = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t dedicatedTransferID = std::numeric_limits<std::uint32_t>::max();
    
    device.presentCapableQueues.reserve(queueFamilyCount);
    
    for (std::uint32_t i = 0; i < queueFamilyCount; ++i) {
        bool hasGraphics = false;
        bool hasCompute = false;
        bool hasTransfer = false;
        // bool hasSparse = false; // We don't care about sparse binding support... YET
        
        std::stringstream ss;
        ss << "Queue family " << i << " contains " << device.queueFamilyProperties[i].queueCount << " queues";
        ss << "\n\t\t" << "Number of valid timestamp bits: " << device.queueFamilyProperties[i].timestampValidBits;
        ss << "\n\t\t" << "Minimal transfer granularity: ";
        ss << "\n\t\t\tW:"<< device.queueFamilyProperties[i].minImageTransferGranularity.width;
        ss << "\n\t\t\tH:"<< device.queueFamilyProperties[i].minImageTransferGranularity.height;
        ss << "\n\t\t\tD:"<< device.queueFamilyProperties[i].minImageTransferGranularity.depth;
        
        if (device.queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            ss << "\n\t\tSupports graphics";
            hasGraphics = true;
        }
        
        if (device.queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            ss << "\n\t\tSupports compute";
            hasCompute = true;
        }
        
        if (device.queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            hasTransfer = true;
            ss << "\n\t\tSupports transfer";
        }
        
        if (device.queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            // hasSparse = true;
            ss << "\n\t\tSupports sparse binds";
        }
        
        VkBool32 canPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device.handle, i, surface, &canPresent);
        
        if (canPresent == VK_TRUE) {
            device.presentCapableQueues.push_back(i);
            ss << "\n\t\tCan present";
        }
        
        LOG_V("{}", ss.str())
        
        if (mainQueueID == std::numeric_limits<std::uint32_t>::max() && hasGraphics && hasCompute) {
            mainQueueID = i;
        }
        
        if (dedicatedComputeID == std::numeric_limits<std::uint32_t>::max() && !hasGraphics && hasCompute) {
            dedicatedComputeID = i;
        }
        
        if (dedicatedTransferID == std::numeric_limits<std::uint32_t>::max() && hasTransfer && !hasGraphics && !hasCompute) {
            dedicatedTransferID = i;
        }
        // If we've already found what we need, just print the rest
        // TODO we're currently looking for a single queue that does everything. I should consider using separate graphics, compute and transfer queues (if available)
//        if (!queueFound && hasGraphics && hasCompute && (canPresent == VK_TRUE)) {
//            queueFound = true;
//            chosenQueueFamilyId = i;
//        }
    }
    
    if (mainQueueID == std::numeric_limits<std::uint32_t>::max() || device.presentCapableQueues.empty()) {
        LOG_V("Required queue not found");
        return false;
    }
    
    // TODO start using separate queues if they are available AND if I can get better performance by doing so. Simply 
    // changing the values here won't do since EVERYTHING ELSE only uses chosenMainQueueFamilyId
    device.usingDedicatedComputeQueueFamily = false;
    device.usingDedicatedTransferQueueFamily = false;
    
    device.chosenMainQueueFamilyId = mainQueueID;
    device.chosenComputeQueueFamilyId = device.chosenMainQueueFamilyId;
    device.chosenTransferQueueFamilyId = device.chosenComputeQueueFamilyId;
    
    // Check if the present capable queue is separate from the main one.
    device.presentQueueFamilySeparateFromMain = true;
    for (std::uint32_t id : device.presentCapableQueues) {
        if (id == device.chosenMainQueueFamilyId) {
            device.presentQueueFamilySeparateFromMain = false;
            device.chosenPresentQueueFamilyId = id;
            break;
        }
    }
    
    // If the presentation queue(s) really is(are) separate, pick the first one from the list
    if (device.presentQueueFamilySeparateFromMain) {
        device.chosenPresentQueueFamilyId = device.presentCapableQueues[0];
    }
    
    LOG_V("Chosen queue family id: {}", device.chosenMainQueueFamilyId);
    
    return true;
}

void VulkanAPI::createLogicalDevice() {
    // Device layers are deprecated, however, the Vulkan Spec (version 1.0.42) says that
    //    device layers should be enumerated and enabled device layers to maximize compatibility
    // AND
    //     that list of layers returned by vkEnumerateDeviceLayerProperties must match those enabled
    // for the instance
    
    if (isDebug) {
        findLayers(LayerType::Device, validationLayerNames);
    }
    
    std::vector<VkDeviceQueueCreateInfo> qcis;
    
    float queuePriorities[1] = {1.0};
    VkDeviceQueueCreateInfo qci;
    qci.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.pNext            = nullptr;
    qci.flags            = 0;
    qci.queueFamilyIndex = physicalDevice.chosenMainQueueFamilyId;
    qci.queueCount       = 1;
    qci.pQueuePriorities = queuePriorities;
    
    qcis.push_back(qci);
    
    if (physicalDevice.usingDedicatedComputeQueueFamily) {
        float computeQueuePriorities[1] = {1.0};
        
        VkDeviceQueueCreateInfo qciCompute;
        qciCompute.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciCompute.pNext            = nullptr;
        qciCompute.flags            = 0;
        qciCompute.queueFamilyIndex = physicalDevice.chosenComputeQueueFamilyId;
        qciCompute.queueCount       = 1;
        qciCompute.pQueuePriorities = computeQueuePriorities;

        qcis.push_back(qciCompute);
    }
    
    if (physicalDevice.usingDedicatedTransferQueueFamily) {
        float transferQueuePriorities[1] = {1.0};
        
        VkDeviceQueueCreateInfo qciTransfer;
        qciTransfer.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciTransfer.pNext            = nullptr;
        qciTransfer.flags            = 0;
        qciTransfer.queueFamilyIndex = physicalDevice.chosenTransferQueueFamilyId;
        qciTransfer.queueCount       = 1;
        qciTransfer.pQueuePriorities = transferQueuePriorities;

        qcis.push_back(qciTransfer);
    }
    
    if (physicalDevice.presentQueueFamilySeparateFromMain) {
        float presentQueuePriorities[1] = {1.0};
        
        VkDeviceQueueCreateInfo qciPresent;
        qciPresent.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qciPresent.pNext            = nullptr;
        qciPresent.flags            = 0;
        qciPresent.queueFamilyIndex = physicalDevice.chosenPresentQueueFamilyId;
        qciPresent.queueCount       = 1;
        qciPresent.pQueuePriorities = presentQueuePriorities;

        qcis.push_back(qciPresent);
    }
    
//    assert(physicalDevice.features.textureCompressionBC == VK_TRUE && physicalDevice.features.multiViewport == VK_FALSE && physicalDevice.features.largePoints == VK_TRUE);

    VkDeviceCreateInfo dci;
    dci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.pNext                   = nullptr;
    dci.flags                   = 0;
    dci.queueCreateInfoCount    = static_cast<std::uint32_t>(qcis.size());
    dci.pQueueCreateInfos       = qcis.data();
    if (!isDebug) {
        dci.enabledLayerCount   = 0;
        dci.ppEnabledLayerNames = nullptr;
    } else {
        dci.enabledLayerCount   = static_cast<std::uint32_t>(validationLayerNames.size());
        dci.ppEnabledLayerNames = validationLayerNames.data();
    }
    dci.enabledExtensionCount   = physicalDevice.enabledExtensions.size();
    dci.ppEnabledExtensionNames = physicalDevice.enabledExtensions.data();
    dci.pEnabledFeatures        = &physicalDevice.features;
    
    checkResult(vkCreateDevice(physicalDevice.handle, &dci, nullptr, &logicalDevice.handle), "Failed to create a device.");
    
    vkGetDeviceQueue(logicalDevice.handle, physicalDevice.chosenMainQueueFamilyId, 0, &logicalDevice.mainQueue);
    
    // TODO Implement multiple queue use. At the moment, NOTHING else is implemented. All other methods only use the 'queue' queue
    if (physicalDevice.usingDedicatedComputeQueueFamily) {
        vkGetDeviceQueue(logicalDevice.handle, physicalDevice.chosenComputeQueueFamilyId, 0, &logicalDevice.computeQueue);
    }
    
    if (physicalDevice.usingDedicatedTransferQueueFamily) {
        vkGetDeviceQueue(logicalDevice.handle, physicalDevice.chosenTransferQueueFamilyId, 0, &logicalDevice.transferQueue);
    }
    
    if (physicalDevice.presentQueueFamilySeparateFromMain) {
        vkGetDeviceQueue(logicalDevice.handle, physicalDevice.chosenPresentQueueFamilyId, 0, &logicalDevice.presentQueue);
    }
}

void VulkanAPI::createVulkanMemoryAllocatorAndHelperBuffers() {
    
}

bool VulkanAPI::evaluatePhysicalDeviceSurfaceCapabilities(PhysicalDevice& device) {
    std::uint32_t presentModeCount;
    checkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(device.handle, surface, &presentModeCount, nullptr), "Failed to opbtain surface present mode count.");
    
    if (presentModeCount == 0) {
        return false;
    }
    
    device.presentModes.resize(presentModeCount);    
    checkResult(vkGetPhysicalDeviceSurfacePresentModesKHR(device.handle, surface, &presentModeCount, device.presentModes.data()), "Failed to enumerate surface present modes.");
    
    std::uint32_t surfaceFormatCount = 0;
    checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(device.handle, surface, &surfaceFormatCount, VK_NULL_HANDLE), "Failed to obtain surface format count.");
    
    if (surfaceFormatCount == 0) {
        return false;
    }
    
    device.surfaceFormats.resize(surfaceFormatCount);
    checkResult(vkGetPhysicalDeviceSurfaceFormatsKHR(device.handle, surface, &surfaceFormatCount, device.surfaceFormats.data()), "Failed to enumerate surface formats.");
    
    checkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.handle, surface, &device.surfaceCapabilities), "Failed to obtain surface capabilities.");
    
    return true;
}

VkSurfaceFormatKHR VulkanAPI::chooseSwapchainImageFormat() {
    // Undefined means "take whichever you want", so we take sRGB
    if (physicalDevice.surfaceFormats.size() == 1 && physicalDevice.surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
        VkSurfaceFormatKHR format;
        format.format     = VK_FORMAT_B8G8R8A8_SRGB;
        format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        
        return format;
    } else if (physicalDevice.surfaceFormats.size() >= 1) {
        std::stringstream ss;
        
        for (std::uint32_t i = 0; i < physicalDevice.surfaceFormats.size(); ++i) {
            ss << "\n\t\t" << getFormatName(Format(physicalDevice.surfaceFormats[i].format));
        }

        LOG_V("Supported surface formats: {}", ss.str());
        
        // Looking for an sRGB format
        bool sRGBFound = false;
        std::uint32_t sRGBBufferId = 0;
        
        for (std::uint32_t i = 0; i < physicalDevice.surfaceFormats.size(); ++i) {
            if ((physicalDevice.surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB  ||
                 physicalDevice.surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB) &&
                 physicalDevice.surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                sRGBBufferId = i;
                sRGBFound = true;
                
                break;
            }
        }
        
        VkSurfaceFormatKHR finalFormat;
        
        if (sRGBFound) {
            finalFormat.format     = physicalDevice.surfaceFormats[sRGBBufferId].format;
            finalFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        } else {
            // TODO figure out a way to handle this OR at very least make a mapping for surfaceFormatEngine
            // Failed to find an sRGB format.
            throw std::runtime_error("No sRGB surface formats were found.");
        }
        
        return finalFormat;
    } else {
        throw std::runtime_error("No sRGB surface formats were found.");
    }
}

VkPresentModeKHR VulkanAPI::chooseSwapchainPresentMode() {
    swapchain.mailboxAvailable     = false;
    swapchain.immediateAvailable   = false;
    swapchain.fifoAvailable        = false;
    swapchain.fifoRelaxedAvailable = false;
    
    // TODO what about these:
//     VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR
//     VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
    
    for (std::uint32_t i = 0; i < physicalDevice.presentModes.size(); ++i) {
        if (physicalDevice.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchain.mailboxAvailable = true;
        } else if (physicalDevice.presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            swapchain.fifoAvailable = true;
        } else if (physicalDevice.presentModes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR) {
            swapchain.fifoRelaxedAvailable = true;
        } else if (physicalDevice.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            swapchain.immediateAvailable = true;
        }
    }
    
    std::stringstream ss;
    ss << "Available present modes: ";
    
    if (swapchain.mailboxAvailable) {
        ss << "\n\t\tmailbox";
    }
    
    if (swapchain.immediateAvailable) {
        ss << "\n\t\timmediate";
    }
    
    if (swapchain.fifoAvailable) {
        ss << "\n\t\tfifo";
    }
    
    if (swapchain.fifoRelaxedAvailable) {
        ss << "\n\t\tfifoRelaxed";
    }
    
    LOG_V("{}", ss.str());
    
    // TODO Make this changeable via CONFIGURATION
    
    VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    if (swapchain.mailboxAvailable) {
        chosenPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    } else if (swapchain.fifoAvailable) {
        chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    } else if (swapchain.fifoRelaxedAvailable) {
        chosenPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    } else {
        chosenPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    
    ss = std::stringstream();
    ss << "Chosen present mode: ";

    switch (chosenPresentMode) {
    case VK_PRESENT_MODE_FIFO_KHR:
        ss << "\"FIFO\"";
        break;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        ss << "\"mailbox\"";
        break;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        ss << "\"immediate\"";
        break;
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        ss << "\"FIFO relaxed\"";
        break;
    default:
        ss << "Unknown";
    }

    LOG_V("{}", ss.str())
    
    return chosenPresentMode;
}

void VulkanAPI::recreateSwapchain() {
    vkDeviceWaitIdle(logicalDevice.handle);
    
    createSwapchain();
    setupPresentationBarrierCommandBuffers();
}

void VulkanAPI::createSwapchain() {
    const bool recreatingSwapchain = (swapchain.handle != VK_NULL_HANDLE);
    Swapchain oldSwapchain = recreatingSwapchain ? swapchain : Swapchain();
    
    if (recreatingSwapchain) {
        LOG_V("Recreating swapchain");
        
        // We need to re-evaluate physical device surface capabilities to update required data structs
        if (!evaluatePhysicalDeviceSurfaceCapabilities(physicalDevice)) {
            // This should NEVER happen
            throw std::runtime_error("Unexpected incompatible change in surface capabilities");
        }
    }
    
    VkSurfaceFormatKHR format    = chooseSwapchainImageFormat();
    VkPresentModeKHR presentMode = chooseSwapchainPresentMode();
    
    const VkSurfaceCapabilitiesKHR& capabilities = physicalDevice.surfaceCapabilities;

    if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
        surfaceFormatEngine = Format::B8_G8_R8_A8_sRGB;
    } else {
        surfaceFormatEngine = Format::R8_G8_B8_A8_sRGB;
    }
    
    swapchain.imageFormat     = format.format;
    swapchain.imageColorSpace = format.colorSpace;
    
    LOG_V("Surface extent limits.\n\t\tMIN W: {} H: {}\n\t\tMAX W: {}, H: {}",
          capabilities.minImageExtent.width, capabilities.minImageExtent.height, capabilities.maxImageExtent.width, capabilities.maxImageExtent.height);

    glm::uvec2 determinedSize = getWindowSize();
    
    VkExtent2D swapchainExtent;
    // 0xFFFFFFFF (-1) means that size of surface will change depending on the extents of the swapchain images
    if (capabilities.currentExtent.width == static_cast<uint32_t>(-1)) {
        LOG_V("Surface width and height depend of swapchain image extents and are currently equal to\n\tW: {}\n\tH: {}", determinedSize.x, determinedSize.y);
                
        swapchainExtent.width  = std::max(capabilities.minImageExtent.width,  std::min(capabilities.maxImageExtent.width,  determinedSize.x));
        swapchainExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, determinedSize.y));
        
        determinedSize.x  = swapchainExtent.width;
        determinedSize.y = swapchainExtent.height;
    } else {
        LOG_V("Surface width and height are set and are equal to\n\tW: {}\n\tH: {}", capabilities.currentExtent.width, capabilities.currentExtent.height);
        swapchainExtent = capabilities.currentExtent;
        
        determinedSize.x  = capabilities.currentExtent.width;
        determinedSize.y = capabilities.currentExtent.height;
    }
    
//     // TODO do I still need this? Should I transfer images from intermediate effect buffers using
//     // transfer ops or by rendering a full screen quad?
//     // If this gets removed, VK_IMAGE_USAGE_TRANSFER_DST_BIT should be removed from sci.imageUsage as well
//     if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
//         throw std::runtime_error("Surface does not support image transfer.");
//     }
    
    // We MAY not get as many images as we want, hence the check
    std::uint32_t numImagesToRequest = capabilities.minImageCount + 1;
    if ((capabilities.maxImageCount > 0) && (numImagesToRequest > capabilities.maxImageCount)) {
        numImagesToRequest = capabilities.maxImageCount;
    }
    
    LOG_V("Number of requested swapchain images: {}", numImagesToRequest);

    VkSurfaceTransformFlagsKHR preTransformFlags;
    if (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransformFlags = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransformFlags = capabilities.currentTransform;
    }
    
    swapchain.imageExtent = swapchainExtent;

    VkSwapchainCreateInfoKHR sci;
    sci.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.pNext                 = nullptr;
    sci.flags                 = 0;
    sci.surface               = surface;
    sci.minImageCount         = numImagesToRequest;
    sci.imageFormat           = swapchain.imageFormat;
    sci.imageColorSpace       = swapchain.imageColorSpace;
    sci.imageExtent           = swapchainExtent;
    sci.imageArrayLayers      = 1;
    sci.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;// | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    const ImageUsageFlags usage = ImageUsageFlagBits::ColorAtachment;// | TransferDestination;
    
    std::array<std::uint32_t, 2> queueFamilyIndices = {
        physicalDevice.chosenMainQueueFamilyId,
        physicalDevice.chosenPresentQueueFamilyId
    };
    
    if (physicalDevice.presentQueueFamilySeparateFromMain) {
        // TODO check the impact to performance. May be better to use explicit ownership transfers
        sci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices   = queueFamilyIndices.data();
    } else {
        sci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        sci.queueFamilyIndexCount = 0;
        sci.pQueueFamilyIndices   = nullptr;
    }
    
    sci.preTransform          = static_cast<VkSurfaceTransformFlagBitsKHR>(preTransformFlags);
    sci.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode           = presentMode;
    sci.clipped               = VK_TRUE;
    sci.oldSwapchain          = recreatingSwapchain ? oldSwapchain.handle : VK_NULL_HANDLE;
    
    checkResult(vkCreateSwapchain(logicalDevice.handle, &sci, nullptr, &swapchain.handle), "Failed to create a swapchain.");
    
    // Don't forget to clean the vectors.
    swapchain.clearImageDataVectors();

    // Retrieving the swapchain images
    std::uint32_t swapchainImageCount = 0;
    checkResult(vkGetSwapchainImages(logicalDevice.handle, swapchain.handle, &swapchainImageCount, nullptr), "Failed to obtain swapchain image count.");

    swapchain.images.resize(swapchainImageCount);
    swapchain.engineImages.reserve(swapchainImageCount);
    swapchain.imageViews.resize(swapchainImageCount);
    checkResult(vkGetSwapchainImages(logicalDevice.handle, swapchain.handle, &swapchainImageCount, swapchain.images.data()), "Failed to enumerate swapchain images.");
    
    for (VkImage image : swapchain.images) {
        swapchain.engineImages.emplace_back(ImageHnd(image), glm::uvec3(determinedSize.x, determinedSize.y, 0), 1, 1, usage, getSurfaceFormat(), ImageViewType::Im2D, nullptr);
    }
    
    swapchain.version += 1;
    
    createSwapchainImageViews();
    
    // It is now safe to delete old data and create new objects
    if (recreatingSwapchain) {
        for (SwapchainChangeListener* listener : swapchainChangeListeners) {
            listener->onSwapchainChange();
        }
        
        disposeOfSwapchainAndDependencies(oldSwapchain);
    } else {
        if (maxFramesInFlight > swapchain.images.size()) {
            maxFramesInFlight = swapchain.images.size();
        }
    }
}

void VulkanAPI::chooseDepthStencilFormat() {
    // All depth stencil formats in the order of quality
    std::vector<VkFormat> depthStencilFormats = { 
        VK_FORMAT_D32_SFLOAT_S8_UINT, 
        //VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT, 
        VK_FORMAT_D16_UNORM_S8_UINT, 
        //VK_FORMAT_D16_UNORM 
    };
    
    std::vector<Format> depthStencilFormatsEngine = {
        Format::D32_sFloat_S8_uInt,
//        Format::D32_sFloat,
        Format::D24_uNorm_S8_uInt,
        Format::D16_uNorm_S8_uInt,
//        Format::D16_uNorm
    };

    for (std::size_t i = 0; i < depthStencilFormats.size(); ++i) {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice.handle, depthStencilFormats[i], &formatProperties);

        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthStencilFormat = depthStencilFormats[i];
            depthStencilFormatEngine = depthStencilFormatsEngine[i];
            
            break;
        }
    }
    
    std::string formatName = getFormatName(depthStencilFormatEngine);
    LOG_V("Chosen depth stencil format: {}", formatName);
}

void VulkanAPI::createSwapchainImageViews() {
    for (std::uint32_t i = 0; i < swapchain.images.size(); i++) {
        VkImageSubresourceRange isr;
        isr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        isr.baseMipLevel   = 0;
        isr.levelCount     = 1;
        isr.baseArrayLayer = 0;
        isr.layerCount     = 1;
        
        VkImageViewCreateInfo ivci;
        ivci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.pNext            = nullptr;
        ivci.flags            = 0;
        ivci.image            = swapchain.images[i];
        ivci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format           = swapchain.imageFormat;
        ivci.components       = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
        };
        ivci.subresourceRange = isr;

        checkResult(vkCreateImageView(logicalDevice.handle, &ivci, nullptr, &swapchain.imageViews[i]), "Failed to create swapchain image views.");
    }
}
}
