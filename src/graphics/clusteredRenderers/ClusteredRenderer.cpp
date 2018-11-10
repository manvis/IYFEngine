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
#include "graphics/clusteredRenderers/ClusteredRendererConstants.hpp"
#include "graphics/VertexDataLayouts.hpp"
#include "graphics/Camera.hpp"
#include "graphics/CameraAndLightBufferLayout.hpp"
#include "graphics/TransformationBufferLayout.hpp"
#include "graphics/MaterialBufferLayout.hpp"
#include "graphics/Skybox.hpp"
#include "graphics/DebugRenderer.hpp"

#include "assets/typeManagers/MeshTypeManager.hpp"
#include "assets/assetTypes/Shader.hpp"

#include "core/World.hpp"
#include "core/Engine.hpp"
#include "core/InputState.hpp"

#include "physics/PhysicsSystem.hpp"

#include "threading/ThreadProfiler.hpp"

#include "utilities/DataSizes.hpp"

// TODO remove this

#include <iostream>

namespace iyf {
const std::string FullScreenQuadSource = 
R"code(#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 pos;

layout (location = 0) out VertexData {
    vec2 UV;
} vertOut;

out gl_PerVertex {
    vec4 gl_Position;
};

void main () {
    gl_Position = vec4(pos.xy, 0.0f, 1.0f);
    vertOut.UV = (pos.xy + vec2(1.0f, 1.0f)) * 0.5f;
})code";

const std::string TonemapSource = 
R"code(#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in VertexData {
    vec2 UV;
} inData;

layout (location = 0) out vec4 color;

layout(std140, push_constant) uniform adjustmentPushConstants {
    vec4 exposureAndEmpty;
} adjustments;

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput hdrData;

// Based on https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 acesTonemap(vec3 x) {
    float A = 2.51f;
    float B = 0.03f;
    float C = 2.43f;
    float D = 0.59f;
    float E = 0.14f;
    
    return clamp((x * (A * x + B)) / (x * (C * x + D) + E), 0.0f, 1.0f);
}

void main () {
    float exposure = adjustments.exposureAndEmpty.x;
    
    color.rgb = acesTonemap(0.6f * exposure * subpassLoad(hdrData).rgb);
    color.a = 1.0f;
})code";

const std::string IDFragmentSource = 
R"code(#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out uint value;

layout(std140, push_constant) uniform pickerPushBuffer {
    mat4 MVP;
    uint meshID;
} push;

void main () {
    value = push.meshID;
})code";

const std::string PositionOnlyVertexSource = 
R"code(#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 pos;

layout(std140, push_constant) uniform pickerPushBuffer {
    mat4 MVP;
    uint meshID;
} push;

out gl_PerVertex {
    vec4 gl_Position;
};

void main () {
    gl_Position = push.MVP * vec4(pos.xyz, 1.0f);
})code";

struct PushBuffer {
    glm::mat4 MVP;
    glm::mat4 M;
};

struct AdjustmentPushBuffer {
    float exposure;
    glm::vec3 padding;
};

struct PickerPushBuffer {
    glm::mat4 MVP;
    std::uint32_t objectID;
};

struct Cluster {
    std::uint32_t offset;
    std::uint32_t lightCount;
};

struct ClusterData {
    glm::vec4 gridParameters;
    Cluster clusters[MaxClusters];
    std::uint32_t lightIDs[MaxLightIDs];
};

ClusteredRenderer::ClusteredRenderer(Engine* engine, GraphicsAPI* gfx) : Renderer(engine, gfx) {
//     pickingEnabled = false;
}

