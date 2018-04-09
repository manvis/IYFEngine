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

#ifndef COMPONENT_TYPE_HPP
#define COMPONENT_TYPE_HPP

#include <array>
#include <vector>
#include <bitset>
#include <typeinfo>
#include "utilities/hashing/Hashing.hpp"

namespace iyf {
/// Enumerator of all major component types (graphics, physics, AI, etc.) supported by the Engine. Each ComponentBaseType must also have a 
/// corresponding subtype enumerator with at least one value. All such enumerators should be defined in this file, below the "SUBTYPES START
/// HERE" line.
///
/// \warning Game developers: if you need any, you can add custom ComponentBaseTypes between the dashed lines. DO NOT TOUCH the Engine 
/// ComponentBaseTypes (above the first line) and, even more importantly, DO NOT touch enum values that are written IN ALL CAPS.
/// 
/// \warning Engine developers: ALWAYS update the LAST_ENGINE_COMPONENT when modifying the list Engine components.
/// 
/// \warning Everyone: First of all, all IDs must be sequential and they CANNOT be reordered or removed without breaking Projects that use
/// them. Therefore, if you add a new ComponentBaseType and, mid-project, realize that you no longer need it, KEEP IT IN THIS ENUM, OR 
/// ELSE!!! *cue scary music*. Next, all ComponentBaseTypes must have corresponding localization string hashes in the ComponentBaseTypeNames
/// array or the editor will either segfault or print grabage (or both ☺). Finally, the ComponentBaseType values are used as offsets in a 
/// 64 bit std::bitset that's wrapped by the EntityState. EntityState has to store some additional data, therefore the ComponentBaseType::COUNT
/// is limited to 48. Some of the higher bytes are reserved, yet unused. In other words, you MAY be able to squeeze in more systems, but be careful
/// and adjust the static_assert here and in the EntityState class.
/// 
/// \remark The undelying type is std::uint32_t because of alignment requirements.
enum class ComponentBaseType : std::uint32_t {
    Graphics = 0,
    Physics = 1,
    //----------------------------------------------------
    // Your components go here:
    
    //----------------------------------------------------
    COUNT,
    LAST_ENGINE_COMPONENT = Physics
};

static_assert(static_cast<std::size_t>(ComponentBaseType::COUNT) <= 48, "You cannot have more than 48 ComponentBaseTypes");

// TODO Would it be better if these became real types?
using ComponentSubTypeFlags = std::bitset<64>;

class ComponentType {
public:
    template <typename T>
    constexpr inline ComponentType(ComponentBaseType baseType, T subType) : baseType(baseType), subType(static_cast<std::uint32_t>(subType)) {
        static_assert(std::is_enum_v<T>, "An enumerator must be provided.");
        static_assert(std::is_same_v<typename std::underlying_type<T>::type, std::uint32_t>, "The undelying type of the provided enumerator must be std::uint32_t");
    }
    
    constexpr inline ComponentType(ComponentBaseType baseType, std::size_t subType) : baseType(baseType), subType(subType) {}
    
    inline ComponentBaseType getBaseType() const {
        return baseType;
    }
    
    inline std::uint32_t getSubType() const {
        return subType;
    }
    
    friend bool operator==(const ComponentType& a, const ComponentType& b) {
        return (a.baseType == b.baseType) && (a.subType == b.subType);
    }
private:
    ComponentBaseType baseType;
    std::uint32_t subType;
};

namespace con {
extern const std::array<hash32_t, static_cast<std::size_t>(ComponentBaseType::COUNT)> ComponentBaseTypeNames;
}

// ------------------------------------------- SUBTYPES START HERE --------------------------------------------------
// WARNING A single enumerator must not have more than 64 values and the values be sequential. Reordering the said
// values will break the Projects that use the current order.
//
// Each and every Component listed in these enums should have a corresponding class deriving from Component and a 
// corresponding localization handle stored in ComponentNames

enum class GraphicsComponent : std::uint32_t {
    Mesh             = 0,
    SkeletalMesh     = 1,
    DirectionalLight = 2,
    PointLight       = 3,
    SpotLight        = 4,
    ParticleSystem   = 5,
    Camera           = 6,
    COUNT
};

static_assert(static_cast<std::size_t>(GraphicsComponent::COUNT) < 64, "You cannot have more than 64 GraphicsComponents");

enum class PhysicsComponent : std::uint32_t {
    Collider = 0,
    COUNT
};

static_assert(static_cast<std::size_t>(PhysicsComponent::COUNT) < 64, "You cannot have more than 64 PhysicsComponents");

namespace con {
extern const std::array<std::vector<hash32_t>, static_cast<std::size_t>(ComponentBaseType::COUNT)> ComponentNames;
}

}

#endif //COMPONENT_TYPE_HPP
