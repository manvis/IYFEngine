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

#ifndef IYF_GRAPHICS_API_HPP
#define IYF_GRAPHICS_API_HPP

#include <vector>
#include <utility>
#include <cstdint>
#include <variant>

#include "configuration/interfaces/Configurable.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/GraphicsAPIHandles.hpp"
#include "utilities/DataSizes.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/ForceInline.hpp"

#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

struct SDL_Window;

namespace iyf {
class TextureData;
class SwapchainChangeListener;

/// \todo What about queue families?
class BufferCreateInfo  {
public:
    BufferCreateInfo() :  size(0), flags(), memoryUsage(MemoryUsage::CPUOnly), frequentHostAccess(false) {}
    BufferCreateInfo(BufferUsageFlags flags, Bytes size, MemoryUsage memoryUsage, bool frequentHostAccess)
        : size(size), flags(flags), memoryUsage(memoryUsage), frequentHostAccess(frequentHostAccess) {}
    
    Bytes size;
    BufferUsageFlags flags;
    MemoryUsage memoryUsage;
public:
    /// If memoryUsage is MemoryUsage::GPUOnly and this bool is set, the GraphicsAPI will try to search for a memory heap that's both
    /// on device and CPU mappable. E.g., AMD GPUs have a 256 MB heap of such memory. It's quite useful for storing per-frame data
    /// because it avoids staging buffer copies.
    bool frequentHostAccess;
};

class Buffer {
public:
    Buffer() : bufferHandle(nullptr), bufferSize(0), info(nullptr), bufferUsage(), memUsage(MemoryUsage::CPUOnly) {}
    Buffer(BufferHnd handle, BufferUsageFlags flags, MemoryUsage memoryUsage, Bytes size, void* allocationInfo) 
        : bufferHandle(handle), bufferSize(size), info(allocationInfo), bufferUsage(flags), memUsage(memoryUsage) {}
    
    inline BufferHnd handle() const {
        return bufferHandle;
    }
    
    inline BufferUsageFlags bufferUsageFlags() const {
        return bufferUsage;
    }
    
    inline MemoryUsage memoryUsage() const {
        return memUsage;
    }
    
    /// Returns the actual size of the buffer.
    /// \warning This value MAY end up slightly bigger than what was supplied to BufferCreateInfo during buffer creation because of memory alignment requirements
    inline Bytes size() const {
        return bufferSize;
    }

    /// Returns a pointer to GraphicsAPI specific allocation info. Used internally.
    inline const void* allocationInfo() const {
        return info;
    }
protected:
    BufferHnd bufferHandle;
    Bytes bufferSize;
    void* info;
    BufferUsageFlags bufferUsage;
    MemoryUsage memUsage;
};

class ImageSubresourceLayers {
public:
    ImageSubresourceLayers() : aspectMask(ImageAspectFlagBits::Color), mipLevel(0), baseArrayLayer(0), layerCount(1) {}
    ImageSubresourceLayers(ImageAspectFlags mask, std::uint32_t mipLevel, std::uint32_t baseArrayLayer, std::uint32_t layerCount) 
        : aspectMask(mask), mipLevel(mipLevel), baseArrayLayer(baseArrayLayer), layerCount(layerCount) {}
    
    ImageAspectFlags aspectMask;
    std::uint32_t mipLevel;
    std::uint32_t baseArrayLayer;
    std::uint32_t layerCount;
};

class BufferImageCopy {
public:
    std::uint64_t bufferOffset;
    std::uint32_t bufferRowLength;
    std::uint32_t bufferImageHeight;
    ImageSubresourceLayers imageSubresource;
    glm::ivec3 imageOffset;
    glm::uvec3 imageExtent;
};

class BufferCopy {
public:
    BufferCopy() : srcOffset(0), dstOffset(0), size(0) {}
    BufferCopy(std::uint64_t srcOffset, std::uint64_t dstOffset, std::uint64_t size) : srcOffset(srcOffset), dstOffset(dstOffset), size(size) {}
            
    std::uint64_t srcOffset;
    std::uint64_t dstOffset;
    std::uint64_t size;
};

class BufferSubSlice {
public:
    BufferSubSlice() : dataOffset(0), dataSize(0) { }
    BufferSubSlice(std::uint64_t offset, std::uint64_t size) : dataOffset(offset), dataSize(size) { }
    
    inline std::uint64_t offset() const {
        return dataOffset;
    }
    
    inline std::uint64_t size() const {
        return dataSize;
    }
private:
    std::uint64_t dataOffset;
    std::uint64_t dataSize;
};

class Image {
public:
    Image() : handle(nullptr), extent(0, 0, 0), mipLevels(0), arrayLayers(0), info(nullptr), format(Format::Undefined), type(ImageViewType::Im1D) {}
    Image(ImageHnd handle, glm::uvec3 extent, std::uint32_t mipLevels, std::uint32_t arrayLayers, ImageUsageFlags usage, Format format, ImageViewType type, void* info) 
        : handle(handle), extent(std::move(extent)), mipLevels(mipLevels), arrayLayers(arrayLayers), usage(usage), info(info), format(format), type(type) {}
    
    bool isValid() const {
        return (handle.isValid()) && (format != Format::Undefined);
    }
    
    inline const glm::uvec3& getExtent() const {
        return extent;
    }
    