void ClusteredRenderer::initializeRenderPasses() {
    RenderPassCreateInfo rpci;
    rpci.attachments.reserve(3);
    rpci.subpasses.reserve(2);
    
    // TODO transient attachments
    // 0 - this is where the rendered HDR world data goes to
    AttachmentDescription hdrColorAttachment;
    hdrColorAttachment.flags = AttachmentDescriptionFlags();
    hdrColorAttachment.format = Format::R16_G16_B16_A16_sFloat;
    hdrColorAttachment.samples = SampleCountFlagBits::X1;
    hdrColorAttachment.loadOp = AttachmentLoadOp::DoNotCare;
    hdrColorAttachment.storeOp = AttachmentStoreOp::DoNotCare;
    hdrColorAttachment.stencilLoadOp = AttachmentLoadOp::DoNotCare;
    hdrColorAttachment.stencilStoreOp = AttachmentStoreOp::DoNotCare;
    hdrColorAttachment.initialLayout = ImageLayout::Undefined;
    hdrColorAttachment.finalLayout = ImageLayout::ShaderReadOnlyOptimal;
    rpci.attachments.push_back(std::move(hdrColorAttachment));
    
    // 1 - the depth buffer
    AttachmentDescription depthAttachment;
    depthAttachment.flags = AttachmentDescriptionFlags();
    depthAttachment.format = gfx->getDepthStencilFormat();
    depthAttachment.samples = SampleCountFlagBits::X1;
    depthAttachment.loadOp = AttachmentLoadOp::Clear;
    depthAttachment.storeOp = AttachmentStoreOp::DoNotCare;
    depthAttachment.stencilLoadOp = AttachmentLoadOp::Clear;
    depthAttachment.stencilStoreOp = AttachmentStoreOp::DoNotCare;
    depthAttachment.initialLayout = ImageLayout::Undefined;
    depthAttachment.finalLayout = ImageLayout::DepthStencilAttachmentOptimal;
    rpci.attachments.push_back(std::move(depthAttachment));

    // 2 - the final destination. The tonemapped image and the UI are written to this attachment.
    AttachmentDescription finalAttachment;
    finalAttachment.flags = AttachmentDescriptionFlags();
    finalAttachment.format = gfx->getSurfaceFormat();
    finalAttachment.samples = SampleCountFlagBits::X1;
    finalAttachment.loadOp = AttachmentLoadOp::DoNotCare;
    finalAttachment.storeOp = AttachmentStoreOp::Store;
    finalAttachment.stencilLoadOp = AttachmentLoadOp::DoNotCare;
    finalAttachment.stencilStoreOp = AttachmentStoreOp::DoNotCare;
    finalAttachment.initialLayout = ImageLayout::ColorAttachmentOptimal;
    finalAttachment.finalLayout = ImageLayout::ColorAttachmentOptimal;
    rpci.attachments.push_back(std::move(finalAttachment));
    
    if (isPickingEnabled()) {
        // 3 - the ID buffer for pixel perfect picking
        AttachmentDescription idBufferAttachment;
        idBufferAttachment.flags = AttachmentDescriptionFlags();
        idBufferAttachment.format = Format::R32_uInt;
        idBufferAttachment.samples = SampleCountFlagBits::X1;
        idBufferAttachment.loadOp = AttachmentLoadOp::Clear;
        idBufferAttachment.storeOp = AttachmentStoreOp::Store;
        idBufferAttachment.stencilLoadOp = AttachmentLoadOp::DoNotCare;
        idBufferAttachment.stencilStoreOp = AttachmentStoreOp::DoNotCare;
        idBufferAttachment.initialLayout = ImageLayout::Undefined;
        idBufferAttachment.finalLayout = ImageLayout::TransferSrcOptimal;
        rpci.attachments.push_back(std::move(idBufferAttachment));
    }
    
    // First subpass
    AttachmentReference hdrColorWriteReference;
    hdrColorWriteReference.attachment = 0;
    hdrColorWriteReference.layout = ImageLayout::ColorAttachmentOptimal;
    
    AttachmentReference depthAttachmentReference;
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = ImageLayout::DepthStencilAttachmentOptimal;
    
    SubpassDescription worldSubpass;
    worldSubpass.pipelineBindPoint = PipelineBindPoint::Graphics;
    worldSubpass.colorAttachments.push_back(hdrColorWriteReference);
    worldSubpass.depthStencilAttachment = depthAttachmentReference;
    
    rpci.subpasses.push_back(std::move(worldSubpass));
    
    // Second subpass
    AttachmentReference finalColorWriteReference;
    finalColorWriteReference.attachment = 2;
    finalColorWriteReference.layout = ImageLayout::ColorAttachmentOptimal;
    
    AttachmentReference hdrColorReadReference;
    hdrColorReadReference.attachment = 0;
    hdrColorReadReference.layout = ImageLayout::ShaderReadOnlyOptimal;
    
    SubpassDescription finalSubpass;
    finalSubpass.pipelineBindPoint = PipelineBindPoint::Graphics;
    finalSubpass.colorAttachments.push_back(finalColorWriteReference);
    if (isPickingEnabled()) {
        finalSubpass.preserveAttachments.push_back(1);
    }
    finalSubpass.inputAttachments.push_back(hdrColorReadReference);
    
    rpci.subpasses.push_back(std::move(finalSubpass));
    
    if (isPickingEnabled()) {
        // Third subpass
        AttachmentReference idBufferAttachmentReference;
        idBufferAttachmentReference.attachment = 3;
        idBufferAttachmentReference.layout = ImageLayout::ColorAttachmentOptimal;
        
        AttachmentReference depthReadAttachmentReference;
        depthReadAttachmentReference.attachment = 1;
        depthReadAttachmentReference.layout = ImageLayout::DepthStencilReadOnlyOptimal;
        
        SubpassDescription idSubpass;
        idSubpass.pipelineBindPoint = PipelineBindPoint::Graphics;
        idSubpass.colorAttachments.push_back(idBufferAttachmentReference);
        idSubpass.depthStencilAttachment = depthReadAttachmentReference;
        
        rpci.subpasses.push_back(std::move(idSubpass));
    }
    
    // Dependencies
    SubpassDependency hdrWriteToReadDependency;
    hdrWriteToReadDependency.srcSubpass = 0;
    hdrWriteToReadDependency.dstSubpass = 1;
    hdrWriteToReadDependency.srcStageMask = PipelineStageFlagBits::ColorAttachmentOutput;
    hdrWriteToReadDependency.dstStageMask = PipelineStageFlagBits::FragmentShader;
    hdrWriteToReadDependency.srcAccessMask = AccessFlagBits::ColorAttachmentWrite;
    hdrWriteToReadDependency.dstAccessMask = AccessFlagBits::InputAttachmentRead;
    hdrWriteToReadDependency.dependencyFlags = DependencyFlagBits::ByRegion;
    
    rpci.dependencies.push_back(std::move(hdrWriteToReadDependency));
    
    if (isPickingEnabled()) {
        SubpassDependency depthReadDependency;
        depthReadDependency.srcSubpass = 0;
        depthReadDependency.dstSubpass = 2;
        depthReadDependency.srcStageMask = PipelineStageFlagBits::EarlyFragmentTests | PipelineStageFlagBits::LateFragmentTests;
        depthReadDependency.dstStageMask = PipelineStageFlagBits::EarlyFragmentTests;
        depthReadDependency.srcAccessMask = AccessFlagBits::DepthStencilAttachmentWrite;
        depthReadDependency.dstAccessMask = AccessFlagBits::DepthStencilAttachmentRead;
        depthReadDependency.dependencyFlags = DependencyFlagBits::ByRegion;
        
        rpci.dependencies.push_back(std::move(depthReadDependency));
        
        // TODO is this enough?
        SubpassDependency idBufferTransfer;
        idBufferTransfer.srcSubpass = 2;
        idBufferTransfer.dstSubpass = renderingConstants::ExternalSubpass;
        idBufferTransfer.srcStageMask = PipelineStageFlagBits::ColorAttachmentOutput;
        idBufferTransfer.dstStageMask = PipelineStageFlagBits::Transfer;
        idBufferTransfer.srcAccessMask = AccessFlagBits::ColorAttachmentWrite;
        idBufferTransfer.dstAccessMask = AccessFlagBits::TransferRead;
        idBufferTransfer.dependencyFlags = DependencyFlagBits::ByRegion;
        
        rpci.dependencies.push_back(std::move(idBufferTransfer));
    }
    
    mainRenderPass = gfx->createRenderPass(rpci);
}

