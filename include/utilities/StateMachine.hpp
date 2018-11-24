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

#ifndef IYF_STATE_MACHINE_HPP
#define IYF_STATE_MACHINE_HPP

#include <type_traits>
#include <stdexcept>
#include <vector>
#include <cassert>

namespace iyf {
template <typename ObjType, typename StateTypeID>
class StateMachineManager;

template <typename ObjType, typename StateTypeID>
class StateMachineState {
public:
    StateMachineState(StateMachineManager<ObjType, StateTypeID>* manager) : manager(manager) {}
    
    static_assert(std::is_enum_v<StateTypeID>, "StateTypeID must be an enumerator type.");
    
    /// Called when the object enters a new state.
    virtual void enter(ObjType& object) = 0;
    
    /// Called whenever the object that contains this state is updated.
    virtual void update(ObjType& object) = 0;
    
    /// Called when the object leaves this state.
    virtual void exit(ObjType& object) = 0;
    
    /// Uniquely identifies the type of this StateMachineState.
    virtual StateTypeID getTypeID() const = 0;
    
    /// Used by the StateMachineManager. If true (default), it's sufficient to
    /// allocate a single instance because the StateMachineState doesn't store any
    /// object specific data inside.
    ///
    /// If it's false, a new StateMachineState must be instantiated for each object.
    virtual bool isStateless() const {
        return true;
    }
    
    virtual ~StateMachineState() {}
protected:
    StateMachineManager<ObjType, StateTypeID>* manager;
};

/// An initialized that is used to build stateful states
struct StateInitializer {
    virtual ~StateInitializer() {}
};


template <typename ObjType, typename StateTypeID>
class StateMachineManager {  
public:  
    using StateBase = StateMachineState<ObjType, StateTypeID>;
    
    static_assert(std::is_enum_v<StateTypeID>, "StateTypeID must be an enumerator type.");
    
    StateMachineManager() : statelessStates(static_cast<std::size_t>(StateTypeID::COUNT), nullptr) {}
    virtual ~StateMachineManager() {}
    
    virtual StateBase* getState(StateTypeID id, StateInitializer* initializer = nullptr) = 0;
    virtual void returnState(StateBase* state) = 0;
protected:
    template <typename T>
    StateBase* getStatelessState(StateTypeID id) {
        StateBase*& statePtr = statelessStates[static_cast<std::size_t>(id)];
        
        if (statePtr != nullptr) {
            return statePtr;
        } else {
            T* newState = new T(this);
            assert(newState->isStateless());
            statePtr = newState;
        }
        
        assert(statePtr->getTypeID() == id);
        return statePtr;
    }
    
    template <typename T>
    StateBase* getStatefulState(StateTypeID id, StateInitializer* initializer = nullptr) {
        T* newState = new T(this, initializer);
        assert(!newState->isStateless());
        assert(newState->getTypeID() == id);
        
        return newState;
    }

private:
    std::vector<StateBase*> statelessStates;
};

template <typename ObjType, typename StateTypeID>
class StateMachine {
public:
    using Manager = StateMachineManager<ObjType, StateTypeID>;
    using StateBase = StateMachineState<ObjType, StateTypeID>;
    
    static_assert(std::is_enum_v<StateTypeID>, "StateTypeID must be an enumerator type.");
    
    StateMachine(Manager* manager, ObjType* container) : manager(manager), container(container), globalState(nullptr),
        currentState(nullptr), previousState(nullptr), initialized(false)
    {
        if (manager == nullptr) {
            throw std::runtime_error("Managers can't be null");
        }
        
        if (container == nullptr) {
            throw std::runtime_error("Container can't be null");
        }
    }
    
    inline bool isInitialized() const {
        return initialized;
    }
    
    inline StateBase* getGlobalState() const {
        return globalState;
    }
    
    inline StateBase* getCurrentState() const {
        return currentState;
    }
    
    inline StateBase* getPreviousState() const {
        return previousState;
    }
    
    inline void update() {
        if (!initialized) {
            return;
        }
        
        if (globalState != nullptr) {
            globalState->update(*container);
        }
        
        if (currentState != nullptr) {
            currentState->update(*container);
        }
    }
    
    bool initialize(StateTypeID globalStateID, StateTypeID currentStateID, StateInitializer* currentStateInitializer = nullptr) {
        if (initialized) {
            return false;
        }
        
        this->globalState = manager->getState(globalStateID);
        this->currentState = manager->getState(currentStateID, currentStateInitializer);
        
        initialized = true;
        return true;
    }
    
    /// \warning No exit functions will be called, therefore, only use this when actually destroying the object.
    void dispose() {
        if (globalState != nullptr) {
            manager->returnState(globalState);
            globalState = nullptr;
        }
        
        if (currentState != nullptr) {
            manager->returnState(currentState);
            currentState = nullptr;
        }
        
        if (previousState != nullptr) {
            manager->returnState(previousState);
            previousState = nullptr;
        }
        
        initialized = false;
    }
    
    void changeState(StateTypeID stateID, StateInitializer* stateInitializer = nullptr) {
        if (previousState != nullptr) {
            manager->returnState(previousState);
        }
        
        previousState = currentState;
        if (currentState != nullptr) {
            currentState->exit(*container);
        }
        
        currentState = manager->getState(stateID, stateInitializer);
        if (currentState != nullptr) {
            currentState->enter(*container);
        }
    }
    
    void revertToPreviousState() {
        StateBase* temp = previousState;
        
        previousState = currentState;
        if (currentState != nullptr) {
            currentState->exit(*container);
        }
        
        currentState = temp;
        if (currentState != nullptr) {
            currentState->enter(*container);
        }
    }
private:
    Manager* manager;
    ObjType* container;
    StateBase* globalState;
    StateBase* currentState;
    StateBase* previousState;
    
    bool initialized;
};

}

#endif // IYF_STATE_MACHINE_HPP
