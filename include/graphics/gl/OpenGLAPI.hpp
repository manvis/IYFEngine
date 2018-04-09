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

#ifndef OPENGL_BACKEND_HPP
#define OPENGL_BACKEND_HPP

#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <functional>

#include <GL/glew.h>
#include "graphics/GraphicsAPIBase.hpp"

//TODO SYNC https://ferransole.wordpress.com/2014/06/08/persistent-mapped-buffers/
//#define PERSISTENT_COHERENT_BUFFER_WRITES
//#define PERSISTENT_EXPLICITLY_FLUSHED_BUFFER_WRITES

namespace iyf {
// Kaip boost::hash_combine
template <class T>
inline void hashCombine(std::size_t seed, T& val) {
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

class OpenGLBackend : public RenderingBackend {
public:
    virtual bool initialize() override;
    virtual void dispose() override;
    //virtual void swapBuffers() override;
    
    virtual void setClear(glm::vec4 color, double depth, int stencil) override;
    //virtual void setRenderArea(glm::uvec4 rectangle) override;
    virtual void setViewport(std::uint32_t first, std::uint32_t count, const std::vector<Viewport>& viewports) override;
    virtual void setScissor(std::uint32_t first, std::uint32_t count, const std::vector<Rect2D>& rectangles) override;
    
    virtual bool startFrame() override;
    virtual bool endFrame() override;
    
    virtual void beginPass(ClearFlag clearFlags) override;
    virtual void endPass() override;
    
    virtual ShaderHnd createShader(ShaderStageFlag shaderStageFlag, const std::string& path) override;
    virtual ShaderHnd createShaderFromSource(ShaderStageFlag shaderStageFlag, const std::string& source) override;
    virtual bool destroyShader(ShaderHnd handle) override;
    
    virtual GFXPipelineHnd createGraphicsPipeline(const PipelineCreateInfo& info) override;
    virtual bool destroyGraphicsPipeline(GFXPipelineHnd handle) override;
    virtual void bindGraphicsPipeline(GFXPipelineHnd handle) override;
    virtual void pushConstants(PipelineLayoutHnd handle, ShaderStageFlag flags, std::uint32_t offset, std::uint32_t size, const void* data) override;
    
    virtual PipelineLayoutHnd createPipelineLayout(const PipelineLayoutCreateInfo& info) override;
    virtual bool destroyPipelineLayout(PipelineLayoutHnd handle) override;
    virtual DescriptorSetLayoutHnd createDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& info) override;
    virtual bool destroyDescriptorSetLayout(DescriptorSetLayoutHnd handle) override;
    virtual std::vector<DescriptorSetHnd> allocateDescriptorSets(const DescriptorSetAllocateInfo& info) override;
    virtual bool updateDescriptorSets(const std::vector<WriteDescriptorSet>& set) override;
    virtual DescriptorPoolHnd createDescriptorPool(const DescriptorPoolCreateInfo& info) override;
    virtual bool destroyDescriptorPool(DescriptorPoolHnd handle) override;
    virtual bool bindDescriptorSets(PipelineBindPoints point, PipelineLayoutHnd layout, std::uint32_t firstSet, const std::vector<DescriptorSetHnd> descriptorSets, const std::vector<std::uint32_t> dynamicOffsets) override;
    
    virtual Image createImage(const std::string& path) override;
    virtual Image create2DImageFromMemory(ImageMemoryType type, const glm::ivec2& dimensions, const void* data) override;
    virtual bool destroyImage(const Image& image) override;
    virtual SamplerHnd createSampler(const SamplerCreateInfo& info) override;
    virtual bool destroySampler(SamplerHnd handle) override;
    virtual ImageViewHnd createImageView(const ImageViewCreateInfo& info) override;
    virtual bool destroyImageView(ImageViewHnd handle) override;
    
    virtual UniformBufferSlice createUniformBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) override;
    virtual bool setUniformBufferData(const UniformBufferSlice& slice, const void* data) override;
    virtual bool updateUniformBufferData(const UniformBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) override;
    virtual bool destroyUniformBuffer(const UniformBufferSlice& slice) override;
    
