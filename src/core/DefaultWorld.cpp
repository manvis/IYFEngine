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

#include "core/DefaultWorld.hpp"
#include "graphics/Skybox.hpp"
#include "core/Engine.hpp"
#include "core/InputState.hpp"
#include "graphics/GraphicsSystem.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/MeshComponent.hpp"
#include "physics/bullet/BulletPhysicsSystem.hpp"
#include "graphics/DebugRenderer.hpp"

namespace iyf {
DefaultWorld::DefaultWorld(std::string name, Configuration* configuration, EntitySystemManagerCreateInfo createInfo) : World(std::move(name), configuration, std::move(createInfo)) {
    //
}

void DefaultWorld::initialize() {
    if (isWorldInitialized) {
        return;
    }
    
    World::initialize();
    
    
    
//     Entity e = entitySystemManager.create();
//     RenderDataComponent rmc;
//     rmc.setRenderMode(con::MaterialRenderMode::Transparent);
//     BoundingSphere bs(glm::vec3(0.0f, 0.0f, 0.0f), 20.0f);
//     rmc.setBoundingSphere(bs);
//     entitySystemManager.attachComponent(e, ComponentType::RenderData, rmc);
    
    isWorldInitialized = true;
}

void DefaultWorld::initializeSystems() {
    registerSystem(std::make_unique<GraphicsSystem>(this, getEngine()->getGraphicsAPI()));
#ifdef IYF_PHYSICS_BULLET
    registerSystem(std::make_unique<BulletPhysicsSystem>(this));
#else
#error "Physics engine was not set"
#endif
}

void DefaultWorld::dispose() {
    if (!isWorldInitialized) {
        return;
    }
    
    World::dispose();
    
    isWorldInitialized = false;
}

void DefaultWorld::update(float delta) {
    GraphicsSystem* graphicsSystem = static_cast<GraphicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Graphics));
    graphicsSystem->setCameraInputPaused(isInputProcPaused);
    
    World::update(delta);
}
}