bool ClusteredRenderer::isRenderSurfaceSizeDynamic() const {
    return false;
}

glm::uvec2 ClusteredRenderer::getRenderSurfaceSize() const {
    return gfx->getSwapchainImageSize();
}

void ClusteredRenderer::initializeFramebuffers() {
    const glm::uvec2 extent = gfx->getSwapchainImageSize();
    
    UncompressedImageCreateInfo uiciHDR;
    uiciHDR.type = ImageMemoryType::RGBAHalf;
    uiciHDR.dimensions = extent;
    uiciHDR.usedAsColorOrDepthAttachment = true;
    uiciHDR.usedAsInputAttachment = true;
    hdrAttachmentImage = gfx->createUncompressedImage(uiciHDR);
    
    UncompressedImageCreateInfo uiciDepth;
    uiciDepth.type = ImageMemoryType::DepthStencilFloat;
    uiciDepth.dimensions = extent;
    uiciDepth.usedAsColorOrDepthAttachment = true;
    uiciDepth.usedAsInputAttachment = false;
    depthImage = gfx->createUncompressedImage(uiciDepth);
    
    if (isPickingEnabled()) {
        UncompressedImageCreateInfo uiciID;
        uiciID.type = ImageMemoryType::R32;
        uiciID.dimensions = extent;
        uiciID.usedAsColorOrDepthAttachment = true;
        uiciID.usedAsInputAttachment = false;
        uiciID.usedAsTransferSource = true;
        idImage = gfx->createUncompressedImage(uiciID);
    }
    
    for (std::uint32_t i = 0; i < gfx->getSwapImageCount(); ++i) {
        std::vector<std::variant<Image, FramebufferAttachmentCreateInfo>> createInfo;
        
        createInfo.push_back(hdrAttachmentImage);
        createInfo.push_back(depthImage);
        createInfo.push_back(gfx->getSwapImage(i));
        
        if (isPickingEnabled()) {
            createInfo.push_back(idImage);
        }
        
        mainFramebuffers.push_back(gfx->createFramebufferWithAttachments(extent, mainRenderPass, createInfo));
    }
    
    LOG_V("Finished initializing the framebuffer");
}