    inline std::uint32_t getMipLevels() const {
        return mipLevels;
    }
    
    inline std::uint32_t getArrayLayers() const {
        return arrayLayers;
    }
    
    inline ImageUsageFlags getUsage() const {
        return usage;
    }
    
    inline Format getFormat() const {
        return format;
    }
    
    inline ImageViewType getType() const {
        return type;
    }
    
    inline ImageHnd getHandle() const {
        return handle;
    }
    
    /// Returns a pointer to GraphicsAPI specific allocation info. Used internally.
    inline const void* allocationInfo() const {
        return info;
    }
private:
    ImageHnd handle;
    glm::uvec3 extent;
    std::uint32_t mipLevels;
    std::uint32_t arrayLayers;
    
    ImageUsageFlags usage;
    
    void* info;
    
    /// The data format of the Image you want to create.
    Format format;
    
    ImageViewType type;
};

enum class MemoryBatch {
    MeshAssetData = 0, ///< Used to upload asynchronously loaded mesh asset data.
    TextureAssetData = 1, ///< Used to upload asynchronously loaded texture asset data.
    PerFrameData = 2, ///< Used to upload per frame data, e.g., uniform buffers storing matrices.
    COUNT,
    Instant = COUNT, ///< Not batched, executes the upload immediately. If needed, this will create a temporary staging buffer.
};

class ImageCreateInfo {
public:
    ImageCreateInfo() : extent(0, 0, 0), mipLevels(0), arrayLayers(0), format(Format::Undefined), isCube(false) {}
    ImageCreateInfo(glm::uvec3 extent, std::uint32_t mipLevels, std::uint32_t arrayLayers, ImageUsageFlags usage, Format format, bool isCube)
        : extent(std::move(extent)), mipLevels(mipLevels), arrayLayers(arrayLayers), usage(usage), format(format), isCube(isCube) {}
    
    glm::uvec3 extent;
    std::uint32_t mipLevels;
    std::uint32_t arrayLayers;
    
    ImageUsageFlags usage;
    
    /// The data format of the Image you want to create.
    Format format;
    
    /// If this is true, the Image will be a cubemap. If false, it's going to be a regular 2D Image.
    bool isCube;
};


/// Handles all data uploads to the GPU
///
/// \remark The DeviceMemoryManager is NOT thread safe an should only be called from the main thread.
class DeviceMemoryManager {
public:
    /// \brief Creates a new DeviceMemoryManager.
    ///
    /// \remark You shouldn't create a DeviceMemoryManager but use GraphicsAPI::getDeviceMemoryManager() instead.
    ///
    /// \param stagingBufferSizes The initial sizes of the staging buffers. Each element corresponds to a MemoryBatch enum value. All values must be
    /// set and > 0. The implementation is allowed to combine MemoryBatch::MeshAssetData and MemoryBatch::extureAssetData staging buffers into one.
    DeviceMemoryManager(std::vector<Bytes> stagingBufferSizes);
    
    virtual void initialize() = 0;
    virtual void initializeAllocator() = 0;
    virtual void dispose() = 0;
    virtual void disposeAllocator() = 0;
    
    virtual ~DeviceMemoryManager() {}
    
    /// Mark the start of a new frame.
    virtual void beginFrame() = 0;
    
    /// Checks if updateBuffer() will need to use a staging buffer in order to upload data to the device.
    virtual bool isStagingBufferNeeded(const Buffer& destinationBuffer) = 0;
    
    virtual Bytes computeUploadSize(const std::vector<BufferCopy>& copies) const {
        Bytes totalSize(0);
        
        for (const BufferCopy& bc : copies) {
            totalSize += Bytes(bc.size);
        }
        
        return totalSize;
    }
    
    /// Checks if the staging buffer assigned to the batch can fit the data you want to upload.
    ///
    /// \todo In some cases, e.g., inside the MeshTypeManager, I can't know the destination buffer in advance. Because of that reason, I can't call
    /// isStagingBufferNeeded() to figure out if one is needed or not. This may cause this function to return false negatives. I need to measure
    /// the inpact and fix it if it's big enough.
    ///
    /// \remark You should use computeUploadSize() to compute the totalSize parameter
    virtual bool canBatchFitData(MemoryBatch batch, Bytes totalSize) = 0;
    
    /// Updates the data in the specified buffer. Transparently handles staging and batches data uploads.
    ///
    /// \warning For performance reasons, this function assumes canBatchFitData() has already been called and returned true.
    virtual bool updateBuffer(MemoryBatch batch, const Buffer& destinationBuffer, const std::vector<BufferCopy>& copies, const void* data) = 0;
    
    /// Starts any pending uploads.
    ///
    /// \warning A MemoryBatch can only be used once per frame. MemoryBatch::Instant must never be used when calling this function.
    virtual bool beginBatchUpload(MemoryBatch batch) = 0;
    
    inline const std::vector<Bytes>& getStagingBufferSizes() const {
        return stagingBufferSizes;
    }
    
    /// Send new Image data to device memory from the provided TextureData object that should have been filled with data by the TextureLoader.
    ///
    /// Transparently handles staging and batches data uploads.
    virtual bool updateImage(MemoryBatch batch, const Image& image, const TextureData& data) = 0;
    
