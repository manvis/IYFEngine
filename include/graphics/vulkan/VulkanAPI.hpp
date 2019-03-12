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

#ifndef VULKAN_BACKEND_HPP
#define VULKAN_BACKEND_HPP

#ifdef BUILD_WITH_VULKAN
#include <unordered_map>

#include "graphics/GraphicsAPI.hpp"
#include "graphics/vulkan/vk_mem_alloc.h"

#include <vulkan/vulkan.h>

namespace iyf {
class VulkanAPI;
class VulkanCommandPool : public CommandPool {
public:
    VulkanCommandPool(VulkanAPI* backend, VkCommandPool commandPool) : CommandPool(), commandPool(commandPool), backend(backend) {}
    
    virtual CommandBuffer* allocateCommandBuffer(const char* name, BufferLevel level = BufferLevel::Primary, bool beginBuffer = true) override;
    virtual std::vector<CommandBuffer*> allocateCommandBuffers(const std::vector<const char*>* names, std::uint32_t count, BufferLevel level = BufferLevel::Primary, bool beginBuffers = true) override;
    virtual void freeCommandBuffer(CommandBuffer* cmdBuf) override;
    virtual void freeCommandBuffers(const std::vector<CommandBuffer*>& cmdBuffs) override;
    
    VkCommandPool getVulkanCommandPool() const {
        return commandPool;
    }
protected:
    VkCommandPool commandPool;
    VulkanAPI* backend;
};

class VulkanCommandBuffer : public CommandBuffer {
public:
    friend class VulkanCommandPool;
    // TODO Hide? so that only a friendly command pool could create these
    //VulkanCommandBuffer(CommandPool* pool, VkCommandBuffer buffer, const VkPhysicalDeviceProperties& gpuProperties) : CommandBuffer(pool), cmdBuff(buffer) {
    VulkanCommandBuffer(BufferLevel level, VulkanAPI* backend, VkCommandBuffer buffer, const VkPhysicalDeviceProperties& gpuProperties) : CommandBuffer(level), backend(backend), cmdBuff(buffer), recording(false) {
        tempBuffers.resize(gpuProperties.limits.maxVertexInputBindings);
        tempOffsets.resize(gpuProperties.limits.maxVertexInputBindings);
    }
    
    virtual void setViewports(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) override;
    virtual void setScissors(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) override;
    
    virtual void setViewports(std::uint32_t first, std::uint32_t count, const Viewport* viewports) override;
    virtual void setScissors(std::uint32_t first, std::uint32_t count, const Rect2D* rectangles) override;
    
    virtual void setViewport(std::uint32_t first, const Viewport& viewport) override;
    virtual void setScissor(std::uint32_t first, const Rect2D& rectangle) override;
    
    virtual void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) override;
    virtual void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) override;
    
    virtual void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) override;
    
    virtual void bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<Buffer>& buffers) override;
    virtual void bindVertexBuffer(std::uint32_t firstBinding, const Buffer& buffer) override;
    virtual void bindIndexBuffer(const Buffer& buffer, IndexType indexType) override;
    
    virtual void pushConstants(PipelineLayoutHnd handle, ShaderStageFlags flags, std::uint32_t offset, std::uint32_t size, const void* data) override;
    virtual bool bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) override;
    virtual bool bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, std::uint32_t descriptorSetCount, const DescriptorSetHnd* descriptorSets, std::uint32_t dynamicOffsetCount, const std::uint32_t* dynamicOffsets) override;
    
    virtual void bindPipeline(const Pipeline& pipeline) override;
    
    virtual void begin(const CommandBufferBeginInfo& cbbi = CommandBufferBeginInfo()) override;
    virtual void end() override;
    
    virtual bool isRecording() const override {
        return recording;
    }
    
    virtual void beginRenderPass(const RenderPassBeginInfo& rpbi, SubpassContents contents = SubpassContents::Inline) override;
    virtual void nextSubpass(SubpassContents contents = SubpassContents::Inline) override;
    virtual void endRenderPass() override;
    
    virtual void copyImageToBuffer(const Image& srcImage, ImageLayout layout, const Buffer& dstBuffer, const std::vector<BufferImageCopy>& regions) final override;
    
    virtual CommandBufferHnd getHandle() override {
        return CommandBufferHnd(cmdBuff);
    }