    virtual VertexBufferSlice createVertexBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) override;
    virtual bool setVertexBufferData(const VertexBufferSlice& slice, const void* data) override;
    virtual bool updateVertexBufferData(const VertexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) override;
    virtual bool destroyVertexBuffer(const VertexBufferSlice& slice) override;
    
    virtual IndexBufferSlice createIndexBuffer(std::uint64_t size, IndexType type, BufferUpdateFrequency flag, const void* data = nullptr) override;
    virtual bool setIndexBufferData(const IndexBufferSlice& slice, const void* data) override;
    virtual bool updateIndexBufferData(const IndexBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) override;
    virtual bool destroyIndexBuffer(const IndexBufferSlice& slice) override;
    
    virtual StorageBufferSlice createStorageBuffer(std::uint64_t size, BufferUpdateFrequency flag, const void* data = nullptr) override;
    virtual bool setStorageBufferData(const StorageBufferSlice& slice, const void* data) override;
    virtual bool updateStorageBufferData(const StorageBufferSlice& slice, const BufferSubSlice& subSlice, const void* data) override;
    virtual bool destroyStorageBuffer(const StorageBufferSlice& slice) override;
    
    virtual void bindVertexBuffers(std::uint32_t firstBinding, std::uint32_t bindingCount, const std::vector<VertexBufferSlice>& buffers) override;
    virtual void bindIndexBuffer(const IndexBufferSlice& slice) override;
    
    //virtual VertexLayoutHnd createVertexLayout(const VertexLayoutCreateInfo& vlCreateInfo) override;
    //virtual void bindVertexLayout(VertexLayoutHnd handle) override;
    //virtual bool destroyVertexLayout(VertexLayoutHnd handle) override;
    
    //virtual void useDebug(bool status) override;
    
    virtual void draw(std::uint32_t vertexCount, std::uint32_t instanceCount, std::uint32_t firstVertex, std::uint32_t firstInstance) override;
    virtual void drawIndexed(std::uint32_t indexCount, std::uint32_t instanceCount, std::uint32_t firstIndex, std::int32_t vertexOffset, std::uint32_t firstInstance) override;
    
    virtual ~OpenGLBackend() { }
