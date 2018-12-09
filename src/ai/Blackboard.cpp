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

#include "ai/Blackboard.hpp"

#include <sstream>
#include <algorithm>

namespace iyf {
Blackboard::Blackboard(const BlackboardInitializer& initializer) : name(initializer.name) {
    values.reserve(initializer.initialValues.size());
    
    for (const auto& iv : initializer.initialValues) {
        StringHash hashedName = HS(iv.first);
        auto result = values.emplace(hashedName, BlackboardValueContainer(iv.first, hashedName, iv.second));
        if (!result.second) {
            throw std::logic_error("Dupilcate values or a hash collision detected in a blackboard");
        }
    }
}

std::string Blackboard::toString() const {
    std::stringstream ss;
    
    ss << "Blackboard: " << name;
    for (auto& v : values) {
        const std::string val = std::visit([](auto&& a) -> std::string {
            using T = std::decay_t<decltype(a)>;
            
            if constexpr (std::is_same_v<T, bool>) {
                return a ? "True" : "False";
            } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, int>) {
                return std::to_string(a);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return a;
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                return glm::to_string(a);
            } else if constexpr (std::is_same_v<T, void*>) {
                std::stringstream ss;
                ss << "ptr 0x" << std::hex << a;
                return ss.str();
            } else {
                static_assert(FalseType<T>::value, "Unhandled type.");
            }
            
        }, v.second.value);
        ss << "\n\t" << v.second.name << ": " << val;
    }
    
    return ss.str();
}

bool Blackboard::registerListener(StringHash nameHash, BlackboardCallbackListener* listener) {
    auto result = values.find(nameHash);
    if (result == values.end()) {
        return false;
    }
    
    result->second.listeners.emplace_back(listener);
    return true;
}

std::size_t Blackboard::unregisterListener(StringHash nameHash, BlackboardCallbackListener* listener) {
    auto result = values.find(nameHash);
    if (result == values.end()) {
        return false;
    }
    
    std::vector<BlackboardCallbackListener*>& listeners = result->second.listeners;
    
    const std::size_t preRemove = listeners.size();
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
    const std::size_t postRemove = listeners.size();
    
    return preRemove - postRemove;
}

std::string Blackboard::makeMissingValueMessage(StringHash nameHash) const {
    std::stringstream ss;
    ss << "Value with name hash " << nameHash << " wasn't found on the blackboard called \"" << name << "\"";
    return ss.str();
}

}
