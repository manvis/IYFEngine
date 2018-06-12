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

#include "graphics/clusteredRenderers/ClusteredRenderer.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/assetTypes/Shader.hpp"
#include "core/World.hpp"
#include "graphics/Camera.hpp"
#include "core/Engine.hpp"
#include "graphics/Skybox.hpp"
#include "graphics/Camera.hpp"
#include "graphics/DebugRenderer.hpp"
#include "physics/PhysicsSystem.hpp"
#include "threading/ThreadProfiler.hpp"

#include <iostream>

namespace iyf {
const std::uint32_t ClusterVolumeX = 16;
const std::uint32_t ClusterVolumeY =  8;
const std::uint32_t ClusterVolumeZ = 24;

const std::uint32_t MaxClusters = ClusterVolumeX * ClusterVolumeY * ClusterVolumeZ;
const std::uint32_t MaxLightIDs = MaxClusters * 16;

const std::string MaxClustersName = "MAX_CLUSTERS";
const std::string MaxLightIDsName = "MAX_LIGHT_IDS";

struct PushBuffer {
    glm::mat4 MVP;
    glm::mat4 M;
};

ClusteredRenderer::ClusteredRenderer(Engine* engine, GraphicsAPI* api) : Renderer(engine, api), specializationConstants(con::DefaultSpecializationConstants.begin(), con::DefaultSpecializationConstants.end()) {
    specializationConstants.emplace_back(MaxClustersName, Format::R32_uInt, MaxClusters);
    specializationConstants.emplace_back(MaxLightIDsName, Format::R32_uInt, MaxLightIDs);
}

void ClusteredRenderer::initialize() {
    commandPool = api->createCommandPool(QueueType::Graphics, 0);
    commandBuffers = commandPool->allocateCommandBuffers(static_cast<std::uint32_t>(CommandBufferID::COUNT));
    worldRenderComplete = api->createSemaphore();
    preGUIFence = api->createFence(false);
    
    // TODO remove -------------------------------------------------------------------------------------------------------
    iyf::PipelineLayoutCreateInfo plci{{}, {{iyf::ShaderStageFlagBits::Vertex, 0, sizeof(PushBuffer)}}};
    pipelineLayout = api->createPipelineLayout(plci);
    
    iyf::ShaderStageFlags ssf = iyf::ShaderStageFlagBits::Vertex | iyf::ShaderStageFlagBits::Fragment;
    
    iyf::PipelineCreateInfo pci;
    
    AssetManager* manager = engine->getAssetManager();
    vsSimpleFlat = manager->getSystemAsset<Shader>("defaultVertex.vert");
    fsSimpleFlat = manager->getSystemAsset<Shader>("randomTests.frag");
    pci.shaders = {{iyf::ShaderStageFlagBits::Vertex, vsSimpleFlat->handle}, {iyf::ShaderStageFlagBits::Fragment, fsSimpleFlat->handle}};
    pci.layout = pipelineLayout;
    pci.rasterizationState.frontFace = iyf::FrontFace::Clockwise; // TODO if reverse z, GreaterEqual or Greater?
    pci.rasterizationState.lineWidth = 2.0f;
    pci.depthStencilState.depthCompareOp = CompareOp::Greater;
    //pci.rasterizationState.polygonMode = iyf::PolygonMode::Line;
    pci.inputAssemblyState.topology = iyf::PrimitiveTopology::LineList;
    pci.renderPass = api->getWriteToScreenRenderPass();
    
    
    pci.vertexInputState = con::VertexDataLayoutDefinitions[static_cast<std::size_t>(VertexDataLayout::MeshVertex)].createVertexInputStateCreateInfo(0);
    
    pci.inputAssemblyState.topology = iyf::PrimitiveTopology::TriangleList;//iyf::PrimitiveTopology::PointList;
    //pci.rasterizationState.cullMode = iyf::CullModeFlagBits::None;
    simpleFlatPipeline = api->createPipeline(pci);
    // TODO remove END -----------------------------------------------------------------------------------------------------
    
    initialized = true;
}

void ClusteredRenderer::dispose() {
    api->destroyPipeline(simpleFlatPipeline);
    vsSimpleFlat.release();
    fsSimpleFlat.release();
    api->destroyPipelineLayout(pipelineLayout);
    
    commandPool->freeCommandBuffers(commandBuffers);
    api->destroyCommandPool(commandPool);
    api->destroySemaphore(worldRenderComplete);
    api->destroyFence(preGUIFence);
    
    initialized = false;
}

std::pair<RenderPassHnd, std::uint32_t> ClusteredRenderer::getSkyboxRenderPassAndSubPass() {
    return {api->getWriteToScreenRenderPass(), 0};
}

// void ClusteredRenderer::drawWorld(const World* world) {
// 
//     Renderer::drawWorld(world);
//     
//     
// }

void ClusteredRenderer::drawWorld(const World* world) {
    IYFT_PROFILE(DrawWorld, iyft::ProfilerTag::Graphics);
    assert(world != nullptr);
    
    if (drawingWorldThisFrame) {
        throw std::runtime_error("Only one world can be drawn every frame");
    }
    
    drawingWorldThisFrame = true;
    
    const GraphicsSystem* graphicsSystem = dynamic_cast<const GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    assert(graphicsSystem != nullptr);
    
    const Camera& camera = graphicsSystem->getActiveCamera();
    if (camera.getMode() != Camera::Mode::ReverseZ) {
        LOG_W("The Camera used with this Renderer must be in ReverseZ mode.");
    }
    
    CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
    worldBuffer->begin();
    
    RenderPassBeginInfo rpbi;
    rpbi.framebuffer = api->getScreenFramebuffer();
    rpbi.renderPass = api->getWriteToScreenRenderPass();
    rpbi.renderArea.offset = {0, 0};
    //LOG_D(api->getRenderSurfaceWidth() << " " << api->getRenderSurfaceHeight())
    rpbi.renderArea.extent = {api->getRenderSurfaceWidth(), api->getRenderSurfaceHeight()};
    rpbi.clearValues.push_back(ClearColorValue(0.0f, 1.0f, 0.0f, 1.0f));
    rpbi.clearValues.push_back(ClearDepthStencilValue(0.0f, 0));

    worldBuffer->beginRenderPass(rpbi);
    
    //LOG_D(availableComponents.size() << " " << manager->getEntityCount() << " " << visibleTransparentEntityIDs.size() << " " << visibleOpaqueEntityIDs.size());
    drawVisibleOpaque(graphicsSystem);
    //renderer->drawVisibleTransparent(visibleTransparentEntityIDs, manager->getEntityTransformations(), components);
    drawVisibleTransparent(graphicsSystem);
    
    const PhysicsSystem* physicsSystem = dynamic_cast<const PhysicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Physics));
    if (physicsSystem != nullptr) {
        drawDebugAndHelperMeshes(world, physicsSystem->getDebugRenderer());
    }
    
    drawSky(world);
    // beginPostProcess
    
    drawImGui(engine->getImGuiImplementation());
}

