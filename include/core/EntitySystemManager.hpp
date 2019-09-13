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

#ifndef ENTITY_SYSTEM_MANAGER_HPP
#define ENTITY_SYSTEM_MANAGER_HPP

#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "core/interfaces/ComponentContainer.hpp"
#include "core/TransformationComponent.hpp"
#include "core/ComponentType.hpp"
#include "core/Component.hpp"
#include "core/filesystem/cppFilesystem.hpp"
#include "core/interfaces/GarbageCollecting.hpp"
#include "utilities/ChunkedVector.hpp"
#include "utilities/IntegerPacking.hpp"
#include "utilities/NonCopyable.hpp"
#include "localization/LocalizationHandle.hpp"

namespace iyf {
/// \todo Is 8192 as good chunk size?
constexpr std::size_t SystemChunkSize = 8192;

/// \warning The container used here must ensure that all pointers remain STABLE when it grows or the contents of the elements
/// are replaced (they will never be removed).
using TransformationVector = ChunkedVector<TransformationComponent, SystemChunkSize>;

class Engine;
class EntitySystemManager;
class World;

enum class EntityMode {
    /// If this mode is set, the Entity MUST forever remain in its initial place.
    /// Setting this mode on objects that are not supposed to move is really important
    /// for optimization.
    /// \warning Enabling this and moving the object via the scripting system will invoke UNDEFINED BEHAVIOUR.
    /// You may end up causing graphics bugs, navigation bugs, "invisible walls" or even crash the engine.
    /// Even if everything "works" on your machine it may not work for other people, so don't do it.
    /// \todo Should this forbid certain components, e.g. skeletal animation?
    Static,
    /// If this mode is set, the Entity can be safely moved and animated. Setting this mode on
    /// objects that don't move is a waste of resources
    Dynamic
};

/// The Entities can easily self destruct or be destroyed by other Entities. Moreover, the EntitySystemManager recycles destoryed Entities to
/// conserve memory and to keep the data close for CPU cache coherence. This means that using simple pointers or references is often a recipe
/// for disaster because it is impossible to know if the Entity is still the same Entity that was retrieved several frames ago. Enter EntityKey.
/// This class stores the ID of the Entity (used to determine offsets into various arrays) and its version. The version is incremented every 
/// time an Entity is destoryed. By comparing a previously stored EntityKey with an EntityKey stored in an Entity, you can determine if it's
/// still valid or not. To make these checks easier (or at least less verbose), you can use EntityHandle objects that can be created by calling
/// Entity::makeHandle().
class EntityKey {
public:
    static const std::uint32_t InvalidID = std::numeric_limits<std::uint32_t>::max();
    static const std::uint32_t InvalidVersion = 0;
    static const std::uint64_t InvalidHandle = util::Int32sToInt64(InvalidID, InvalidVersion);
    
    /// Empty constructor that creates an invalid Entity that can be used as a placeholder
    ///
    /// To create a valid Entity, use one of EntitySystemManager.create...() methods
    EntityKey() : handle(InvalidHandle) {}
    
    inline std::uint32_t getID() const {
        return util::Int64ToInt32a(handle);
    }
    
    inline std::uint32_t getVersion() const {
        return util::Int64ToInt32b(handle);
    }
    
    friend bool operator==(const EntityKey& a, const EntityKey& b) {
        return a.handle == b.handle;
    }
private:
    friend class EntitySystemManager;
    friend class Entity;
    
    /// Creates a new valid Entity. This constructor is only accessible to EntitySystemManager and Entity
    EntityKey(std::uint32_t id, std::uint32_t version) : handle(util::Int32sToInt64(id, version)) {}
    std::uint64_t handle;
};

class EntityState {
private:
    static_assert(static_cast<std::size_t>(ComponentBaseType::COUNT) <= 48, "You cannot have more than 48 ComponentBaseTypes");
    static const int Active = 63;
    static const int Initialized = 62;
    static const int AwaitingDestruction = 61;
    /// This bit is only used if the EntitySystemManager containing the Entity was created in editor mode.
    static const int Selected = 60;
public:
    inline bool isSelected() const {
        return data[Selected];
    }
    
    /// Sets the selection state. Used to determine if a selection outline needs to be drawn.
    inline void setSelected(bool selected) {
        data[Selected] = selected;
    }
    
    inline bool isActive() const {
        return data[Active];
    }
    
    /// Enables or disables the Entity.
    inline void setActive(bool active) {
        data[Active] = active;
    }
    
    inline bool isInitialized() const {
        return data[Initialized];
    }
    
    inline bool isAwaitingDestruction() const {
        return data[AwaitingDestruction];
    }
    
    /// Checks if the Entity has any components of the specified ComponentBaseType. Typically used to quickly determine if
    /// a System needs to process a specific Entity or not.
    inline bool hasComponentsOfType(ComponentBaseType component) const {
        return data[static_cast<std::size_t>(component)];
    }
    
