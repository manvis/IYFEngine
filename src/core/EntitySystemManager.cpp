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

#include <stdexcept>

#include "graphics/GraphicsSystem.hpp"
#include "physics/PhysicsSystem.hpp"
#include "core/EntitySystemManager.hpp"
#include "threading/ThreadProfiler.hpp"

namespace iyf {
bool EntitySystemManagerCreateInfo::validate(bool throwException) {
    bool initialCapacityInvalid = (initialCapacity == 0);
    if (initialCapacityInvalid && throwException) {
        throw std::invalid_argument("Initial EntitySystemManager capacity was set to 0.");
    } else if (initialCapacityInvalid) {
        return false;
    }
    
    bool entityCapacityGrowthInalid = (capacityGrowthInterval == 0);
    if (entityCapacityGrowthInalid && throwException) {
        throw std::invalid_argument("CapacityGrowthInterval was set to 0.");
    } else if (entityCapacityGrowthInalid) {
        return false;
    }

    bool maxMoreThanInitial = (maxCapacity < initialCapacity);
    if (maxMoreThanInitial && throwException) {
        throw std::invalid_argument("Maximum EntitySystemManager capacity can't be less than initial capacity.");
    } else if (maxMoreThanInitial) {
        return false;
    }

    bool conditionNotSatisfied = ((maxCapacity - initialCapacity) % capacityGrowthInterval != 0);
    if (conditionNotSatisfied && throwException) {
        throw std::invalid_argument("capacityGrowthInterval does not satisfy (maxCapacity - initialCapacity) % capacityGrowthInterval == 0");
    } else if (conditionNotSatisfied) {
        return false;
    }
    
    return true;
}

EntitySystemManager::EntitySystemManager(EntitySystemManagerCreateInfo createInfo) : createInfo(createInfo), initialized(false) {
    this->createInfo.validate(true);
}

void EntitySystemManager::initialize() {
    std::uint32_t initialCapacity = createInfo.getInitialCapacity();
    
    entityStates.resize(initialCapacity);
    transformations.resize(initialCapacity);
    entities.resize(initialCapacity);
    
    // TODO smarter sizing for these vectors
    awaitingInitialization.reserve(initialCapacity * 2);
    awaitingDestruction.reserve(initialCapacity * 2);
    
    // TODO is this sufficient for all cases?
    const std::size_t minFreeSlots = 10;
    const std::size_t estimatedFreeSlotCount = (initialCapacity / 4);
    freeSlots.reserve((minFreeSlots < estimatedFreeSlotCount) ? estimatedFreeSlotCount : minFreeSlots);
    
    nextID = 0;
    currentCapacity = initialCapacity;
    
    for (std::size_t i = 0; i < systems.size(); ++i) {
        //ComponentBaseType ct = systems[i]->getComponentType();
        systems[i]->initialize();
        systems[i]->resize(initialCapacity);
    }
    
    initialized = true;
}

void EntitySystemManager::dispose() {
    for (auto& s : systems) {
        s->dispose();
    }
    
    for (auto& system : systems) {
        system = nullptr;
    }
    
    managedComponents.reset();
    
    // Trigger the destructors that clear all the elements
    entityStates.clear();
    transformations.clear();
    entities.clear();
    
    freeSlots.clear();
    
    initialized = false;
}

void EntitySystemManager::resize(std::uint32_t newSize) {
    if (newSize < currentCapacity) {
        return;
    }
    
    currentCapacity += createInfo.getCapacityGrowthInterval();
    
    if (currentCapacity > createInfo.getMaxCapacity()) {
        throw std::runtime_error("EntitySystemManager capacity was exceeded.");
    }
    
    entityStates.resize(currentCapacity);
    transformations.resize(currentCapacity);
    entities.resize(currentCapacity);
    freeSlots.reserve(currentCapacity / 4);
    
    for (auto& s : systems) {
        s->resize(newSize);
    }
}

void EntitySystemManager::registerSystem(std::unique_ptr<System> system) {
    if (initialized) {
        throw std::logic_error("Systems can only be registered before initialization is complete.");
    }
    
    ComponentBaseType baseType = system->getManagedComponentType();
    std::size_t baseTypeID = static_cast<std::size_t>(baseType);
    
    if (managedComponents[baseTypeID]) {
        throw std::logic_error("System conflict. This component type is already managed by another system.");
    } else {
       managedComponents.set(baseTypeID);
    }
    
    // TODO systems with multiple component types. Maybe insert multiple times? And check that they use contiguous
    // ComponentType identifiers
    systems[baseTypeID] = std::move(system);
}

void EntitySystemManager::manageEntityLifecycles([[maybe_unused]]float delta) {
    // Initialize new Entities. For now, this simply flips the bit that prevented the Entity from going
    // through a partial update.
    for (const EntityKey& key : awaitingInitialization) {
        entityStates[key.getID()].setInitialized(true);
    }
    awaitingInitialization.clear();
    
    // Destroy old Entities and their Components. I may need to integrate this into the the grabage collection system.
    // Even if I do integrate it into grabage collection, the Entities MUST BE DEACTIVATED HERE to prevent them from
    // lingering in the World after death. No one likes zombies :D
    // TODO hierarchies! This code assumes that there are no parent-child relationships
    for (const EntityKey& key : awaitingDestruction) {
        // Check if the Entity is still valid. Someone may have been added it to awaitingDestruction multiple times.
        if (!isEntityValid(key)) {
            return;
        }
        
        std::uint32_t id = key.getID();
        
        Entity& entity = entities[id];
        
        if (createInfo.isEditorMode()) {
            auto result = entityHierachy.find(entity.getName());
            assert(result != entityHierachy.end());
            
            // TODO remove once hierarchies are implemented
            assert(result->second.parent == nullptr && result->second.children.size() == 0);
            
            entityHierachy.erase(result);
        }
        
        EntityState& state = entityStates[id];
        for (std::size_t i = 0; i < static_cast<std::size_t>(ComponentBaseType::COUNT); ++i) {
            // TODO systems with multiple component types
            if (state.hasComponentsOfType(static_cast<ComponentBaseType>(i))) {
                systems[i]->destroyAllComponents(id);
            }
        }
        
        std::size_t count = nameToEntity.erase(entity.getName());
        assert(count > 0);
        
        // This will advance the version
        entity.clear();
        
        state.reset();
        
        transformations[id].clear();
        
        freeSlots.push_back(id);
    }
    awaitingDestruction.clear();
}

void EntitySystemManager::update(float delta) {
    IYFT_PROFILE(EntitySystemManagerUpdate, iyft::ProfilerTag::World);
    
    // TODO figure out where to call non-Engine components
    
    // Get rid of objects destroyed in the previous frame and initialize new ones
    manageEntityLifecycles(delta);
    
    for (auto& s : systems) {
        // TODO limited collection
        s->collectGarbage(GarbageCollectionRunPolicy::FullCollection);
    }
    
    // Physics system is the first to get updated, since it may change TransformationComponents that other
    // components may depend on
    PhysicsSystem* physicsSystem = static_cast<PhysicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Physics));
    physicsSystem->update(delta, entityStates);
    
    // TODO where do script ticks go?
    
    GraphicsSystem* graphicsSystem = static_cast<GraphicsSystem*>(getSystemManagingComponentType(ComponentBaseType::Graphics));
    