void ClusteredRenderer::initializeMainRenderpassComponents() {
    SamplerCreateInfo ciHdrSampler;
    ciHdrSampler.mipmapMode = SamplerMipmapMode::Nearest;// TODO Linear?
    ciHdrSampler.addressModeU = SamplerAddressMode::ClampToEdge;
    ciHdrSampler.addressModeV = SamplerAddressMode::ClampToEdge;
    ciHdrSampler.addressModeW = SamplerAddressMode::ClampToEdge;
    ciHdrSampler.minLod = 0.0f;
    ciHdrSampler.maxLod = 1.0;
    ciHdrSampler.borderColor = BorderColor::FloatOpaqueWhite;
    hdrAttachmentSampler = gfx->createSampler(ciHdrSampler);
    
    DescriptorSetLayoutBinding hdrSourceBinding;
    hdrSourceBinding.binding = 0;
    hdrSourceBinding.descriptorType = DescriptorType::InputAttachment;
    hdrSourceBinding.descriptorCount = 1;
    hdrSourceBinding.stageFlags = ShaderStageFlagBits::Fragment;
    
    DescriptorSetLayoutCreateInfo dslciTonemap;
    dslciTonemap.bindings.push_back(std::move(hdrSourceBinding));
    tonemapSourceDescriptorSetLayout = gfx->createDescriptorSetLayout(dslciTonemap);
    
    DescriptorSetAllocateInfo dsaiTonemap;
    dsaiTonemap.descriptorPool = internalDescriptorPool;
    dsaiTonemap.setLayouts.push_back(tonemapSourceDescriptorSetLayout);
    tonemapSourceDescriptorSet = gfx->allocateDescriptorSets(dsaiTonemap);
    
    DescriptorImageInfo hdrImageInfo;
    // All framebuffers use the same image, therefore we can use the same view
    hdrImageInfo.imageView = mainFramebuffers[0].imageViews[0];
    hdrImageInfo.sampler = hdrAttachmentSampler;
    hdrImageInfo.imageLayout = ImageLayout::ShaderReadOnlyOptimal;
    
    WriteDescriptorSet wdsTonemap;
    wdsTonemap.dstBinding = 0;
    wdsTonemap.dstArrayElement = 0;
    wdsTonemap.descriptorCount = 1;
    wdsTonemap.descriptorType = DescriptorType::InputAttachment;
    wdsTonemap.dstSet = tonemapSourceDescriptorSet[0];
    wdsTonemap.imageInfos.push_back(std::move(hdrImageInfo));
    
    gfx->updateDescriptorSets({wdsTonemap});
}

void ClusteredRenderer::initializePickingPipeline() {
    if (!isPickingEnabled()) {
        return;
    }
    
    idVS = gfx->createShaderFromSource(iyf::ShaderStageFlagBits::Vertex, PositionOnlyVertexSource);
    idFS = gfx->createShaderFromSource(iyf::ShaderStageFlagBits::Fragment, IDFragmentSource);
    
    PushConstantRange pickerPushBuffer;
    pickerPushBuffer.offset = 0;
    pickerPushBuffer.size = sizeof(PickerPushBuffer);
    pickerPushBuffer.stageFlags = iyf::ShaderStageFlagBits::Vertex | iyf::ShaderStageFlagBits::Fragment;
    
    iyf::PipelineLayoutCreateInfo plciPicker;
    plciPicker.pushConstantRanges.push_back(std::move(pickerPushBuffer));
    pickingPipelineLayout = gfx->createPipelineLayout(plciPicker);
    
    PipelineCreateInfo pci;
    pci.shaders = {{iyf::ShaderStageFlagBits::Vertex, idVS}, {iyf::ShaderStageFlagBits::Fragment, idFS}};
    pci.layout = pickingPipelineLayout;
    pci.rasterizationState.frontFace = iyf::FrontFace::Clockwise;
    pci.depthStencilState.depthTestEnable = true;
    pci.depthStencilState.depthWriteEnable = false;
    pci.depthStencilState.depthCompareOp = CompareOp::GreaterEqual;
    pci.vertexInputState = con::GetVertexDataLayoutDefinition(VertexDataLayout::MeshVertex).createVertexInputStateCreateInfo(0);
    pci.inputAssemblyState.topology = iyf::PrimitiveTopology::TriangleList;
    pci.renderPass = mainRenderPass;
    pci.subpass = 2;
    
    pickingPipeline = gfx->createPipeline(pci);
    
    BufferCreateInfo bci(BufferUsageFlagBits::TransferDestination, Bytes(128), MemoryUsage::CPUOnly, false, "PickResultBuffer");
    
    pickResultBuffer = gfx->createBuffer(bci);
}