    virtual Buffer createBuffer(const BufferCreateInfo& info, const char* name) = 0;
    virtual bool destroyBuffer(const Buffer& buffer) = 0;
    virtual bool readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) = 0;
    virtual Image createImage(const ImageCreateInfo& info, const char* name) = 0;
    virtual bool destroyImage(const Image& image) = 0;
protected:
    std::vector<Bytes> stagingBufferSizes;
};

class Viewport {
public:
    Viewport() : x(0.0f), y(0.0f), width(100.0f), height(100.0f), minDepth(0.0f), maxDepth(1.0f) { }
    Viewport(float x, float y, float width, float height, float minDepth, float maxDepth) : x(x), y(y), width(width), height(height), minDepth(minDepth), maxDepth(maxDepth) { }
    
    float x;
    float y;
    float width;
    float height;
    float minDepth;
    float maxDepth;
};

class Rect2D {
public:
    Rect2D() : offset(-1, -1), extent(1, 1) { }
    Rect2D(const glm::vec2& offset, const glm::vec2& extent) : offset(offset), extent(extent) { }
    
    glm::ivec2 offset;
    glm::uvec2 extent;
    static_assert(sizeof(glm::ivec2) == 8 && sizeof(glm::uvec2) == 8, "vector components aren't 32 bits");
};

class Pipeline {
public:
    PipelineHnd handle;
    PipelineBindPoint bindPoint;
};

class ColorBlendAttachmentState {
public:
    ColorBlendAttachmentState() : blendEnable(false), srcColorBlendFactor(BlendFactor::One), dstColorBlendFactor(BlendFactor::Zero), srcAlphaBlendFactor(BlendFactor::One), 
        dstAlphaBlendFactor(BlendFactor::Zero), colorBlendOp(BlendOp::Add), alphaBlendOp(BlendOp::Add), 
        colorWriteMask(ColorWriteMaskFlagBits::Red | ColorWriteMaskFlagBits::Green | ColorWriteMaskFlagBits::Blue | ColorWriteMaskFlagBits::Alpha) {}
        
    bool blendEnable;
    BlendFactor srcColorBlendFactor;
    BlendFactor dstColorBlendFactor;
    BlendFactor srcAlphaBlendFactor;
    BlendFactor dstAlphaBlendFactor;
    BlendOp colorBlendOp;
    BlendOp alphaBlendOp;
    ColorWriteMaskFlags colorWriteMask;
};

class ColorBlendStateCreateInfo {
public:
    ColorBlendStateCreateInfo() : logicOpEnable(false), logicOp(LogicOp::Copy), attachments({ColorBlendAttachmentState()}) {}

    bool logicOpEnable;
    LogicOp logicOp;
    std::vector<ColorBlendAttachmentState> attachments;
    glm::vec4 blendConstants;
};

class RasterizationStateCreateInfo {
public:
    RasterizationStateCreateInfo() : depthClampEnable(false), rasterizerDiscardEnable(false), polygonMode(PolygonMode::Fill), cullMode(CullModeFlagBits::Back), frontFace(FrontFace::CounterClockwise), depthBiasEnable(false), depthBiasConstantFactor(0.0f), depthBiasClamp(0.0f), depthBiasSlopeFactor(0.0f), lineWidth(1.0f)  {}

    bool depthClampEnable;
    bool rasterizerDiscardEnable;
    PolygonMode polygonMode;
    CullModeFlags cullMode;
    FrontFace frontFace;
    bool depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasClamp;
    float depthBiasSlopeFactor;
    float lineWidth; 
};

class StencilOpState {
public:
    StencilOpState() : failOp(StencilOp::Keep), passOp(StencilOp::Keep), depthFailOp(StencilOp::Keep), compareOp(CompareOp::Always), compareMask(-1), writeMask(-1), reference(0) {}

    StencilOp failOp;
    StencilOp passOp;
    StencilOp depthFailOp;
    CompareOp compareOp;
    std::uint32_t compareMask;
    std::uint32_t writeMask;
    std::uint32_t reference;
};

// TODO stencil state
// Gal naudoti šitą?
// https://www.opengl.org/wiki/Early_Fragment_Test
// layout(early_fragment_tests) in;
class DepthStencilStateCreateInfo {
public:
    DepthStencilStateCreateInfo() : depthTestEnable(true), depthWriteEnable(true), depthCompareOp(CompareOp::Less), depthBoundsTestEnable(false), stencilTestEnable(false), minDepthBounds(0.0f), maxDepthBounds(1.0f) { }

    bool depthTestEnable;
    bool depthWriteEnable;
    CompareOp depthCompareOp;
    bool depthBoundsTestEnable;
    bool stencilTestEnable;
    StencilOpState front;
    StencilOpState back;
    float minDepthBounds;
    float maxDepthBounds;
};

class ViewportStateCreateInfo {
public:
    ViewportStateCreateInfo() : viewports(), scissors() {}

    std::vector<Viewport> viewports;
    std::vector<Rect2D> scissors;
};

using SampleMask = std::uint32_t;

class MultisampleStateCreateInfo {
public:
    MultisampleStateCreateInfo() : rasterizationSamples(SampleCountFlagBits::X1), sampleShadingEnable(false), minSampleShading(1.0f), sampleMask(), alphaToCoverageEnable(false), alphaToOneEnable(false) {}