    /// Clears the EntityState
    inline void reset() {
        data.reset();
    }
private:
    friend class EntitySystemManager;
    
    /// Enables or disables the bit indicating the presence of components of the specified ComponentBaseType.
    inline void setHasComponentsAvailable(ComponentBaseType component, bool available) {
        data[static_cast<std::size_t>(component)] = available;
    }
    
    inline void setInitialized(bool initialized) {
        data[Initialized] = initialized;
    }
    
    inline void setAwaitingDestruction(bool awaitingDestruction) {
        data[AwaitingDestruction] = awaitingDestruction;
    }
    
    std::bitset<64> data;
};

/// Used to make sure that an Entity is valid. The typical usage scenario is to first call isValid() and, if it returns true,
/// obtain the reference to the Entity by calling get(). Even if you call EntitySystemManager::free, the Entity will persist
/// until the end of the frame. Therefore, calling isValid() once per frame is sufficent.
class EntityHandle {
public:
    EntityHandle(Entity& entity);
    
    /// Checks if the Entity is still valid by comparing an EntityKey retrieved from it with an EntityKey stored in this object.
    /// \remark Entities do not get destroyed or replaced mid-frame. Calling this method once per frame is sufficent.
    bool isValid() const;
    
    /// Obtains a reference to the Entity. If isValid() is false, it may return a different Entity, stale data or an empty region
    /// that may cause undefined behaviour when accessed. Remember: Entity::getKey() is THE ONLY member function that can be
    /// trusted when an Entity is empty.
    inline Entity& get() const {
        return entity;
    }
private:
    Entity& entity;
    EntityKey key;
};

/// \warning The container used here must ensure that all pointers remain STABLE when it grows or the contents of the elements
/// are replaced (they will never be removed).
using EntityStateVector = ChunkedVector<EntityState, SystemChunkSize>;

/// Represents an object in the world. Each object is constructed from multiple Component objects that are managed by specific
/// System objects. All Entities have a TransformationComponent, all other components are optional and depend on desired functionality.
///
/// \todo Implement Entity parent-child relationships. Some work has already been done (e.g., EntityHierarchy that's used by the Editor
/// was made with hierarchies in mind), however, a lot of work still remains.
class Entity {
public:
    /// \warning This is only public for the sake of the containers that store the entities. This constructor creates
    /// an invalid and unusable Entity. To create and initialize valid entities, use EntitySystemManager::create() and
    /// other similar methods from the EntitySystemManager class.
    inline Entity() : manager(nullptr), transformation(nullptr), state(nullptr) {
        
    }
    
    template <typename T>
    inline T& getComponent();
    
    template <typename T>
    inline const T& getComponent() const;
    
    inline const EntityState& getState() const {
        return *state;
    }
    
    inline bool isSelected() const {
        return state->isSelected();
    }
    
    inline void setSelected(bool selected) {
        state->setSelected(selected);
    }
    
    inline const TransformationComponent& getTransformation() const {
        return *transformation;
    }
    
    inline TransformationComponent& getTransformation() {
        return *transformation;
    }
    
    inline EntitySystemManager* getManager() const {
        return manager;
    }
    
    inline EntityKey getKey() const {
        return key;
    }
    
    inline const std::string& getName() const {
        return name;
    }
    
    inline EntityHandle makeHandle() {
        return EntityHandle(*this);
    }
    
    const std::vector<Component*>& getAllComponents() const;
    
    /// See EntitySystemManager::attachComponent(const Component&)
    /// \todo I think it should be possible to avoid additional checks here and use the key.getID() directly
    bool attachComponent(const Component& component);
    
    /// See EntitySystemManager::attachComponent(Component&&)
    /// \todo I think it should be possible to avoid additional checks here and use the key.getID() directly
    bool attachComponent(Component&& component);
    
    bool removeComponent(const ComponentType& type);
    
    /// Checks if the Entity has any components of the specifed ComponentBaseType
    bool hasComponents(ComponentBaseType baseType) const;
    
    /// Checks if the Entity has a Component of the specifed ComponentType
    bool hasComponent(const ComponentType& type) const;
    
    /// Rename the Entity.
    /// \throws std::logic_error If the EntitySystemManager is not running in Editor Mode.
    void setName(std::string newName);
private:
    friend class EntitySystemManager;
    
    /// Called by the EntitySystemManager when a completely new Entity is created
    inline void initialize(EntitySystemManager* manager, std::string name, EntityKey key, TransformationComponent* transformation, EntityState* state) {
        assert(manager != nullptr);
        assert(transformation != nullptr);
        assert(state != nullptr);
        
        this->manager = manager;
        this->key = key;
        this->transformation = transformation;
        this->name = std::move(name);
        this->state = state;
    }
    
