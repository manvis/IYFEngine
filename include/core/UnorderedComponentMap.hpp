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

#ifndef IYF_UNORDERED_COMPONENT_MAP_HPP
#define IYF_UNORDERED_COMPONENT_MAP_HPP

#include "core/interfaces/ComponentContainer.hpp"

#include <type_traits>
#include <unordered_map>

namespace iyf {
/// This functions because: "References and pointers to either key or data stored in the container are only invalidated by
/// erasing that element, even when the corresponding iterator is invalidated."
/// Source: http://en.cppreference.com/w/cpp/container/unordered_map
template <typename T>
class UnorderedComponentMap : public ComponentContainer {
public:
    static_assert(std::is_base_of_v<Component, T>, "T must be derived from Component");
    static_assert(std::is_copy_assignable_v<T>, "T must be copy assignable");
    static_assert(std::is_move_assignable_v<T>, "T must be move assignable");
    
    using MapType = std::unordered_map<std::uint32_t, T>;
    
    UnorderedComponentMap(System* system, ComponentType componentType) : ComponentContainer(system, componentType) {}
    
    virtual Component& get(std::uint32_t id) final override {
        return components[id];
    }
    
    virtual const Component& get(std::uint32_t id) const final override {
        return components.find(id)->second;
    }
    
    virtual void set(std::uint32_t id, const Component& component) final override {
        components[id] = static_cast<const T&>(component);
        components[id].attach(getSystem(), id);
    }
    
    virtual void set(std::uint32_t id, Component&& component) final override {
        components[id] = std::move(static_cast<T&&>(component));
        components[id].attach(getSystem(), id);
    }
    
    virtual void destroy(std::uint32_t id) final override {
        components[id].detach(getSystem(), id);
    }
    
    virtual void move(std::uint32_t source, std::uint32_t destination) final override {
        components[source].detach(getSystem(), source);
        
        components[destination] = std::move(components[source]);
        
        components[destination].attach(getSystem(), destination);
    }
    
    virtual void resize(std::uint32_t) final override {
        // Not required for a map. I could call reserve, however, this container is used for components
        // that are attached to very few objects, e.g., Cameras. 
    }
    
    inline typename MapType::iterator begin() {
        return components.begin();
    }
    
    inline typename MapType::iterator end() {
        return components.end();
    }
    
    inline typename MapType::iterator begin() const {
        return components.begin();
    }
    
    inline typename MapType::iterator end() const {
        return components.end();
    }
private:
    MapType components;
};
}

#endif // IYF_UNORDERED_COMPONENT_MAP_HPP
