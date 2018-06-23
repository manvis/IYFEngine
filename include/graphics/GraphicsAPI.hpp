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

#ifndef GRAPHICS_API_HPP
#define GRAPHICS_API_HPP

#include <vector>
#include <utility>
#include <cstdint>

#include <variant>

#include "core/interfaces/Configurable.hpp"
#include "graphics/GraphicsAPIConstants.hpp"
#include "graphics/GraphicsAPIHandles.hpp"
#include "utilities/DataSizes.hpp"
#include "utilities/NonCopyable.hpp"
#include "utilities/ForceInline.hpp"

#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

struct SDL_Window;

namespace iyf {
/// \todo What about queue families?
class BufferCreateInfo {
public:
    BufferCreateInfo() : flags(), size(0) {}
    BufferCreateInfo(BufferUsageFlags flags, Bytes size) : flags(flags), size(size) {}
    
    BufferUsageFlags flags;
    Bytes size;
};

class Buffer {// MEMORY TYPE? Memory handle?
public:
    Buffer() : bufferHandle(nullptr), bufferUsageFlags(), backingMemoryType(MemoryType::HostVisible), bufferSize(0), memoryOffset(0) {}
    Buffer(BufferHnd handle, BufferUsageFlags flags, MemoryType memoryType, Bytes size, Bytes offset) : bufferHandle(handle), bufferUsageFlags(flags), backingMemoryType(memoryType), bufferSize(size), memoryOffset(offset) {}
    
    inline BufferHnd handle() const {
        return bufferHandle;
    }
    
    inline BufferUsageFlags usageFlags() const {
        return bufferUsageFlags;
    }
    
    /// Returns the actual size of the buffer.
    /// \warning This value MAY end up slightly bigger than what was supplied to BufferCreateInfo during buffer creation because of memory alignment requirements
    inline Bytes size() const {
        return bufferSize;
    }
    
    inline MemoryType memoryType() const {
        return backingMemoryType;
    }
    
    /// Returns the offset of this buffer in the allocation that's backing it.
    inline Bytes offset() const {
        return memoryOffset;
    }
    
protected:
    BufferHnd bufferHandle;
    BufferUsageFlags bufferUsageFlags;
    MemoryType backingMemoryType;
    Bytes bufferSize;
    Bytes memoryOffset;
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
    Image() : handle(nullptr), format(Format::Undefined) {}
    Image(ImageHnd handle, std::uint64_t width, std::uint64_t height, std::uint64_t levels, std::uint64_t layers, ImageViewType type, Format format) 
        : handle(handle), width(width), height(height), levels(levels), layers(layers), type(type), format(format) {}
    
    bool isValid() const {
        return (handle.isValid()) && (format != Format::Undefined);
    }
    
    ImageHnd handle;
    std::uint64_t width, height, levels, layers;
    ImageViewType type;
    Format format;
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

class FramebufferAttachmentCreateInfo {
public:
    Format format;
    //bool isDepthStencil; //TODO decide from format
    bool isAttachment;
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
    
    virtual void setViewport(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) = 0;
    virtual void setScissor(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) = 0;
    
    virtual void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) = 0;
    virtual void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) = 0;
    