    /// Called by the EntitySystemManager when an Entity is reused
    inline void reinitialize(std::string name) {
        // We only need to update the name. The key is updated when the Entity is cleared, everything else is managed by the EntitySystemManager.
        this->name = std::move(name);
    }
    
    inline void clear() {
        EntityKey newKey(key.getID(), key.getVersion() + 1);
        key = newKey;
    }
    
    EntitySystemManager* manager;
    EntityKey key;
    TransformationComponent* transformation;
    std::string name;
    EntityState* state;
};

inline EntityHandle::EntityHandle(Entity& entity) : entity(entity) {
    key = entity.getKey();
}

inline bool EntityHandle::isValid() const {
    return entity.getKey() == key;
}

enum class SystemSetting {
    HasPreAttachCallback = 0,
    HasPostDetachCallback = 1,
};

class SystemSettings {
public:
    SystemSettings() {}
    
    inline void activateSetting(SystemSetting setting) {
        settings[static_cast<std::size_t>(setting)] = true;
    }
    
    inline bool isSettingActive(SystemSetting setting) const {
        return settings[static_cast<std::size_t>(setting)];
    }
    
    inline std::bitset<64> getSettings() const {
        return settings;
    }
private:
    std::bitset<64> settings;
};

/// A base class for all systems used in the Engine.
class System : public GarbageCollecting {
public:
    System(EntitySystemManager* manager, SystemSettings settings, ComponentBaseType baseType, std::uint32_t subtypeCount)
        : settings(settings.getSettings()), baseType(baseType), subtypeCount(subtypeCount), manager(manager) {}
    
    inline EntitySystemManager* getManager() const {
        return manager;
    }
    
    inline bool isSettingActive(SystemSetting setting) const {
        return settings[static_cast<std::size_t>(setting)];
    }
    
    inline std::bitset<64> getSettings() const {
        return settings;
    }
    
    /// Each system is responsible for a single ComponentBaseType
    inline ComponentBaseType getManagedComponentType() const {
        return baseType;
    }
    
    /// A bitset that indicates which components of this System are attached to the Entity
    /// with the specifed id.
    inline ComponentSubTypeFlags getAvailableComponents(std::uint32_t id) const {
        return availableComponents[id];
    }
    
    /// Checks if a specific component is attached to the Entity with the specifed id
    inline bool hasComponent(std::uint32_t id, const ComponentType& componentType) const {
        if (componentType.getBaseType() != baseType) {
            return false;
        }
        
        return hasComponent(id, componentType.getSubType());
    }
    
    inline bool hasComponent(std::uint32_t id, std::uint32_t componentSubType) const {
        return availableComponents[id][componentSubType];
    }
    
    /// Checks if the Entity with the specifed id has any components that are managed by this System
    inline bool hasAnyComponents(std::uint32_t id) const {
        return availableComponents[id].any();
    }
    
    virtual void initialize() = 0;
    virtual void dispose() = 0;
    virtual void update(float delta, const EntityStateVector& entityStates) = 0;
    
    /// Obtain the number of Component subtypes that are managed by this System. The return value must match
    /// the COUNT value from a subtype enumerator (located in ComponentTypes.hpp) that corresponds to this System.
    /// E.g., GraphicsSystem must return GraphicsComponent::COUNT here.
    virtual std::size_t getSubTypeCount() const = 0;
    
    inline LocalizationHandle getManagedComponentTypeName() const {
        return con::ComponentBaseTypeNames[static_cast<std::size_t>(getManagedComponentType())];
    }
    
    template <typename T, bool safe = false>
    inline T& getComponent(std::uint32_t id) {
        static_assert(std::is_base_of_v<Component, T>, "Must provide a class derived from Component.");
        
        if (safe) {
            return dynamic_cast<T&>(getComponentBase(id, T::Type));
        } else {
            return static_cast<T&>(getComponentBase(id, T::Type));
        }
    }
    
    template <typename T, bool safe = false>
    inline const T& getComponent(std::uint32_t id) const {
        static_assert(std::is_base_of_v<Component, T>, "Must provide a class derived from Component.");
        
        if (safe) {
            return dynamic_cast<const T&>(getComponentBase(id, T::Type));
        } else {
            return static_cast<const T&>(getComponentBase(id, T::Type));
        }
    }
    
    inline Component& getComponentBase(std::uint32_t id, const ComponentType& type) {
        if (type.getBaseType() != baseType) {
            throw std::invalid_argument("The requested base type does not match the base type of the System.");
        }
        
        if (!availableComponents[id][type.getSubType()]) {
            throw std::runtime_error("The Entity does not have any components of the requested subtype.");
        }
        
        ComponentContainer* container = getContainer(type.getSubType());
        return container->get(id);
    }
    
    inline const Component& getComponentBase(std::uint32_t id, const ComponentType& type) const {
        if (type.getBaseType() != baseType) {
            throw std::invalid_argument("The requested base type does not match the base type of the System.");
        }
        
        if (!availableComponents[id][type.getSubType()]) {
            throw std::runtime_error("The Entity does not have any components of the requested subtype.");
        }
        
        const ComponentContainer* container = getContainer(type.getSubType());
        return container->get(id);
    }
    
