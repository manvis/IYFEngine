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
#ifndef IYF_BLACKBOARD_HPP
#define IYF_BLACKBOARD_HPP

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <stdexcept>

#include "glm/gtx/string_cast.hpp"
#include "utilities/hashing/Hashing.hpp"

#include "ai/BlackboardCallbackListener.hpp"
#include "ai/BlackboardValue.hpp"

namespace iyf {
template<class T>
struct FalseType : std::false_type {};

enum class BlackboardValueAvailability {
    Available,
    NotAvailable,
    Invalid
};

struct BlackboardInitializer {
    BlackboardInitializer() : name("UnnamedBlackboard") {}
    
    std::string name;
    
    /// A vector that contains names hashes of ALL values that can be stored on the Blackboard and their initial values.
    std::vector<std::pair<std::string, BlackboardValue>> initialValues;
};

class Blackboard {
public:
    Blackboard(const BlackboardInitializer& initializer);
    
    inline const std::string& getName() const {
        return name;
    }
    
    /// Checks if a value with the nameHash is present in the value map
    inline bool hasValue(StringHash nameHash, BlackboardValueType type = BlackboardValueType::ANY) const {
        auto result = values.find(nameHash);
        if (result != values.end()) {
            const BlackboardValueContainer& container = result->second;
            return (type == BlackboardValueType::ANY) ? true : (container.value.index() == static_cast<std::size_t>(type));
        } else {
            return false;
        }
    }
    
    /// Checks if a value with the nameHash is present in the value map and retrieve its type.
    inline std::pair<bool, BlackboardValueType> getValueType(StringHash nameHash) const {
        auto result = values.find(nameHash);
        if (result != values.end()) {
            return {true, static_cast<BlackboardValueType>(result->second.value.index())};
        } else {
            return {false, BlackboardValueType::ANY};
        }
    }
    
    /// Checks if the value is available.
    ///
    /// The returned BlackboardValueAvailability value depends on two things:
    /// 1. Presence. If the value can't be found in the map, that is, hasValue() is false, BlackboardValueAvailability::Invalid
    /// will be returned.
    /// 2. Content. If the value was found in the map, it's nullable (BlackboardValueType::Pointer) and its value is nullptr,
    /// BlackboardValueAvailability::NotAvailable will be returned. BlackboardValueAvailability::Available will be returned in all
    /// other cases.
    inline BlackboardValueAvailability isValueAvailable(StringHash nameHash) const {
        auto result = values.find(nameHash);
        if (result != values.end()) {
            const BlackboardValueContainer& container = result->second;
            const BlackboardValue& value = container.value;
            
            if (value.index() == static_cast<std::size_t>(BlackboardValueType::Pointer) && std::get<void*>(value) == nullptr) {
                return BlackboardValueAvailability::NotAvailable;
            } else {
                return BlackboardValueAvailability::Available;
            }
        } else {
            return BlackboardValueAvailability::Invalid;
        }
    }
    
    void setValue(StringHash nameHash, BlackboardValue newValue) {
        auto result = values.find(nameHash);
        if (result == values.end()) {
            throw std::out_of_range(makeMissingValueMessage(nameHash));
        }
        
        BlackboardValueContainer& container = result->second;
        BlackboardValue& value = container.value;
        
        if (value.index() != newValue.index()) {
            throw std::logic_error("The new value is of a different type");
        }
        
        value = std::move(newValue);
        
        if (value.index() != static_cast<std::size_t>(BlackboardValueType::Pointer)) {
            for (BlackboardCallbackListener* l : container.listeners) {
                l->valueUpdated(nameHash);
            }
        } else {
            void* ptr = std::get<void*>(value);
            for (BlackboardCallbackListener* l : container.listeners) {
                l->availabilityUpdated(nameHash, (ptr != nullptr));
            }
        }
    }
    
    template <typename T>
    inline T getValue(StringHash nameHash) const {
        auto result = values.find(nameHash);
        if (result == values.end()) {
            throw std::out_of_range(makeMissingValueMessage(nameHash));
        }
        
        const BlackboardValueContainer& container = result->second;
        const BlackboardValue& value = container.value;
        
        if constexpr (std::is_pointer_v<T>) {
            void* pointer = std::get<void*>(value);
            
            return static_cast<T>(pointer);
        } else {
            return std::get<T>(value);
        }
    }
    
    inline BlackboardValue getRawValue(StringHash nameHash) const {
        auto result = values.find(nameHash);
        if (result == values.end()) {
            throw std::out_of_range(makeMissingValueMessage(nameHash));
        }
        
        const BlackboardValueContainer& container = result->second;
        const BlackboardValue& value = container.value;
        
        return value;
    }
    
    std::string toString() const;
    
    bool registerListener(StringHash nameHash, BlackboardCallbackListener* listener);
    std::size_t unregisterListener(StringHash nameHash, BlackboardCallbackListener* listener);
private:
    std::string makeMissingValueMessage(StringHash nameHash) const;
    
    std::string name;
    std::unordered_map<StringHash, BlackboardValueContainer> values;
};
}

#endif // IYF_BLACKBOARD_HPP
