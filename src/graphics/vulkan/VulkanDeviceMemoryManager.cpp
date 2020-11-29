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

#include "graphics/vulkan/VulkanDeviceMemoryManager.hpp"
#include "graphics/vulkan/VulkanUtilities.hpp"
#include "graphics/vulkan/vk_mem_alloc.h"
#include "core/Logger.hpp"
#include "threading/ThreadProfiler.hpp"
#include "assets/loaders/TextureLoader.hpp"
#include "utilities/DataSizes.hpp"

namespace iyf {
using namespace literals;

inline Bytes getSize(const std::vector<Bytes>& sizes, MemoryBatch batch) {
    return sizes[static_cast<std::size_t>(batch)];
}

inline std::string getDebugName(MemoryBatch batch) {
    switch (batch) {
        case MemoryBatch::MeshAssetData:
        case MemoryBatch::TextureAssetData:
            return "mesh and texture data";
        case MemoryBatch::PerFrameData:
            return "per frame data";
        case MemoryBatch::Instant:
            return "instant transfer data";
    }
    
    throw std::runtime_error("Invalid or unknown MemoryBatch");
}

VulkanDeviceMemoryManager::VulkanDeviceMemoryManager(VulkanAPI* gfx, std::vector<Bytes> stagingBufferSizes)
    : DeviceMemoryManager(std::move(stagingBufferSizes)), gfx(gfx), firstFrame(true) {}

void VulkanDeviceMemoryManager::initializeAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    
    if (gfx->physicalDevice.dedicatedAllocationExtensionEnabled) {
        allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        LOG_V("The VK_KHR_dedicated_allocation extension was found and enabled. Enabling its use in the allocator as well.");
    }
    
    allocatorInfo.physicalDevice = gfx->physicalDevice.handle;
    allocatorInfo.device = gfx->logicalDevice.handle;
    // TODO set the heap size limit to simulate devices with less memory
    // TODO set callbacks for profiling functions
    
    vmaCreateAllocator(&allocatorInfo, &allocator);
    
    std::string imageTransferSourceName = "imageTransferSource";
    
    VkBufferCreateInfo bci;
    bci.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext                 = nullptr;
    bci.flags                 = 0;
    bci.size                  = Bytes(64_MiB).count(); // TODO make configurable
    bci.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bci.sharingMode           = VK_SHARING_MODE_EXCLUSIVE; // TODO use a transfer queue (if available)
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices   = nullptr;
    
    VmaAllocationCreateInfo aci = {};
    aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
    aci.usage = VMA_MEMORY_USAGE_CPU_ONLY; // Guarantees HOST_VISIBLE and HOST_COHERENT
    aci.pUserData = &imageTransferSourceName[0];
    
    gfx->checkResult(vmaCreateBuffer(allocator, &bci, &aci, &imageTransferSource, &imageTransferSourceAllocation, nullptr), "Failed to create the image transfer source buffer.");
}

void VulkanDeviceMemoryManager::disposeAllocator() {
    vmaDestroyBuffer(allocator, imageTransferSource, imageTransferSourceAllocation);
    vmaDestroyAllocator(allocator);
}

void VulkanDeviceMemoryManager::initialize() {
    const auto& sizes = getStagingBufferSizes();
    
    if (sizes.size() != static_cast<std::size_t>(MemoryBatch::COUNT) + 1) {
        throw std::runtime_error("stagingBufferSizes vector doesn't specify all initial stagin buffer data sizes");
    }
    
    // MemoryBatch::MeshAssetData and MemoryBatch::TextureAssetData share the staging buffer
    assert(&getStagingBufferForBatch(MemoryBatch::MeshAssetData) == &getStagingBufferForBatch(MemoryBatch::TextureAssetData));
    
    // TODO support transfer queues, which requires me to actually implement createCommandPool and add ownership transfers to this class
    commandPool = gfx->createCommandPool(QueueType::Graphics, 0, "Device memory manager command pool");
    
    const Bytes totalSize = getSize(sizes, MemoryBatch::MeshAssetData) + getSize(sizes, MemoryBatch::TextureAssetData);
    bool created = initOrUpdateStagingBuffer(MemoryBatch::MeshAssetData, totalSize);
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for mesh and texture data");
    }
    
    // MemoryBatch::PerFrameData.
    created = initOrUpdateStagingBuffer(MemoryBatch::PerFrameData, getSize(sizes, MemoryBatch::PerFrameData));
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for per frame data");
    }
    
    // MemoryBatch::Instant
    created = initOrUpdateStagingBuffer(MemoryBatch::Instant, getSize(sizes, MemoryBatch::Instant));
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for instantly transferred data");
    }
    
    uploadCompleteFence = gfx->createFence(false, "Device memory manager upload complete fence");
}

VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager() {}

void VulkanDeviceMemoryManager::dispose() {
    gfx->destroyFence(uploadCompleteFence);
    
    for (StagingBufferData& data : stagingBuffers) {
        gfx->destroyBuffers(data.buffers);
        commandPool->freeCommandBuffers(data.commandBuffers);
    }
    
    gfx->destroyCommandPool(commandPool);
}

VulkanDeviceMemoryManager::StagingBufferData& VulkanDeviceMemoryManager::getStagingBufferForBatch(MemoryBatch batch) {
    switch (batch) {
        case MemoryBatch::MeshAssetData:
        case MemoryBatch::TextureAssetData:
            return stagingBuffers[0];
        case MemoryBatch::PerFrameData:
            return stagingBuffers[1];
        case MemoryBatch::Instant:
            return stagingBuffers[2];
    }
    
    throw std::runtime_error("Invalid or unknown MemoryBatch");
}

bool VulkanDeviceMemoryManager::initOrUpdateStagingBuffer(MemoryBatch batch, Bytes size) {
    StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
    
    BufferCreateInfo bci(BufferUsageFlagBits::TransferSource, size, MemoryUsage::CPUOnly, false);
    stagingBufferData.batch = batch;
    
    if (!stagingBufferData.APIObjectsCreated) {
        const std::uint32_t swapImageCount = gfx->getSwapImageCount();
        
        std::vector<std::string> names;
        names.reserve(swapImageCount);
        
        std::vector<const char*> namesCStr;
        namesCStr.reserve(swapImageCount);
        
        for (std::uint32_t i = 0; i < swapImageCount; ++i) {
            names.emplace_back(fmt::format("Vulkan Device Memory Manager command buffer {} for {}", i, getDebugName(batch)));
            namesCStr.emplace_back(names[i].c_str());
        }
        
        stagingBufferData.commandBuffers = commandPool->allocateCommandBuffers(&namesCStr, swapImageCount, BufferLevel::Primary, false);
        
        names.clear();
        namesCStr.clear();
        
        std::vector<BufferCreateInfo> bcis;
        bcis.reserve(swapImageCount);
        
        for (std::uint32_t i = 0; i < swapImageCount; ++i) {
            names.emplace_back(fmt::format("Vulkan Device Memory Manager staging buffer {} for {}", i, getDebugName(batch)));
            namesCStr.emplace_back(names[i].c_str());
            bcis.emplace_back(bci);
        }
        
        stagingBufferData.buffers = gfx->createBuffers(bcis, &namesCStr);
        stagingBufferData.APIObjectsCreated = true;
/*        
        for (const auto& b : stagingBufferData.buffers) {
            assert(b.size() == size);
        }*/
    } else {
        Buffer& buffer = stagingBufferData.getBufferForThisFrame(gfx);
        gfx->destroyBuffer(buffer);
        buffer = gfx->createBuffer(bci, getDebugName(batch).c_str());
        
        assert(static_cast<const AllocationAndInfo*>(buffer.allocationInfo())->info.pMappedData != nullptr);
    }
    
    return true;
}

void VulkanDeviceMemoryManager::beginFrame() {
    IYFT_PROFILE(beginDeviceMemoryManagerFrame, iyft::ProfilerTag::Graphics)
    
    for (StagingBufferData& data : stagingBuffers) {
        resetData(data);
    }
    
    firstFrame = false;
}

void VulkanDeviceMemoryManager::resetData(StagingBufferData& data) {
    Buffer& buffer = data.getBufferForThisFrame(gfx);
    
    if (buffer.size() < data.maxRequestedUploadSize) {
        initOrUpdateStagingBuffer(data.batch, Bytes(data.maxRequestedUploadSize));
    }
    
    data.currentOffset = 0;
    
    if (!firstFrame && data.batch != MemoryBatch::Instant && data.uploadCalls == 0) {
        LOG_W("beginBatchUpload() wasn't called last frame for MemoryBatch with ID {}", static_cast<std::uint32_t>(data.batch));
    }
    
    data.uploadCalls = 0;
}

