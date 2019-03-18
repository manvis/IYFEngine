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

#include "graphics/LightComponent.hpp"
#include "graphics/GraphicsSystem.hpp"

namespace iyf {
LightComponent::LightComponent() : Component(ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Light)), parent(nullptr), lightType(LightType::Point) {
    light.position = glm::vec3(0.0f, 0.0f, 0.0f);
    light.radius = 1.0f;
    light.color = glm::vec3(1.0f, 1.0f, 1.0f);
    light.intensity = 1.0f;
    light.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    light.angle = 25.0f;
}

void LightComponent::onTransformationChanged(TransformationComponent* transformation) {
    fetchDataFromTransform(transformation);
}

void LightComponent::attach(System* system, std::uint32_t ownID) {
    parent = system;
    id = ownID;
    
    fetchDataFromTransform(nullptr);
}

void LightComponent::detach(System*, std::uint32_t) {
    parent = nullptr;
}

void LightComponent::setLightType(LightType lightType) {
    this->lightType = lightType;
    
    fetchDataFromTransform(nullptr);
}

void LightComponent::fetchDataFromTransform(const TransformationComponent* transformation) {
    if (parent == nullptr) {
        return;
    }
    
    if (transformation == nullptr) {
        assert(dynamic_cast<GraphicsSystem*>(parent) != nullptr);
        
        GraphicsSystem* gfxSystem = static_cast<GraphicsSystem*>(parent);
        EntitySystemManager* manager = gfxSystem->getManager();
        
        transformation = &manager->getEntityTransformation(id);
    }
    
    switch (lightType) {
        case LightType::Directional:
            light.position = transformation->getRotation() * glm::vec3(0.0, -1.0f, 0.0f);
            break;
        case LightType::Point:
            light.position = transformation->getPosition();
            break;
        case LightType::Spot:
            light.position = transformation->getPosition();
            light.direction = transformation->getRotation() * glm::vec3(0.0, -1.0f, 0.0f);
            break;
    }
}

}