    virtual ~System() {};
protected:
    friend class EntitySystemManager;
    
    virtual void preAttach([[maybe_unused]] Component& component, [[maybe_unused]] std::uint32_t id) {}
    virtual void postDetach([[maybe_unused]] Component& component, [[maybe_unused]] std::uint32_t id) {}
    
    /// Creates a default constructed Component of specifed ComponentType and attaches it to an Entity. Returns false if an
    /// existing Component is attached to the specifed Entity. This should only be called by our friend, the EntitySystemManager
    /// because some additional bookkeeping needs to be performed.
    virtual Component& createAndAttachComponent(const EntityKey& key, const ComponentType& type) = 0;
    
    inline ComponentContainer* getContainer(std::uint32_t subtype) {
        return components[subtype].get();
    }
    
    inline const ComponentContainer* getContainer(std::uint32_t subtype) const {
        return components[subtype].get();
    }
    
    inline Component& setComponent(std::uint32_t id, const Component& component) {
        ComponentType type = component.getType();
        
        if (type.getBaseType() != baseType) {
            throw std::invalid_argument("The requested base type does not match the base type of the System.");
        }
        
        ComponentContainer* container = getContainer(type.getSubType());
        Component& ret = container->set(id, component);
        availableComponents[id][type.getSubType()] = true;
        
        if (isSettingActive(SystemSetting::HasPreAttachCallback)) {
            preAttach(ret, id);
        }
        
        ret.attach(this, id);
        
        return ret;
    }
    
    inline Component& setComponent(std::uint32_t id, Component&& component) {
        ComponentType type = component.getType();
        
        if (type.getBaseType() != baseType) {
            throw std::invalid_argument("The requested base type does not match the base type of the System.");
        }
        
        ComponentContainer* container = getContainer(type.getSubType());
        assert(container != nullptr);
        Component& ret = container->set(id, std::move(component));
        availableComponents[id][type.getSubType()] = true;
        
        if (isSettingActive(SystemSetting::HasPreAttachCallback)) {
            preAttach(ret, id);
        }
        
        ret.attach(this, id);
        
        return ret;
    }
    
    inline void destroyAllComponents(std::uint32_t id) {
        ComponentSubTypeFlags flags = availableComponents[id];
        throw std::runtime_error("I'm broken. FIX ME.");
        for (std::size_t i = 0; i < static_cast<std::size_t>(GraphicsComponent::COUNT); ++i) {
            if (flags[i]) {
                destroyComponentUnchecked(id, i);
            }
        }
    }
    
    inline Component& destroyComponent(std::uint32_t id, const ComponentType& type) {
        if (type.getBaseType() != baseType) {
            throw std::invalid_argument("The requested base type does not match the base type of the System.");
        }
        
        if (!availableComponents[id][type.getSubType()]) {
            throw std::runtime_error("The Entity does not have any components of the requested subtype.");
        }
        
        return destroyComponentUnchecked(id, type.getSubType());
    }
    
    inline Component& destroyComponentUnchecked(std::uint32_t id, std::uint32_t subtype) {
        ComponentContainer* container = getContainer(subtype);
        
        Component& ptr = container->get(id);
        ptr.detach(this, id);
        
        if (isSettingActive(SystemSetting::HasPostDetachCallback)) {
            postDetach(ptr, id);
        }

        availableComponents[id][subtype] = false;
        
        return ptr;
    }
    
    inline void resize(std::uint32_t newSize) {
        availableComponents.resize(newSize);
        
        for (std::size_t i = 0; i < subtypeCount; ++i) {
            ComponentContainer* container = getContainer(i);
            if (container != nullptr) {
                container->resize(newSize);
            }
        }
    }
    
    std::bitset<64> settings;
    ComponentBaseType baseType;
    std::uint32_t subtypeCount;
    std::array<std::unique_ptr<ComponentContainer>, 64> components;
    EntitySystemManager* manager;
    std::vector<ComponentSubTypeFlags> availableComponents;
};

class EntitySystemManagerCreateInfo {
public:
    /// \param[in] engine Pointer to the Engine instance. Must not be nullptr
    inline EntitySystemManagerCreateInfo(Engine* engine) : engine(engine), initialCapacity(1024), capacityGrowthInterval(1024), maxCapacity(8192), editorMode(false) {
        if (engine == nullptr) {
            throw std::invalid_argument("Engine pointer was null");
        }
        
        // Used to determine if initial values are correct
        validate(true);
    }
    
    Engine* getEngine() const {
        return engine;
    }
    
    /// See setInitialCapacity()
    inline std::uint32_t getInitialCapacity() const {
        return initialCapacity;
    }
    