//     // Update dirty transform matrices
//     // We use a separate counter here because accessing a ChunkedVector element via ID is much slower than via the iterator
//     std::uint32_t i = 0;
//     for (auto& t : transformations) {
//         // TODO replace entityHasAnyComponents with an iterator advance. The current solution is likely bad for performance
//         if (t.update() && entityHasAnyComponents(i, ComponentBaseType::Graphics)) {
//             // TODO this is no longer valid!!!
//             //RenderDataComponent& renderData = graphicsSystem->getComponent(i);
//             //renderData.updateCurrentBounds(t.getModelMatrix(), t.getScale());
//         }
//         
//         i++;
//     }
    
    auto transform = transformations.begin();
    auto state = entityStates.begin();
    
    // Update dirty transform matrices and mesh bounds
    // We use iterators here because accessing a ChunkedVector element via ID is much slower than via the iterator
    for (std::uint32_t i = 0; i < nextID; ++i) {
        if ((*transform).update() && (*state).hasComponentsOfType(ComponentBaseType::Graphics)) {
            if (graphicsSystem->hasComponent(i, static_cast<std::uint32_t>(GraphicsComponent::Mesh))) {
                MeshComponent& mc = graphicsSystem->getComponent<MeshComponent>(i);
                mc.updateCurrentBounds((*transform).getModelMatrix(), (*transform).getScale());
//                 LOG_D(mc.getCurrentBoundingSphere().radius << " " << mc.getPreTransformBoundingSphere().radius);
                //LOG_D("+++++++++++++++++++++++++++++++++++UPDATED");
            }
        }
        
        ++transform;
        ++state;
    }
    
    graphicsSystem->update(delta, entityStates);
}