    virtual void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) = 0;
    
    virtual void bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<Buffer>& buffers) = 0;//TODO offset'ai?
    virtual void bindVertexBuffers(std::uint32_t firstBinding, const Buffer& buffer) = 0;
    virtual void bindIndexBuffer(const Buffer& buffer, IndexType indexType) = 0;
    
    virtual void pushConstants(PipelineLayoutHnd handle, ShaderStageFlags flags, std::uint32_t offset, std::uint32_t size, const void* data) = 0;
    virtual bool bindDescriptorSets(PipelineBindPoint point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) = 0;
    
    virtual void bindPipeline(const Pipeline& pipeline) = 0;
    
    virtual void begin(const CommandBufferBeginInfo& cbbi = CommandBufferBeginInfo()) = 0;
    virtual void end() = 0;
    
    virtual void beginRenderPass(const RenderPassBeginInfo& rpbi, SubpassContents contents = SubpassContents::Inline) = 0;
    virtual void nextSubpass(SubpassContents contents = SubpassContents::Inline) = 0;
    virtual void endRenderPass() = 0;
    
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
    
    virtual CommandBuffer* allocateCommandBuffer(BufferLevel level = BufferLevel::Primary, bool beginBuffer = true) = 0;
    virtual std::vector<CommandBuffer*> allocateCommandBuffers(std::uint32_t count, BufferLevel level = BufferLevel::Primary, bool beginBuffer = false) = 0;
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
    GraphicsAPI(Engine* engine, bool useDebugAndValidation, Configuration* config) : Configurable(config), engine(engine), isDebug(useDebugAndValidation), isInit(false) { }
    
    virtual bool initialize() = 0;
    virtual void dispose() = 0;
    
    bool isInitialized() {
        return isInit;
    }

    virtual RenderPassHnd createRenderPass(const RenderPassCreateInfo& info) = 0;
    virtual bool destroyRenderPass(RenderPassHnd handle) = 0;
    
    virtual bool startFrame() = 0;
    virtual bool endFrame() = 0;
    
    virtual CommandPool* createCommandPool(QueueType type, std::uint32_t queueId) = 0;
    virtual bool destroyCommandPool(CommandPool* pool) = 0;
    
    /// \brief Create a shader from bytecode.
    ///
    /// On APIs that don't support bytecode, this function needs to be passed a source code string and its length.
    virtual ShaderHnd createShader(ShaderStageFlags shaderStageFlag, const void* data, std::size_t byteCount) = 0;
    /// \brief Create a shader from a source code string.
    ///
    /// \throws std::runtime_error if the API does not support loading shaders from source code.
    virtual ShaderHnd createShaderFromSource(ShaderStageFlags shaderStageFlag, const std::string& source) = 0;
    virtual bool destroyShader(ShaderHnd handle) = 0;
    
    virtual Pipeline createPipeline(const PipelineCreateInfo& info) = 0;
    virtual Pipeline createPipeline(const ComputePipelineCreateInfo& info) = 0;
    virtual bool destroyPipeline(const Pipeline& pipeline) = 0;
    
    virtual PipelineLayoutHnd createPipelineLayout(const PipelineLayoutCreateInfo& info) = 0;
    virtual bool destroyPipelineLayout(PipelineLayoutHnd handle) = 0;
    virtual DescriptorSetLayoutHnd createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) = 0;
    virtual bool destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) = 0;
    virtual std::vector<DescriptorSetHnd> allocateDescriptorSets(const DescriptorSetAllocateInfo& info) = 0;
    virtual bool updateDescriptorSets(const std::vector<WriteDescriptorSet>& set) = 0;
    virtual DescriptorPoolHnd createDescriptorPool(const DescriptorPoolCreateInfo& info) = 0;
    virtual bool destroyDescriptorPool(DescriptorPoolHnd handle) = 0;
    
    //virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<FramebufferAttachmentCreateInfo>& info) = 0;
    virtual Framebuffer createFramebufferWithAttachments(const glm::uvec2& extent, RenderPassHnd renderPass, const std::vector<std::variant<Image, FramebufferAttachmentCreateInfo>>& info) = 0;
    virtual void destroyFramebufferWithAttachments(const Framebuffer& framebuffer) = 0;
    /// Creates a compressed image from the provided memory buffer. The memory buffer is expected to contain a texture in
    /// the IYFEngine's native compression format (including all headers).
    ///
    /// To put it simply, read an **entire** file that was created by the TextureConverter into a buffer, pass it here and
    /// you'll get your Image.
    ///
    /// This function transfers the processed data to the GPU immediately.
    ///
    /// \todo Implement async/delayed loading.
    virtual Image createCompressedImage(const void* data, std::size_t size) = 0;
    /// Creates an uncompressed Image from the provided memory buffer. This function creates a 2D image (ImageViewType::Im2D) with 1 layer and 1 level.
    ///
    /// This is used for some internal and debug stuff, e.g., ImGui's font atlas, empty textures used as framebuffer images, etc. Use createCompressedImage() for in-game textures.
    virtual Image createUncompressedImage(ImageMemoryType type, const glm::uvec2& dimensions, bool isWritable = false, bool usedAsColorOrDepthAttachment = false, bool usedAsInputAttachment = false, const void* data = nullptr) = 0;
    virtual bool destroyImage(const Image& image) = 0;
    virtual SamplerHnd createSampler(const SamplerCreateInfo& info) = 0;
    virtual SamplerHnd createPresetSampler(SamplerPreset preset, float maxLod = 0.0f);
    virtual bool destroySampler(SamplerHnd handle) = 0;
    ImageViewHnd createDefaultImageView(const Image& image);
    virtual ImageViewHnd createImageView(const ImageViewCreateInfo& info) = 0;
    virtual bool destroyImageView(ImageViewHnd handle) = 0;
    
    /// This function tries to make sure that all buffers that are being created end up using the same allocation
    virtual bool createBuffers(const std::vector<BufferCreateInfo>& info, MemoryType memoryType, std::vector<Buffer>& outBuffers) = 0;
    virtual bool destroyBuffers(const std::vector<Buffer>& buffers) = 0;
    
    // TODO stop unmapping buffers
    
    /// Updates data in a host visible buffer. To update buffers that reside in device memory, use a DeviceMemoryUpdateBatcher or
    /// a blocking function called updateDeviceVisibleBuffer()
    ///
    /// \warning Bad things will happen if this function is used to update buffers that reside in device memory (buffer.memoryType() == MemoryType::Device).
    virtual bool updateHostVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, const void* data) = 0;
    
    /// Updates data in a device visible buffer, using a temporary staging buffer
    ///
    /// \warning This is a blocking function. It will wait until the data ends up in GPU memory. You should use a DeviceMemoryUpdateBatcher instead.
    virtual void updateDeviceVisibleBuffer(const Buffer& buffer, const std::vector<BufferCopy>& copies, const void* data) = 0;
