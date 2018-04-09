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

#ifndef DEBUG_RENDERER_HPP
#define DEBUG_RENDERER_HPP

#include "graphics/GraphicsAPI.hpp"
#include "graphics/VertexDataTypes.hpp"

#include "glm/vec3.hpp"
#include "glm/packing.hpp"

#include <chrono>
#include <vector>

namespace iyf {
class CommandBuffer;
class Camera;
class Renderer;

class DebugRenderer {
protected:
    /// How many line vertices is the DebugRenderer allowed to use? Determines the size of the vertex buffer.
    /// May need to be adjusted for bigger worlds
    ///
    /// \todo Dynamic resize? A different (bigger) default limit?
    static const std::size_t MaxDebugLineVertices = 32768 * 16;
    
    /// How many contact point vertices is the DebugRenderer allowed to use? Determines the size of the vertex buffer.
    /// May need to be adjusted for bigger worlds
    ///
    /// \todo Dynamic resize? A different (bigger) default limit?
    static const std::size_t MaxDebugPontVertices = 8192;
public:
    DebugRenderer(Renderer* renderer) : renderer(renderer), isInit(false) { }
    
    virtual void initialize();
    virtual void dispose();
    
    virtual void update(float delta);
    virtual void draw(CommandBuffer* commandBuffer, const Camera* camera) const;
    
    virtual void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) final {
        assert(lineVertexCount < MaxDebugLineVertices);
        
        vertices[lineVertexCount].position.x = start.x;
        vertices[lineVertexCount].position.y = start.y;
        vertices[lineVertexCount].position.z = start.z;
        
        vertices[lineVertexCount].color = glm::packUnorm4x8(glm::vec4(color, 1.0f));
        
        vertices[lineVertexCount + 1].position.x = end.x;
        vertices[lineVertexCount + 1].position.y = end.y;
        vertices[lineVertexCount + 1].position.z = end.z;
        
        vertices[lineVertexCount + 1].color = glm::packUnorm4x8(glm::vec4(color, 1.0f));
        
        lineVertexCount += 2;
    }
    
    virtual ~DebugRenderer() {}

    bool isInitialized() const {
        return isInit;
    }
protected:
    Renderer* renderer;
    bool isInit;
    
    std::vector<ColoredDebugVertex> vertices;
    std::size_t lineVertexCount;
    std::size_t contactPointVertexCount;
    Pipeline physicsDebugPipeline;
    PipelineLayoutHnd pipelineLayout;
    ShaderHnd vs;
    ShaderHnd fs;
    Buffer vbo;
};
}

#endif // DEBUG_RENDERER_HPP
