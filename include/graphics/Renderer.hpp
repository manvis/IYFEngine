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

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "graphics/MeshComponent.hpp"
#include "graphics/GraphicsAPI.hpp"
#include "graphics/ShaderConstants.hpp"
#include "graphics/GraphicsSystem.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/NonCopyable.hpp"

#include <initializer_list>
#include <future>

namespace iyf {
class World;
class ImGuiImplementation;
class Engine;
class Skybox;
class Camera;
class DebugRenderer;

/// \warning All derived classes should be friends with Engine and their constructors should be protected to ensure
/// they are not constructed in inappropriate places.
class Renderer : private NonCopyable {
public:
    virtual void initialize() = 0;
    virtual void dispose() = 0;
    
    virtual void drawWorld(const World* world) = 0;
    
    virtual void drawImGui(ImGuiImplementation* impl);
    virtual CommandBuffer* getImGuiDesignatedCommandBuffer() = 0;
    
    /// Used by the shader generator. Returns a string that contains all structs and buffer layout definitions
    /// that can be used to store data specific to a Renderer (e.g., various acceleration structs).
    virtual std::string makeRenderDataSet(ShaderLanguage language) const = 0;
    
    /// Used by the shader generator. Returns the loops that process visible lights.
    virtual std::string makeLightLoops(ShaderLanguage language, const std::string& lightingFunction) const = 0;
    
    virtual void submitCommandBuffers() {
        // Reset this that we could draw the next frame
        drawingWorldThisFrame = false;
    }
    
    /// Indicates if this renderer uses one pass to build G-Buffers and another to compute the final
    /// shading of the surfaces or not. If this flag is set, compatible ShadingPipeline objects
    /// must have Shaders for both passes defined.
    ///
    /// NOTE: this does NOT imply that only two passes will be used when this is set and one when
    /// it isn't. E.g., some renderers may use an early Z pass.
    ///
    /// \return if this renderer separates entity shading into a separate pass
    virtual bool usesSeparateShadingPass() = 0;
    
    /// Indicates if it's possible to use multiple lighting models in a compatible ShadingPipeline.
    /// Forward renderers typically can do this without any modifications, while deferred ones can
    /// achieve similar results by writing the material IDs to their G-buffers.
    /// 
    /// \return if this renderer can use ShadingPipelines with multiple lighting models defined
    virtual bool canUseMultipleLightingModels() = 0;
    
    /// Returns a hashed string id, which, when supplied to the LOC macros, will return a human readable
    /// name of this renderer
    /// 
    /// \return hashed localization handle
    virtual hash32_t getNameLocalizationHandle() const = 0;
    
    /// Returns a simple, filename friendly string (no spaces or special characters) that can be used by the shader generator or in logs.
    virtual std::string getName() const = 0;
    
    /// Returns a RenderPassHnd and sub-pass id which must be used in all skyboxes that were constructed using this renderer.
    /// 
    /// \warning The returned values will only be valid after initialize() successfully completes.
    /// 
    /// \return std::pair of a RenderPassHnd and sub-pass id.
    virtual std::pair<RenderPassHnd, std::uint32_t> getSkyboxRenderPassAndSubPass() = 0;
    
    /// The returned vector must contain a copy of con::DefaultSpecializationConstants array with Renderer specific
    /// constants (if any) appended at the end.
    virtual const std::vector<SpecializationConstant>& getShaderSpecializationConstants() const = 0;
    
    virtual std::pair<RenderPassHnd, std::uint32_t> getImGuiRenderPassAndSubPass() = 0;
    
    /// If true, meshes are rendered into a separate ID buffer that can be used for pixel perfect 3D picking.
    ///
    /// \remark The ID buffer is only enabled if the Engine is running in edior mode. For picking during gameplay, use
    /// the physics engine.
    inline bool isPickingEnabled() const {
        return pickingEnabled;
    }
    
    bool isInitialized() const {
        return initialized;
    }
    
    GraphicsAPI* getGraphicsAPI() const {
        return api;
    }
    
    /// If isPickingEnabled() is true, this function fetches the data from the ID buffer and pushes it to
    /// all std::future objects that were retrieved from getHoveredItemID().
    ///
    /// \warning This must be executed AFTER rendering of the current frame completes and before the rendering
    /// of the next frame starts. Otherwise, you may get stale data, invalid ID values or cause validation errors.
    ///
    /// \throws std::logic_error if isPickingEnabled() is false
    virtual void retrieveDataFromIDBuffer() = 0;
    
    /// Request the ID of the currently hovered item.
    ///
    /// This uses the current mouse coordinates. The result will be available during the next frame, after
    /// retrieveDataFromIDBuffer() is called.
    ///
    /// \throws std::logic_error if isPickingEnabled() is false
    virtual std::future<std::uint32_t> getHoveredItemID() = 0;
    
    /// \brief Indicates if dynamic resolution is used or not.
    virtual bool isRenderSurfaceSizeDynamic() const = 0;
    
    /// \brief Returns the size of the render surface.
    ///
    /// If isRenderSurfaceSizeDynamic() is true, then some sort of a dynamic resolution technique 
    /// is in use and this value may be different every frame.
    virtual glm::uvec2 getRenderSurfaceSize() const = 0;
    
    virtual ~Renderer() {}
protected:
    /// Create the render pass (or passes) that the renderer needs.
    ///
    /// This must be called inside initialize()
    virtual void initializeRenderPasses() = 0;
    /// Create all framebuffers that the renderer needs.
    ///
    /// This must be called inside initialize()
    virtual void initializeFramebuffers() = 0;
    
    /// Destroy the render pass (or passes) that the renderer used.
    ///
    /// This must be called inside dispose()
    virtual void disposeRenderPasses() = 0;
    /// Destory all framebuffers that the renderer used.
    ///
    /// This must be called inside dispose()
    virtual void disposeFramebuffers() = 0;
    
    // The following pure virtual functions don't necessarily have to be called if they don't fit the renderer you're creating.
    // Think of them as guidelines for what a renderer should do every frame.
    virtual void drawVisibleOpaque(const GraphicsSystem* graphicsSystem) = 0;
    virtual void drawVisibleTransparent(const GraphicsSystem* graphicsSystem) = 0;
    virtual void drawIDBuffer(const GraphicsSystem* graphicsSystem) = 0;
    virtual void drawSky(const World* world) = 0;
    virtual void drawDebugAndHelperMeshes(const World* world, const DebugRenderer* renderer) = 0;
    
    Renderer(Engine* engine, GraphicsAPI* api);
    
    Engine* engine;
    GraphicsAPI* api;
    bool imGuiSubmissionRequired;
    bool drawingWorldThisFrame;
    bool pickingEnabled;
    bool initialized;
};
}

#endif /* RENDERER_HPP */

