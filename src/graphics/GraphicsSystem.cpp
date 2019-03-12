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

#include "graphics/GraphicsSystem.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/clusteredRenderers/ClusteredRenderer.hpp"
#include "graphics/Camera.hpp"
#include "graphics/CubemapSkybox.hpp"
#include "core/EntitySystemManager.hpp"
#include "core/UnorderedComponentMap.hpp"
#include "core/Engine.hpp"
#include "core/InputState.hpp"
#include "core/World.hpp"
#include "physics/PhysicsSystem.hpp"
#include "physics/BulletPhysicsDebugRenderer.hpp"
#include "threading/ThreadProfiler.hpp"

namespace iyf {
static void checkEditorMode(const EntitySystemManager* manager) {
    if (!manager->isEditorMode()) {
        throw std::runtime_error("This method can only be called if the EntitySystemManager has been created in editor mode.");
    }
}

void GraphicsSystem::VisibleComponents::reset() {
    opaqueMeshEntityIDs.clear();
    transparentMeshEntityIDs.clear();
}

void GraphicsSystem::VisibleComponents::sort() {
    std::sort(opaqueMeshEntityIDs.begin(), opaqueMeshEntityIDs.end());
    std::sort(transparentMeshEntityIDs.begin(), transparentMeshEntityIDs.end());
}

GraphicsSystem::GraphicsSystem(EntitySystemManager* manager, GraphicsAPI* api) : System(manager, ComponentBaseType::Graphics, static_cast<std::size_t>(GraphicsComponent::COUNT)), cameraInputPaused(false), api(api), drawFrustum(false), drawnFrustumID(EntityKey::InvalidID), activeCamera(EntityKey::InvalidID), viewingFromEditorCamera(false) {}

bool GraphicsSystem::isViewingFromEditorCamera() const {
    checkEditorMode(manager);
    
    return viewingFromEditorCamera;
}

void GraphicsSystem::setViewingFromEditorCamera(bool viewingFromEditorCamera) {
    this->viewingFromEditorCamera = viewingFromEditorCamera;
}

void GraphicsSystem::setActiveCameraID(std::uint32_t id) {
    if (id == EntityKey::InvalidID) {
        throw std::invalid_argument("A reserved ID was used.");
    }
    
    if (!hasComponent(id, ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Camera))) {
        throw std::invalid_argument("The Entity does not have a Camera component.");
    }
    
    activeCamera = id;
}

std::uint32_t GraphicsSystem::getActiveCameraID() const {
    if (activeCamera == EntityKey::InvalidID) {
        throw std::runtime_error("No active camera was set.");
    }
    
    return activeCamera;
}

const Camera& GraphicsSystem::getActiveCamera() const {
    if (manager->isEditorMode() && isViewingFromEditorCamera()) {
        return editorCamera;
    }
    
    return getComponent<Camera>(getActiveCameraID());
}

Camera& GraphicsSystem::getActiveCamera() {
    if (manager->isEditorMode() && isViewingFromEditorCamera()) {
        return editorCamera;
    }
    
    return getComponent<Camera>(getActiveCameraID());
}

void GraphicsSystem::setCameraInputPaused(bool status) {
    cameraInputPaused = status;
}

std::unique_ptr<Skybox> GraphicsSystem::setSkybox(std::unique_ptr<Skybox> newSkybox) {
    std::swap(skybox, newSkybox);
    
    return newSkybox;
}

