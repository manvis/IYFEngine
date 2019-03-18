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

#ifndef WORLD_HPP
#define WORLD_HPP

// TODO at least some of these should no longer be required
#include "graphics/Lights.hpp"

#include "core/EntitySystemManager.hpp"
#include "core/interfaces/Configurable.hpp"

#include "core/interfaces/Serializable.hpp"

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <cstring>

namespace iyf {
class Engine;
class Skybox;
class AssetManager;
class Renderer;

/// A manager for all gameplay systems and entities
class World : public Configurable, public Serializable, public EntitySystemManager {
public:
    /// Creates a new World object
    /// 
    /// \param[in] name Name of the World. Must not be an empty string. UTF-8. Length must be less than 64.
    /// \param[in] configuration Pointer to the Configuration object. Required as a part of Configurable interface which allows the World to react to Configuration changes
    World(std::string name, Configuration* configuration, EntitySystemManagerCreateInfo createInfo);
    
    /// \return a const reference to the current name of the World
    inline const std::string& getName() const {
        return name;
    }
    
    /// Change the name of the World
    /// \param[in] name Name of the World. Must not be an empty string. UTF-8. Length must be less than 64.
    void setName(std::string name);
    
    void handleConfigChange(const ConfigurationValueMap& changedValues) override;
    
    /// Initializes the World
    ///
    /// \warning All overriding classes MUST call this method during their own initialization
    virtual void initialize() override;
    
    /// Destroys all the data in the World
    ///
    /// \warning All overriding classes MUST call this method during their own dispose calls
    virtual void dispose() override;
    
    /// \warning All overriding classes MUST call this method during their own update calls
    virtual void update(float delta) override;
    
    inline bool isInitialized() {
        return isWorldInitialized;
    }
    
    /// Returns a list of all lights currently existing in scene
    /// \todo A light is also an entity and it should be possible to attach it to other entities. Should this only contain static lights?
    inline const std::vector<PointLight>& getLights() const {
        return lights;
    }
    
    inline void setInputProcessingPaused(bool isPaused) {
        isInputProcPaused = isPaused;
    }
    
    inline bool isInputProcessingPaused() const {
        return isInputProcPaused;
    }
    
    /// Picks an object in the World by using the physics engine.
    ///
    /// \warning Do not call this method if the World has no physics or render systems registered
    void rayPick(std::uint32_t x, std::uint32_t y);
    
    /// Check if the physics engine is drawing debug data or not.
    inline bool isPhysicsDebugDrawn() const {
        return physicsDebugDrawn;
    }
    
    /// Tells the physics engine to start or stop drawing debug data.
    ///
    /// \warning Do not call this method if this World has no physics or render systems registered
    void setPhysicsDebugDrawn(bool newValue);
    
    void addStaticMesh(StringHash nameHash);
    void addDynamicMesh(StringHash nameHash);
    void addCamera(bool setAsDefault);
    void addLight();
    void addEmpty();
    
    virtual ~World();
    
    // Serializable interface
    virtual void serialize(Serializer& fw) const override;
    virtual void deserialize(Serializer& fr) override;
protected:
    
    /// Called during initialization to initialize System objects and register them to the EntitySystemManager
    virtual void initializeSystems() = 0;
    
    std::string name;
    
    bool isWorldInitialized;
    std::int32_t screenWidth;
    std::int32_t screenHeight;
    
    bool isInputProcPaused;
    
    AssetManager* assetManager;
    std::vector<PointLight> lights;
    
    //EntitySystemManager entitySystemManager;

//     /// \todo This should map EntityName to some kind of a meta-Entity in order to support attachments, Moreover, that bool
//     /// screws with packing.
//     std::map<std::string, std::pair<EntityKey, bool>> entities;
    
    /// Maps an ID of a material definition to a position in the materials vector
    std::unordered_map<StringHash, std::size_t> definitionToMaterial;
    
    bool physicsDebugDrawn;
};

}

#endif /* WORLD_HPP */

