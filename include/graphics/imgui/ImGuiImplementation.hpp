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

#ifndef IMGUIRENDERER_HPP
#define IMGUIRENDERER_HPP

#include "core/InputListener.hpp"
#include "graphics/GraphicsAPI.hpp"
#include "utilities/NonCopyable.hpp"
#include "assets/AssetHandle.hpp"

class ImGuiContext;
namespace iyf {
class Engine;
class Renderer;
class Shader;

class ImGuiImplementation : public InputListener, private NonCopyable {
public:
    bool requestRenderThisFrame();
    
    bool isRenderRequiredThisFrame() const {
        return frameHasAlreadyBegun;
    }
    
    virtual void onMouseMoved(int cursorXPos, int cursorYPos, int deltaX, int deltaY, bool windowHasFocus) final;
    virtual void onMouseWheelMoved(int deltaX, int deltaY) final;
    virtual void onMouseButtonDown(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) final;
    virtual void onMouseButtonUp(int cursorXPos, int cursorYPos, int clickCount, MouseButton button) final;
    virtual void onKeyPressed(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) final;
    virtual void onKeyReleased(SDL_Keycode keycode, SDL_Scancode scancode, KeyModifierFlags keyModifier) final;
    virtual void onTextInput(const char* text) final;
    
protected:
    friend Engine;
    friend Renderer;
    
    ImGuiImplementation(Engine* engine) : engine(engine), frameHasAlreadyBegun(false), assetsInitialized(false) {}
    void initialize();
    void dispose();
    
    void initializeAssets();
    void disposeAssets();
    
    /// \brief Performs the draw
    ///
    /// \return if something was drawn. If not, skip command buffer submission to queue
    bool draw(CommandBuffer* cmdBuff);
    
    static const char* getClipboardText(void* userData);
    static void setClipboardText(void* userData, const char* text);

    Engine* engine;
    ImGuiContext* context;
    bool frameHasAlreadyBegun, assetsInitialized;
    
    Image fontAtlas;
    SamplerHnd fontSampler;
    ImageViewHnd fontView;
    DescriptorPoolHnd descriptorPool;
    DescriptorSetLayoutHnd descriptorSetLayout;
    DescriptorSetHnd atlasDescriptorSet;
    AssetHandle<Shader> vertexShader;
    AssetHandle<Shader> fragmentShader;
    PipelineLayoutHnd pipelineLayout;
    Pipeline pipeline;
//    std::vector<VertexBufferSlice> VBOs;
//    IndexBufferSlice IBO;
//    
    Buffer IBO;
    std::vector<Buffer> VBOs;
    
    std::string temp;
//    Renderer* renderer;
};
}

#endif /* IMGUIRENDERER_HPP */

