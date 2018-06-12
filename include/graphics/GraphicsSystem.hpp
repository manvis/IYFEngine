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

#ifndef GRAPHICS_SYSTEM_HPP
#define GRAPHICS_SYSTEM_HPP

#include "core/EntitySystemManager.hpp"
#include "graphics/Camera.hpp"
#include "graphics/MeshComponent.hpp"
#include "graphics/GraphicsAPI.hpp" // TODO maybe separate objects created by the API (e.g. buffers) from API for faster compilation times
#include "graphics/culling/Frustum.hpp"
#include "graphics/RenderDataKey.hpp"
#include "core/ChunkedComponentVector.hpp"

namespace iyf {
class Renderer;
class Camera;
class Skybox;

using ChunkedMeshVector = ChunkedComponentVector<MeshComponent>;

class GraphicsSystem : public System {
public:
    struct DrawingListElement {
        std::uint32_t componentID;
        RenderDataKey key;
        
        inline bool operator<(const DrawingListElement& other) const {
            return key < other.key;
        }
    };
    
    struct VisibleComponents {
        void reset();
        void sort();
        
        std::vector<DrawingListElement> opaqueMeshEntityIDs;
        std::vector<DrawingListElement> transparentMeshEntityIDs;
    };
    
    GraphicsSystem(EntitySystemManager* manager, GraphicsAPI* api);
    
//     Renderer* getRenderer() {
//         return renderer;
//     }
//     
//     const Renderer* getRenderer() const {
//         return renderer;
//     }
    
    virtual void initialize() final override;
    virtual void dispose() final override;
    
    virtual void update(float delta, const EntityStateVector& entityStates) final override;
    virtual void collectGarbage(GarbageCollectionRunPolicy) final override {
        // This class does not have garbage that neesds to be collected
    }
    
    virtual void createAndAttachComponent(const EntityKey& key, const ComponentType& type) final override;
    
    virtual std::size_t getSubTypeCount() const final override {
        return static_cast<std::size_t>(GraphicsComponent::COUNT);
    }
    
    
    bool isDrawingFrustum() const {
        return drawFrustum;
    }
    
    /// Used by the editor to draw a frustum of a selected Camera
    void setDrawingFrustum(bool drawFrustum) {
        this->drawFrustum = drawFrustum;
    }
    
    /// Used by the editor to draw a frustum of a selected Camera
    /// \todo perform CHECKS like in setActiveCameraID
    void setDrawnCameraFrustumID(std::uint32_t id) {
        drawnFrustumID = id;
    }
    
    void setCameraInputPaused(bool status);
    void setActiveCameraID(std::uint32_t id);
    std::uint32_t getActiveCameraID() const;
    
    /// \warning When running in editor mode and isViewingFromEditorCamera() is true, this function will return a special free
    /// Camera that does not have a corresponding Entity.
    const Camera& getActiveCamera() const;
    
    /// \warning When running in editor mode and isViewingFromEditorCamera() is true, this function will return a special free
    /// Camera that does not have a corresponding Entity.
    Camera& getActiveCamera();
    
    bool isViewingFromEditorCamera() const;
    void setViewingFromEditorCamera(bool viewingFromEditorCamera);
    
    inline const ChunkedMeshVector& getMeshComponents() const {
        return *(dynamic_cast<const ChunkedMeshVector*>(getContainer(static_cast<std::size_t>(GraphicsComponent::Mesh))));
    }
    
    inline const Skybox* getSkybox() const {
        return skybox.get();
    }
    
    std::unique_ptr<Skybox> setSkybox(std::unique_ptr<Skybox> newSkybox);
    
    const VisibleComponents& getVisibleComponents() const {
        return visibleComponents;
    }
    
    bool cameraInputPaused;
protected:
    void performCulling();
    
    AssetManager* assetManager;
    GraphicsAPI* api;
    Renderer* renderer;
    std::unique_ptr<Skybox> skybox;
    
    bool drawFrustum;
    std::uint32_t drawnFrustumID;
    Frustum frustum;
    
    VisibleComponents visibleComponents;
    
    std::uint32_t activeCamera;
    bool viewingFromEditorCamera;
    Camera editorCamera;
    TransformationComponent editorCameraTransformation;
};

}

#endif // GRAPHICS_SYSTEM_HPP