void ClusteredRenderer::drawVisibleOpaque(const GraphicsSystem* graphicsSystem) {
    IYFT_PROFILE(DrawOpaque, iyft::ProfilerTag::Graphics);
    
    //visibleOpaqueEntityIDs, manager->getEntityTransformations(), components;
    
    const GraphicsSystem::VisibleComponents& visibleComponents = graphicsSystem->getVisibleComponents();
    
    if (visibleComponents.opaqueMeshEntityIDs.size() == 0) {
        return;
    }
    
    const MeshTypeManager* meshManager = dynamic_cast<const MeshTypeManager*>(engine->getAssetManager()->getTypeManager(AssetType::Mesh));
    CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
    
    const ChunkedMeshVector& components = graphicsSystem->getMeshComponents();
    const MeshComponent& firstComponent = static_cast<const MeshComponent&>(components.get(visibleComponents.opaqueMeshEntityIDs[0].componentID));
    const AssetHandle<Mesh>& firstMesh = firstComponent.getMesh();
    
    std::uint8_t previousVBO = firstMesh->vboID;
    std::uint8_t previousIBO = firstMesh->iboID;
    
    Buffer ibo = meshManager->getIndexBuffer(previousIBO);
    Buffer vbo = meshManager->getVertexBuffer(previousVBO);
    
    worldBuffer->bindVertexBuffers(0, vbo);
    worldBuffer->bindIndexBuffer(ibo, IndexType::UInt16);
    
    const Camera& camera = graphicsSystem->getActiveCamera();
    glm::mat4 VP = camera.getProjection() * camera.getViewMatrix();
    worldBuffer->bindPipeline(simpleFlatPipeline);
    
    for (const auto& vc : visibleComponents.opaqueMeshEntityIDs) {
        const MeshComponent& c = static_cast<const MeshComponent&>(components.get(visibleComponents.opaqueMeshEntityIDs[0].componentID));
        const AssetHandle<Mesh>& mesh = c.getMesh();
        
        if (previousVBO != mesh->vboID) {
            previousVBO = mesh->vboID;
            vbo = meshManager->getVertexBuffer(previousVBO);
            
            worldBuffer->bindVertexBuffers(0, vbo);
        }
        
        if (previousIBO != mesh->iboID) {
            previousIBO = mesh->iboID;
            ibo = meshManager->getIndexBuffer(previousIBO);
            
            worldBuffer->bindIndexBuffer(ibo, IndexType::UInt16);
        }
        
        const EntitySystemManager* manager = graphicsSystem->getManager();
        const TransformationVector& transformations = manager->getEntityTransformations();
        
        const TransformationComponent transform = transformations[vc.componentID];
        if (mesh->submeshCount == 1) {
            PushBuffer pushBuffer;
            pushBuffer.MVP = VP * transform.getModelMatrix();
            pushBuffer.M = transform.getModelMatrix();
            
//             glm::mat4 MVPmatrix = camera.getProjection() * camera.getViewMatrix() * transform.getModelMatrix();
//             assert(pushBuffer.MVP == MVPmatrix);
//             LOG_D("HEY");
            
            worldBuffer->pushConstants(pipelineLayout, ShaderStageFlagBits::Vertex, 0, sizeof(PushBuffer), &pushBuffer);
            
            const PrimitiveData& primitiveData = mesh->getMeshPrimitiveData();
            worldBuffer->drawIndexed(primitiveData.indexCount, 1, primitiveData.indexOffset, primitiveData.vertexOffset, 0);
        } else {
            throw std::runtime_error("TODO IMPLEMENT ME");
        }
    }
}

