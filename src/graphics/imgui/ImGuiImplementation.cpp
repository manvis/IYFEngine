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

#include "core/Engine.hpp"
#include "core/InputState.hpp"
#include "ImGuiImplementation.hpp"
#include "core/Constants.hpp"
#include "logging/Logger.hpp"
#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Shader.hpp"
#include "assets/assetTypes/Font.hpp"
#include "assets/assetTypes/Texture.hpp"
#include "threading/ThreadProfiler.hpp"
#include "graphics/Renderer.hpp"

#include "imgui.h"
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TODO use engine native handles for scancodes, avoiding this explicit SDL dependency
#include <SDL2/SDL.h>

namespace iyf {

// TODO would be nice if the engine worked in 32 bit systems as well.
static_assert(sizeof(ImTextureID) == 8, "ImTextureID (a.k.a. a void pointer) must fit 64 bits of data");
    
static const size_t MAX_IMGUI_VBO = sizeof(ImDrawVert) * 8192 * 8;
static const size_t MAX_IMGUI_IBO = sizeof(ImDrawIdx) * 8192 * 8;

static const std::uint32_t MAX_DESCRIPTOR_COUNT = 64;
static const std::uint32_t MAX_UNIFORM_BUFFER_DESCRIPTOR_COUNT = 1;
static const std::uint32_t MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT = 63;
    
bool ImGuiImplementation::requestRenderThisFrame() {
    // Don't repeat initialization. Just in case 2 states get drawn at the same time (e.g., game and pause) and both have separate GUIs
    if (frameHasAlreadyBegun) {
        return false;
    }
    
    // Various buffers, render passes, etc. are only initialized when some component actually needs ImGui in order not to wase resources.
    // ImGui use should be extremely rare in end user's gameplay (e.g., when making mods). A game should use its own "prettier" GUI.
    if (!assetsInitialized) {
        initializeAssets();
    }
    
    ImGuiIO& io = ImGui::GetIO();

    GraphicsAPI* gfxAPI = engine->getGraphicsAPI();
    InputState* is = engine->getInputState();
    
    const glm::uvec2 windowSize = gfxAPI->getWindowSize();
    // Imgui should always be rendered to the final swapchain image
    const glm::uvec2 swapImageSize = gfxAPI->getSwapchainImageSize();
    
    io.DisplaySize = ImVec2(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
    
    float widthScale = windowSize.x > 0 ? (static_cast<float>(swapImageSize.x) / windowSize.x) : 0;
    float heightScale = windowSize.y > 0 ? (static_cast<float>(swapImageSize.y) / windowSize.y) : 0;
    io.DisplayFramebufferScale = ImVec2(widthScale, heightScale);

    // Update Delta
    io.DeltaTime = engine->getGraphicsDelta();

    bool hasMouseFocus = SDL_GetWindowFlags(engine->getWindow()) & SDL_WINDOW_MOUSE_FOCUS;
    if (hasMouseFocus) {
        io.MousePos = ImVec2(static_cast<float>(is->getMouseX()), static_cast<float>(is->getMouseY()));
    } else {
        // No focus? Hide the rodent
        io.MousePos = ImVec2(-1.0f, -1.0f);
    }

    if (is->getMouseWheelDeltaX() > 0) {
        io.MouseWheelH =  1.0f;
    } else if (is->getMouseWheelDeltaX() < 0) {
        io.MouseWheelH = -1.0f;
    } else {
        io.MouseWheelH =  0.0f;
    }
    
    if (is->getMouseWheelDeltaY() > 0) {
        io.MouseWheel =  1.0f;
    } else if (is->getMouseWheelDeltaY() < 0) {
        io.MouseWheel = -1.0f;
    } else {
        io.MouseWheel =  0.0f;
    }
    
    io.MouseDown[0] = is->isMouseClicked(MouseButton::Left);
    io.MouseDown[1] = is->isMouseClicked(MouseButton::Right);
    io.MouseDown[2] = is->isMouseClicked(MouseButton::Middle);
    io.MouseDown[3] = is->isMouseClicked(MouseButton::Extra1);
    io.MouseDown[4] = is->isMouseClicked(MouseButton::Extra2);

    io.KeyShift = is->getKeyModifierState(KeyModifierFlagBits::Shift);
    io.KeyCtrl  = is->getKeyModifierState(KeyModifierFlagBits::Ctrl);
    io.KeyAlt   = is->getKeyModifierState(KeyModifierFlagBits::Alt);
    io.KeySuper = is->getKeyModifierState(KeyModifierFlagBits::GUI);
    
    io.AddInputCharactersUTF8(temp.c_str());
    temp.clear();
    ImGui::NewFrame();
    
    frameHasAlreadyBegun = true;
    return true;
}

void ImGuiImplementation::onMouseMoved(int, int, int, int, bool) { }
void ImGuiImplementation::onMouseWheelMoved(int, int) { }
void ImGuiImplementation::onMouseButtonDown(int, int, int, MouseButton) { }
void ImGuiImplementation::onMouseButtonUp(int, int, int, MouseButton) { }

void ImGuiImplementation::onKeyPressed(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags) {
    ImGuiIO& io = ImGui::GetIO();
    int val = keycode & ~SDLK_SCANCODE_MASK;
    
    //assert(val == scancode);
    io.KeysDown[val] = true;
    
//     LOG_D("PRESS" << val << " " << scancode << " " << keycode)
}
void ImGuiImplementation::onKeyReleased(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags) {
    ImGuiIO& io = ImGui::GetIO();
    int val = keycode & ~SDLK_SCANCODE_MASK;
    
    //assert(val == scancode);
    io.KeysDown[val] = false;
    
//     LOG_D("RELEASE" <<  val << " " << scancode << " " << keycode)
}
void ImGuiImplementation::onTextInput(const char* text) {
    temp += text;
}

void ImGuiImplementation::initialize() {
    context = ImGui::CreateContext();
    ImGui::SetCurrentContext(context);
    
    // TODO scancodes or keycodes? Does it break in other locales?
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SDLK_a;
    io.KeyMap[ImGuiKey_C] = SDLK_c;
    io.KeyMap[ImGuiKey_V] = SDLK_v;
    io.KeyMap[ImGuiKey_X] = SDLK_x;
    io.KeyMap[ImGuiKey_Y] = SDLK_y;
    io.KeyMap[ImGuiKey_Z] = SDLK_z;

    //io.RenderDrawListsFn = nullptr;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;
    io.ClipboardUserData = engine;
    io.UserData = engine;
    
    engine->getInputState()->addInputListener(this);
}

void ImGuiImplementation::dispose() {
    if (assetsInitialized) {
        disposeAssets();
    }
    
    engine->getInputState()->removeInputListener(this);
    ImGui::DestroyContext(context);
}

void ImGuiImplementation::initializeAssets() {
    GraphicsAPI* gfxAPI = engine->getGraphicsAPI();
    assetManager = engine->getAssetManager();
    Renderer* renderer = engine->getRenderer();
    
    // Font texture atlas
    ImGuiIO& io = ImGui::GetIO();
    
    // TODO fetch only those glyphs that are actually used
    // Glyph ranges for English + Lithuanian
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0,
    };
    
    AssetHandle<Font> fontAsset = assetManager->getSystemAsset<Font>(con::ImGuiFont());
    
    void* fontData = std::malloc(fontAsset->size);
    std::memcpy(fontData, fontAsset->data, fontAsset->size);
    
    ImFontConfig fontCfg;
    
    // Oversampling helps us reduce the amount of blur that appears when zooming fonts (e.g., in the material
    // node editor).
    //
    // TODO Signed distance fields would be better (stb_true type can generate them, I think).
    fontCfg.OversampleH = 8;
    fontCfg.OversampleV = 8;
    
    // This transfers the ownership to ImGui
    io.Fonts->AddFontFromMemoryTTF(fontData, fontAsset->size, con::ImGuiFontSize, &fontCfg, ranges);
    int fontAtlasWidth, fontAtlasHeight;
    unsigned char* pixelData;
    
    //io.Fonts->GetTexDataAsAlpha8(&pixelData, &fontAtlasWidth, &fontAtlasHeight);
    io.Fonts->GetTexDataAsRGBA32(&pixelData, &fontAtlasWidth, &fontAtlasHeight);
    
    LOG_V("Creating a font atlas for ImGui: {}px x {}px", fontAtlasWidth, fontAtlasHeight)
    
    UncompressedImageCreateInfo uici;
    uici.type = ImageMemoryType::RGBA;
    uici.dimensions = glm::ivec2(fontAtlasWidth, fontAtlasHeight);
    uici.data = pixelData;
    
    fontAtlas = gfxAPI->createUncompressedImage(uici, "ImGui font atlas image");
    
    imguiDefaultSampler = gfxAPI->createPresetSampler(SamplerPreset::ImguiTexture);
    customTextureSampler = gfxAPI->createPresetSampler(SamplerPreset::Default3DModel);
    
    fontView = gfxAPI->createDefaultImageView(fontAtlas, "ImGui font atlas image view");
    io.Fonts->TexID = nullptr;//fontView.toNative<void*>();
    
    // Descriptor pool and layouts
    DescriptorPoolCreateInfo dpci{MAX_DESCRIPTOR_COUNT, {{DescriptorType::UniformBuffer, MAX_UNIFORM_BUFFER_DESCRIPTOR_COUNT},
                                                         {DescriptorType::CombinedImageSampler, MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT}}};
    descriptorPool = gfxAPI->createDescriptorPool(dpci, "ImGui descriptor pool");
    
    DescriptorSetLayoutCreateInfo dslci{{{0, DescriptorType::CombinedImageSampler, 1, ShaderStageFlagBits::Fragment, {}}}};
    descriptorSetLayout = gfxAPI->createDescriptorSetLayout(dslci, "ImGui descriptor set layout");
    
    // Shaders and pipelines
    PipelineLayoutCreateInfo plci{{descriptorSetLayout}, {{ShaderStageFlagBits::Vertex, 0, sizeof(glm::mat4)}}};
    pipelineLayout = gfxAPI->createPipelineLayout(plci, "ImGui pipeline layout");
    
    PipelineCreateInfo pci;
    
    vertexShader = assetManager->getSystemAsset<Shader>("imgui.vert");
    fragmentShader = assetManager->getSystemAsset<Shader>("imgui.frag");
    
    pci.shaders = {{ShaderStageFlagBits::Vertex, vertexShader->handle}, {ShaderStageFlagBits::Fragment, fragmentShader->handle}};
    
    pci.depthStencilState.depthTestEnable = false;
    pci.depthStencilState.depthWriteEnable = false;
    
    pci.colorBlendState.attachments[0].blendEnable = true;
    pci.colorBlendState.attachments[0].srcColorBlendFactor = BlendFactor::SrcAlpha;
    pci.colorBlendState.attachments[0].dstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
    pci.colorBlendState.attachments[0].colorBlendOp = BlendOp::Add;
    pci.colorBlendState.attachments[0].srcAlphaBlendFactor = BlendFactor::OneMinusSrcAlpha;
    pci.colorBlendState.attachments[0].dstAlphaBlendFactor = BlendFactor::Zero;
    pci.colorBlendState.attachments[0].alphaBlendOp = BlendOp::Add;
    
    pci.rasterizationState.cullMode = CullModeFlagBits::None;
    pci.rasterizationState.frontFace = FrontFace::CounterClockwise;
    
    pci.vertexInputState.vertexBindingDescriptions = 
        {{0, static_cast<std::uint32_t>(sizeof(ImDrawVert)), VertexInputRate::Vertex}};
        
    pci.vertexInputState.vertexAttributeDescriptions =
        {{0, 0, Format::R32_G32_sFloat, offsetof(ImDrawVert, pos)},
         {1, 0, Format::R32_G32_sFloat, offsetof(ImDrawVert, uv)},
         {2, 0, Format::R8_G8_B8_A8_uNorm, offsetof(ImDrawVert, col)}};
    pci.dynamicState.dynamicStates = {DynamicState::Scissor, DynamicState::Viewport};
    
         
    pci.layout = pipelineLayout;
    
    auto passData = renderer->getImGuiRenderPassAndSubPass();
    pci.renderPass = passData.first;
    pci.subpass = passData.second;
    
    pipeline = gfxAPI->createPipeline(pci, "ImGui pipeline");
    
    // Create index and vertex buffers
    
//    IndexType it = (sizeof(ImDrawIdx) == 2) ? IndexType::UInt16 : IndexType::UInt32;
    
    const std::size_t swapImageCount = gfxAPI->getSwapImageCount();
    
    // VBOs
    std::vector<BufferCreateInfo> bci;
    bci.reserve(swapImageCount);
    
    std::vector<std::string> bufferNames;
    bufferNames.reserve(swapImageCount);
    
    std::vector<const char*> bufferNamesCStr;
    bufferNamesCStr.reserve(swapImageCount);
    
    for (std::size_t i = 0; i < swapImageCount; ++i) {
        bci.emplace_back(BufferUsageFlagBits::VertexBuffer, Bytes(MAX_IMGUI_VBO), MemoryUsage::CPUToGPU, true);
        bufferNames.emplace_back(fmt::format("ImGui VBO {}", i));
        bufferNamesCStr.emplace_back(bufferNames[i].data());
    }
    
    VBOs = gfxAPI->createBuffers(bci, &bufferNamesCStr);
    
    // IBOs
    bci.clear();
    bufferNames.clear();
    bufferNamesCStr.clear();
    
    for (std::size_t i = 0; i < swapImageCount; ++i) {
        bci.emplace_back(BufferUsageFlagBits::IndexBuffer,  Bytes(MAX_IMGUI_IBO), MemoryUsage::CPUToGPU, true);
        bufferNames.emplace_back(fmt::format("ImGui IBO {}", i));
        bufferNamesCStr.emplace_back(bufferNames[i].data());
    }
    
    IBOs = gfxAPI->createBuffers(bci, &bufferNamesCStr);
    
    // Write descriptors for font atlas texture
    DescriptorSetAllocateInfo dsai{descriptorPool, {descriptorSetLayout}};
    atlasDescriptorSet = gfxAPI->allocateDescriptorSets(dsai)[0];
    
    WriteDescriptorSet wds{atlasDescriptorSet, 0, 0, 1, DescriptorType::CombinedImageSampler, {{imguiDefaultSampler, fontView, ImageLayout::ShaderReadOnlyOptimal}}, {}, {}};
    gfxAPI->updateDescriptorSets({wds});
    
    // Reserve empty descriptor sets
    const std::uint32_t emptySetCount = MAX_COMBINED_IMAGE_SAMPLER_DESCRIPTOR_COUNT - 1; // We've already allocated one for the main texture atlas
    freeSets.resize(emptySetCount);
    std::vector<DescriptorSetLayoutHnd> setLayouts(emptySetCount, descriptorSetLayout);
    
    DescriptorSetAllocateInfo dsaiMulti{descriptorPool, std::move(setLayouts)};
    freeSets = gfxAPI->allocateDescriptorSets(dsaiMulti);
    
    assetsInitialized = true;
}

void ImGuiImplementation::disposeAssets() {
    assetsInitialized = false;
    
    GraphicsAPI* gfxAPI = engine->getGraphicsAPI();
    
    for (const auto& td : texureData) {
        // The descriptor sets will get freed together with the pool
        gfxAPI->destroyImageView(td.second.imageView);
    }
    
    // Images will get cleared once their handles are freed.
    freeSets.clear();
    texureData.clear();
    
    gfxAPI->destroyDescriptorSetLayout(descriptorSetLayout);
    gfxAPI->destroyDescriptorPool(descriptorPool);

    gfxAPI->destroyPipelineLayout(pipelineLayout);

    gfxAPI->destroyPipeline(pipeline);

    vertexShader.release();
    fragmentShader.release();
    
    gfxAPI->destroyImageView(fontView);
    gfxAPI->destroyImage(fontAtlas);
    ImGui::GetIO().Fonts->TexID = 0;
    gfxAPI->destroySampler(imguiDefaultSampler);
    gfxAPI->destroySampler(customTextureSampler);
    
    gfxAPI->destroyBuffers(VBOs);
    gfxAPI->destroyBuffers(IBOs);
    
    VBOs.clear();
    IBOs.clear();
}

bool ImGuiImplementation::draw(CommandBuffer* cmdBuff) {
    if (cmdBuff == nullptr) {
        throw std::runtime_error("No buffer ;-;");
    }
    
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    
    ImGuiIO& io = ImGui::GetIO();
    int scaledWidth  = static_cast<int>(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int scaledHeight = static_cast<int>(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    
    if (scaledWidth == 0 || scaledHeight == 0) {
        return false;
    }

    drawData->ScaleClipRects(io.DisplayFramebufferScale);
    
    GraphicsAPI* gfxAPI = engine->getGraphicsAPI();
//     // Begin the buffer
//     cmdBuff->begin();
//     
//     
//     RenderPassBeginInfo rpbi;
//     rpbi.framebuffer = gfxAPI->getScreenFramebuffer();
//     rpbi.renderPass = guiRenderPass;
//     rpbi.renderArea.offset = {0, 0};
//     rpbi.renderArea.extent = {gfxAPI->getRenderSurfaceWidth(), gfxAPI->getRenderSurfaceHeight()};
//     cmdBuff->beginRenderPass(rpbi);
    
    cmdBuff->bindPipeline(pipeline);
    
    Viewport vp;
    vp.width = scaledWidth;
    vp.height = scaledHeight;
    
    cmdBuff->setViewport(0, vp);
    
    size_t totalVertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    size_t totalIndexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    
    if (totalVertexSize > MAX_IMGUI_VBO || totalIndexSize > MAX_IMGUI_IBO) {
        throw std::runtime_error("Imgui buffers lack size.");
    }
    
    size_t startVBO = 0;
    size_t startIBO = 0;
    
    const glm::mat4 P = glm::orthoRH(0.0f, static_cast<float>(io.DisplaySize.x), 0.0f, static_cast<float>(io.DisplaySize.y), 0.0f, 1.0f);
    cmdBuff->pushConstants(pipelineLayout, ShaderStageFlagBits::Vertex, 0, sizeof(glm::mat4), glm::value_ptr(P));
    
    const std::size_t swapImageCount = gfxAPI->getSwapImageCount();
    const std::size_t currentSwapchainImage = gfxAPI->getCurrentSwapImage();
    
    const Buffer& currentVBO = VBOs[currentSwapchainImage];
    const Buffer& currentIBO = IBOs[currentSwapchainImage];
    
    {
        IYFT_PROFILE(UploadImGuiData, iyft::ProfilerTag::Graphics);
        
        for (int n = 0; n < drawData->CmdListsCount; ++n) {
            const ImDrawList* cmdList = drawData->CmdLists[n];
            
            DeviceMemoryManager* memoryManager = gfxAPI->getDeviceMemoryManager();
            
            std::vector<BufferCopy> vertexBufferCopy = {{0, startVBO, cmdList->VtxBuffer.size() * sizeof(ImDrawVert)}};
            std::vector<BufferCopy> indexBufferCopy = {{0, startIBO, cmdList->IdxBuffer.size() * sizeof(ImDrawIdx)}};
            
            void* vertexData = cmdList->VtxBuffer.Data;
            void* indexData = cmdList->IdxBuffer.Data;
            
            const Bytes totalVertexSize = memoryManager->computeUploadSize(vertexBufferCopy);
            if (memoryManager->isStagingBufferNeeded(currentVBO) && !memoryManager->canBatchFitData(MemoryBatch::PerFrameData, totalVertexSize)) {
                throw std::runtime_error("Not enough memory in the staging buffer to transfer ImGui vertex data");
            }
            
            memoryManager->updateBuffer(MemoryBatch::PerFrameData, currentVBO, vertexBufferCopy, vertexData);
            
            
            const Bytes totalIndexSize = memoryManager->computeUploadSize(indexBufferCopy);
            if (memoryManager->isStagingBufferNeeded(currentIBO) && !memoryManager->canBatchFitData(MemoryBatch::PerFrameData, totalIndexSize)) {
                throw std::runtime_error("Not enough memory in the staging buffer to transfer ImGui index data");
            }
            
            memoryManager->updateBuffer(MemoryBatch::PerFrameData, currentIBO, indexBufferCopy, indexData);
            
            startVBO += cmdList->VtxBuffer.size() * sizeof(ImDrawVert);
            startIBO += cmdList->IdxBuffer.size() * sizeof(ImDrawIdx);
        }
    }

    cmdBuff->bindDescriptorSets(PipelineBindPoint::Graphics, pipelineLayout, 0, 1, &atlasDescriptorSet, 0, nullptr);
    cmdBuff->bindVertexBuffer(0, currentVBO);
    cmdBuff->bindIndexBuffer(currentIBO, (sizeof(ImDrawIdx) == 2) ? IndexType::UInt16 : IndexType::UInt32);
    //gfxAPI->bindVertexLayout(vertLayout);
    
    std::size_t IBOOffset = 0;
    std::size_t VBOOffset = 0;
    
    const std::uint64_t frameID = engine->getFrameID();
    
    ImTextureID previousID = nullptr;
    Rect2D scissorRect;
    
    for (int n = 0; n < drawData->CmdListsCount; ++n) {
        const ImDrawList* cmd_list = drawData->CmdLists[n];
        
        for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); ++pcmd) {
            if (pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                float oX = pcmd->ClipRect.x;
                float oY = pcmd->ClipRect.y;
                float eX = pcmd->ClipRect.z - pcmd->ClipRect.x;
                float eY = pcmd->ClipRect.w - pcmd->ClipRect.y + 1;
                
                scissorRect.offset.x = oX < 0.0f ? 0.0f : oX;
                scissorRect.offset.y = oY < 0.0f ? 0.0f : oY;
                
                scissorRect.extent.x = eX < 0.0f ? 0.0f : eX;
                scissorRect.extent.y = eY < 0.0f ? 0.0f : eY;
                
                if (pcmd->TextureId != previousID) {
                    if (pcmd->TextureId == nullptr) {
                        // Bind the font atlas
                        cmdBuff->bindDescriptorSets(PipelineBindPoint::Graphics, pipelineLayout, 0, 1, &atlasDescriptorSet, 0, nullptr);
                    } else {
                        StringHash name = StringHash(reinterpret_cast<std::uint64_t>(pcmd->TextureId));
                        
                        auto result = texureData.find(name);
                        if (result == texureData.end()) {
                            ImGuiTextureData textureData;
                            textureData.lastUsedOnFrame = frameID;
                            textureData.texture = assetManager->load<Texture>(name, false);
                            textureData.imageView = gfxAPI->createDefaultImageView(textureData.texture->image, "Temporary editor image view");
                            
                            if (freeSets.empty()) {
                                // TODO resize or add extra
                                throw std::logic_error("Not enough free slots to fit a descriptor set for one more texture.");
                            }
                            
                            DescriptorSetHnd descriptorSet = freeSets.back();
                            freeSets.pop_back();
                            
                            textureData.descriptorSet = descriptorSet;
                            
                            WriteDescriptorSet wds{textureData.descriptorSet, 0, 0, 1, DescriptorType::CombinedImageSampler, {{imguiDefaultSampler, textureData.imageView, ImageLayout::ShaderReadOnlyOptimal}}, {}, {}};
                            gfxAPI->updateDescriptorSets({wds});
                            
                            result = texureData.emplace(std::make_pair(name, std::move(textureData))).first;
                        } else {
                            result->second.lastUsedOnFrame = frameID;
                        }
                        
                        cmdBuff->bindDescriptorSets(PipelineBindPoint::Graphics, pipelineLayout, 0, 1, &(result->second.descriptorSet), 0, nullptr);
                    }
                    
                    previousID = pcmd->TextureId;
                }
                
                //cmdBuff->
                cmdBuff->setScissor(0, scissorRect);
                cmdBuff->drawIndexed(pcmd->ElemCount, 1, IBOOffset, VBOOffset, 0);
            }
            
            IBOOffset += pcmd->ElemCount;
        }
        
        VBOOffset += cmd_list->VtxBuffer.size();
    }
    
    const std::size_t framesBeforeClear = swapImageCount + 2;
    for (auto it = texureData.begin(); it != texureData.end();) {
        if ((frameID - it->second.lastUsedOnFrame) > framesBeforeClear) {
            std::vector<DescriptorSetHnd> handles = {it->second.descriptorSet};
            freeSets.emplace_back(it->second.descriptorSet);
            gfxAPI->destroyImageView(it->second.imageView);
            it = texureData.erase(it);
        } else {
            ++it;
        }
    }
    
//     cmdBuff->endRenderPass();
//     cmdBuff->end();
    
    frameHasAlreadyBegun = false;
    return true;
}

// Static methods for clipboard access
void ImGuiImplementation::setClipboardText(void* userData, const char* text) {
    Engine* engine = static_cast<Engine*>(userData);
    engine->getInputState()->setClipboardValue(text);
}

static std::string clipboardTemp;

const char* ImGuiImplementation::getClipboardText(void* userData) {
    Engine* engine = static_cast<Engine*>(userData);
    clipboardTemp = engine->getInputState()->getClipboardValue();
    
    return clipboardTemp.c_str();
}

}