    /// \warning Growth requires memory allocations or reallocations that can cause lag spikes. You should always try to set
    /// initialCapacity to a sufficiently high value that won't be reached during gameplay.
    /// 
    /// \param[in] capacity when EntitySystemManager::initialize() is called, all Entity data vectors will be resized to accomodate
    /// getInitialCapacity() entities. Must not be 0.
    inline void setInitialCapacity(std::uint32_t capacity) {
        initialCapacity = capacity;
    }
    
    /// See setCapacityGrowthInterval()
    inline std::uint32_t getCapacityGrowthInterval() const {
        return capacityGrowthInterval;
    }
    
    /// \param[in] growth How much should internal data vectors grow every time their capacity is exceeded? This happens when a new Entity
    /// needs to be created, but internal vectors are full. In that case, EntitySystemManager::resize(currentCapaity + capacityGrowthInterval)
    /// will be called. Must not be 0. Must also satisfy this condition: (maxCapacity - initialCapacity) % capacityGrowthInterval == 0
    inline void setCapacityGrowthInterval(std::uint32_t growth) {
        capacityGrowthInterval = growth;
    }
    
    /// See setMaxCapacity()
    inline std::uint32_t getMaxCapacity() const {
        return maxCapacity;
    }
    
    
    /// \param[in] max This value sets the maximum number of entities that the EntitySystemManager is allowed to create. Must be >= initialCapacity.
    /// If the maximumCapacity is exceeded, an exception will be thrown. Used to limit memory use on purpose. Set to max value of unit32_t by default.
    inline void setMaxCapacity(std::uint32_t max) {
        maxCapacity = max;
    }
    
    /// See setEditorMode()
    inline bool isEditorMode() const {
        return editorMode;
    }
    
    /// If this is set, the EntitySystemManager will be created in "editor mode". This changes certain behaviours
    inline void setEditorMode(bool editor) {
        editorMode = editor;
    }
    
    /// Validates the values in this struct.
    /// \throws std::invalid_argument if throwException is true and validation fails
    bool validate(bool throwException = false);
private:
    Engine* engine;
    std::uint32_t initialCapacity;
    std::uint32_t capacityGrowthInterval;
    std::uint32_t maxCapacity;
    bool editorMode;
};

class EntityHierarchyNode;
using EntityHierarchy = std::map<std::string, EntityHierarchyNode>;

/// For use in the EntitySystemManager::entityHierachy (check the documentation there) and the Editor.
class EntityHierarchyNode {
public:
    EntityHierarchyNode() : entity(nullptr), parent(nullptr), selected(false) {}
    
    void setSelected(bool selected) {
        this->selected = selected;
        entity->setSelected(selected);
    }
    
    bool isSelected() const {
        return selected;
    }
    
    const EntityHierarchyNode* getParent() const {
        return parent;
    }
    
    const EntityHierarchy& getChildren() const {
        return children;
    }
    
    Entity& getEntity() const {
        return *entity;
    }
private:
    friend class EntitySystemManager;
    
    Entity* entity;
    EntityHierarchyNode* parent;
    EntityHierarchy children;
    bool selected;
};

using SystemArray = std::array<std::unique_ptr<System>, static_cast<std::size_t>(ComponentBaseType::COUNT)>;

/// \todo Functions that take EntityKey objects should check if they are valid. Those that take just the ID, should not.
class EntitySystemManager : private NonCopyable {
public:
    /// Constructs a new EntitySystemManager
    ///
    /// \todo Now that we mostly use ChunkedVector, does the warning still apply? Is it ok to turn the remaining std::vectors into ChunkedVectors?
    ///
    /// \todo Maybe remove initialCapacity and capacityGrowthInterval? They should probably match the chunk sizes.
    EntitySystemManager(EntitySystemManagerCreateInfo createInfo);
    virtual ~EntitySystemManager() { }
    
    inline Engine* getEngine() {
        return createInfo.getEngine();
    }
    
    virtual void initialize();
    virtual void dispose();
    virtual void update(float delta);
    
    inline bool isEditorMode() const {
        return getCreateInfo().isEditorMode();
    }
    
    /// Returns the number of Entity objects that have been created, including free slots that are not currently used at the moment.
    inline std::uint32_t getEntityCount() const {
        return nextID;
    }
    
    
    /// Turns a file path into a unique name for an Entity by extracting the filename without an extension and
    /// appending a number to it if the initial result was not unique.
    ///
    /// \remark This method is not particularly fast 
    std::string filePathToEntityName(const fs::path& name) const;
    
    /// Turns a string into a unique name for an Entity
    std::string makeUniqueName(const std::string& name) const;
    
//    inline void getComponent(const Entity& entity, ComponentType ct) {
//        SystemBase* system = getSystemManagingComponentType(ct);
//    }
    