void ClusteredRenderer::drawVisibleTransparent(const GraphicsSystem* graphicsSystem) {
    IYFT_PROFILE(DrawTransparent, iyft::ProfilerTag::Graphics);
//    for (const auto& i : elements) {
//        //
//    }
}

void ClusteredRenderer::drawSky(const World* world) {
    IYFT_PROFILE(DrawSky, iyft::ProfilerTag::Graphics);
    
    const GraphicsSystem* graphicsSystem = dynamic_cast<const GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
    assert(graphicsSystem != nullptr);
    
    const Skybox* skybox = graphicsSystem->getSkybox();
    if (skybox != nullptr && skybox->isInitialized()) {
        const Camera& camera = graphicsSystem->getActiveCamera();
        
        skybox->draw(commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)], &camera);
    }
}

void ClusteredRenderer::drawDebugAndHelperMeshes(const World* world, const DebugRenderer* renderer) {
    IYFT_PROFILE(DrawDebugAndHelper, iyft::ProfilerTag::Graphics);
    
    if (world->isPhysicsDebugDrawn()) {
        const GraphicsSystem* graphicsSystem = dynamic_cast<const GraphicsSystem*>(world->getSystemManagingComponentType(ComponentBaseType::Graphics));
        assert(graphicsSystem != nullptr);
        
        const Camera& camera = graphicsSystem->getActiveCamera();
        
        renderer->draw(commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)], &camera);
    }
}

void ClusteredRenderer::submitCommandBuffers() {
    IYFT_PROFILE(SubmitCommandBuffers, iyft::ProfilerTag::Graphics);
    
    CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
    
    worldBuffer->endRenderPass();
    worldBuffer->end();
    
    // Just clear everything
    if (!drawingWorldThisFrame) {
        drawingWorldThisFrame = true;
        
        CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
        worldBuffer->begin();
        
        RenderPassBeginInfo rpbi;
        rpbi.framebuffer = api->getScreenFramebuffer();
        rpbi.renderPass = api->getWriteToScreenRenderPass();;
        rpbi.renderArea.offset = {0, 0};
        rpbi.renderArea.extent = {api->getRenderSurfaceWidth(), api->getRenderSurfaceHeight()};
        rpbi.clearValues.push_back(ClearColorValue(1.0f, 1.0f, 0.0f, 1.0f));
        rpbi.clearValues.push_back(ClearDepthStencilValue(0.0f, 0));
        
        worldBuffer->beginRenderPass(rpbi);
        worldBuffer->endRenderPass();
        
        worldBuffer->end();
    }
    
    if (imGuiSubmissionRequired) {
        SubmitInfo si;
        // World
        si.waitSemaphores = {api->getPresentationCompleteSemaphore()};
        si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
        si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)]->getHandle()};
        si.signalSemaphores = {worldRenderComplete};
