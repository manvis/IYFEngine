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

#include "TestState.hpp"
//#include "resources/VertexFormats.hpp"
//#include "resources/ResourceManager.hpp"
#include "core/InputState.hpp"
#include "core/DefaultWorld.hpp"
#include "core/Engine.hpp"
#include "imgui.h"
#include "ImGuiImplementation.hpp"
//#include <GL/glew.h>

namespace iyf::test {
    
TestState::TestState(Engine* engine) : GameState(engine) {
    //
}

TestState::~TestState() {}

void TestState::initialize() {
    EntitySystemManagerCreateInfo ci(engine);
    ci.setEditorMode(true);
    world = std::make_unique<DefaultWorld>("defaultWorld", engine->getConfiguration(), ci);
    world->initialize();
//    mz = engine->getResourceManager()->load<Music>("data/music/Hydrate-Kenny_Beltrey.ogg");
//    snd = engine->getResourceManager()->load<Sound>("data/sfx/boom.ogg");
//    try {
//        File file("bnurbn", File::OpenMode::Read);
//    } catch(FileException e) {
//        LOG_D(e.what());
//    }

    //shared_ptr<Music> muz = engine->getResourceManager()->loadMusic("data/music/Hydrate-Kenny_Beltrey.ogg");
    //mz = muz;

    memRel = true;
    //FileWriter fb("ccc");

//    LOG_D(sizeof(PosTDNUV) << " " << offsetof(PosTDNUV, position) <<
//          " " << offsetof(PosTDNUV, normal) <<
//          " " << offsetof(PosTDNUV, tangentBias) <<
//          " " << offsetof(PosTDNUV, uv));
}

void TestState::pause() {
    //
}

void TestState::resume() {
    //
}

void TestState::dispose() {
//    mz = nullptr;
}

void TestState::step() {
//    shared_ptr<Music> muz = engine->getResourceManager()->loadMusic("data/music/Hydrate-Kenny_Beltrey.ogg");
//    if(!mz->isPlaying()){
//        muz->play(1);
//    }


//    if(!mz->isPlaying()){
//        mz->setVolume(128);
//        mz->play(1);
//    }
    InputState* is = engine->getInputState();
    if(is->isKeyPressed(SDL_SCANCODE_Q)) {
        engine->quit();
    }

//    if(is->isKeyPressed(SDL_SCANCODE_W)) {
//        if(!snd->isPlaying()){
//            snd->play(1);
//        }
//    }

    if(is->isKeyPressed(SDL_SCANCODE_M) && memRel) {
        memRel = false;

        // These use NVIDIA's extension. Problem is, that even if
        // NVX_gpu_memory_info is available on the system, GLEW
        // reports that it isn't. Therefore, we don't check for extension
        // and instead check if glGetIntegerv returns valid values
//        GLint dedicatedMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,
//                      &dedicatedMemoryKB);
//
//        GLint totalMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX,
//                      &totalMemoryKB);
//
//        GLint currentMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,
//                      &currentMemoryKB);
//
//        if(dedicatedMemoryKB != GL_INVALID_ENUM &&
//            totalMemoryKB != GL_INVALID_ENUM &&
//            currentMemoryKB != GL_INVALID_ENUM) {
//
//            LOG_D("Total memory: " << totalMemoryKB <<
//              "\n\t Dedicated memory: " << dedicatedMemoryKB <<
//              "\n\t Currently available memory: " << currentMemoryKB);
//        }

        // These use ATI's (AMD's ?) extension. Didn't have a machine
        // with their card, so doing the same thing as with NVIDIA's
        // extension
//        GLint VBOMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,
//                      &VBOMemoryKB);
//
//        GLint TextureMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,
//                      &TextureMemoryKB);
//
//        GLint RenderBufferMemoryKB = 0;
//        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,
//                      &RenderBufferMemoryKB);
//
//        if(VBOMemoryKB != GL_INVALID_ENUM &&
//            TextureMemoryKB != GL_INVALID_ENUM &&
//            RenderBufferMemoryKB != GL_INVALID_ENUM) {
//
//            LOG_D("VBO memory: " << VBOMemoryKB <<
//              "\n\t Texture memory: " << TextureMemoryKB <<
//              "\n\t RenderBuffer memory: " << RenderBufferMemoryKB);
//        }

    } else if (!is->isKeyPressed(SDL_SCANCODE_M) && !memRel) {
        memRel = true;
    }

    //int val = static_cast<int>(ClearFlags::Color);
    //LOG_D(val);
}

void TestState::frame(float delta) {
    ImGuiImplementation* impl = engine->getImGuiImplementation();
    impl->requestRenderThisFrame();
    
    ImGui::Text("Hurrah");
    
    ImGui::GetIO().MouseDrawCursor = true;
    
    world->setInputProcessingPaused(true);
    world->update(delta);
}

}
