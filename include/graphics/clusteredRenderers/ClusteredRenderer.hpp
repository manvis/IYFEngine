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

#ifndef CLUSTEREDRENDERER_HPP
#define CLUSTEREDRENDERER_HPP

#include "graphics/Renderer.hpp"
#include "assets/AssetHandle.hpp"

#include <mutex>
#include <list>

namespace iyf {
class Shader;

class ClusteredRenderer : public Renderer {
public:
    virtual void initialize() final override;
    virtual void dispose() final override;
    
    virtual void drawWorld(const World* world) final override;
    
    virtual CommandBuffer* getImGuiDesignatedCommandBuffer() final override;
    
    virtual std::pair<RenderPassHnd, std::uint32_t> getImGuiRenderPassAndSubPass() final override;

    virtual void submitCommandBuffers() final override;

    virtual std::pair<RenderPassHnd, std::uint32_t> getSkyboxRenderPassAndSubPass() final override;
    
    virtual void retrieveDataFromIDBuffer() final override;
    virtual std::future<std::uint32_t> getHoveredItemID() final override;
    
    virtual bool isRenderSurfaceSizeDynamic() const final override;
    virtual glm::uvec2 getRenderSurfaceSize() const final override;
    
protected:
    virtual void initializeRenderPasses() final override;
    virtual void initializeFramebuffers() final override;
    virtual void disposeRenderPasses() final override;
    virtual void disposeFramebuffers() final override;
    
    virtual void drawVisibleOpaque(const GraphicsSystem* graphicsSystem) final override;
    virtual void drawVisibleTransparent(const GraphicsSystem* graphicsSystem) final override;
    virtual void drawSky(const World* world) final override;
    virtual void drawDebugAndHelperMeshes(const World* world, const DebugRenderer* renderer) final override;
    virtual void drawIDBuffer(const GraphicsSystem* graphicsSystem) final override;
    
    void initializePickingPipeline();
    void destroyPickingPipeline();
    
    void initializeTonemappingAndAdjustmentPipeline();
    void initializeMainRenderpassComponents();
    
    friend class Engine;
    ClusteredRenderer(Engine* engine, GraphicsAPI* gfx);
    
    enum class CommandBufferID {
        World = 0, Picking = 1, COUNT = 2
    };
    
    CommandBuffer* getCommandBuffer(CommandBufferID id) const;
    
    CommandPool* commandPool;
    std::vector<CommandBuffer*> commandBuffers;
    SemaphoreHnd worldRenderComplete;
    FenceHnd preGUIFence;
    
    RenderPassHnd mainRenderPass;
    std::vector<Framebuffer> mainFramebuffers;
    
    RenderPassHnd itemPickRenderPass;
    Framebuffer itemPickFramebuffer;
    Buffer pickResultBuffer;
    std::mutex promiseMutex;
    std::list<std::promise<std::uint32_t>> promiseList;
    
    Image depthImage;
    Image hdrAttachmentImage;
    Image idImage;
    SamplerHnd hdrAttachmentSampler;
    
    PipelineLayoutHnd pickingPipelineLayout;
    Pipeline pickingPipeline;
    ShaderHnd idFS;
    ShaderHnd idVS;
    
    PipelineLayoutHnd pipelineLayout;
    Pipeline simpleFlatPipeline;
    AssetHandle<Shader> vsSimple;
    AssetHandle<Shader> fsSimpleFlat;
    
    ShaderHnd fullScreenQuadVS;
    ShaderHnd tonemapFS;
    PipelineLayoutHnd tonemapPipelineLayout;
    Pipeline tonemapPipeline;
    std::vector<DescriptorSetHnd> tonemapSourceDescriptorSet;
    DescriptorSetLayoutHnd tonemapSourceDescriptorSetLayout;
    AssetHandle<Mesh> fullScreenQuad;
    
    // Helpers and other internal stuff
    DescriptorPoolHnd internalDescriptorPool;
    std::vector<std::uint32_t> emptyDynamicOffsets;
};
}

#endif /* CLUSTEREDRENDERER_HPP */