void ClusteredRenderer::initializeTonemappingAndAdjustmentPipeline() {
    // TODO turn these into system assets
    fullScreenQuadVS = gfx->createShaderFromSource(iyf::ShaderStageFlagBits::Vertex, FullScreenQuadSource);
    tonemapFS = gfx->createShaderFromSource(iyf::ShaderStageFlagBits::Fragment, TonemapSource);
    
    PushConstantRange adjustmentPushBuffer;
    adjustmentPushBuffer.offset = 0;
    adjustmentPushBuffer.size = sizeof(AdjustmentPushBuffer);
    adjustmentPushBuffer.stageFlags = iyf::ShaderStageFlagBits::Fragment;
    
    iyf::PipelineLayoutCreateInfo plciTonemap;
    plciTonemap.setLayouts.push_back(tonemapSourceDescriptorSetLayout);
    plciTonemap.pushConstantRanges.push_back(std::move(adjustmentPushBuffer));
    tonemapPipelineLayout = gfx->createPipelineLayout(plciTonemap);
    
    iyf::PipelineCreateInfo pciTonemap;
    pciTonemap.shaders = {{iyf::ShaderStageFlagBits::Vertex, fullScreenQuadVS}, {iyf::ShaderStageFlagBits::Fragment, tonemapFS}};
    pciTonemap.layout = tonemapPipelineLayout;
    pciTonemap.renderPass = mainRenderPass;
    pciTonemap.rasterizationState.frontFace = iyf::FrontFace::Clockwise;
    pciTonemap.depthStencilState.depthCompareOp = CompareOp::Greater;
    pciTonemap.depthStencilState.depthTestEnable = false;
    pciTonemap.depthStencilState.depthWriteEnable = false;
    pciTonemap.inputAssemblyState.topology = iyf::PrimitiveTopology::TriangleList;
    pciTonemap.rasterizationState.cullMode = iyf::CullModeFlagBits::None;
    pciTonemap.subpass = 1;
    pciTonemap.vertexInputState = con::GetVertexDataLayoutDefinition(VertexDataLayout::MeshVertex).createVertexInputStateCreateInfo(0);
    tonemapPipeline = gfx->createPipeline(pciTonemap);
}