    SampleCountFlags rasterizationSamples;
    bool sampleShadingEnable;
    float minSampleShading;
    std::vector<SampleMask> sampleMask;
    bool alphaToCoverageEnable;
    bool alphaToOneEnable;
};

class TessellationStateCreateInfo {
public:
    TessellationStateCreateInfo() : patchControlPoints(1) { } // TODO 1 gerai, ar ne kaip numatytas parametras

    std::uint32_t patchControlPoints;
};
//TODO constructors to all new 
class DynamicStateCreateInfo {
public:
    std::vector<DynamicState> dynamicStates;
};

class InputAssemblyStateCreateInfo {
public:
    InputAssemblyStateCreateInfo() : topology(PrimitiveTopology::TriangleList), primitiveRestartEnable(false) {}

    PrimitiveTopology topology;
    bool primitiveRestartEnable;
};

class VertexInputBindingDescription {
public:
    VertexInputBindingDescription(std::uint32_t binding, std::uint32_t stride, VertexInputRate inputRate) : binding(binding), stride(stride), inputRate(inputRate) {}

    std::uint32_t binding;
    std::uint32_t stride;
    VertexInputRate inputRate;
};

class VertexInputAttributeDescription {
public:
    VertexInputAttributeDescription(std::uint32_t location, std::uint32_t binding, Format format, std::uint32_t offset) : location(location), binding(binding), format(format), offset(offset) {}
    
    std::uint32_t location;
    std::uint32_t binding;
    Format format;
    std::uint32_t offset;
};

class VertexInputStateCreateInfo {
public:
    std::vector<VertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VertexInputAttributeDescription> vertexAttributeDescriptions;
};

class SamplerCreateInfo {
public:
    SamplerCreateInfo() : magFilter(Filter::Nearest), minFilter(Filter::Nearest), mipmapMode(SamplerMipmapMode::Nearest), addressModeU(SamplerAddressMode::Repeat), addressModeV(SamplerAddressMode::Repeat), addressModeW(SamplerAddressMode::Repeat), mipLodBias(0.0f), anisotropyEnable(false), maxAnisotropy(1.0f), compareEnable(false), compareOp(CompareOp::Never), minLod(0.0f), maxLod(0.0f), borderColor(BorderColor::IntOpaqueWhite), unnormalizedCoordinates(false) {}
    
    Filter magFilter;
    Filter minFilter;
    SamplerMipmapMode mipmapMode;
    SamplerAddressMode addressModeU;
    SamplerAddressMode addressModeV;
    SamplerAddressMode addressModeW;
    float mipLodBias;
    bool anisotropyEnable;
    float maxAnisotropy;
    bool compareEnable;
    CompareOp compareOp;
    float minLod;
    float maxLod;
    BorderColor borderColor;
    bool unnormalizedCoordinates;
};

class ComponentMapping {
public:
    ComponentMapping() : r(ComponentSwizzle::Identity), g(ComponentSwizzle::Identity), b(ComponentSwizzle::Identity), a(ComponentSwizzle::Identity) {}
    ComponentMapping(ComponentSwizzle r, ComponentSwizzle g, ComponentSwizzle b, ComponentSwizzle a) : r(r), g(g), b(b), a(a) {}
    
    ComponentSwizzle r;
    ComponentSwizzle g;
    ComponentSwizzle b;
    ComponentSwizzle a;
};

class ImageSubresourceRange {
public:
    ImageSubresourceRange() : aspectMask(ImageAspectFlagBits::Color), baseMipLevel(0), levelCount(1), baseArrayLayer(0), layerCount(1) {}
    ImageSubresourceRange(ImageAspectFlags mask, std::uint32_t baseMipLevel, std::uint32_t levelCount, std::uint32_t baseArrayLayer, std::uint32_t layerCount) : aspectMask(mask), baseMipLevel(baseMipLevel), levelCount(levelCount), baseArrayLayer(baseArrayLayer), layerCount(layerCount) {}
    
    ImageAspectFlags aspectMask;
    std::uint32_t baseMipLevel;
    std::uint32_t levelCount;
    std::uint32_t baseArrayLayer;
    std::uint32_t layerCount;
};

class ImageViewCreateInfo {
public:
    ImageHnd image;
    ImageViewType viewType;
    Format format;
    ComponentMapping components;
    ImageSubresourceRange subresourceRange;
};

class DescriptorSetAllocateInfo {
public:
    DescriptorSetAllocateInfo() : descriptorPool(0) {}
    DescriptorSetAllocateInfo(DescriptorPoolHnd pool, std::vector<DescriptorSetLayoutHnd> layouts) : descriptorPool(pool), setLayouts(layouts) {}
    
    DescriptorPoolHnd descriptorPool;
    std::vector<DescriptorSetLayoutHnd> setLayouts;
};

class DescriptorImageInfo {
public:
    SamplerHnd sampler;
    ImageViewHnd imageView;
    ImageLayout imageLayout;
};

class DescriptorBufferInfo {
public:
    BufferHnd buffer;
    std::uint64_t offset;
    std::uint64_t range;
};

class WriteDescriptorSet {
public:
    DescriptorSetHnd dstSet;
    std::uint32_t dstBinding;
    std::uint32_t dstArrayElement;
    std::uint32_t descriptorCount;
    DescriptorType descriptorType;
    std::vector<DescriptorImageInfo> imageInfos;
    std::vector<DescriptorBufferInfo> bufferInfos;
    std::vector<BufferViewHnd> texelBufferViews;
};

