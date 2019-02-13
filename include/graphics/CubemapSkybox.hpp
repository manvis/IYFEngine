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

#ifndef CUBEMAPSKYBOX_HPP
#define CUBEMAPSKYBOX_HPP

#include "graphics/Skybox.hpp"
#include "graphics/GraphicsAPI.hpp"
#include "assets/AssetHandle.hpp"
#include "utilities/hashing/Hashing.hpp"

#include <string>

namespace iyf {
class AssetManager;
class Shader;
class Texture;
class Mesh;

class CubemapSkybox : public Skybox {
public:
    CubemapSkybox(AssetManager* assetManager, Renderer* renderer, StringHash textureNameHash) : Skybox(renderer), assetManager(assetManager), textureNameHash(textureNameHash) {}
    
    virtual void initialize() final;
    virtual void dispose() final;

    virtual void update(float delta) final;
    virtual void draw(CommandBuffer* commandBuffer, const Camera* camera) const final;
protected:
    AssetManager* assetManager;
    StringHash textureNameHash;
    
    AssetHandle<Texture> skyCubemap;
    AssetHandle<Mesh> sphereMesh;
    ImageViewHnd skyCubemapView;
    SamplerHnd skyCubemapSampler;
    
    DescriptorPoolHnd descriptorPool;
    DescriptorSetLayoutHnd skyDescriptorSetLayout;
    DescriptorSetHnd skyTextureDescriptorSet;
    PipelineLayoutHnd skyPipelineLayout;
    AssetHandle<Shader> skyVertexShader;
    AssetHandle<Shader> skyFragmentShader;
    Pipeline skyPipeline;
    
    Buffer auxVBOHandle;
    Buffer auxIBOHandle;
    std::uint32_t skySphereOffsetVBO, skySphereOffsetIBO, skySphereSizeVBO, skySphereSizeIBO;
};
}

#endif /* CUBEMAPSKYBOX_HPP */