protected:
    std::vector<VkBuffer> tempBuffers;
    std::vector<VkDeviceSize> tempOffsets;
    std::vector<VkViewport> tempViewports;
    std::vector<VkRect2D> tempScissors;
    
    VulkanAPI* backend;
    VkCommandBuffer cmdBuff;
    
    bool recording;
};

struct AllocationAndInfo {
    AllocationAndInfo(VmaAllocation allocation, VmaAllocationInfo info, VkMemoryPropertyFlags memoryFlags)
        : allocation(allocation), info(std::move(info)), memoryFlags(memoryFlags) {}
    
    VmaAllocation allocation;
    VmaAllocationInfo info;
    VkMemoryPropertyFlags memoryFlags;
};

struct VulkanDebugUserData {
    VulkanDebugUserData() : abortOnError(true) {}
    
    bool abortOnError;
};

class VulkanAPI : public GraphicsAPI {
public:
    friend class VulkanCommandPool;
    friend class VulkanCommandBuffer;
    
    /// Initializes the Vulkan API. Must be called before any other function
    /// \return if the initialization succeeded
    virtual bool initialize() override;
    
    virtual void dispose() override;
    
    virtual bool backendSupportsMultipleFramesInFlight() const override;
    
    virtual RenderPassHnd createRenderPass(const RenderPassCreateInfo& info, const char* name) override;
    virtual bool destroyRenderPass(RenderPassHnd handle) override;
    
    virtual bool startFrame() override;
    virtual bool endFrame() override;
    
    virtual CommandPool* createCommandPool(QueueType type, std::uint32_t queueId, const char* name) override;
    virtual bool destroyCommandPool(CommandPool* pool) override;
    
    virtual ShaderHnd createShader(ShaderStageFlags shaderStageFlag, const void* data, std::size_t byteCount, const char* name) override;
    virtual ShaderHnd createShaderFromSource(ShaderStageFlags shaderStageFlags, const std::string& source, const char* name) override;
    virtual bool destroyShader(ShaderHnd handle) override;
    
    virtual Pipeline createPipeline(const PipelineCreateInfo& info, const char* name) override;
    virtual Pipeline createPipeline(const ComputePipelineCreateInfo& info, const char* name) override;
    virtual bool destroyPipeline(const Pipeline& pipeline) override;
    
    virtual PipelineLayoutHnd createPipelineLayout(const PipelineLayoutCreateInfo& info, const char* name) override;
    virtual bool destroyPipelineLayout(PipelineLayoutHnd handle) override;
    virtual DescriptorSetLayoutHnd createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info, const char* name) override;
    virtual bool destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) override;
    virtual std::vector<DescriptorSetHnd> allocateDescriptorSets(const DescriptorSetAllocateInfo& info) override;
    virtual bool freeDescriptorSets(DescriptorPoolHnd handle, std::vector<DescriptorSetHnd>& sets) override;
    virtual bool updateDescriptorSets(const std::vector<WriteDescriptorSet>& set) override;
    virtual DescriptorPoolHnd createDescriptorPool(const DescriptorPoolCreateInfo& info, const char* name) override;
    virtual bool destroyDescriptorPool(DescriptorPoolHnd handle) override;
    