bool VulkanDeviceMemoryManager::isStagingBufferNeeded(const Buffer& destinationBuffer) {
    // We use VMA_ALLOCATION_CREATE_MAPPED_BIT to ensure host visible memory is always mapped.
    const AllocationAndInfo* allocationInfo = static_cast<const AllocationAndInfo*>(destinationBuffer.allocationInfo());
    return allocationInfo->info.pMappedData == nullptr;
}

bool VulkanDeviceMemoryManager::canBatchFitData(MemoryBatch batch, Bytes totalSize) {
    StagingBufferData& sbData = getStagingBufferForBatch(batch);
    assert(sbData.APIObjectsCreated);
    
    sbData.maxRequestedUploadSize = std::max(totalSize.count(), sbData.maxRequestedUploadSize);
    
    const Buffer& buffer = sbData.getBufferForThisFrame(gfx);
    
    const Bytes bufferSize = buffer.size();
    const std::uint64_t sizeAfterUpload = totalSize.count() + sbData.currentOffset;
    
//     LOG_D("{} {}", bufferSize.count(), sizeAfterUpload);
    
    return sizeAfterUpload <= bufferSize;
}

bool VulkanDeviceMemoryManager::updateBuffer(MemoryBatch batch, const Buffer& destinationBuffer, const std::vector<BufferCopy>& copies, const void* data) {
    IYFT_PROFILE(updateBuffer, iyft::ProfilerTag::Graphics)
    
    if (copies.empty() || data == nullptr) {
        return true;
    }

    assert(canBatchFitData(batch, computeUploadSize(copies)));
    
    const AllocationAndInfo* destinationAllocationInfo = static_cast<const AllocationAndInfo*>(destinationBuffer.allocationInfo());
    
    const bool hostVisible = (destinationAllocationInfo->info.pMappedData != nullptr);
    
    if (hostVisible) {
        const bool needsFlushing = !(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & destinationAllocationInfo->memoryFlags);
        void* destinationMemory = destinationAllocationInfo->info.pMappedData;
        
        for (const auto& c : copies) {
            const char* source = static_cast<const char*>(data);
            source += c.srcOffset;
            
            char* destination = static_cast<char*>(destinationMemory);
            destination += c.dstOffset;
            
            std::memcpy(destination, source, c.size);
            
            if (needsFlushing) {
                vmaFlushAllocation(allocator, destinationAllocationInfo->allocation, c.dstOffset, c.size);
            }
        }
    } else {
        StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
        assert(stagingBufferData.APIObjectsCreated);
        
        Buffer& buffer = stagingBufferData.getBufferForThisFrame(gfx);
        CommandBuffer* commandBuffer = stagingBufferData.getCommandBufferForThisFrame(gfx);
        
        if (!commandBuffer->isRecording()) {
            commandBuffer->begin();
        }
        
        const AllocationAndInfo* stagingAllocationInfo = static_cast<const AllocationAndInfo*>(buffer.allocationInfo());
        
        std::vector<VkBufferCopy> stagingCopies;
        stagingCopies.reserve(copies.size());
        
        const bool needsFlushing = !(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & stagingAllocationInfo->memoryFlags);
        
        std::uint64_t offset = stagingBufferData.currentOffset;
        for (const auto& c : copies) {
            const char* source = static_cast<const char*>(data);
            source += c.srcOffset;
            
            char* destination = static_cast<char*>(stagingAllocationInfo->info.pMappedData);
            destination += offset;
            offset += c.size;
            
            std::memcpy(destination, source, c.size);
            
            if (needsFlushing) {
                vmaFlushAllocation(allocator, stagingAllocationInfo->allocation, c.dstOffset, c.size);
            }
        }
        
        offset = stagingBufferData.currentOffset;
        for (const auto& c : copies) {
            VkBufferCopy bc;
            bc.srcOffset = offset;
            offset += c.size;
            
            bc.dstOffset = c.dstOffset;
            bc.size = c.size;
            
            stagingCopies.push_back(std::move(bc));
        }
        
        VkCommandBuffer copyBuff = commandBuffer->getHandle().toNative<VkCommandBuffer>();
        vkCmdCopyBuffer(copyBuff, buffer.handle().toNative<VkBuffer>(), destinationBuffer.handle().toNative<VkBuffer>(), stagingCopies.size(), stagingCopies.data());
        
        stagingBufferData.currentOffset = offset;
        
        if (batch == MemoryBatch::Instant) {
            executeUpload(commandBuffer, true);
            resetData(stagingBufferData);
        }
    }
    
    return true;
}