void GraphicsSystem::initialize() {
    components[static_cast<std::size_t>(GraphicsComponent::Mesh)] = std::make_unique<ChunkedMeshVector>(this, ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Mesh));
    components[static_cast<std::size_t>(GraphicsComponent::Camera)] = std::make_unique<UnorderedComponentMap<Camera>>(this, ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Camera));
    
    if (!api->isInitialized()) {
        throw std::runtime_error("API must be initialized before initializing the GraphicsSystem.");
    }
    
    renderer = manager->getEngine()->getRenderer();
    assetManager = manager->getEngine()->getAssetManager();
    
    // TODO I need to refactor the Skybox so that it would be drawn, instead of drawing itself
    StringHash cubemapTextureNameHash = assetManager->getSystemAssetNameHash("skybox.png");
    setSkybox(std::make_unique<CubemapSkybox>(assetManager, renderer, cubemapTextureNameHash));
    if (skybox != nullptr) {
        skybox->initialize();
    }
    
    if (manager->isEditorMode()) {
        viewingFromEditorCamera = true;

        editorCamera.transformation = &editorCameraTransformation;
        
        assert(!renderer->isRenderSurfaceSizeDynamic());
        editorCamera.setRenderSurfaceSize(renderer->getRenderSurfaceSize());
        
        // TODO these should be loaded from file
        editorCameraTransformation.setPosition(10.0f, 5.0f, 10.0f);
        editorCameraTransformation.setRotation(glm::quat(-1.0f, 0.0f, 0.0f, 0.0f));
        editorCamera.setHorizontalFOV(glm::radians(105.518));
        
        editorCameraTransformation.forcedUpdate();
        editorCamera.update();
    }
}

void GraphicsSystem::dispose() {
    if (skybox != nullptr) {
        skybox->dispose();
    }
}

void GraphicsSystem::performCulling() {
    IYFT_PROFILE(EntityCulling, iyft::ProfilerTag::Graphics);
    
    visibleComponents.reset();
    
    ChunkedMeshVector* meshes = static_cast<ChunkedMeshVector*>(getContainer(static_cast<std::size_t>(GraphicsComponent::Mesh)));
    auto meshesIt = meshes->begin();
    
    for (std::uint32_t i = 0; i < manager->getEntityCount(); ++i) {
        const auto& entityComponents = availableComponents[i];
        
        // No components we care about. Move on to the next entity
        if (!entityComponents[static_cast<std::size_t>(GraphicsComponent::Mesh)]) {
            meshesIt++;
            continue;
        }
        
        const MeshComponent& mc = *meshesIt;
        const BoundingVolume& bounds = mc.getCurrentBoundingVolume();
        
//         if (frustum.doesSphereIntersectPlane(Frustum::Plane::Bottom, bounds)) {
//             LOG_D("YAY");
//         } else {
//             LOG_D("NAY"); 
//         }
        if (frustum.isBoundingVolumeInFrustum(bounds)) {
            if (mc.getRenderMode() == MaterialRenderMode::Opaque) {
                visibleComponents.opaqueMeshEntityIDs.push_back({i, mc.getRenderDataKey()});
            } else {
                visibleComponents.transparentMeshEntityIDs.push_back({i, mc.getRenderDataKey()});
            }
        }
        
        meshesIt++;
    }
    
    visibleComponents.sort();
}