class DescriptorPoolSize {
public:
    DescriptorType type;
    std::uint32_t descriptorCount;
};

class DescriptorPoolCreateInfo {
public:
    std::uint32_t maxSets;
    std::vector<DescriptorPoolSize> poolSizes;
};

class DescriptorSetLayoutBinding {
public:
    std::uint32_t binding;
    DescriptorType descriptorType;
    std::uint32_t descriptorCount;
    ShaderStageFlags stageFlags;
    std::vector<SamplerHnd> immutableSamplers;
};

class DescriptorSetLayoutCreateInfo {
public:
    std::vector<DescriptorSetLayoutBinding> bindings;
};

class PushConstantRange {
public:
    ShaderStageFlags stageFlags;
    std::uint32_t offset;
    std::uint32_t size;
};

class PipelineLayoutCreateInfo {
public:
    std::vector<DescriptorSetLayoutHnd> setLayouts;
    std::vector<PushConstantRange> pushConstantRanges;
};

using PipelineShadersInfo = std::pair<ShaderStageFlags, ShaderHnd>;

class PipelineCreateInfo {
public:
    PipelineCreateInfo() : layout(0), renderPass(0), subpass(0) {}
    
    //TODO CreateFlags, base pipeline...
    std::vector<PipelineShadersInfo> shaders;
    VertexInputStateCreateInfo vertexInputState;
    InputAssemblyStateCreateInfo inputAssemblyState;
    TessellationStateCreateInfo tessellationState;
    ViewportStateCreateInfo viewportState;
    RasterizationStateCreateInfo rasterizationState;
    MultisampleStateCreateInfo multisampleState;
    DepthStencilStateCreateInfo depthStencilState;
    ColorBlendStateCreateInfo colorBlendState;
    DynamicStateCreateInfo dynamicState;
    PipelineLayoutHnd layout;
    RenderPassHnd renderPass;
    std::uint32_t subpass;
};

class ComputePipelineCreateInfo {
public:
    ComputePipelineCreateInfo() {}
    // TODO flags, base pipeline
    PipelineShadersInfo shader;
    PipelineLayoutHnd layout;
};

class CommandBufferInheritanceInfo {
public:
    CommandBufferInheritanceInfo() : renderPass(0), subpass(0), framebuffer(0) {}
    RenderPassHnd renderPass;
    std::uint32_t subpass;
    FramebufferHnd framebuffer;
    // TODO pridėti šituos, kai (jei) reikės
//    bool occlusionQueryEnable;
//    QueryControlFlags queryFlags;
//    QueryPipelineStatisticFlags pipelineStatistics;
};

class CommandBufferBeginInfo {
public:
    CommandBufferBeginInfo() : flags() {}
    CommandBufferUsageFlags flags;
    CommandBufferInheritanceInfo inheritanceInfo;
};

class ClearColorValue {
public:
    ClearColorValue() = default;
    
    ClearColorValue(float r, float g, float b, float a) : type(Type::Float) {
        val.float32[0] = r;
        val.float32[1] = g;
        val.float32[2] = b;
        val.float32[3] = a;
    }
    
    ClearColorValue(std::int32_t r, std::int32_t g, std::int32_t b, std::int32_t a, bool unisgned) {
        if (unisgned) {
            type = Type::UInt32;
            val.uint32[0] = r;
            val.uint32[1] = g;
            val.uint32[2] = b;
            val.uint32[3] = a;
        } else {
            type = Type::Int32;
            val.int32[0] = r;
            val.int32[1] = g;
            val.int32[2] = b;
            val.int32[3] = a;
        }
    }
    
    enum class Type {
        Float = 0, Int32 = 1, UInt32 = 2
    };

    union Value {
        float float32[4];
        std::int32_t int32[4];
        std::uint32_t uint32[4];
    };
    
    inline Value getValue() const {
        return val;
    }
    
    inline Type getType() const {
        return type;
    }
    
private:
    Value val;
    Type type;
};

class ClearDepthStencilValue {
public:
    ClearDepthStencilValue() = default;
    ClearDepthStencilValue(float depth, std::uint32_t stencil) : depth(depth), stencil(stencil) {}
    
    float depth;
    std::uint32_t stencil;
};

class ClearValue {
public:
    ClearValue(ClearColorValue color) : type(Type::Color) {
        val.color = color;
    }
    
    ClearValue(ClearDepthStencilValue depthStencil) : type(Type::DepthStencil) {
        val.depthStencil = depthStencil;
    }
    
    union Value {
        ClearColorValue color;
        ClearDepthStencilValue depthStencil;
    };
    
    enum class Type {
        Color, DepthStencil
    };
    
    inline Value getValue() const {
        return val;
    }
    
    inline Type getType() const {
        return type;
    }
    
private:
    Value val;
    Type type;
};

class RenderPassBeginInfo {
public:
    RenderPassHnd renderPass;
    FramebufferHnd framebuffer;
    Rect2D renderArea;
    std::vector<ClearValue> clearValues;
};

