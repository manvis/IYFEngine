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

#ifndef IYF_COMPONENT_CONTAINER_HPP
#define IYF_COMPONENT_CONTAINER_HPP

#include "core/ComponentType.hpp"
#include "core/Component.hpp"

namespace iyf {
class System;

/// This class and all of its derivatives are dumb containers that do not call certain important methods.
/// They should only be used from inside a System.
class ComponentContainer {
public:
    ComponentContainer(System* system, ComponentType componentType);
    virtual ~ComponentContainer() {};
    
    inline const ComponentType& getType() const {
        return componentType;
    }
    
    inline System* getSystem() const {
        return system;
    }
    
    /// Obtains a reference to a Component with the specified Entity id.
    virtual Component& get(std::uint32_t id) = 0;
    /// Obtains a const reference to a Component with the specified Entity id.
    virtual const Component& get(std::uint32_t id) const = 0;
    
    /// Assigns a copy of the provided Component to a Component with the specified Entity id
    /// and calls Component::attach() on it.
    ///
    /// \warning This method does not call Component::detach() or ComponentContainer::destroy()
    /// on any previously created Components. Component lifecycles are tracked inside the System
    /// and it is responsible for cleanup (if it is required).
    virtual Component& set(std::uint32_t id, const Component& component) = 0;
    
    /// Moves the provided Component into the Component with the specified Entity id
    /// and calls Component::attach() on it.
    ///
    /// \warning This method does not call Component::detach() or ComponentContainer::destroy()
    /// on any previously created Components. Component lifecycles are tracked inside the System
    /// and it is responsible for cleanup (if it is required).
    virtual Component& set(std::uint32_t id, Component&& component) = 0;
    
//     /// Moves a component from one Entity id to another. Component::detach() is called on the source,
//     /// it is then moved and Component::attach() is called on the destination.
//     /// 
//     /// \warning This method does not call Component::detach() or ComponentContainer::destroy()
//     /// on the destination Component. Component lifecycles are tracked inside the System
//     /// and it is responsible for cleanup (if it is required).
//     virtual void move(std::uint32_t source, std::uint32_t destination) = 0;
    
    /// Resizes (if applicable) the container to accomodate at least newSize components.
    /// Shrinking is not allowed - only growing.
    virtual void resize(std::uint32_t newSize) = 0;
private:
    System* system;
    ComponentType componentType;
};
}

#endif // IYF_COMPONENT_CONTAINER_HPP