//         api->submitQueue(si);
//         // TODO why does this FLICKER without a fence. What am I missing? I know what's missing and I need to implement it.
        api->submitQueue(si, preGUIFence);
        
        {
            IYFT_PROFILE(BubbleToRemove, iyft::ProfilerTag::Graphics);
            api->waitForFence(preGUIFence, std::numeric_limits<std::uint64_t>::max());
            api->resetFence(preGUIFence);
        }

        // ImGui
        si.waitSemaphores = {worldRenderComplete};
        si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
        si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::ImGui)]->getHandle()};
        si.signalSemaphores = {api->getRenderCompleteSemaphore()};
        api->submitQueue(si);
    } else {
        SubmitInfo si;
        // World only
        si.waitSemaphores = {api->getPresentationCompleteSemaphore()};
        si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
        si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)]->getHandle()};
        si.signalSemaphores = {api->getRenderCompleteSemaphore()};
        api->submitQueue(si);
    }
    
    // This resets flags
    Renderer::submitCommandBuffers();
}

CommandBuffer* ClusteredRenderer::getImGuiDesignatedCommandBuffer() {
    return commandBuffers[static_cast<std::uint32_t>(CommandBufferID::ImGui)];
}

std::string ClusteredRenderer::makeRenderDataSet(ShaderLanguage language) const {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Only GLSLVulkan is supported by this renderer");
    }
    
    std::stringstream ss;
    ss << "struct Cluster {\n";
    ss << "    uint offset;\n";
    ss << "    uint lightCount;\n";
    ss << "};\n\n";
    
    ss << "layout(std" << "430" << ", set = " << con::RendererDataBuffer.set << ", binding = " << con::RendererDataBuffer.binding << ") buffer ClusterDataBuffer {\n";
    ss << "    vec4 gridParameters;\n";
    ss << "    Cluster clusters[" << MaxClustersName << "];\n";
    ss << "    uint lightIDs[" << MaxLightIDsName << "];\n";
    ss << "} clusterData; \n\n";
    
    return ss.str();
}

std::string ClusteredRenderer::makeLightLoops(ShaderLanguage language, const std::string& lightingFunction) const {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Only GLSLVulkan is supported by this renderer");
    }
    
    std::stringstream ss;
    
    ss << "    // WARNING - TEST ONLY!!! These are poorly performing regular forward rendering loops.\n";
    ss << "    // Directional lights\n";
    ss << "    for (int i = 0; i < cameraAndLights.directionalLightCount; ++i) {\n";
    ss << "        vec3 lightDirection = cameraAndLights.directionalLights[i].direction;\n";
    ss << "        vec3 lightColor = cameraAndLights.directionalLights[i].color;\n";
    ss << "        float lightIntensity = cameraAndLights.directionalLights[i].intensity;\n";
    ss << "\n";
    ss << "        " << lightingFunction << "\n";
    ss << "    }\n\n";
    
//         float falloff = clamp(1.0f - (DdivR * DdivR * DdivR * DdivR), 0.0f, 1.0f);
//         falloff *= falloff;
//         falloff = falloff / (lightDistance * lightDistance + 1);
// 
//         if(falloff > 0.0f) {
//             color.rgb += falloff * cookTorranceGGX(normal, viewDirection, lightDirection, lightsAndCamera.lightData[i].color.xyz, diffuseColor, 1.0f, 0.1f, 0.3f); // Point specular + diffuse
//         }
    ss << "    // Point lights\n";
    ss << "    for (int i = 0; i < cameraAndLights.pointLightCount; ++i) {\n";
    ss << "        vec3 lightDirection = normalize(cameraAndLights.pointLights[i].position - fragmentInput.positionWS);\n";
    ss << "        float lightDistance = length(cameraAndLights.pointLights[i].position - fragmentInput.positionWS);\n\n";
    ss << "        float DdivR = lightDistance / cameraAndLights.pointLights[i].radius;\n";
    ss << "        float falloff = clamp(1.0f - (DdivR * DdivR * DdivR * DdivR), 0.0f, 1.0f);\n";
    ss << "        falloff *= falloff;\n";
    ss << "        falloff = falloff / (lightDistance * lightDistance + 1);\n\n";
    ss << "        vec3 lightColor = cameraAndLights.pointLights[i].color;\n";
    ss << "        float lightIntensity = cameraAndLights.pointLights[i].intensity;\n\n";
    ss << "        if (falloff > 0.0f) {\n";
    ss << "            " << lightingFunction << "\n";
    ss << "        }\n";
    ss << "    }\n\n";
    
    // TODO implement spotlights
    ss << "   // TODO implement spotlights\n\n";
    
    return ss.str();
}

}