class SubmitInfo {
public:
    std::vector<SemaphoreHnd> waitSemaphores;
    std::vector<PipelineStageFlags> waitDstStageMask;
    std::vector<CommandBufferHnd> commandBuffers;
    std::vector<SemaphoreHnd> signalSemaphores;
};

class AttachmentDescription {
public:
    AttachmentDescriptionFlags flags;
    Format format;
    SampleCountFlags samples;
    AttachmentLoadOp loadOp;
    AttachmentStoreOp storeOp;
    AttachmentLoadOp stencilLoadOp;
    AttachmentStoreOp stencilStoreOp;
    ImageLayout initialLayout;
    ImageLayout finalLayout;
};

class AttachmentReference {
public:
    /// \warning ~0U is important. It matches VK_ATTACHMENT_UNUSED
    AttachmentReference() : attachment(~0U), layout(ImageLayout::Undefined) {}
    AttachmentReference(std::uint32_t attachment, ImageLayout layout) : attachment(attachment), layout(layout) {}

    std::uint32_t attachment;
    ImageLayout layout;
};

class SubpassDescription {
public:
    PipelineBindPoint pipelineBindPoint;
    std::vector<AttachmentReference> inputAttachments;
    std::vector<AttachmentReference> colorAttachments;
    std::vector<AttachmentReference> resolveAttachments;
    AttachmentReference depthStencilAttachment;
    std::vector<std::uint32_t> preserveAttachments;
//    bool usesDepth = true; // TODO optional or sth
};

class SubpassDependency {
public:
    //TODO constructor with good defaults SubpassDependency() : srcSubpass(0), dstSubpass(0) {}
    std::uint32_t srcSubpass;
    std::uint32_t dstSubpass;
    PipelineStageFlags srcStageMask;
    PipelineStageFlags dstStageMask;
    AccessFlags srcAccessMask;
    AccessFlags dstAccessMask;
    DependencyFlags dependencyFlags;
};

class RenderPassCreateInfo {
public:
    std::vector<AttachmentDescription> attachments;
    std::vector<SubpassDescription> subpasses;
    std::vector<SubpassDependency> dependencies;
};

class UncompressedImageCreateInfo {
public:
    UncompressedImageCreateInfo() 
        : type(ImageMemoryType::RGBA), dimensions(100, 100), isWritable(false), usedAsColorOrDepthAttachment(false),
          usedAsInputAttachment(false), usedAsTransferSource(false), data(nullptr) {}
    
    ImageMemoryType type;
    glm::uvec2 dimensions;
    bool isWritable;
    bool usedAsColorOrDepthAttachment;
    bool usedAsInputAttachment;
    bool usedAsTransferSource;
    void* data;
};

class FramebufferAttachmentCreateInfo {
public:
    Format format;
    bool isAttachment;
};

class FramebufferCreateInfo {
    
};

class Framebuffer {
public://TODO sizes
    std::vector<Image> images;
    std::vector<ImageViewHnd> imageViews;
    std::vector<bool> isImageOwned;
    FramebufferHnd handle;
};

struct AuxiliaryMesh {
    std::uint32_t vboOffset, vboCount, iboOffset, iboCount;
};

struct AuxiliaryMeshData {
    Buffer vbo;
    Buffer ibo;
    
    AuxiliaryMesh sphereLowRes;
    AuxiliaryMesh sphereHighRes;
    AuxiliaryMesh cube;
    AuxiliaryMesh arrowX;
    AuxiliaryMesh arrowY;
    AuxiliaryMesh arrowZ;
    AuxiliaryMesh fullScreenQuad;
    //const std::uint32_t LowResSphereVertexOffset = 0;
    //const std::uint32_t LowResSphereVertexCount = 720 / 3;
    //const std::uint32_t HighResSphereVertexOffset = 720 / 3;
    //const std::uint32_t HighResSphereVertexCount = 8640 / 3;
};

class CommandBuffer : private NonCopyable {
public:
    //CommandBuffer(CommandPool* pool) : pool(pool) {}
    CommandBuffer(BufferLevel level) : level(level) {}//valid(false) {}
    
    virtual void setViewports(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) = 0;
    virtual void setScissors(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) = 0;
    
    virtual void setViewports(std::uint32_t first, std::uint32_t count, const Viewport* viewports) = 0;
    virtual void setScissors(std::uint32_t first, std::uint32_t count, const Rect2D* rectangles) = 0;
    
    virtual void setViewport(std::uint32_t first, const Viewport& viewport) = 0;
    virtual void setScissor(std::uint32_t first, const Rect2D& rectangle) = 0;
    
    virtual void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) = 0;
    virtual void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) = 0;
    
    virtual void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) = 0;
    
    virtual void bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<Buffer>& buffers) = 0;//TODO offsets
    virtual void bindVertexBuffer(std::uint32_t firstBinding, const Buffer& buffer) = 0;
    virtual void bindIndexBuffer(const Buffer& buffer, IndexType indexType) = 0;
    
    virtual void pushConstants(PipelineLayoutHnd handle, ShaderStageFlags flags, std::uint32_t offset, std::uint32_t size, const void* data) = 0;
    virtual bool bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) = 0;
    virtual bool bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, std::uint32_t descriptorSetCount, const DescriptorSetHnd* descriptorSets, std::uint32_t dynamicOffsetCount, const std::uint32_t* dynamicOffsets) = 0;
    
    virtual void bindPipeline(const Pipeline& pipeline) = 0;
    
    virtual void begin(const CommandBufferBeginInfo& cbbi = CommandBufferBeginInfo()) = 0;
    virtual void end() = 0;
    
    virtual bool isRecording() const = 0;
    
    virtual void beginRenderPass(const RenderPassBeginInfo& rpbi, SubpassContents contents = SubpassContents::Inline) = 0;
    virtual void nextSubpass(SubpassContents contents = SubpassContents::Inline) = 0;
    virtual void endRenderPass() = 0;
    
    virtual void copyImageToBuffer(const Image& srcImage, ImageLayout layout, const Buffer& dstBuffer, const std::vector<BufferImageCopy>& regions) = 0;
    
    virtual CommandBufferHnd getHandle() = 0;
