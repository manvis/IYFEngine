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
#include "graphics/vulkan/vk_mem_alloc.h"
#include "core/Logger.hpp"

namespace iyf {
inline Bytes getSize(const std::vector<Bytes>& sizes, MemoryBatch batch) {
    return sizes[static_cast<std::size_t>(batch)];
}

inline std::string getDebugName(MemoryBatch batch) {
    switch (batch) {
        case MemoryBatch::MeshAssetData:
        case MemoryBatch::TextureAssetData:
            return "StagingMeshAndTextureBuffer";
        case MemoryBatch::PerFrameData:
            return "StagingPerFrameDataBuffer";
        case MemoryBatch::Instant:
            return "StagingInstantTransferDataBuffer";
    }
    
    throw std::runtime_error("Invalid or unknown MemoryBatch");
}

VulkanDeviceMemoryManager::VulkanDeviceMemoryManager(VulkanAPI* gfx, std::vector<Bytes> stagingBufferSizes)
    : DeviceMemoryManager(std::move(stagingBufferSizes)), gfx(gfx) {
    const auto& sizes = getStagingBufferSizes();
    
    if (sizes.size() != static_cast<std::size_t>(MemoryBatch::COUNT) + 1) {
        throw std::runtime_error("stagingBufferSizes vector doesn't specify all initial stagin buffer data sizes");
    }
    
    // MemoryBatch::MeshAssetData and MemoryBatch::TextureAssetData share the staging buffer
    assert(&getStagingBufferForBatch(MemoryBatch::MeshAssetData) == &getStagingBufferForBatch(MemoryBatch::TextureAssetData));
    
    // TODO support transfer queues, which requires me to actually implement createCommandPool and add ownership transfers to this class
    commandPool = gfx->createCommandPool(QueueType::Graphics, 0);
    
    const Bytes totalSize = getSize(sizes, MemoryBatch::MeshAssetData) + getSize(sizes, MemoryBatch::TextureAssetData);
    bool created = initStagingBufferForBatch(MemoryBatch::MeshAssetData, totalSize, true);
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for mesh and texture data");
    }
    
    // MemoryBatch::PerFrameData.
    created = initStagingBufferForBatch(MemoryBatch::PerFrameData, getSize(sizes, MemoryBatch::PerFrameData), true);
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for per frame data");
    }
    
    // MemoryBatch::Instant
    created = initStagingBufferForBatch(MemoryBatch::Instant, getSize(sizes, MemoryBatch::Instant), true);
    if (!created) {
        throw std::runtime_error("Failed to create a staging buffer for instantly transferred data");
    }
    
    uploadCompleteFence = gfx->createFence(false);
}

VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager() {
    gfx->destroyFence(uploadCompleteFence);
    
    for (StagingBufferData& data : stagingBuffers) {
        gfx->destroyBuffer(data.buffer);
        commandPool->freeCommandBuffer(data.commandBuffer);
    }
    
    gfx->destroyCommandPool(commandPool);
}

bool VulkanDeviceMemoryManager::initStagingBufferForBatch(MemoryBatch batch, Bytes size, bool firstCreation) {
    StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
    
    stagingBufferData.batch = batch;
    
    if (firstCreation) {
        stagingBufferData.commandBuffer = commandPool->allocateCommandBuffer(BufferLevel::Primary, true);
    } else {
        gfx->destroyBuffer(stagingBufferData.buffer);
    }
    
    BufferCreateInfo bci(BufferUsageFlagBits::TransferSource, size, MemoryUsage::CPUOnly, false, getDebugName(batch));
    const bool created = gfx->createBuffer(bci, stagingBufferData.buffer);
    
    assert(static_cast<const AllocationAndInfo*>(stagingBufferData.buffer.allocationInfo())->info.pMappedData != nullptr);
    
    return created;
}

void VulkanDeviceMemoryManager::beginFrame() {
    for (StagingBufferData& data : stagingBuffers) {
        resetData(data);
    }
}

void VulkanDeviceMemoryManager::resetData(StagingBufferData& data) {
    if (data.buffer.size() < data.maxRequestLastFrame) {
        initStagingBufferForBatch(data.batch, Bytes(data.maxRequestLastFrame), true);
    }
    
    data.maxRequestLastFrame = 0;
    data.currentOffset = 0;
    
    if (data.batch != MemoryBatch::Instant && data.uploadCalls == 0) {
        LOG_W("beginBatchUpload() wasn't called last frame for MemoryBatch with ID " << static_cast<std::uint32_t>(data.batch));
    }
    
    data.uploadCalls = 0;
}

bool VulkanDeviceMemoryManager::isStagingBufferNeeded(const Buffer& destinationBuffer) {
    // We use VMA_ALLOCATION_CREATE_MAPPED_BIT to ensure host visible memory is always mapped.
    const AllocationAndInfo* allocationInfo = static_cast<const AllocationAndInfo*>(destinationBuffer.allocationInfo());
    return allocationInfo->info.pMappedData == nullptr;
}

bool VulkanDeviceMemoryManager::canBatchFitData(MemoryBatch batch, const std::vector<BufferCopy>& copies) {
    std::uint64_t total = 0;
    for (const BufferCopy& copy : copies) {
        total += copy.size;
    }
    
    StagingBufferData& sbData = getStagingBufferForBatch(batch);
    sbData.maxRequestLastFrame = std::max(total, sbData.maxRequestLastFrame);
    
    return (total + sbData.currentOffset) <= sbData.buffer.size();
}

bool VulkanDeviceMemoryManager::updateBuffer(MemoryBatch batch, const Buffer& destinationBuffer, const std::vector<BufferCopy>& copies, const void* data) {
    if (copies.empty() || data == nullptr) {
        return true;
    }
    
    assert(canBatchFitData(batch, copies));
    
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
                vmaFlushAllocation(gfx->allocator, destinationAllocationInfo->allocation, c.dstOffset, c.size);
            }
        }
    } else {
        StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
        const AllocationAndInfo* stagingAllocationInfo = static_cast<const AllocationAndInfo*>(stagingBufferData.buffer.allocationInfo());
        
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
                vmaFlushAllocation(gfx->allocator, stagingAllocationInfo->allocation, c.dstOffset, c.size);
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
        
        VkCommandBuffer copyBuff = stagingBufferData.commandBuffer->getHandle().toNative<VkCommandBuffer>();
        vkCmdCopyBuffer(copyBuff, stagingBufferData.buffer.handle().toNative<VkBuffer>(), destinationBuffer.handle().toNative<VkBuffer>(), stagingCopies.size(), stagingCopies.data());
        
        stagingBufferData.currentOffset = offset;
        
        if (batch == MemoryBatch::Instant) {
            executeUpload(stagingBufferData.commandBuffer);
            resetData(stagingBufferData);
        }
    }
    
    return true;
}

bool VulkanDeviceMemoryManager::beginBatchUpload(MemoryBatch batch) {
    StagingBufferData& stagingBufferData = getStagingBufferForBatch(batch);
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
        executeUpload(stagingBufferData.commandBuffer);
    }
    
    return true;
}

void VulkanDeviceMemoryManager::executeUpload(CommandBuffer* commandBuffer) {
    commandBuffer->end();
    
    // TODO start using pipeline barriers instead of waiting for operations to complete.
    SubmitInfo si;
    si.commandBuffers = {commandBuffer->getHandle()};
    
    gfx->submitQueue(si, uploadCompleteFence);
    gfx->waitForFence(uploadCompleteFence, 50000000000);
    gfx->resetFence(uploadCompleteFence);
    
    commandBuffer->begin();
}

}
