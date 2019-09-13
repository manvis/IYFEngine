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

// Do not include this in VulkanAPI.hpp or any other headers that get included in many places.
// This library pulls in vulkan.h and that includes a ton of OS specific stuff.
#include "graphics/vulkan/vk_mem_alloc.h"

namespace iyf {
struct AllocationAndInfo {
    AllocationAndInfo(VmaAllocation allocation, VmaAllocationInfo info, VkMemoryPropertyFlags memoryFlags)
        : allocation(allocation), info(std::move(info)), memoryFlags(memoryFlags) {}
    
    VmaAllocation allocation;
    VmaAllocationInfo info;
    VkMemoryPropertyFlags memoryFlags;
};

class VulkanDeviceMemoryManager : public DeviceMemoryManager {
public:
    VulkanDeviceMemoryManager(VulkanAPI* gfx, std::vector<Bytes> stagingBufferSizes);
    virtual ~VulkanDeviceMemoryManager();
    
    virtual void initialize() final override;
    virtual void initializeAllocator() final override;
    virtual void dispose() final override;
    virtual void disposeAllocator() final override;
    
    virtual void beginFrame() final override;
    virtual bool isStagingBufferNeeded(const Buffer& destinationBuffer) final override;
    virtual bool canBatchFitData(MemoryBatch batch, Bytes totalSize) final override;
    virtual bool updateBuffer(MemoryBatch batch, const Buffer& destinationBuffer, const std::vector<BufferCopy>& copies, const void* data) final override;
    virtual bool updateImage(MemoryBatch batch, const Image& image, const TextureData& data) final override;
    virtual bool beginBatchUpload(MemoryBatch batch) final override;
    
    virtual Buffer createBuffer(const BufferCreateInfo& info, const char* name) final override;
    virtual bool destroyBuffer(const Buffer& buffer) final override;
    virtual bool readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) final override;
    virtual Image createImage(const ImageCreateInfo& info, const char* name) final override;
    virtual bool destroyImage(const Image& image) final override;
private:
    struct StagingBufferData {
        StagingBufferData() : currentOffset(0), maxRequestedUploadSize(0), uploadCalls(0), APIObjectsCreated(false) {}
        
        std::vector<Buffer> buffers;
        std::vector<CommandBuffer*> commandBuffers;
        
        std::uint64_t currentOffset;
        std::uint64_t maxRequestedUploadSize;
        std::uint32_t uploadCalls;
        MemoryBatch batch;
        bool APIObjectsCreated;
        
        inline bool hasDataThisFrame() const {
            return currentOffset != 0;
        }
        
        inline Buffer& getBufferForThisFrame(GraphicsAPI* gfx) {
            const std::uint32_t swapImage = gfx->getCurrentSwapImage();
            
            assert(!buffers.empty());
            assert(swapImage < buffers.size());
            
            return buffers[swapImage];
        }
        
        inline CommandBuffer* getCommandBufferForThisFrame(GraphicsAPI* gfx) {
            const std::uint32_t swapImage = gfx->getCurrentSwapImage();
            
            assert(!commandBuffers.empty());
            assert(swapImage < commandBuffers.size());
            
            return commandBuffers[swapImage];
        }
    };
    
    constexpr static std::size_t StagingBufferCount = 3;
    inline StagingBufferData& getStagingBufferForBatch(MemoryBatch batch) {
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
    
    bool initOrUpdateStagingBuffer(MemoryBatch batch, Bytes size);
    void executeUpload(CommandBuffer* commandBuffer, bool waitForCompletion);
    void resetData(StagingBufferData& data);
    
    VulkanAPI* gfx;
    CommandPool* commandPool;
    FenceHnd uploadCompleteFence;
    std::array<StagingBufferData, StagingBufferCount> stagingBuffers;
    bool firstFrame;
    
    // Vulkan memory allocator library stuff
    VmaAllocator allocator;
    VkBuffer imageTransferSource;
    VmaAllocation imageTransferSourceAllocation;
    /// Allows us to retrieve various info about backing memory based on a buffer's handle
    std::unordered_map<VkBuffer, AllocationAndInfo> bufferToMemory;
    std::unordered_map<VkImage, AllocationAndInfo> imageToMemory;
};
}

