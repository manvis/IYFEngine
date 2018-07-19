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

#include "graphics/ShaderMacros.hpp"

#include "core/Constants.hpp"
#include "graphics/VertexDataLayouts.hpp"

#include <stdexcept>
#include <functional>
#include <type_traits>

namespace iyf {
std::string GetShaderMacroName(ShaderMacro macro) {
    switch (macro) {
//         case ShaderMacro:::
//             return;
        case ShaderMacro::VertexType:
            return "VERTEX_TYPE";
        case ShaderMacro::NormalMapTextureAvailable:
            return "NORMAL_MAP_TEXTURE_AVAILABLE";
        case ShaderMacro::NormalMappingMode:
            return "NORMAL_MAPPING_MODE";
        case ShaderMacro::TextureInputCount:
            return "TEXTURE_INPUT_COUNT";
        case ShaderMacro::Custom:
            throw std::logic_error("Custom or COUNT must not be used in GetShaderMacroName()");
    }
    
    throw std::logic_error("Invalid macro ID");
}

template <typename T>
bool ValidateValueImpl(const ShaderMacroValue& value, std::function<bool(const T&)> validator) {
   if (!std::holds_alternative<T>(value)) {
       return false;
   }
   
   return validator(std::get<T>(value));
}

template<typename T>
std::function<bool(const std::int64_t&)> MakeIntervalValidator(T min, T max, bool maxInclusive = true) {
    if (min > max) {
        throw std::logic_error("min must be < max");
    }
    
    return [min, max, maxInclusive](const std::int64_t& val) {
        const bool aboveMin = val >= static_cast<std::int64_t>(min);
        const bool belowMax = maxInclusive ? (val <= static_cast<std::int64_t>(max)) : (val < static_cast<std::int64_t>(max));
        return aboveMin && belowMax;
    };
}

const std::function<bool(const std::monostate&)> EmptyValidator = [](const std::monostate&){return true;};

bool ValidateShaderMacroValue(ShaderMacro macro, const ShaderMacroValue& value) {
    switch (macro) {
        case ShaderMacro::VertexType: // Need a valid vertex data layout here
            static_assert(static_cast<std::size_t>(VertexDataLayout::MeshVertex) == 0);
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(VertexDataLayout::MeshVertex, VertexDataLayout::COUNT, false));
        case ShaderMacro::NormalMapTextureAvailable: // Must be defined or not.
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::NormalMappingMode: // Must be equal to 0, 1 or 2.
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(0, 2));
        case ShaderMacro::TextureInputCount: {
            const std::int64_t min = 0;
            const std::int64_t max = static_cast<std::int64_t>(con::MaxMaterialTextures);
            
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(min, max));
        }
        case ShaderMacro::Custom:
            throw std::logic_error("Custom or COUNT must not be used in GetShaderMacroName()");
    }
    
    throw std::logic_error("Invalid macro ID");
}

/// Used in std::visit. Based on sample from https://en.cppreference.com/w/cpp/utility/variant/visit
template<class T>
struct AlwaysFalseType : std::false_type {};

inline std::string StringifyShaderMacroValue(const ShaderMacroValue& value) {
    return std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return std::string("");
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(arg);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return arg;
        } else {
            static_assert(AlwaysFalseType<T>::value, "Unhandled value");
        }
    }, value);
}

ShaderMacroWithValue::ShaderMacroWithValue(ShaderMacro macro, ShaderMacroValue value) : macro(macro), name(GetShaderMacroName(macro)) {
    if (macro >= ShaderMacro::Custom) {
        throw std::logic_error("Invalid shader macro value.");
    }
    
    if (!ValidateShaderMacroValue(macro, value)) {
        throw std::invalid_argument("Invalid shader macro value.");
    }
    
    // We have to stringify the value
    this->value = StringifyShaderMacroValue(value);
}

ShaderMacroWithValue::ShaderMacroWithValue(std::string name, ShaderMacroValue value) : macro(ShaderMacro::Custom), name(name){
    this->value = StringifyShaderMacroValue(value);
}
}