//    virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<FramebufferAttachmentCreateInfo>& info) override;
    virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<std::variant<Image, FramebufferAttachmentCreateInfo>>& info, const char* name) override;
    virtual void destroyFramebufferWithAttachments(const Framebuffer& framebuffer) override;
    
    virtual Image createImage(const ImageCreateInfo& info, const char* name) final override;
    virtual Image createUncompressedImage(const UncompressedImageCreateInfo& info, const char* name) override;
    virtual bool destroyImage(const Image& image) override;
    virtual SamplerHnd createSampler(const SamplerCreateInfo& info, const char* name) override;
    virtual bool destroySampler(SamplerHnd handle) override;
    virtual ImageViewHnd createImageView(const ImageViewCreateInfo& info, const char* name) override;
    virtual bool destroyImageView(ImageViewHnd handle) override;
    
    virtual Buffer createBuffer(const BufferCreateInfo& info, const char* name) final override;
    virtual bool destroyBuffer(const Buffer& buffer) final override;

    virtual bool readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) final override;
    
    virtual SemaphoreHnd createSemaphore(const char* name) override;
    virtual void destroySemaphore(SemaphoreHnd hnd) override;
    
    virtual FenceHnd createFence(bool createSignaled, const char* name) override;
    virtual void destroyFence(FenceHnd fence) override;
    virtual bool getFenceStatus(FenceHnd fence) override;
    virtual bool waitForFences(const std::vector<FenceHnd>& fences, bool waitForAll, std::uint64_t timeout) override;
    virtual bool waitForFence(FenceHnd fence, std::uint64_t timeout) override;
    virtual void resetFences(const std::vector<FenceHnd>& fences) override;
    virtual void resetFence(FenceHnd fence) override;
    
    // TODO >1 queue
    virtual void submitQueue(const SubmitInfo& info, FenceHnd fence = FenceHnd()) override;
    virtual void waitUntilDone() override {
        vkDeviceWaitIdle(logicalDevice.handle);
    }
    
    virtual void waitUntilFrameCompletes() override;
    
    //virtual VertexLayoutHnd createVertexLayout(const VertexLayoutCreateInfo& vlCreateInfo) override;
    //virtual void bindVertexLayout(VertexLayoutHnd handle) override;
    //virtual bool destroyVertexLayout(VertexLayoutHnd handle) override;
    
    //virtual void useDebug(bool status) override;
    virtual MultithreadingSupport doesBackendSupportMultithreading() override {
        return MultithreadingSupport::Full;
    }
    
    virtual bool exposesMultipleCommandBuffers() const override {
        return true;
    }
    
    virtual Format getSurfaceFormat() override;
    virtual Format getDepthStencilFormat() override;
    virtual std::uint32_t getCurrentSwapImage() const final override;
    virtual std::uint32_t getSwapImageCount() const final override;
    virtual const Image& getSwapImage(std::uint32_t id) const final override;
    
    virtual SemaphoreHnd getRenderCompleteSemaphore() override;
    virtual SemaphoreHnd getPresentationCompleteSemaphore() override;
    
    virtual glm::uvec2 getSwapchainImageSize() const final override {
        return glm::uvec2(swapchain.imageExtent.width, swapchain.imageExtent.height);
    }
    
    virtual ~VulkanAPI() { }