void ClusteredRenderer::initialize() {
    const std::size_t cameraAndLightDataSize = sizeof(CameraAndLightData);
    const std::size_t transformationDataSize = sizeof(TransformationData);
    const std::size_t clusterDataSize = sizeof(ClusterData);
    const std::size_t materialDataSize = sizeof(MaterialData);
    const std::size_t sumOfSizes = cameraAndLightDataSize + transformationDataSize + clusterDataSize + materialDataSize;
    LOG_D("TOTAL SIZE: " << cameraAndLightDataSize << " + " 
                         << transformationDataSize << " + "
                         << clusterDataSize << " + "
                         << materialDataSize << " = "
                         << sumOfSizes);
    
    commandPool = gfx->createCommandPool(QueueType::Graphics, 0);
    commandBuffers = commandPool->allocateCommandBuffers(static_cast<std::uint32_t>(CommandBufferID::COUNT));
    worldRenderComplete = gfx->createSemaphore();
    preGUIFence = gfx->createFence(false);
    
    initializeRenderPasses();
    initializeFramebuffers();
    
    AssetManager* manager = engine->getAssetManager();
    fullScreenQuad = manager->getSystemAsset<Mesh>("fullScreenQuad.dae");
    vsSimple = manager->getSystemAsset<Shader>("defaultVertex.vert");
    
    DescriptorPoolCreateInfo dpciInternal;
    dpciInternal.maxSets = 1;
    dpciInternal.poolSizes.push_back({DescriptorType::InputAttachment, 1});
    internalDescriptorPool = gfx->createDescriptorPool(dpciInternal);
    
    initializeMainRenderpassComponents();
    initializeTonemappingAndAdjustmentPipeline();
    initializePickingPipeline();
    
    // TODO remove -------------------------------------------------------------------------------------------------------
    iyf::PipelineLayoutCreateInfo plci{{}, {{iyf::ShaderStageFlagBits::Vertex, 0, sizeof(PushBuffer)}}};
    pipelineLayout = gfx->createPipelineLayout(plci);
    
    iyf::ShaderStageFlags ssf = iyf::ShaderStageFlagBits::Vertex | iyf::ShaderStageFlagBits::Fragment;
    
    iyf::PipelineCreateInfo pci;

    fsSimpleFlat = manager->getSystemAsset<Shader>("randomTests.frag");
    pci.shaders = {{iyf::ShaderStageFlagBits::Vertex, vsSimple->handle}, {iyf::ShaderStageFlagBits::Fragment, fsSimpleFlat->handle}};
    pci.layout = pipelineLayout;
    pci.rasterizationState.frontFace = iyf::FrontFace::Clockwise; // TODO if reverse z, GreaterEqual or Greater?
    pci.rasterizationState.lineWidth = 2.0f;
    pci.depthStencilState.depthCompareOp = CompareOp::Greater;
    //pci.rasterizationState.polygonMode = iyf::PolygonMode::Line;
    pci.inputAssemblyState.topology = iyf::PrimitiveTopology::LineList;
    pci.renderPass = mainRenderPass;
    
    pci.vertexInputState = con::GetVertexDataLayoutDefinition(VertexDataLayout::MeshVertex).createVertexInputStateCreateInfo(0);
    
    pci.inputAssemblyState.topology = iyf::PrimitiveTopology::TriangleList;//iyf::PrimitiveTopology::PointList;
    //pci.rasterizationState.cullMode = iyf::CullModeFlagBits::None;
    simpleFlatPipeline = gfx->createPipeline(pci);
    // TODO remove END -----------------------------------------------------------------------------------------------------
    
    initialized = true;
    LOG_V("Finished initializing the renderer")
}

std::pair<RenderPassHnd, std::uint32_t> ClusteredRenderer::getImGuiRenderPassAndSubPass() {
    return {mainRenderPass, 1};
}

void ClusteredRenderer::disposeRenderPasses() {
    gfx->destroyRenderPass(mainRenderPass);
}

void ClusteredRenderer::disposeFramebuffers() {
    for (const auto& framebuffer : mainFramebuffers) {
        gfx->destroyFramebufferWithAttachments(framebuffer);
    }
    
    gfx->destroyImage(depthImage);
    gfx->destroyImage(hdrAttachmentImage);
    
    if (!isPickingEnabled()) {
        return;
    }
    
    gfx->destroyImage(idImage);
}

void ClusteredRenderer::destroyPickingPipeline() {
    if (!isPickingEnabled()) {
        throw std::logic_error("This function can only be used when picking is enabled.");
    }
    
    gfx->destroyBuffers({pickResultBuffer});
    gfx->destroyPipelineLayout(pickingPipelineLayout);
    gfx->destroyPipeline(pickingPipeline);
    gfx->destroyShader(idVS);
    gfx->destroyShader(idFS);
}

void ClusteredRenderer::dispose() {
    gfx->destroyPipeline(simpleFlatPipeline);
    vsSimple.release();
    fsSimpleFlat.release();
    gfx->destroyPipelineLayout(pipelineLayout);
    
    commandPool->freeCommandBuffers(commandBuffers);
    gfx->destroyCommandPool(commandPool);
    gfx->destroySemaphore(worldRenderComplete);
    gfx->destroyFence(preGUIFence);
    
    gfx->destroyPipeline(tonemapPipeline);
    gfx->destroyPipelineLayout(tonemapPipelineLayout);
    gfx->destroyDescriptorSetLayout(tonemapSourceDescriptorSetLayout);
    gfx->destroySampler(hdrAttachmentSampler);
    gfx->destroyDescriptorPool(internalDescriptorPool);
    
    gfx->destroyShader(fullScreenQuadVS);
    gfx->destroyShader(tonemapFS);
    
    destroyPickingPipeline();
    disposeFramebuffers();
    disposeRenderPasses();
    
    initialized = false;
}

