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

#include "graphics/CubemapSkybox.hpp"

#include "graphics/Renderer.hpp"
#include "graphics/Skybox.hpp"
#include "graphics/Camera.hpp"

#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Shader.hpp"
#include "assets/assetTypes/Texture.hpp"
#include "assets/assetTypes/Mesh.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "core/Logger.hpp"

#include <glm/mat4x4.hpp>

namespace iyf {
void CubemapSkybox::initialize() {
    if (!renderer->isInitialized()) {
        throw std::runtime_error("Can't initialize a skybox before its parent renderer is initialized.");
    }
    
    if (isInit) {
        return;
    }
    
    auto passInfo = renderer->getSkyboxRenderPassAndSubPass();
    
    GraphicsAPI* api = renderer->getGraphicsAPI();
    
    // TODO asynchronous loading? Pass texture directly instead of a name hash?
    skyCubemap = assetManager->load<Texture>(textureNameHash, false);
    skyCubemapView = api->createDefaultImageView(skyCubemap->image);
    skyCubemapSampler = api->createPresetSampler(SamplerPreset::SkyBox, static_cast<float>(skyCubemap->image.getMipLevels()));
    
    DescriptorPoolCreateInfo dpci{1, {{DescriptorType::CombinedImageSampler, 1}}};
    descriptorPool = api->createDescriptorPool(dpci);
    
    DescriptorSetLayoutCreateInfo dslci{{{0, DescriptorType::CombinedImageSampler, 1, ShaderStageFlagBits::Fragment, {}}}};
    skyDescriptorSetLayout = api->createDescriptorSetLayout(dslci);
    
    DescriptorSetAllocateInfo dsai{descriptorPool, {skyDescriptorSetLayout}};
    skyTextureDescriptorSet = api->allocateDescriptorSets(dsai)[0];
    
    WriteDescriptorSet wds{skyTextureDescriptorSet, 0, 0, 1, DescriptorType::CombinedImageSampler, {{skyCubemapSampler, skyCubemapView, ImageLayout::General}}, {}, {}};
    api->updateDescriptorSets({wds});
    
    PipelineLayoutCreateInfo plci{{skyDescriptorSetLayout}, {{ShaderStageFlagBits::Vertex, 0, sizeof(glm::mat4)}}};
    skyPipelineLayout = api->createPipelineLayout(plci);
    
    skyVertexShader = assetManager->getSystemAsset<Shader>("skyBox.vert");
    skyFragmentShader = assetManager->getSystemAsset<Shader>("skyBox.frag");
    
    PipelineCreateInfo pci;
    pci.shaders = {{ShaderStageFlagBits::Vertex, skyVertexShader->handle}, {ShaderStageFlagBits::Fragment, skyFragmentShader->handle}};
    pci.depthStencilState.depthWriteEnable = false;
    pci.depthStencilState.depthCompareOp = CompareOp::GreaterEqual; // TODO if reverse z, GreaterEqual or Greater?
    pci.rasterizationState.frontFace = FrontFace::CounterClockwise;
    pci.renderPass = passInfo.first;
    pci.subpass = passInfo.second;
    pci.layout = skyPipelineLayout;
    pci.vertexInputState = con::GetVertexDataLayoutDefinition(VertexDataLayout::MeshVertex).createVertexInputStateCreateInfo(0);
    
    skyPipeline = api->createPipeline(pci);
    
    sphereMesh = assetManager->getSystemAsset<Mesh>("sphere.dae");
    
    isInit = true;
}

void CubemapSkybox::dispose() {
    if (!isInit) {
        return;
    }
    
    GraphicsAPI* api = renderer->getGraphicsAPI();
    api->destroyPipeline(skyPipeline);
    skyVertexShader.release();
    skyFragmentShader.release();
    
    api->destroyPipelineLayout(skyPipelineLayout);
    
    api->destroyDescriptorSetLayout(skyDescriptorSetLayout);
    api->destroyDescriptorPool(descriptorPool);
    
    api->destroySampler(skyCubemapSampler);
    api->destroyImageView(skyCubemapView);
    skyCubemap.release();
    
    isInit = false;
}

void CubemapSkybox::draw(CommandBuffer* commandBuffer, const Camera* camera) const {
    if (!isInit) {
        throw std::runtime_error("Attempted to render a non-initialized skybox.");
    }

    const MeshTypeManager* meshManager = static_cast<const MeshTypeManager*>(assetManager->getTypeManager(AssetType::Mesh));
    commandBuffer->bindPipeline(skyPipeline);
    
    const Buffer& VBO = meshManager->getVertexBuffer(sphereMesh->vboID);
    
    const Buffer& IBO = meshManager->getIndexBuffer(sphereMesh->iboID);
    commandBuffer->bindVertexBuffers(0, VBO);
    commandBuffer->bindIndexBuffer(IBO, IndexType::UInt16);
    
    glm::mat4 mvp = camera->getProjection() * glm::mat4(glm::mat3(camera->getViewMatrix()));

    commandBuffer->pushConstants(skyPipelineLayout, ShaderStageFlagBits::Vertex, 0, sizeof(glm::mat4), &mvp);
    commandBuffer->bindDescriptorSets(PipelineBindPoint::Graphics, skyPipelineLayout, 0, {skyTextureDescriptorSet}, {});
    
    const PrimitiveData& primitiveData = sphereMesh->getMeshPrimitiveData();
    commandBuffer->drawIndexed(primitiveData.indexCount, 1, primitiveData.indexOffset, primitiveData.vertexOffset, 0);
}

void CubemapSkybox::update(float delta) {
    //This type of skybox does not need updates
}

}
