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

#include "core/World.hpp"

#include "core/Engine.hpp"
#include "graphics/GraphicsSystem.hpp"
#include "physics/PhysicsSystem.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"

namespace iyf {
static void validateName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("The name of the World cannot be an empty string.");
    }
    
    if (name.length() >= 64) {
        throw std::logic_error("The name of the World is longer than 64 characters.");
    }
}

World::World(std::string name, Configuration* configuration, EntitySystemManagerCreateInfo createInfo) : Configurable(configuration), EntitySystemManager(std::move(createInfo)), isWorldInitialized(false), physicsDebugDrawn(false) {
    // Constructor needed for the sake of using unique_ptr with forward declarations
    assetManager = getEngine()->getAssetManager();
    
    validateName(name);
}

void World::setName(std::string name) {
    if (!isEditorMode()) {
        throw std::runtime_error("This method can only be called when the World is in editor mode");
    }
    validateName(name);
    
    this->name = name;
}

World::~World() {
    // Needed for the sake of using unique_ptr with forward declarations
}

void World::initialize() {
    if (isWorldInitialized) {
        return;
    }
    
    initializeSystems();
    
    EntitySystemManager::initialize();
    
    isWorldInitialized = true;
}

void World::dispose() {
    if (!isWorldInitialized) {
        return;
    }
    
    EntitySystemManager::dispose();
    
    isWorldInitialized = false;
}

void World::update(float delta) {
    EntitySystemManager::update(delta);
}

void World::addStaticMesh(hash32_t nameHash) {
    MeshComponent mc;
    mc.setRenderMode(MaterialRenderMode::Opaque);
    const fs::path& path = assetManager->getAssetPath(nameHash);
    mc.setMesh(assetManager->load<Mesh>(nameHash));
    
#if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
    mc.setPreTransformBoundingVolume(mc.getMesh()->boundingSphere);
#elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
    mc.setPreTransformBoundingVolume(mc.getMesh()->aabb);
#endif // IYF_BOUNDING_VOLUME
    
    mc.updateRenderDataKey();
    
    EntityKey entity = create(filePathToEntityName(path));
    TransformationComponent& transformation = getEntityTransformation(entity.getID());
    
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Physics));
    
    const MeshTypeManager* meshManager = static_cast<const MeshTypeManager*>(assetManager->getTypeManager(AssetType::Mesh));
    const Mesh& mesh = &(mc.getMesh());
    const auto& mapping = meshManager->getGraphicsToPhysicsDataMapping(mesh);
    
    attachComponent(entity, std::move(mc));
    
    TriangleMeshCollisionShapeCreateInfo createInfo(mapping.first, mapping.second, nameHash);
    attachComponent(entity, physicsSystem->prepareTriangleMeshRigidBody(createInfo, &(transformation)));
    //attachComponent(entity, ComponentType::RigidBody, physicsSystem->prepareSphereRigidBody(SphereCollisionShapeCreateInfo(1), 3, &(transformation)));
}

void World::addDynamicMesh(hash32_t nameHash) {
    MeshComponent mc;
    mc.setRenderMode(MaterialRenderMode::Opaque);
    const fs::path& path = assetManager->getAssetPath(nameHash);
    mc.setMesh(assetManager->load<Mesh>(nameHash));
    //mc.setMesh(assetManager->getMissingAsset<Mesh>(AssetType::Mesh));
    
#if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
    mc.setPreTransformBoundingVolume(mc.getMesh()->boundingSphere);
#elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
    mc.setPreTransformBoundingVolume(mc.getMesh()->aabb);
#endif // IYF_BOUNDING_VOLUME
    
    mc.updateRenderDataKey();
    
    EntityKey entity = create(filePathToEntityName(path));
    TransformationComponent& transformation = getEntityTransformation(entity.getID());
    transformation.setStatic(false);
    transformation.setPosition(0, 12, 0);
    transformation.setRotation(glm::quat());
    //transformation.setOrientation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)));
    transformation.setScale(1.0f, 1.0f, 1.0f);
    
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Physics));
    
    attachComponent(entity, std::move(mc));
    attachComponent(entity, physicsSystem->prepareSphereRigidBody(SphereCollisionShapeCreateInfo(1), 1, &(transformation)));
    //entities.push_back(entity);
}

void World::addCamera(bool setAsDefault) {
    EntityKey entity = create(makeUniqueName("Camera"));
    
    Camera camera;
    attachComponent(entity, std::move(camera));
    
    if (setAsDefault) {
        GraphicsSystem* graphicsSystem = static_cast<GraphicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Graphics));
        graphicsSystem->setActiveCameraID(entity.getID());
    }
}

void World::rayPick(std::uint32_t x, std::uint32_t y) {
    GraphicsSystem* renderSystem = static_cast<GraphicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Graphics));
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Physics));
    
    assert(physicsSystem != nullptr);
    assert(renderSystem != nullptr);
    
    assert(renderSystem != nullptr && physicsSystem != nullptr);
    physicsSystem->rayPick(x, y, renderSystem->getActiveCamera());
}

void World::setPhysicsDebugDrawn(bool newValue) {
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Physics));
    
    assert(physicsSystem != nullptr);
    assert(getSystemManagingComponentType(ComponentBaseType::Graphics) != nullptr);
    
    physicsSystem->setDrawDebug(newValue);
    physicsDebugDrawn = newValue;
}

void World::serialize(Serializer& fw) const {
    // 
}

void World::deserialize(Serializer& fr) {
    //
}
}