//     virtual UniformBufferSlice createUniformBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) = 0;
//     virtual bool setUniformBufferData(const UniformBufferSlice& slice, const void* data) = 0;
//     virtual bool updateUniformBufferData(const UniformBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) = 0;
//     virtual bool destroyUniformBuffer(const UniformBufferSlice& slice) = 0;
//     
//     virtual VertexBufferSlice createVertexBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) = 0;
//     virtual bool setVertexBufferData(const VertexBufferSlice& slice, const void* data) = 0;
//     virtual bool updateVertexBufferData(const VertexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) = 0;
//     virtual bool destroyVertexBuffer(const VertexBufferSlice& slice) = 0;
//     
//     virtual IndexBufferSlice createIndexBuffer(std::uint64_t size, IndexType type, BufferUpdateFrequency flag, const void* data = nullptr) = 0;
//     virtual bool setIndexBufferData(const IndexBufferSlice& slice, const void* data) = 0;
//     virtual bool updateIndexBufferData(const IndexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) = 0;
//     virtual bool destroyIndexBuffer(const IndexBufferSlice& slice) = 0;
//     
//     virtual StorageBufferSlice createStorageBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) = 0;
//     virtual bool setStorageBufferData(const StorageBufferSlice& slice, const void* data) = 0;
//     virtual bool updateStorageBufferData(const StorageBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) = 0;
//     virtual bool destroyStorageBuffer(const StorageBufferSlice& slice) = 0;
    
    virtual SemaphoreHnd createSemaphore() = 0;
    virtual void destroySemaphore(SemaphoreHnd hnd) = 0;
    
    virtual FenceHnd createFence(bool createSignaled) = 0;
    virtual void destroyFence(FenceHnd fence) = 0;
    virtual bool getFenceStatus(FenceHnd fence) = 0;
    virtual bool waitForFences(const std::vector<FenceHnd>& fences, bool waitForAll, std::uint64_t timeout) = 0;
    virtual bool waitForFence(FenceHnd fence, std::uint64_t timeout) = 0;
    virtual void resetFences(const std::vector<FenceHnd>& fences) = 0;
    virtual void resetFence(FenceHnd fence) = 0;
    
    virtual void submitQueue(const SubmitInfo& info, FenceHnd fence = FenceHnd()) = 0;
    virtual void waitUntilDone() = 0;
    
    virtual MultithreadingSupport doesBackendSupportMultithreading() = 0;
    virtual bool exposesMultipleCommandBuffers() const = 0;
    
    std::uint32_t getScreenWidth() {
        return screenWidth;
    }

    std::uint32_t getScreenHeight() {
        return screenHeight;
    }
    
    // TODO tikro buferio dydis
    std::uint32_t getRenderSurfaceWidth() {
        return screenWidth;
    }
    
    std::uint32_t getRenderSurfaceHeight() {
        return screenHeight;
    }
    
    virtual ImageViewHnd getDefaultDepthBufferViewHnd() const = 0;
    virtual Image getDefaultDepthBufferImage() const = 0;
    
    SDL_Window* getWindow() {
        return window;
    }
    
    RenderPassHnd getWriteToScreenRenderPass() const {
        return writeToScreenPass;
    }
    
    RenderPassCreateInfo getWriteToScreenRenderPassCreateInfo() const {
        return rpciScreen;
    }
    
    virtual FramebufferHnd getScreenFramebuffer() = 0;
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
    //TODO HANDLE window opening and updates here    
protected:
    Engine* engine;
    SDL_Window* window;
    std::uint32_t screenHeight, screenWidth;
    
    bool isDebug;
    bool isInit;
    
    void openWindow();
    void printWMInfo();
    virtual BackendType getBackendType() = 0;
    void createHelperMeshesAndObjects();
    void destroyHelperMeshesAndObjectsaryMeshes();
    
    RenderPassHnd writeToScreenPass;
    RenderPassCreateInfo rpciScreen;
};
}

#endif /* GRAPHICS_API_HPP */