    /// Creates a new Entity with no Components and returns an reference to it. You may start adding Components 
    /// and using its name in lookups immediately. Even if active is true, no updates or scripts will be called until the 
    /// Entity is initialized at the start of the next frame.
    ///
    /// \remark If you're planning to use the Entity for more than a single frame, calling Entity::makeHandle() may be 
    /// a good idea.
    ///
    /// \remark This method is not thread safe.
    /// \todo Should I make it thread safe?
    ///
    /// \param[in] name The name of the Entity. Must be unique in this EntitySystemManager. Use filePathToEntityName()
    /// or makeUniqueName() to generate unique names when creating Entities manually
    /// \param[in] active Should the Entity be automatically activated when it is initialized the next frame.
    /// \return An reference to the new Entity.
    EntityKey create(const std::string& name, bool active = true);
    
    // FIXME this dude is borked.
//     /// Creates n empty Entities that are guaranteed to be stored contiguously and returns their keys. Typically used when creating Entity hierarchies
//     /// or when loading.
//     ///
//     /// \remark This method is not thread safe.
//     /// \todo Should I make it thread safe?
//     ///
//     /// \note You should make sure that the output vector has enough space reserved to accomodate the returned entities before calling this function.
//     /// This function calls push_back n times and using a vector of insufficient capacity will trigger a resize (or several).
//     ///
//     /// \param[in] n Number of Entity objects to create
//     /// \param[out] output New EntityKey values, corresponding to created entities, will be pushed to the back of this vector.
//     /// \param[in] atEnd Optional parameter. Setting it to false will tell the function to look for a contiguous range of freeSlots and place the new
//     /// entities there (if a sufficent free range is found). This can be expensive because it sorts the freeSlots vector, however, it will likely save memory.
//     /// Setting it to true (default) will append the entities at the end of data arrays, ignoring the freeSlots vector altogether.
//     ///
//     /// \todo THIS IS BROKEN AT THE MOMENT!!!
//     void create(std::uint32_t n, std::vector<EntityKey>& output, bool atEnd = true);
    
    /// Adds the Entity to the awaitingDestruction vector and sets the AwaitingDestruction bit. Actual destruction will take place some time later.
    /// Freeing a parent will automatically free all child Entities as well.
    ///
    /// The Entity and its children are guaraneed to stay alive and usable until the end of this frame.
    ///
    /// \remark This method is not thread safe.
    /// \todo Should I make it thread safe?
    ///
    /// \param[in] key The EntityKey of the Entity to destroy
    void free(const EntityKey& key);
    
    inline void free(std::vector<EntityKey>& keys) {
        for (const auto& key : keys) {
            free(key);
        }
    }
    
    /// Retrieve an Entity by ID. In most cases, you should prefer to use getEntityByName().
    ///
    /// \warning This function should only be used in extremely rare cases. It does not perform ANY checks - it's possible to get an invalid Entity or 
    /// even crash the Engine. Moreover, the IDs are not guaranteed to stay consistent over multiple executions.
    inline const Entity& getEntityByID(std::uint32_t id) const {
        return entities[id];
    }
    
    /// Retrieve an Entity by ID. In most cases, you should prefer to use getEntityByName().
    ///
    /// \warning This function should only be used in extremely rare cases. It does not perform ANY checks - it's possible to get an invalid Entity or 
    /// even crash the Engine. Moreover, the IDs are not guaranteed to stay consistent over multiple executions.
    inline Entity& getEntityByID(std::uint32_t id) {
        return entities[id];
    }
    
    /// Returns a pointer to a System that manages the specified ComponentBaseType.
    ///
    /// \warning The EntitySystemManager retains ownership of the pointer. DO NOT call delete on it.
    /// \todo This function's name is dumb. Think of something better
    ///
    /// \param[in] ct ComponentBaseType that's managed by the system you want to get
    inline System* getSystemManagingComponentType(ComponentBaseType ct) {
        assert(ct != ComponentBaseType::COUNT);
        
        return systems[static_cast<std::size_t>(ct)].get();
    }
    
    /// Returns a const pointer to a System that manages the specified ComponentBaseType.
    ///
    /// \warning The EntitySystemManager retains ownership of the pointer. DO NOT call delete on it.
    ///
    /// \param[in] ct ComponentBaseType that's managed by the system you want to get
    inline const System* getSystemManagingComponentType(ComponentBaseType ct) const {
        assert(ct != ComponentBaseType::COUNT);
        
        return systems[static_cast<std::size_t>(ct)].get();
    }
    
    /// Attaches a default constructed Component to an Entity. Any existing components won't be replaced (this function will return 
    /// false) because they need to be explicitly destroyed.
    ///
    /// \param key the EntityKey of the Entity to attach to.
    /// \param component The component you want to attach.
    /// \return true if attachment succeeded
    bool attachComponent(const EntityKey& key, const ComponentType& type);
    