protected:
    friend class Engine;
    OpenGLBackend(Engine* engine, bool useDebugAndValidation, Configuration* config) : RenderingBackend(engine, useDebugAndValidation, config), currentPipelineHnd(0) { }
    
    using VAOHnd = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t>;
    
    struct VAOHash {//TODO gal tie?
        inline std::size_t operator()(const VAOHnd& vao) const {
            size_t seed = 0;

            std::uint64_t h1 = std::get<0>(vao);
            hashCombine(seed, h1);
            
            std::uint64_t h2 = std::get<1>(vao);
            hashCombine(seed, h2);
            
            std::uint64_t h3 = std::get<2>(vao);
            hashCombine(seed, h3);
            
            return seed;
        }
    };
        
    enum class BufferType {
        Vertex = 0, Index = 1, Uniform = 2, Storage = 3
    };
    
    struct GLBlendStates {
        bool enabled;
        GLenum srcColBlendFac;
        GLenum dstColBlendFac;
        GLenum srcAlphaBlendFac;
        GLenum dstAlphaBlendFac;
        GLenum colBlendOp;
        GLenum alphaBlendOp;
        ColorWriteMaskFlags colorMask;
    };
    
    struct GLRasterizationState {
        bool isDepthClampEnabled;
        bool isRasterizerDiscardEnabled;
        GLenum polygonModeVal;
        GLenum cullModeVal;
        GLenum frontFaceVal;
        bool isDepthBiasEnabled;
        float depthBiasConstantFactorVal;
        float depthBiasClampVal;
        float depthBiasSlopeFactorVal;
        float lineWidthVal;
    };
    
    struct GLStencilOp {
        GLenum fail;
        GLenum pass;
        GLenum depthFail;
        GLenum compare;
        std::uint32_t compareMaskVal;
        std::uint32_t writeMaskVal;
        std::uint32_t referenceVal;
    };
    
    struct GLDepthStencilState {
        bool depthTestOn;
        bool depthWriteOn;
        GLenum depthFunction;
        bool stencilTestOn;
        bool depthBoundsTestOn;
        float depthBoundsMin;
        float depthBoundsMax;
        GLStencilOp front;
        GLStencilOp back;
    };
    
    struct GLMultisampleState {
        std::uint32_t sampleCount;
        bool sampleShadingOn;
        float minSampleShadingVal;
        std::vector<SampleMask> sampleMaskVal;
        bool alphaToCoverageOn;
        bool alphaToOneOn;
    };
    
    struct GLViewportState {
        std::vector<GLfloat> viewports;
        std::vector<GLdouble> depths;
        std::vector<GLint> scissors;
    };
    
    struct GLTessellationState {
        std::uint32_t patchControlPoints;
    };
    
    struct GLColorBlendState {
        glm::vec4 blendConst;
        std::vector<GLBlendStates> blendStates;
        bool logicOpOn;
        GLenum logicOpVal;
    };
    
    struct GLInputAssemblyState {
        GLenum topology;
        bool primitiveRestartEnable;
    };
    
    struct GLPipeline {
        GLuint pipelineName;
        GLuint VAOName;
        
        GLColorBlendState blendState;
        GLRasterizationState rasterizationState;
        GLDepthStencilState depthStencilState;
        GLViewportState viewportState;
        GLMultisampleState multisampleState;
        GLTessellationState tessellation;
        GLInputAssemblyState assembly;
        VertexInputStateCreateInfo inputState;
    };
    
    //struct CurrentBuffers {
    //    std::uint64_t
    //};
    
    //struct GLViewport {
    //    GLfloat x, y, w, h;
    //};
    
    //struct GLDepthRange {
    //    GLdouble near, far;
    //};
    
    //struct GLScissor {
    //    GLint x, y, w, h;
    //};
    
    Image loadDDSImage(const std::string& path);
    Image loadImage(const std::string& path);
    GLuint makeBuffer(BufferType type, std::uint64_t size, BufferUpdateFrequency flag, const void* data);
    bool updateBuffer(BufferType type, std::uint64_t handle, std::uint64_t offset, std::uint64_t size, const void* data);
    bool partialUpdateBuffer(BufferType type, std::uint64_t handle, std::uint64_t offset, std::uint64_t subOffset, std::uint64_t subSize, const void* data);
    static void oglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
    
    std::uint64_t currentPipelineHnd;
    //GLenum currentIndexSize;
    std::unordered_map<GFXPipelineHnd, GLPipeline> pipelines;
    //std::unordered_map<VertexLayoutHnd, GLenum> vertexLayouts;
    std::unordered_map<VAOHnd, GLuint, VAOHash> VAOs;
#if defined(PERSISTENT_COHERENT_BUFFER_WRITES) || defined(PERSISTENT_EXPLICITLY_FLUSHED_BUFFER_WRITES)
    std::unordered_map<GLuint, void*> persistentHandles;
#endif
    std::vector<GLfloat> viewportTemp;
    std::vector<GLdouble> depthTemp;
    std::vector<GLint> scissorTemp;
    
    std::vector<GLuint> tempBuffers;
    std::vector<GLintptr> tempOffsets;
    std::vector<GLsizei> tempStrides;
    
    IndexBufferSlice currentIndexBuffer;
    std::vector<VertexBufferSlice> currentVertexBuffers;
    
    // TODO gal kazka geresnio?
    std::uint64_t getPipelineHandle() {
        currentPipelineHnd++;
        return currentPipelineHnd;
    }
    
    // SDL video
    SDL_GLContext context;
    
    GLenum getGLShaderType(ShaderStageFlag type);
    GLbitfield getGLShaderBitType(ShaderStageFlag type);
    GLenum getCompareOp(CompareOp type);
    GLenum getBlendFactor(BlendFactor type);
    GLenum getBlendOp(BlendOp type);
    GLenum getPolygonMode(PolygonMode type);
    GLenum getCullMode(CullMode type);
    GLenum getFrontFaceType(FrontFace type);
    GLenum getIndexType(IndexType type);
    GLenum getStencilOp(StencilOp op);
    std::uint32_t mapSampleCount(SampleCount count);
    GLenum mapLogicOp(LogicOp op);
    GLenum mapTopology(PrimitiveTopology topology);
    GLint mapAttributeSize(Format format);
    GLenum mapAttributeFormat(Format format);
    GLboolean mapAttributeNormalization(Format format);
    
    //GLenum primitiveMode;
    std::uint64_t currentlyBoundPipeline;
    
    virtual BackendType getBackendType() {
        return BackendType::OpenGL45;
    }
};
}
#endif
