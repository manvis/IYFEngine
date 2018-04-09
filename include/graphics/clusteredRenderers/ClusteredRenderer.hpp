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

namespace iyf {

class ClusteredRenderer : public Renderer {
public:
    virtual void initialize() final override;
    virtual void dispose() final override;
    
    virtual void drawWorld(const World* world) final override;
    
    virtual CommandBuffer* getImGuiDesignatedCommandBuffer() final override;

    virtual void submitCommandBuffers() final override;
    
    virtual bool usesSeparateShadingPass() final override {
        return false;
    }
    
    virtual bool canUseMultipleLightingModels() final override {
        return true;
    }
    
    virtual hash32_t getNameLocalizationHandle() const final override {
        // TODO insert this into string database
        return HS("forward_clustered_renderer");
    }
    
    virtual std::string getName() const final override {
        return "ForwardClustered";
    }
    
    virtual const std::vector<SpecializationConstant>& getShaderSpecializationConstants() const final override {
        return specializationConstants;
    }
    
    virtual std::string makeRenderDataSet(ShaderLanguage language) const final;
    virtual std::string makeLightLoops(ShaderLanguage language, const std::string& lightingFunction) const final override;

    virtual std::pair<RenderPassHnd, std::uint32_t> getSkyboxRenderPassAndSubPass() final;
protected:
    virtual void drawVisibleOpaque(const GraphicsSystem* graphicsSystem) final override;
    virtual void drawVisibleTransparent(const GraphicsSystem* graphicsSystem) final override;
    virtual void drawSky(const World* world) final override;
    virtual void drawDebugAndHelperMeshes(const World* world, const DebugRenderer* renderer) final override;
    
    friend class Engine;
    ClusteredRenderer(Engine* engine, GraphicsAPI* api);
    
    enum class CommandBufferID {
        World = 0, ImGui = 1, COUNT = 2
    };
    
    CommandPool* commandPool;
    std::vector<CommandBuffer*> commandBuffers;
    SemaphoreHnd worldRenderComplete;
    FenceHnd preGUIFence;
    
    PipelineLayoutHnd pipelineLayout;
    Pipeline simpleFlatPipeline;
    ShaderHnd vsSimpleFlat;
    ShaderHnd fsSimpleFlat;
    std::vector<SpecializationConstant> specializationConstants;
    
};
}

#endif /* CLUSTEREDRENDERER_HPP */