//    static void beginAll(const std::vector<CommandBuffer*> cmdBuffs) {
//        
//    }
//    virtual bool isValid() const final {
//        return valid;
//    }
    
    virtual ~CommandBuffer() {}
protected:
    BufferLevel level;
//private:
//    void setValid(bool isValid) {
//        valid = isValid;
//    }
//    bool valid;
    //CommandPool* pool;
    // TODO begin self, begin render pass
};

class CommandPool : private NonCopyable {
public:
    CommandPool() {}
    
    virtual CommandBuffer* allocateCommandBuffer(const char* name, BufferLevel level = BufferLevel::Primary, bool beginBuffer = true) = 0;
    virtual std::vector<CommandBuffer*> allocateCommandBuffers(const std::vector<const char*>* names, std::uint32_t count, BufferLevel level = BufferLevel::Primary, bool beginBuffer = false) = 0;
    virtual void freeCommandBuffer(CommandBuffer* cmdBuf) = 0;
    virtual void freeCommandBuffers(const std::vector<CommandBuffer*>& cmdBuffs) = 0;
    
    virtual ~CommandPool() {}
};

class Engine;
// TODO throw exception instead of returning false.
// TODO stop using so many vectors create other methods that would support arrays or data + size 
// TODO implement the said DeviceMemoryUpdateBatcher
class GraphicsAPI : public Configurable, private NonCopyable {
public:
    GraphicsAPI(Engine* engine, bool useDebugAndValidation, Configuration* config);
    
    /// \warning openWindow() should be the very first call in the initialize() implementations because it sets up some really important data.
    virtual bool initialize() = 0;
    virtual void dispose() = 0;
    
    virtual bool backendSupportsMultipleFramesInFlight() const = 0;
    
    /// \warning This is used for debugging. You probably want to create getSwapImageCount() resources
    /// and access getCurrentSwapImage() when rendering
    inline std::size_t getMaxFramesInFlight() const {
        return maxFramesInFlight;
    }
    
    /// \warning This is used for debugging. You probably want to create getSwapImageCount() resources
    /// and access getCurrentSwapImage() when rendering
    inline std::size_t getCurrentFrameInFlight() const {
        return currentFrameInFlight;
    }
    
    bool isInitialized() const {
        return isInit;
    }
    
    void addSwapchainChangeListener(SwapchainChangeListener* listener);
    void removeSwapchainChangeListener(SwapchainChangeListener* listener);
    
    virtual DeviceMemoryManager* getDeviceMemoryManager() const;

    virtual RenderPassHnd createRenderPass(const RenderPassCreateInfo& info, const char* name) = 0;
    virtual bool destroyRenderPass(RenderPassHnd handle) = 0;
    
    virtual bool startFrame() = 0;
    virtual bool endFrame() = 0;
    
    virtual CommandPool* createCommandPool(QueueType type, std::uint32_t queueId, const char* name) = 0;
    virtual bool destroyCommandPool(CommandPool* pool) = 0;
    
    /// \brief Create a shader from bytecode.
    ///
    /// On APIs that don't support bytecode, this function needs to be passed a source code string and its length.
    virtual ShaderHnd createShader(ShaderStageFlags shaderStageFlag, const void* data, std::size_t byteCount, const char* name) = 0;
    /// \brief Create a shader from a source code string.
    ///
    /// \throws std::runtime_error if the API does not support loading shaders from source code.
    virtual ShaderHnd createShaderFromSource(ShaderStageFlags shaderStageFlag, const std::string& source, const char* name) = 0;
    virtual bool destroyShader(ShaderHnd handle) = 0;
    
    virtual Pipeline createPipeline(const PipelineCreateInfo& info, const char* name) = 0;
    virtual Pipeline createPipeline(const ComputePipelineCreateInfo& info, const char* name) = 0;
    virtual bool destroyPipeline(const Pipeline& pipeline) = 0;
    
