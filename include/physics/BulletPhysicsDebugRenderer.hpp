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

#ifndef BULLET_PHYSICS_DEBUG_RENDERER_HPP
#define BULLET_PHYSICS_DEBUG_RENDERER_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>

#include "graphics/DebugRenderer.hpp"

namespace iyf {
class Renderer;
class AssetManager;

class BulletPhysicsDebugRenderer : public DebugRenderer, public btIDebugDraw {
public:
    BulletPhysicsDebugRenderer(AssetManager* assetManager, Renderer* renderer) : DebugRenderer(assetManager, renderer), debugModeVal(0) { }
    virtual ~BulletPhysicsDebugRenderer() { }
    
    virtual void reportErrorWarning(const char* warningString) final;
    
    using DebugRenderer::drawLine;
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) final {
        DebugRenderer::drawLine(glm::vec3(from.x(), from.y(), from.z()), glm::vec3(to.x(), to.y(), to.z()), glm::vec3(color.x(), color.y(), color.z()));
    }

    /// \todo Draw contact points
    virtual void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) final {
    // I'd prefer to use c++17's maybe_unused, but my IDE does not support it and it messes up the syntax highlighting at the moment
    //virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) final {
        //assert(contactPointVertexCount < MaxDebugLineVertices + MaxDebugPontVertices)
        // TODO draw contact points
    }

    /// \todo Draw text
    virtual void draw3dText(const btVector3&, const char*) final{
    //virtual void draw3dText(const btVector3& location, const char* textString) final{
        // TODO draw text
    }

    virtual void setDebugMode(int debugMode) final {
        debugModeVal = debugMode;
    }

    virtual int getDebugMode() const final {
        return debugModeVal;
    }

//     virtual void flushLines() final;
protected:
    int debugModeVal;
};
}

#endif // BULLET_PHYSICS_DEBUG_RENDERER_HPP