bool VulkanDeviceMemoryManager::beginBatchUpload(MemoryBatch batch) {
    IYFT_PROFILE(beginBatchUpload, iyft::ProfilerTag::Graphics)
    
    StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
    assert(stagingBufferData.APIObjectsCreated);
    
    stagingBufferData.uploadCalls++;
    
    bool shouldUpload = false;
    switch (batch) {
        case MemoryBatch::MeshAssetData:
        case MemoryBatch::TextureAssetData:
            if (stagingBufferData.uploadCalls == 2 && stagingBufferData.hasDataThisFrame()) {
                shouldUpload = true;
            } else if (stagingBufferData.uploadCalls > 2) {
                throw std::runtime_error("beginBatchUpload called too many times");
            }
            
            break;
        case MemoryBatch::PerFrameData:
            if (stagingBufferData.uploadCalls == 1 && stagingBufferData.hasDataThisFrame()) {
                shouldUpload = true;
            } else if (stagingBufferData.uploadCalls > 1) {
                throw std::runtime_error("beginBatchUpload called too many times");
            }
            
            break;
        case MemoryBatch::Instant:
            throw std::logic_error("You cannot call beginBatchUpload() with MemoryBatch::Instant because such uploads aren't batched at all");
    }
    
    if (shouldUpload) {
        CommandBuffer* commandBuffer =  stagingBufferData.getCommandBufferForThisFrame(gfx);
        executeUpload(commandBuffer, false);
    }
    
    return true;
}

