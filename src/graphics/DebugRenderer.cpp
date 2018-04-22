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

#include "graphics/DebugRenderer.hpp"

#include "graphics/Camera.hpp"
#include "graphics/Renderer.hpp"
#include "graphics/VertexDataLayouts.hpp"

#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Shader.hpp"

#include "glm/mat4x4.hpp"

namespace iyf {
struct DebugPushBuffer {
    glm::mat4 VP;
};

void DebugRenderer::initialize() {
    std::uint32_t totalVertices = MaxDebugLineVertices + MaxDebugPontVertices;
    vertices.resize(totalVertices);
    
    GraphicsAPI* api = renderer->getGraphicsAPI();
    
    const VertexDataLayoutDefinition& vertexLayout = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::ColoredDebugVertex)];
    
    std::vector<Buffer> vbos;
    
    BufferCreateInfo bci;
    bci.flags = BufferUsageFlagBits::VertexBuffer | BufferUsageFlagBits::TransferDestination;
    bci.size = Bytes(vertexLayout.getSize() * totalVertices);
    
    LOG_D("Physics debug buffer size: " << Mebibytes(bci.size) << " MiB")
    
    api->createBuffers({bci}, MemoryType::DeviceLocal, vbos);
    
    vbo = vbos[0];
    
    PipelineLayoutCreateInfo plci{{}, {{ShaderStageFlagBits::Vertex, 0, sizeof(DebugPushBuffer)}}};
    pipelineLayout = api->createPipelineLayout(plci);
    
    PipelineCreateInfo pci;
    
    vs = assetManager->getSystemAsset<Shader>("physicsDebug.vert");
    fs = assetManager->getSystemAsset<Shader>("physicsDebug.frag");
    pci.shaders = {{ShaderStageFlagBits::Vertex, vs->handle}, {ShaderStageFlagBits::Fragment, fs->handle}};
    pci.layout = pipelineLayout;
    //pci.rasterizationState.frontFace = FrontFace::CounterClockwise; // TODO if reverse z, GreaterEqual or Greater?
    pci.rasterizationState.frontFace = FrontFace::Clockwise;
    pci.rasterizationState.lineWidth = 1.0f;
    pci.depthStencilState.depthWriteEnable = true;
    pci.depthStencilState.depthTestEnable = true;
    pci.depthStencilState.depthCompareOp = CompareOp::Greater;
    //pci.rasterizationState.polygonMode = PolygonMode::Fill;
    //pci.rasterizationState.cullMode = CullModeFlagBits::None;
    pci.inputAssemblyState.topology = PrimitiveTopology::LineList;
    pci.renderPass = renderer->getSkyboxRenderPassAndSubPass().first;
    pci.subpass = renderer->getSkyboxRenderPassAndSubPass().second;
    pci.vertexInputState = vertexLayout.createVertexInputStateCreateInfo(0);

    physicsDebugPipeline = api->createPipeline(pci);
    
    lineVertexCount = 0;
    contactPointVertexCount = MaxDebugLineVertices;
    
    isInit = true;
}

void DebugRenderer::dispose() {
    isInit = false;
    
    GraphicsAPI* api = renderer->getGraphicsAPI();
    api->destroyPipeline(physicsDebugPipeline);
    vs.release();
    fs.release();
    api->destroyPipelineLayout(pipelineLayout);
    api->destroyBuffers({vbo});
}

// No need for delta.
void DebugRenderer::update(float) {
    // Reset data and that's it
    lineVertexCount = 0;
    // All vertices are stored into the same array. Line vertices go before contact point vertices, hence the offset
    contactPointVertexCount = MaxDebugLineVertices;
}
    
void DebugRenderer::draw(CommandBuffer* commandBuffer, const Camera* camera) const {
    if (lineVertexCount == 0 && contactPointVertexCount == MaxDebugLineVertices) {
        return;
    }
    
    const VertexDataLayoutDefinition& vertexLayout = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::ColoredDebugVertex)];
    
    // TODO This causes horrible performance in large scenes when wireframe is enabled. Unfortunately, partial updates aren't
    // easy when you're given data line by line. Is there anything that can be done?
    GraphicsAPI* api = renderer->getGraphicsAPI();
    api->updateDeviceVisibleBuffer(vbo, {{0, 0, lineVertexCount * vertexLayout.getSize()}}, vertices.data());
    
    commandBuffer->bindPipeline(physicsDebugPipeline);
    commandBuffer->bindVertexBuffers(0, vbo);
    
    DebugPushBuffer dpb;
    dpb.VP = camera->getProjection() * camera->getViewMatrix();
    commandBuffer->pushConstants(pipelineLayout, ShaderStageFlagBits::Vertex, 0, sizeof(DebugPushBuffer), &dpb);
    
    commandBuffer->draw(lineVertexCount, 1, 0, 0);
}
}