void GraphicsSystem::update(float delta, const EntityStateVector& entityStates) {
    IYFT_PROFILE(GraphicsUpdate, iyft::ProfilerTag::Graphics);
    
    const InputState* is = manager->getEngine()->getInputState();
    
    Camera& camera = getActiveCamera();
    assert(!renderer->isRenderSurfaceSizeDynamic());
    
    const glm::uvec2 renderSurfaceSize = renderer->getRenderSurfaceSize();
    if (camera.getRenderSurfaceSize() != renderSurfaceSize) {
        camera.setRenderSurfaceSize(renderSurfaceSize);
    }

    // TODO this should go to a CharacterController
    //---------------------------CAMERA START
    if (!cameraInputPaused) {
        const float keyMultiplier = 6.0f;
        const float mouseMultiplier = 6.0f;
        
        TransformationComponent& transformation = *camera.transformation;
        
        if (is->isKeyPressed(SDL_SCANCODE_W)) {
            transformation.translateRelative(0.0f, 0.0f,  keyMultiplier * delta);
        }

        if (is->isKeyPressed(SDL_SCANCODE_S)) {
            transformation.translateRelative(0.0f, 0.0f, -keyMultiplier * delta);
        }

        if (is->isKeyPressed(SDL_SCANCODE_A)) {
            transformation.translateRelative(-keyMultiplier * delta, 0.0f, 0.0f);
        }

        if (is->isKeyPressed(SDL_SCANCODE_D)) {
            transformation.translateRelative( keyMultiplier * delta, 0.0f, 0.0f);
        }

        int mouseDeltaX = is->getMouseDeltaX();
        int mouseDeltaY = is->getMouseDeltaY();
        if (mouseDeltaY != 0) {
            transformation.rotateRelative(glm::radians(mouseMultiplier * delta * mouseDeltaY), glm::vec3(1.0f, 0.0f, 0.0f));
        }

        if (mouseDeltaX != 0) {
            transformation.rotate(glm::radians(mouseMultiplier * delta * mouseDeltaX), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        transformation.forcedUpdate();
    }
    
    camera.update();
//     glm::vec3 pos = camera->getPosition();
//     LOG_D(pos.x << " " << pos.y << " " << pos.z);
    //---------------------------CAMERA ENDING
    
    UnorderedComponentMap<Camera>* cameras = static_cast<UnorderedComponentMap<Camera>*>(components[static_cast<std::size_t>(GraphicsComponent::Camera)].get());
    
    for (auto& keyCam : *cameras) {
        keyCam.second.update();
    }
    
    frustum.update(camera);
    
    if (skybox != nullptr) {
        skybox->update(delta);
    }

    performCulling();
    
    //LOG_D(visibleComponents.opaqueMeshEntityIDs.size() << " " << visibleComponents.transparentMeshEntityIDs.size());
    
    PhysicsSystem* physicsSystem = dynamic_cast<PhysicsSystem*>(manager->getSystemManagingComponentType(ComponentBaseType::Physics));
    if (manager->isEditorMode() && drawFrustum && physicsSystem != nullptr && physicsSystem->isDrawingDebug()) {
        Camera& tempCamera = getComponent<Camera>(drawnFrustumID);
        
        if (tempCamera.getRenderSurfaceSize() != renderSurfaceSize) {
            tempCamera.setRenderSurfaceSize(renderSurfaceSize);
        }
        
        tempCamera.update();
        Frustum drawnFrustum;
        drawnFrustum.update(tempCamera);
        drawnFrustum.drawDebug(dynamic_cast<BulletPhysicsDebugRenderer*>(physicsSystem->debugRenderer.get()));
    }
    
    World* world = dynamic_cast<World*>(manager);
    assert(world != nullptr);
    renderer->drawWorld(world);
}

void GraphicsSystem::createAndAttachComponent(const EntityKey& key, const ComponentType& type) {
    assert(!hasComponent(key.getID(), type));
    
    GraphicsComponent component = static_cast<GraphicsComponent>(type.getSubType());
    switch (component) {
        case GraphicsComponent::Mesh:{
            MeshComponent mc;
            mc.setRenderMode(MaterialRenderMode::Opaque);
            mc.setMesh(assetManager->getMissingAsset<Mesh>(AssetType::Mesh));
            
            #if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
                mc.setPreTransformBoundingVolume(mc.getMesh()->boundingSphere);
            #elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
                mc.setPreTransformBoundingVolume(mc.getMesh()->aabb);
            #endif // IYF_BOUNDING_VOLUME
            mc.updateRenderDataKey();
            
            setComponent(key.getID(), std::move(mc));
            
            break;
        }
        case GraphicsComponent::SkeletalMesh:
            throw std::runtime_error("NOT YET IMPLEMENTED");
            break;
        case GraphicsComponent::DirectionalLight:
            throw std::runtime_error("NOT YET IMPLEMENTED");
            break;
        case GraphicsComponent::PointLight:
            throw std::runtime_error("NOT YET IMPLEMENTED");
            break;
        case GraphicsComponent::SpotLight:
            throw std::runtime_error("NOT YET IMPLEMENTED");
            break;
        case GraphicsComponent::ParticleSystem:
            throw std::runtime_error("NOT YET IMPLEMENTED");
            break;
        case GraphicsComponent::Camera: {
            assert(!renderer->isRenderSurfaceSizeDynamic());
            Camera c(renderer->getRenderSurfaceSize());
            
            setComponent(key.getID(), std::move(c));
            
            break;
        }
        case GraphicsComponent::COUNT:
            throw std::runtime_error("COUNT is an invalid value");
    }
}
}