std::pair<RenderPassHnd, std::uint32_t> ClusteredRenderer::getSkyboxRenderPassAndSubPass() {
    return {mainRenderPass, 0};
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
    rpbi.framebuffer = mainFramebuffers[gfx->getCurrentSwapImage()].handle;
    rpbi.renderPass = mainRenderPass;
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = getRenderSurfaceSize();
    rpbi.clearValues.push_back(ClearColorValue(0.0f, 1.0f, 0.0f, 1.0f));
    rpbi.clearValues.push_back(ClearDepthStencilValue(0.0f, 0));
    rpbi.clearValues.push_back(ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f));
    rpbi.clearValues.push_back(ClearColorValue(std::numeric_limits<std::uint32_t>::max(), 0, 0, 0, true));

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
    
    worldBuffer->nextSubpass();
    
    const MeshTypeManager* meshManager = dynamic_cast<const MeshTypeManager*>(engine->getAssetManager()->getTypeManager(AssetType::Mesh));
    const PrimitiveData& primitiveData = fullScreenQuad->getMeshPrimitiveData();
    
    AdjustmentPushBuffer adjustmentBuffer;
    adjustmentBuffer.exposure = camera.getExposure();
    
    assert(tonemapSourceDescriptorSet.size() == 1);
    worldBuffer->bindPipeline(tonemapPipeline);
    worldBuffer->bindDescriptorSets(PipelineBindPoint::Graphics, tonemapPipelineLayout, 0, tonemapSourceDescriptorSet, emptyDynamicOffsets);
    worldBuffer->bindVertexBuffers(0, meshManager->getVertexBuffer(fullScreenQuad->vboID));
    worldBuffer->bindIndexBuffer(meshManager->getIndexBuffer(fullScreenQuad->iboID), IndexType::UInt16);
    worldBuffer->pushConstants(tonemapPipelineLayout, ShaderStageFlagBits::Fragment, 0, sizeof(AdjustmentPushBuffer), &adjustmentBuffer);
    
    worldBuffer->drawIndexed(primitiveData.indexCount, 1, primitiveData.indexOffset, primitiveData.vertexOffset, 0);
    
    drawImGui(engine->getImGuiImplementation());
    
    drawIDBuffer(graphicsSystem);
    
    worldBuffer->endRenderPass();
    
    if (isPickingEnabled()) {
        bool pendingPromises;
        {
            std::lock_guard<std::mutex> lock(promiseMutex);
            pendingPromises = !promiseList.empty();
        }
        
        if (pendingPromises) {
            BufferImageCopy bic;
            bic.bufferRowLength = 0;
            bic.bufferImageHeight = 0;
            bic.bufferOffset = 0;
            
            /// WARNING TODO FIXME - If I ever decide to render in higher (or lower) resolution, this will break, hence the assert
            assert(!isRenderSurfaceSizeDynamic());
            bic.imageOffset = glm::ivec3(engine->getInputState()->getMouseX(), engine->getInputState()->getMouseY(), 0);
            bic.imageExtent = glm::uvec3(1, 1, 1);
            
            worldBuffer->copyImageToBuffer(idImage, ImageLayout::TransferSrcOptimal, pickResultBuffer, {std::move(bic)});
        }
    }
    
    worldBuffer->end();
}

void ClusteredRenderer::drawIDBuffer(const GraphicsSystem* graphicsSystem) {
    IYFT_PROFILE(DrawIDBuffer, iyft::ProfilerTag::Graphics);
    
    if (!isPickingEnabled()) {
        return;
    }
    
    CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
    const GraphicsSystem::VisibleComponents& visibleComponents = graphicsSystem->getVisibleComponents();
    
    worldBuffer->nextSubpass();
    
    if (visibleComponents.opaqueMeshEntityIDs.size() == 0) {
        return;
    }
    
    const MeshTypeManager* meshManager = dynamic_cast<const MeshTypeManager*>(engine->getAssetManager()->getTypeManager(AssetType::Mesh));
    
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
    worldBuffer->bindPipeline(pickingPipeline);
    
    for (const auto& vc : visibleComponents.opaqueMeshEntityIDs) {
        //THIS IS PROBABLY BAD
        const MeshComponent& c = static_cast<const MeshComponent&>(components.get(vc.componentID));
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
            PickerPushBuffer pushBuffer;
            pushBuffer.MVP = VP * transform.getModelMatrix();
            pushBuffer.objectID = vc.componentID;
            
            worldBuffer->pushConstants(pipelineLayout, ShaderStageFlagBits::Vertex, 0, sizeof(PushBuffer), &pushBuffer);
            
            const PrimitiveData& primitiveData = mesh->getMeshPrimitiveData();
            worldBuffer->drawIndexed(primitiveData.indexCount, 1, primitiveData.indexOffset, primitiveData.vertexOffset, 0);
        } else {
            throw std::runtime_error("TODO IMPLEMENT ME");
        }
    }
    