bool VulkanDeviceMemoryManager::updateImage(MemoryBatch batch, const Image& image, const TextureData& data) {
    assert(canBatchFitData(batch, Bytes(data.size)));
    
    StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
    assert(stagingBufferData.APIObjectsCreated);
    
    Buffer& buffer = stagingBufferData.getBufferForThisFrame(gfx);
    CommandBuffer* commandBuffer = stagingBufferData.getCommandBufferForThisFrame(gfx);
        
    if (!commandBuffer->isRecording()) {
        commandBuffer->begin();
    }
    
    const AllocationAndInfo* stagingAllocationInfo = static_cast<const AllocationAndInfo*>(buffer.allocationInfo());
    
    std::vector<VkBufferImageCopy> bics;
    std::size_t layerId = 0;
    std::size_t offset = stagingBufferData.currentOffset;
    
    for (std::size_t face = 0; face < data.faceCount; ++face) {
        for (std::size_t level = 0; level < data.mipmapLevelCount; ++level) {
            const glm::uvec3& extents = data.getLevelExtents(level);
            
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
            
            offset += data.getLevelSize(level);
        }
        
        layerId++;
    }
    
    const bool needsFlushing = !(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & stagingAllocationInfo->memoryFlags);
    void* destinationMemory = stagingAllocationInfo->info.pMappedData;
    
    char* destination = static_cast<char*>(destinationMemory);
    destination += stagingBufferData.currentOffset;
        
    std::memcpy(destination, data.data, data.size);
    
    if (needsFlushing) {
        vmaFlushAllocation(allocator, stagingAllocationInfo->allocation, stagingBufferData.currentOffset, data.size);
    }
    
    stagingBufferData.currentOffset = offset;
    
    VkImage vulkanImage = image.getHandle().toNative<VkImage>();
    
    VkImageSubresourceRange sr;
    sr.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    sr.baseMipLevel   = 0;
    sr.levelCount     = data.mipmapLevelCount;
    sr.baseArrayLayer = 0;
    sr.layerCount     = (data.faceCount == 6) ? 6 : data.layers;
    
    VkCommandBuffer uploadBuffer = commandBuffer->getHandle().toNative<VkCommandBuffer>();
    
    stagingBufferData.currentOffset = offset;

    gfx->setImageLayout(uploadBuffer, vulkanImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, sr, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(uploadBuffer, buffer.handle().toNative<VkBuffer>(), vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, bics.size(), bics.data());
    gfx->setImageLayout(uploadBuffer, vulkanImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, sr, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    
    if (batch == MemoryBatch::Instant) {
        executeUpload(commandBuffer, true);
        resetData(stagingBufferData);
    }
    
    return true;
}


void VulkanDeviceMemoryManager::executeUpload(CommandBuffer* commandBuffer, bool /*waitForCompletion*/) {
    IYFT_PROFILE(executeUpload, iyft::ProfilerTag::Graphics)
    
    assert(commandBuffer->isRecording());
    commandBuffer->end();
    
    // TODO start using pipeline barriers instead of waiting for operations to complete.
    SubmitInfo si;
    si.commandBuffers = {commandBuffer->getHandle()};
    
//     if (waitForCompletion) {
        gfx->submitQueue(si, uploadCompleteFence);
        gfx->waitForFence(uploadCompleteFence, 50000000000);
        gfx->resetFence(uploadCompleteFence);
        // TODO use proper synchronisation instead of this fence
//     } else {
//         gfx->submitQueue(si);
//     }
}

Buffer VulkanDeviceMemoryManager::createBuffer(const BufferCreateInfo& info, const char* name) {
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
    
    if (name != nullptr) {
        aci.flags |= VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        aci.pUserData = const_cast<char*>(name); // I'm pretty sure it doesn't actually modify the string
    } else {
        aci.pUserData = nullptr;
    }
    
    VkBuffer handle;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    gfx->checkResult(vmaCreateBuffer(allocator, &bci, &aci, &handle, &allocation, &allocationInfo), "Failed to create a buffer");
    
    // DO NOT set a name. VMA does it for us
    
    if (aci.usage != VMA_MEMORY_USAGE_GPU_ONLY && allocationInfo.pMappedData == nullptr) {
        throw std::runtime_error("Was expecting a mapped buffer.");
    }
    
    const Bytes finalSize = Bytes(allocationInfo.size);
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, allocationInfo.memoryType, &flags);
    
    auto iter = bufferToMemory.emplace(std::make_pair(handle, AllocationAndInfo(allocation, allocationInfo, flags)));
    
    return Buffer(BufferHnd(handle), info.flags, info.memoryUsage, finalSize, &(iter.first->second));
}

bool VulkanDeviceMemoryManager::destroyBuffer(const Buffer& buffer) {
    VkBuffer handle = buffer.handle().toNative<VkBuffer>();
    auto result = bufferToMemory.find(handle);
    
    if (result == bufferToMemory.end()) {
        throw std::runtime_error("Can't destoy a non existing buffer");
    }
    
    vmaDestroyBuffer(allocator, handle, result->second.allocation);
    
    bufferToMemory.erase(result);
    
    return true;
}

bool VulkanDeviceMemoryManager::readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) {
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

Image VulkanDeviceMemoryManager::createImage(const ImageCreateInfo& info, const char* name) {
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
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    if (name != nullptr) {
        aci.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
        aci.pUserData = const_cast<char*>(name); // I'm pretty sure it doesn't actually modify the string
    } else {
        aci.flags = 0;
        aci.pUserData = nullptr;
    }
    
    VkImage image;
    VmaAllocation allocation;
    VmaAllocationInfo allocationInfo;
    gfx->checkResult(vmaCreateImage(allocator, &ici, &aci, &image, &allocation, &allocationInfo), "Failed to create an image");
    
    // DO NOT set a name. VMA does it for us
    
    VkMemoryPropertyFlags flags = 0;
    vmaGetMemoryTypeProperties(allocator, allocationInfo.memoryType, &flags);
    
    auto iter = imageToMemory.emplace(std::make_pair(image, AllocationAndInfo(allocation, allocationInfo, flags)));
    return Image(ImageHnd(image), info.extent, info.mipLevels, info.arrayLayers, info.usage, info.format, info.isCube ? ImageViewType::ImCube : ImageViewType::Im2D, &(iter.first->second));
}

bool VulkanDeviceMemoryManager::destroyImage(const Image& image) {
    VkImage handle = image.getHandle().toNative<VkImage>();
    
    auto iter = imageToMemory.find(handle);
    
    if (iter == imageToMemory.end()) {
        throw std::runtime_error("Can't destoy a non existing image");
    }
    
    vmaDestroyImage(allocator, image.getHandle().toNative<VkImage>(), iter->second.allocation);
    
    imageToMemory.erase(iter);
    
    return true;
}

}