    /// Attaches a Component to an Entity by copying it. Any existing components won't be replaced (this function will return false)
    /// because they need to be explicitly destroyed.
    ///
    /// \param key the EntityKey of the Entity to attach to.
    /// \param component The component you want to attach.
    /// \return true if attachment succeeded
    bool attachComponent(const EntityKey& key, const Component& component);
    
    /// Attaches a Component to an Entity by moving it. Any existing components won't be replaced (this function will return false)
    /// because they need to be explicitly destroyed.
    ///
    /// \param key the EntityKey of the Entity to attach to.
    /// \param component The component you want to attach.
    /// \return true if attachment succeeded
    bool attachComponent(const EntityKey& key, Component&& component);
    
    bool removeComponent(const EntityKey& key, const ComponentType& type);
    
    /// Determines if the Entity is valid by comparing the version components of the EntityKey objects
    inline bool isEntityValid(const EntityKey& key) const {
        return key.getVersion() == entities[key.getID()].getKey().getVersion();
    }
    
    /// Checks if the Entity has any Components of a specific base type.
    /// 
    /// \warning This function does not perform any validation on its own and will return invalid data or
    /// even crash the program if it's called on an Entity that is not valid (isEntityValid() == false)
    ///
    /// \todo This should perform validation, but what needs to be returned when it is invalid? false? Custom enum? Throw an exception?
    /// 
    /// \param[in] key the EntityKey of the Entity to check
    /// \param[in] ct Base type to look for
    /// \return If a component is present or not
    inline bool entityHasAnyComponents(const EntityKey& key, ComponentBaseType ct) const {
        return entityHasAnyComponents(key.getID(), ct);
    }
    
    /// Checks if the Entity has any Components of a specific base type.
    /// 
    /// \warning This function does not perform any validation on its own and will return invalid data or
    /// even crash the program if it's called on an Entity that is not valid (isEntityValid() == false)
    /// 
    /// \param[in] entityID the id of the Entity to check
    /// \param[in] ct Base type to look for
    /// \return If a component is present or not
    inline bool entityHasAnyComponents(std::uint32_t entityID, ComponentBaseType ct) const {
        return entityStates[entityID].hasComponentsOfType(ct);
    }
    
    /// Checks if the entity has the specified component
    inline bool entityHasComponent(const EntityKey& key, const ComponentType& type) const {
        return entityHasComponent(key.getID(), type);
    }
    
    inline bool entityHasComponent(std::uint32_t entityID, const ComponentType& type) const {
        const System* system = getSystemManagingComponentType(type.getBaseType());
        
        return system->hasComponent(entityID, type.getSubType());
    }
    
    inline const std::vector<Component*>& getAllComponents(const EntityKey& key) const {
        return componentsInEntity[key.getID()];
    }
    
    inline const EntityState& getEntityState(const EntityKey& key) const {
        return entityStates[key.getID()];
    }
    
    inline const EntityState& getEntityState(std::uint32_t entityID) const {
        return entityStates[entityID];
    }
    
    inline TransformationComponent& getEntityTransformation(std::uint32_t id) {
        return transformations[id];
    }
    
    inline const TransformationComponent& getEntityTransformation(std::uint32_t id) const {
        return transformations[id];
    }
    
    inline TransformationComponent& getEntityTransformation(const EntityKey& key) {
        return transformations[key.getID()];
    }
    
    inline const TransformationComponent& getEntityTransformation(const EntityKey& key) const {
        return transformations[key.getID()];
    }
    
    inline const TransformationVector& getEntityTransformations() const {
        return transformations;
    }
    
    inline TransformationVector& getEntityTransformations() {
        return transformations;
    }
    
    inline std::size_t getFreeSlotCount() const {
        return freeSlots.size();
    }
    
    inline std::size_t getHighestID() const {
        return nextID - 1;
    }
    
    inline EntityHierarchy& getEntityHierarchy() {
        if (!createInfo.isEditorMode()) {
            std::logic_error("This function can only be called when an EntitySystemManager is created in editor mode.");
        }
        
        return entityHierachy;
    }
    
    /// Obtain a const reference to an array containing all Systems that are running in the Engine. Since every System corresponds to a single 
    /// ComponentBaseType, the array contains ComponentBaseType::COUNT elements. Moreover, when cast to an integer, ComponentBaseType enumeration
    /// values may be used to access specific Systems.
    ///
    /// Typically, you should use getSystemManagingComponentType() to fetch the Systems that you actually need. This function was created for use 
    /// in the Editor.
    const SystemArray& getSystems() const {
        return systems;
    }
protected:
    /// Adds a new System to this manager
    void registerSystem(std::unique_ptr<System> system);
    
    inline const EntitySystemManagerCreateInfo& getCreateInfo() const {
        return createInfo;
    }
private:
    /// Checks if we need to resize and performs the operation if we do
    void resize(std::uint32_t newSize);
    
    void manageEntityLifecycles(float delta);
    
    bool validateComponentAttachment() const;
    