//     worldBuffer->nextSubpass();
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
        const MeshComponent& c = static_cast<const MeshComponent&>(components.get(vc.componentID));
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
    
    // TODO this blocks until upload completes. I should use a pipeline barrier instead.
    gfx->getDeviceMemoryManager()->beginBatchUpload(MemoryBatch::PerFrameData);
    
    CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
    
//     worldBuffer->endRenderPass();
//     worldBuffer->end();
//     
//     // Just clear everything
//     if (!drawingWorldThisFrame) {
//         drawingWorldThisFrame = true;
//         
//         CommandBuffer* worldBuffer = commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
//         worldBuffer->begin();
//         
//         RenderPassBeginInfo rpbi;
//         rpbi.framebuffer = mainFramebuffers[gfx->getCurrentSwapImage()].handle;
//         rpbi.renderPass = mainRenderPass;
//         rpbi.renderArea.offset = {0, 0};
//         rpbi.renderArea.extent = {gfx->getRenderSurfaceWidth(), gfx->getRenderSurfaceHeight()};
//         rpbi.clearValues.push_back(ClearColorValue(1.0f, 1.0f, 0.0f, 1.0f));
//         rpbi.clearValues.push_back(ClearDepthStencilValue(0.0f, 0));
//         
//         worldBuffer->beginRenderPass(rpbi);
//         worldBuffer->endRenderPass();
//         
//         worldBuffer->end();
//     }
//     
//     if (imGuiSubmissionRequired) {
//         SubmitInfo si;
//         // World
//         si.waitSemaphores = {gfx->getPresentationCompleteSemaphore()};
//         si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
//         si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)]->getHandle()};
//         si.signalSemaphores = {worldRenderComplete};
// //         gfx->submitQueue(si);
// //         // TODO why does this FLICKER without a fence. What am I missing? I know what's missing and I need to implement it.
//         gfx->submitQueue(si, preGUIFence);
//         
//         {
//             IYFT_PROFILE(BubbleToRemove, iyft::ProfilerTag::Graphics);
//             gfx->waitForFence(preGUIFence, std::numeric_limits<std::uint64_t>::max());
//             gfx->resetFence(preGUIFence);
//         }
// 
//         // ImGui
//         si.waitSemaphores = {worldRenderComplete};
//         si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
//         si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::ImGui)]->getHandle()};
//         si.signalSemaphores = {gfx->getRenderCompleteSemaphore()};
//         gfx->submitQueue(si);
//     } else {
//         SubmitInfo si;
//         // World only
//         si.waitSemaphores = {gfx->getPresentationCompleteSemaphore()};
//         si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
//         si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)]->getHandle()};
//         si.signalSemaphores = {gfx->getRenderCompleteSemaphore()};
//         gfx->submitQueue(si);
//     }
    
    SubmitInfo si;
    // World only
    si.waitSemaphores = {gfx->getPresentationCompleteSemaphore()};
    si.waitDstStageMask = {PipelineStageFlagBits::BottomOfPipe};
    si.commandBuffers = {commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)]->getHandle()};
    si.signalSemaphores = {gfx->getRenderCompleteSemaphore()};
    gfx->submitQueue(si);
    
    // This resets flags
    Renderer::submitCommandBuffers();
}

CommandBuffer* ClusteredRenderer::getImGuiDesignatedCommandBuffer() {
    return commandBuffers[static_cast<std::uint32_t>(CommandBufferID::World)];
}

void ClusteredRenderer::retrieveDataFromIDBuffer() {
    if (!isPickingEnabled()) {
        throw std::logic_error("This function can only be used when picking is enabled.");
    }
    
    std::lock_guard<std::mutex> guard(promiseMutex);
    
    if (promiseList.empty()) {
        return;
    }
    
    // TODO if I decide to have multiple frames in flight, I should start using different positions for different frames.
    // I should add fence waits here as well
    std::uint32_t value;
    gfx->readHostVisibleBuffer(pickResultBuffer, {BufferCopy(0, 0, sizeof(value))}, &value);
//     LOG_V("VAL: " << value);
    
    for (auto& p : promiseList) {
        p.set_value(value);
    }
    
    promiseList.clear();
}

std::future<std::uint32_t> ClusteredRenderer::getHoveredItemID() {
    if (!isPickingEnabled()) {
        throw std::logic_error("This function can only be used when picking is enabled.");
    }
    
    std::lock_guard<std::mutex> guard(promiseMutex);
    
    promiseList.emplace_back();
    return promiseList.back().get_future();
}
}