EntityKey EntitySystemManager::create(const std::string& name, bool active) {
    std::uint32_t id;
    std::uint32_t version;
    
    if (freeSlots.empty()) {
        id = nextID;
        nextID++;
        
        resize(id);

        EntityState* state = &entityStates[id];
        TransformationComponent* transformation =  &transformations[id];
        transformation->entity = &entities[id];
        entities[id].initialize(this, name, EntityKey(id, 1), transformation, state);
        
        version = 1;
    } else {
        id = freeSlots.back();
        entities[id].reinitialize(name);
        
        version = entities[id].getKey().getVersion();
        
        freeSlots.pop_back();
    }
    
    auto result = nameToEntity.insert({name, &entities[id]});
    if (!result.second) {
        throw std::runtime_error("The name of the Entity was not unique");
    }
    
    /// Add it to the sorted hierarchy used for display if we're running in editor mode
    if (createInfo.isEditorMode()) {
        EntityHierarchyNode node;
        node.entity = &entities[id];
        auto result = entityHierachy.insert({name, node});
        
        // Just in case we forget to remove an Entity. The uniqueness of the name has already been checked
        // by the unordered_map
        assert(result.second);
    }
    
    EntityKey key(id, version);
    
    EntityState* state = &entityStates[id];
    state->setActive(active);
    // state->setInitialized(false); // This should have been set automatically
    awaitingInitialization.push_back(key);
    
    return key;
}

// void EntitySystemManager::create(std::uint32_t n, std::vector<EntityKey>& output, bool atEnd) {
//     if (freeSlots.empty() || atEnd) {
//         std::uint32_t idStart = nextID;
//         nextID += n;
//         
//         resize(nextID - 1);
//         
//         for (std::uint32_t i = idStart; i < nextID; ++i) {
//             currentVersions[i] = 1;
//             output.push_back(EntityKey(i, 1));
//         }
//     } else {
//         //TODO Would a list be better here?
//         std::sort(freeSlots.begin(), freeSlots.end());
//         
//         std::uint32_t rangeStart = 0;
//         std::uint32_t rangeEnd = 0;
//         bool rangeFound = false;
//         
//         for (std::size_t i = 0; i < freeSlots.size(); ++i) {
//             if ((freeSlots[rangeEnd] + 1) == freeSlots[i]) {
//                 rangeEnd = i;
//             } else {
//                 rangeStart = i;
//                 rangeEnd = i;
//             }
//             
//             if ((rangeEnd - rangeStart) == (n - 1)) {
//                 rangeFound = true;
//                 
//                 break;
//             }
//         }
//         
//         if (rangeFound) {
//             // Suitable empty range was found. Fill it.
//             for (std::size_t i = freeSlots[rangeStart]; i <= freeSlots[rangeEnd]; ++i) {
//                 output.push_back(EntityKey(i, currentVersions[i]));
//             }
//             
//             freeSlots.erase(freeSlots.begin() + rangeStart, freeSlots.begin() + rangeEnd + 1);
//         } else if (freeSlots[rangeEnd] == (nextID - 1)) {
//             // It's possible to reuse some elements near the end
//             
//             for (std::size_t i = freeSlots[rangeStart]; i <= freeSlots[rangeEnd]; ++i) {
//                 output.push_back(EntityKey(i, currentVersions[i]));
//                 n--;
//             }
//             
//             freeSlots.erase(freeSlots.begin() + rangeStart, freeSlots.begin() + rangeEnd + 1);
//             
//             create(n, output, true);
//         } else {
//             // No suitable range found. Append all at the end;
//             create(n, output, true);
//         }
//     }
// }