    virtual PipelineLayoutHnd createPipelineLayout(const PipelineLayoutCreateInfo& info, const char* name) = 0;
    virtual bool destroyPipelineLayout(PipelineLayoutHnd handle) = 0;
    virtual DescriptorSetLayoutHnd createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info, const char* name) = 0;
    virtual bool destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) = 0;
    
    virtual std::vector<DescriptorSetHnd> allocateDescriptorSets(const DescriptorSetAllocateInfo& info) = 0;
    virtual bool updateDescriptorSets(const std::vector<WriteDescriptorSet>& sets) = 0;
    virtual bool freeDescriptorSets(DescriptorPoolHnd handle, std::vector<DescriptorSetHnd>& sets) = 0;
    
    virtual DescriptorPoolHnd createDescriptorPool(const DescriptorPoolCreateInfo& info, const char* name) = 0;
    virtual bool destroyDescriptorPool(DescriptorPoolHnd handle) = 0;
    
    //virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<FramebufferAttachmentCreateInfo>& info) = 0;
    virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<std::variant<Image, FramebufferAttachmentCreateInfo>>& info, const char* name) = 0;
    virtual void destroyFramebufferWithAttachments(const Framebuffer& framebuffer) = 0;
    
    /// Build an ImageCreateInfo for a compressed texture from a TextureData instance loaded by the TextureLoader
    virtual ImageCreateInfo buildImageCreateInfo(const TextureData& textureData);
    
    virtual Image createImage(const ImageCreateInfo& info, const char* name) = 0;
    
    /// Creates an uncompressed Image from the provided memory buffer. This function creates a 2D image (ImageViewType::Im2D) with 1 layer and 1 level.
    ///
    /// This is used for some internal and debug stuff, e.g., ImGui's font atlas, empty textures used as framebuffer images, etc. Use createCompressedImage() for in-game textures.
    virtual Image createUncompressedImage(const UncompressedImageCreateInfo& info, const char* name) = 0;
    virtual bool destroyImage(const Image& image) = 0;
    virtual SamplerHnd createSampler(const SamplerCreateInfo& info, const char* name) = 0;
    virtual SamplerHnd createPresetSampler(SamplerPreset preset, float maxLod = 0.0f);
    virtual bool destroySampler(SamplerHnd handle) = 0;
    ImageViewHnd createDefaultImageView(const Image& image, const char* name);
    virtual ImageViewHnd createImageView(const ImageViewCreateInfo& info, const char* name) = 0;
    virtual bool destroyImageView(ImageViewHnd handle) = 0;
    
    virtual Buffer createBuffer(const BufferCreateInfo& info, const char* name) = 0;
    virtual bool destroyBuffer(const Buffer& buffer) = 0;
    virtual std::vector<Buffer> createBuffers(const std::vector<BufferCreateInfo>& infos, const std::vector<const char*>* names);
    virtual bool destroyBuffers(const std::vector<Buffer>& buffers);
    
    virtual bool readHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, void* data) = 0;
    
    virtual SemaphoreHnd createSemaphore(const char* name) = 0;
    virtual void destroySemaphore(SemaphoreHnd hnd) = 0;
    
    virtual FenceHnd createFence(bool createSignaled, const char* name) = 0;
    virtual void destroyFence(FenceHnd fence) = 0;
    virtual bool getFenceStatus(FenceHnd fence) = 0;
    virtual bool waitForFences(const std::vector<FenceHnd>& fences, bool waitForAll, std::uint64_t timeout) = 0;
    virtual bool waitForFence(FenceHnd fence, std::uint64_t timeout) = 0;
    virtual void resetFences(const std::vector<FenceHnd>& fences) = 0;
    virtual void resetFence(FenceHnd fence) = 0;
    
    virtual void submitQueue(const SubmitInfo& info, FenceHnd fence = FenceHnd()) = 0;
    
    /// Waits until ALL work on the GPU finishes.
    ///
    /// \warning Only use this function in extreme circumstances, e.g., when quitting.
    virtual void waitUntilDone() = 0;
    
    /// Waits until the last started frame completes. This function typically uses a fence.
    ///
    /// Used when rendered image data needs to be retrieved by the CPU, e.g., when doing image based 3D picking.
    virtual void waitUntilFrameCompletes() = 0;
    
    virtual MultithreadingSupport doesBackendSupportMultithreading() = 0;
    virtual bool exposesMultipleCommandBuffers() const = 0;
    
    /// \brief Get the size of the window.
    ///
    /// \warning The size of the render surfaces may be different. To get that, use Renderer::getRenderSurfaceSize()
    glm::uvec2 getWindowSize() const;
    
    virtual glm::uvec2 getSwapchainImageSize() const = 0;
    
    SDL_Window* getWindow() {
        return window;
    }
    
    virtual std::uint32_t getCurrentSwapImage() const = 0;
    virtual std::uint32_t getSwapImageCount() const = 0;
    virtual const Image& getSwapImage(std::uint32_t id) const = 0;
    
    virtual SemaphoreHnd getRenderCompleteSemaphore() = 0;
    virtual SemaphoreHnd getPresentationCompleteSemaphore() = 0;
    
    virtual Format getSurfaceFormat() = 0;
    virtual Format getDepthStencilFormat() = 0;
    std::string getFormatName(Format format) const;
    
    virtual void handleConfigChange(const ConfigurationValueMap& changedValues) override;
    
    virtual ~GraphicsAPI() { }
protected:
    Engine* engine;
    DeviceMemoryManager* deviceMemoryManager;
    SDL_Window* window;
    
    std::size_t maxFramesInFlight;
    std::size_t currentFrameInFlight;
    
    bool isDebug;
    bool isInit;
    
    void openWindow();
    void printWMInfo();
    virtual BackendType getBackendType() = 0;
    
    std::vector<SwapchainChangeListener*> swapchainChangeListeners;
};
}

#endif /* GRAPHICS_API_HPP */
