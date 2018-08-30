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

#ifndef MESH_COMPONENT_HPP
#define MESH_COMPONENT_HPP

#include "core/Constants.hpp"
#include "core/Component.hpp"
#include "assets/AssetManager.hpp"
#include "assets/assetTypes/Mesh.hpp"
#include "graphics/Material.hpp"
#include "graphics/RenderDataKey.hpp"
#include "graphics/culling/BoundingVolumes.hpp"

#include <vector>
#include <glm/mat4x4.hpp>

namespace iyf {
/// \todo Use AABBs. Remove sphere methods
class MeshComponent : public Component {
public:
    static constexpr ComponentType Type = ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Mesh);
    MeshComponent() : Component(ComponentType(ComponentBaseType::Graphics, GraphicsComponent::Mesh)), renderMode(MaterialRenderMode::Opaque) { }
    
    virtual ~MeshComponent() { }
    
    inline const BoundingVolume& getCurrentBoundingVolume() const {
        return currentBounds;
    }
    
#if IYF_BOUNDING_VOLUME == IYF_SPHERE_BOUNDS
    inline void updateCurrentBounds(const glm::mat4& modelMatrix, const glm::vec3& scale) {
        currentBounds = preTransformBounds.transform(modelMatrix, scale);
    }
#elif IYF_BOUNDING_VOLUME == IYF_AABB_BOUNDS
    inline void updateCurrentBounds(const glm::mat4& modelMatrix, const glm::vec3&) {
        currentBounds = preTransformBounds.transform(modelMatrix);
    }
#endif // IYF_BOUNDING_VOLUME
    
    inline void setCurrentBoundingVolume(const BoundingVolume& volume) {
        currentBounds = volume;
    }

    inline const BoundingVolume& getPreTransformBoundingVolume() const {
        return preTransformBounds;
    }
    
    inline void setPreTransformBoundingVolume(const BoundingVolume& volume) {
        preTransformBounds = volume;
    }
    
    inline void setRenderMode(MaterialRenderMode mode) {
        renderMode = mode;
    }
    
    inline MaterialRenderMode getRenderMode() const {
        return renderMode;
    }
    
    inline void setMesh(const AssetHandle<Mesh>& meshData) {
        mesh = meshData;
    }
    
    inline const AssetHandle<Mesh>& getMesh() const {
        return mesh;
    }
    
    inline RenderDataKey getRenderDataKey() const {
        return key;
    }
    
    /// Remakes the RenderDataKey using the current AssetHandles
    inline void updateRenderDataKey() {
        //TODO implement render key updates;
    }
    
    virtual void attach(System*, std::uint32_t) final { }
    
    /// We don't need the actual system pointer for this asset type and simply wipe the
    /// AssetHandle objects to reduce their asset counts
    virtual void detach(System*, std::uint32_t) final {
        mesh = AssetHandle<Mesh>();
    }
    
protected:
    AssetHandle<Mesh> mesh;
    BoundingVolume currentBounds;
    RenderDataKey key;
    BoundingVolume preTransformBounds;
    MaterialRenderMode renderMode;
};
}

#endif // MESH_COMPONENT_HPP
