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

namespace iyf {
class VulkanDeviceMemoryManager : public DeviceMemoryManager {
public:
    VulkanDeviceMemoryManager(VulkanAPI* gfx, std::vector<Bytes> stagingBufferSizes);
    virtual ~VulkanDeviceMemoryManager();
    
    virtual void beginFrame() final override;
    virtual bool isStagingBufferNeeded(const Buffer& destinationBuffer) final override;
    virtual bool canBatchFitData(MemoryBatch batch, Bytes totalSize) final override;
    virtual bool updateBuffer(MemoryBatch batch, const Buffer& destinationBuffer, const std::vector<BufferCopy>& copies, const void* data) final override;
    virtual bool updateImage(MemoryBatch batch, const Image& image, const TextureData& data) final override;
    virtual bool beginBatchUpload(MemoryBatch batch) final override;
private:
    struct StagingBufferData {
        StagingBufferData() : currentOffset(0), maxRequestLastFrame(0), uploadCalls(0) {}
        
        Buffer buffer;
        CommandBuffer* commandBuffer;
        
        std::uint64_t currentOffset;
        std::uint64_t maxRequestLastFrame;
        std::uint32_t uploadCalls;
        MemoryBatch batch;
        
        inline bool hasDataThisFrame() const {
            return currentOffset != 0;
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
    
    bool initStagingBufferForBatch(MemoryBatch batch, Bytes size, bool firstCreation);
    void executeUpload(CommandBuffer* commandBuffer);
    void resetData(StagingBufferData& data);
    
    VulkanAPI* gfx;
    CommandPool* commandPool;
    FenceHnd uploadCompleteFence;
    std::array<StagingBufferData, StagingBufferCount> stagingBuffers;
    bool firstFrame;
};
}

