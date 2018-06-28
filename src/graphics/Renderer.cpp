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

#include "graphics/Renderer.hpp"
#include "core/Engine.hpp"
#include "ImGuiImplementation.hpp"

#include <iostream>
#include "graphics/VertexDataLayouts.hpp"
#include "threading/ThreadProfiler.hpp"

namespace iyf {
Renderer::Renderer(Engine* engine, GraphicsAPI* api) : engine(engine), api(api), imGuiSubmissionRequired(false), drawingWorldThisFrame(false), initialized(false) {
    assert(engine != nullptr && api != nullptr);
    
    pickingEnabled = engine->isEditorMode();
}
// void Renderer::drawWorld(const World* world) {
//     if (drawingWorldThisFrame) {
//         throw std::runtime_error("Only one world can be drawn every frame");
//     }
//     
//     drawingWorldThisFrame = true;
//     
//     drawVisibleOpaque(world);
//     drawVisibleTransparent(world);
//     drawDebugAndHelperMeshes(world);
//     drawSky(world);
// }

void Renderer::drawImGui(ImGuiImplementation* impl) {
    IYFT_PROFILE(DrawImGui, iyft::ProfilerTag::Graphics);
    
    if (impl->isRenderRequiredThisFrame()) {
        imGuiSubmissionRequired = impl->draw(getImGuiDesignatedCommandBuffer());
    } else {
        imGuiSubmissionRequired = false;
    }
}

}