void EntitySystemManager::free(const EntityKey& key) {
    if (!isEntityValid(key)) {
        return;
    }
    
    entityStates[key.getID()].setAwaitingDestruction(true);
    
    awaitingDestruction.push_back(key);
}

bool EntitySystemManager::validateComponentAttachment() const {
    throw std::runtime_error("Boom");
}

bool EntitySystemManager::attachComponent(const EntityKey& key, const ComponentType& type) {
    System* system = getSystemManagingComponentType(type.getBaseType());
    
    if (system == nullptr) {
        return false;
    }
    
    ComponentSubTypeFlags systemComponents = system->getAvailableComponents(key.getID());
    
    if (!isEntityValid(key) || systemComponents[type.getSubType()]) {
        return false;
    }
    
    system->createAndAttachComponent(key, type);
    
    entityStates[key.getID()].setHasComponentsAvailable(type.getBaseType(), true);
    return true;
}

bool EntitySystemManager::attachComponent(const EntityKey& key, const Component& component) {
    ComponentBaseType baseType = component.getType().getBaseType();
    
    System* system = getSystemManagingComponentType(baseType);
    
    if (system == nullptr) {
        return false;
    }
    
    ComponentSubTypeFlags systemComponents = system->getAvailableComponents(key.getID());
    
    if (!isEntityValid(key) || systemComponents[component.getType().getSubType()]) {
        return false;
    }
    
    system->setComponent(key.getID(), component);
    
    entityStates[key.getID()].setHasComponentsAvailable(baseType, true);
    return true;
}

bool EntitySystemManager::attachComponent(const EntityKey& key, Component&& component) {
    ComponentBaseType baseType = component.getType().getBaseType();
    
    System* system = getSystemManagingComponentType(baseType);
    
    if (system == nullptr) {
        return false;
    }
    
    ComponentSubTypeFlags systemComponents = system->getAvailableComponents(key.getID());
    
    if (!isEntityValid(key) || systemComponents[component.getType().getSubType()]) {
        return false;
    }
    
    system->setComponent(key.getID(), std::move(component));
    
    entityStates[key.getID()].setHasComponentsAvailable(baseType, true);
    return true;
}

bool EntitySystemManager::removeComponent(const EntityKey& key, const ComponentType& type) {
    System* system = getSystemManagingComponentType(type.getBaseType());
    
    if (system == nullptr) {
        return false;
    }
    
    ComponentSubTypeFlags systemComponents = system->getAvailableComponents(key.getID());
    
    if (!isEntityValid(key) || !systemComponents[type.getSubType()]) {
        return false;
    }
    
    system->destroyComponentUnchecked(key.getID(), type.getSubType());
    
    if (!system->hasAnyComponents(key.getID())) {
        entityStates[key.getID()].setHasComponentsAvailable(type.getBaseType(), false);
    }
    
    return true;
}

void Entity::setName(std::string newName) {
    if (!manager->isEditorMode()) {
        throw std::logic_error("Entities can only be renamed when running in Editor Mode");
    }
    name = std::move(newName);
}

std::string EntitySystemManager::filePathToEntityName(const fs::path& name) const {
    return makeUniqueName(name.stem().generic_string());
}

std::string EntitySystemManager::makeUniqueName(const std::string& name) const {
    auto result = nameToEntity.find(name);
    if (result == nameToEntity.end()) {
        return name;
    }
    
    // If an entity with this name already exists, append a number to the end of the string
    // This can become really slow if a lot of names are already in use.
    std::size_t value = 1;
    while (true) {
        const std::string finalNameWithNumber = name + "_" + std::to_string(value);
        
        auto result = nameToEntity.find(finalNameWithNumber);
        if (result == nameToEntity.end()) {
            return finalNameWithNumber;
        }
        
        value++;
    }
    
    return name;
}

// void System::initialize() {
//     availableComponents.
// }

}
