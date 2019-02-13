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

#ifndef IYF_GRAPHICS_API_HANDLE_HPP
#define IYF_GRAPHICS_API_HANDLE_HPP

#include "utilities/ForceInline.hpp"

namespace iyf {
/// Base class for type safe GraphicsAPI handles
class GraphicsAPIHandle {
public:
    template<typename T>
    IYF_FORCE_INLINE T toNative() const {
        return static_cast<T>(ptr);
    }
    
    IYF_FORCE_INLINE bool isValid() const {
        return ptr != nullptr;
    }
protected:
    GraphicsAPIHandle(void* ptr) : ptr(ptr) {}
    GraphicsAPIHandle() : ptr(nullptr) {}
private:
    void* ptr;
};

// Vulkan uses opaque pointers on 64 bit systems for both dispatchable and non-dispatchable handles. However, on 32
// bit platforms it uses 64 bit ints as non dispatchable handles. GraphicsAPIHandle always expects a pointer, so
// it won't work on a 32 bit system
static_assert(sizeof(GraphicsAPIHandle) == 8, "The engine requires 64 bit pointers");

#define IYF_MAKE_GRAPHICS_API_HANDLE(NAME) class NAME : public GraphicsAPIHandle { \
public: \
    explicit NAME(void* ptr) : GraphicsAPIHandle(ptr) {} \
    NAME() : GraphicsAPIHandle() {} \
};

IYF_MAKE_GRAPHICS_API_HANDLE(CommandBufferHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(SemaphoreHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(FenceHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(BufferHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(ImageHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(ImageViewHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(BufferViewHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(ShaderHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(PipelineLayoutHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(RenderPassHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(PipelineHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(DescriptorSetHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(SamplerHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(DescriptorPoolHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(DescriptorSetLayoutHnd)
IYF_MAKE_GRAPHICS_API_HANDLE(FramebufferHnd)
}

#endif // IYF_GRAPHICS_API_HANDLE_HPP