protected:
    friend class Engine;
    friend class VulkanDeviceMemoryManager;
    
    VulkanAPI(Engine* engine, bool useDebugAndValidation, Configuration* config);
    
    struct PhysicalDevice {
        VkPhysicalDevice handle;
        
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkPhysicalDeviceFeatures features;
        std::vector<VkExtensionProperties> extensionProperties;
        
        std::vector<const char*> enabledExtensions;
        
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        std::vector<std::uint32_t> presentCapableQueues;
        
        bool usingDedicatedComputeQueueFamily;
        bool usingDedicatedTransferQueueFamily;
        bool presentQueueFamilySeparateFromMain;
        
        std::uint32_t chosenMainQueueFamilyId;
        std::uint32_t chosenComputeQueueFamilyId;
        std::uint32_t chosenTransferQueueFamilyId;
        std::uint32_t chosenPresentQueueFamilyId;
        
        bool dedicatedAllocationExtensionEnabled;
        bool getMemoryRequirements2ExtensionEnabled;
        
        /// Capabailities of the surface that is compatible with this physical device
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        /// Present modes available for the surface that is compatible with this device
        std::vector<VkPresentModeKHR> presentModes;
        /// Formats available for the surface that is compatible with this device
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
    };
    
    struct LogicalDevice {
        VkDevice handle;
        
        VkQueue mainQueue;
        VkQueue presentQueue;
        VkQueue computeQueue;
        VkQueue transferQueue;
    };
    
    struct Swapchain {
        Swapchain() : handle(VK_NULL_HANDLE), version(0) {}
        
        VkSwapchainKHR handle;
        
        VkExtent2D imageExtent;
        VkFormat imageFormat;
        VkColorSpaceKHR imageColorSpace;
        
        bool mailboxAvailable;
        bool immediateAvailable;
        bool fifoAvailable;
        bool fifoRelaxedAvailable;
        
        std::uint32_t version;
        
        std::vector<VkImage> images;
        std::vector<Image> engineImages;
        std::vector<VkImageView> imageViews;
        
        void clearImageDataVectors() {
            images.clear();
            engineImages.clear();
            imageViews.clear();
        }
    };

    enum class LayerType {
        Instance, Device
    };
    
    enum class BufferType {
        Vertex, Index, Uniform, Storage
    };
    
    /// Names in std::strings
    std::vector<std::string> layerNamesSplit;
    /// c_str values retrieved from layerNamesSplit. validationLayerNames.data() is passed to actual Vulkan functions
    std::vector<const char*> validationLayerNames;
    
    VulkanDebugUserData debugUserData;
    
    VmaAllocator allocator;
    VkBuffer imageTransferSource;
    VmaAllocation imageTransferSourceAllocation;
    
    VkInstance instance;
    
    PhysicalDevice physicalDevice;
    LogicalDevice logicalDevice;
    VkSurfaceKHR surface;
    Swapchain swapchain;
    
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    VkCommandBuffer imageUploadCommandBuffer;
    VkPipelineCache pipelineCache;
    
    Format surfaceFormatEngine;
    VkFormat depthStencilFormat;
    Format depthStencilFormatEngine;
    
    std::uint32_t currentSwapBuffer;
    
    std::vector<VkCommandBuffer> prePresentationBarrierCommands;
    std::vector<VkCommandBuffer> postPresentationBarrierCommands;
    
    std::vector<VkSemaphore> presentationCompleteSemaphores;
    std::vector<VkSemaphore> renderingCompleteSemaphores;
    std::vector<VkFence> frameCompleteFences;
    
    // Swapchain functions
    PFN_vkCreateSwapchainKHR vkCreateSwapchain;
    PFN_vkDestroySwapchainKHR vkDestroySwapchain;
    PFN_vkGetSwapchainImagesKHR vkGetSwapchainImages;
    PFN_vkAcquireNextImageKHR vkAcquireNextImage;
    PFN_vkQueuePresentKHR vkQueuePresent;
    
    // DEBUG
    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger;
    PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger;
    PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectName;
    VkDebugUtilsMessengerEXT debugMessenger;
    
    void setObjectName(VkObjectType type, std::uint64_t handle, const char* name);
    
    void findLayers(LayerType mode, const std::vector<const char*>& expectedLayers);
    
    virtual BackendType getBackendType() override {
        return BackendType::Vulkan;
    }
    
    void createInstance();
    void createDebugCallback();
    void choosePhysicalDevice();
    bool evaluatePhysicalDeviceMemoryProperties(PhysicalDevice& device);
    bool evaluatePhysicalDeviceProperties(PhysicalDevice& device);
    bool evaluatePhysicalDeviceExtensions(PhysicalDevice& device);
    bool evaluatePhysicalDeviceFeatures(PhysicalDevice& device);
    bool evaluatePhysicalDeviceQueueFamilies(PhysicalDevice& device);
    bool evaluatePhysicalDeviceSurfaceCapabilities(PhysicalDevice& device);
    void createLogicalDevice();
    void createVulkanMemoryAllocatorAndHelperBuffers();
    void createSurface();
    void createSwapchain();
    void recreateSwapchain();
    void disposeOfSwapchainAndDependencies(const Swapchain& swapchain);
    VkSurfaceFormatKHR chooseSwapchainImageFormat();
    VkPresentModeKHR chooseSwapchainPresentMode();
    void createSwapchainImageViews();
    void chooseDepthStencilFormat();
    VkFence getCurrentFrameCompleteFence();
    
    void setupCommandPool();
    void setupPresentationBarrierCommandBuffers();
    void freePresentationBarrierCommandBuffers();
    
    //std::pair<VkBuffer, VkDeviceMemory> createBuffer(BufferType bufferType, std::uint64_t size, BufferUpdateFrequency flag, const void* data);
    std::pair<VkBuffer, VkDeviceMemory> createTemporaryBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, std::uint64_t size, const void* data);
    
    VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel bufferLevel, std::uint32_t bufferCount, bool beginBuffer);
    void freeCommandBuffer(const VkCommandBuffer& commandBuffer);
    
    std::uint32_t getMemoryType(std::uint32_t typeBits, VkFlags properyFlags);
    void setImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange imageSubresourceRange, VkPipelineStageFlags srcStageFlags, VkPipelineStageFlags destStageFlags);
    
    bool checkResult(VkResult result, const std::string& whatFailed, bool throwIfFailed = true);
    
    /// Allows us to retrieve various info about backing memory based on a buffer's handle
    std::unordered_map<VkBuffer, AllocationAndInfo> bufferToMemory;
    
    std::unordered_map<VkImage, AllocationAndInfo> imageToMemory;
    // Temporaries for conversions
    
    VkBufferUsageFlagBits mapBufferType(BufferType bufferType) const;
};
}
#endif //BUILD_WITH_VULKAN
#endif //VULKAN_BACKEND_HPP