    /// All Entity instances and their Component instances are stored in big vectors (some exceptions exist). Simply removing them 
    /// (e.g. when an object gets destroyed) by using vector::erase() is not an option because that would invalidate all Entity IDs
    /// that go after the removed element. Instead, we simply invalidate the data and add the ID to a list of "free slots".
    std::vector<std::uint32_t> freeSlots;
    
    /// The size of this vector must always match the number of existing Entities. It is used to determine which components exist
    /// in each Entity. Using a container that exists inside the EntitySystemManager instead of storing ComponentBaseTypeFlags in each
    /// Entity makes iteration signifficantly faster because a lot less data is pulled to cache.
    ///
    /// \todo Now that I'm using a ChunkedVector, are iterators used whenever possible?
    EntityStateVector entityStates;
    
    /// The EntitySystemManager manages all transformations since they're mandatory for all entities
    TransformationVector transformations;
    
    /// A list of all components assigned to a specific entity
    ChunkedVector<std::vector<Component*>, SystemChunkSize> componentsInEntity;
    
    /// Used to check for conflicts of systems. E.g. someone attempts to register multiple systems that manage a single component type 
    std::bitset<static_cast<std::size_t>(ComponentBaseType::COUNT)> managedComponents;
    
    /// The handle implementation used in this engine takes some ideas from this article: http://gamesfromwithin.com/managing-data-relationships.
    /// The Entity objects stored in this vector have EntityKey variables that contain the "version" (called "counter field" in the article) of each Entity slot.
    ChunkedVector<Entity, SystemChunkSize> entities;
    
    /// Contains all Systems that are derived from System and registered with this manager.
    SystemArray systems;
    
    std::vector<EntityKey> awaitingInitialization;
    std::vector<EntityKey> awaitingDestruction;
    
    /// Maps Entity names to pointers. We're not using hash32_t here because it makes resolving hash colisions difficult.
    std::unordered_map<std::string, Entity*> nameToEntity;
    
    /// A lexicographically sorted hierarchy of EntityHierarchyNode objects. It is used to display a nice sorted list of Entities in the editor and to perform
    /// searches by using incomplete Entity names. For performance reasons, this map is only built and updated when EntitySystemManager.getCreateInfo().isEditorMode()
    /// is true.
    ///
    /// \remark A map is used for several reasons, first of all, it always statys sorted. Next, neither iterators, nor references to existing elements are invalidated
    /// when elements are inserted or erased. Finally, when the hierarchy changes, nodes may easily be moved from one map to another by using std::map::extract().
    /// The best part? Moving an element with std::map::extract() PRESERVES its location in memory and pointers to it stay valid, which means there's no need to
    /// update the parent pointer in child objects.
    EntityHierarchy entityHierachy;
    
    EntitySystemManagerCreateInfo createInfo;
    
    std::uint32_t nextID;
    std::uint32_t currentCapacity;
    
    bool initialized;
};

inline bool Entity::attachComponent(const Component& component) {
    return manager->attachComponent(key, component);
}

inline bool Entity::attachComponent(Component&& component) {
    return manager->attachComponent(key, std::move(component));
}

inline bool Entity::removeComponent(const ComponentType& type) {
    return manager->removeComponent(key, type);
}

inline bool Entity::hasComponents(ComponentBaseType baseType) const {
    return manager->entityHasAnyComponents(key.getID(), baseType);
}
    
inline bool Entity::hasComponent(const ComponentType& type) const {
    return manager->entityHasComponent(key.getID(), type);
}

inline const std::vector<Component*>& Entity::getAllComponents() const {
    return manager->getAllComponents(key);
}

template <typename T>
inline T& Entity::getComponent() {
    static_assert(std::is_base_of_v<Component, T>, "Must provide a class derived from Component.");
    
    System* system = manager->getSystemManagingComponentType(T::Type.getBaseType());
    
    return system->getComponent<T>(key.getID());
}

template <typename T>
inline const T& Entity::getComponent() const {
    static_assert(std::is_base_of_v<Component, T>, "Must provide a class derived from Component.");
    
    const System* system = manager->getSystemManagingComponentType(T::Type.getBaseType());
    
    return system->getComponent<T>(key.getID());
}

// template <typename T>
// const T& System<T>::getComponent(const Entity& entity) const {
//     if (manager->isEntityValid(entity) && manager->entityHasComponent(entity, getType())) {
//         return components[entity.getID()];
//     } else {
//         throw std::out_of_range("An Entity is not valid or it does not have this component.");
//     }
// }
// 
// template <typename T>
// T& System<T>::getComponent(const Entity& entity) {
//     if (manager->isEntityValid(entity) && manager->entityHasComponent(entity, getType())) {
//         return components[entity.getID()];
//     } else {
//         throw std::out_of_range("An Entity is not valid or it does not have this component.");
//     }
// }

}

#endif //ENTITY_SYSTEM_MANAGER_HPP
