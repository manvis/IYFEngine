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
#include "graphics/RendererConstants.hpp"
#include "graphics/VertexDataLayouts.hpp"

#include <stdexcept>
#include <functional>
#include <type_traits>

namespace iyf {
const char* GetShaderMacroName(ShaderMacro macro) {
    switch (macro) {
//         case ShaderMacro:::
//             return;
        case ShaderMacro::VertexDataLayout:
            return "VERTEX_DATA_LAYOUT";
        case ShaderMacro::NormalSetByMaterialGraph:
            return "NORMAL_SET_BY_MATERIAL_GRAPH";
        case ShaderMacro::NormalMappingMode:
            return "NORMAL_MAPPING_MODE";
        case ShaderMacro::NormalTextureChannelCount:
            return "NORMAL_MAP_TEXTURE_COUNT";
        case ShaderMacro::WorldSpacePositionAvailable:
            return "WORLD_SPACE_POSITION_AVAILABLE";
        case ShaderMacro::NormalAvailable:
            return "NORMAL_AVAILABLE";
        case ShaderMacro::VertexColorAvailable:
            return "VERTEX_COLOR_AVAILABLE";
        case ShaderMacro::TextureCoordinatesAvailable:
            return "TEXTURE_COORDINATES_AVAILABLE";
        case ShaderMacro::Renderer:
            return "RENDERER";
        case ShaderMacro::ShadowMode:
            return "SHADOW_MODE";
        case ShaderMacro::FogMode:
            return "FOG_MODE";
        case ShaderMacro::Custom:
            throw std::logic_error("Custom or COUNT must not be used in GetShaderMacroName()");
    }
    
    throw std::logic_error("Invalid macro ID");
}

const char* GetShaderIncludeName(ShaderInclude include) {
    /// WARNING: must not be longer than 64 bytes (including null terminator)
    switch (include) {
        case ShaderInclude::CommonHelpers:
            return "common_helpers.incl";
        case ShaderInclude::VertexShaderHelpers:
            return "vertex_helpers.incl";
        case ShaderInclude::FragmentShaderHelpers:
            return "fragment_helpers.incl";
    }
    
    throw std::logic_error("Invalid shader include ID");
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

bool ValidateShaderMacroValue(ShaderMacro macro, const ShaderMacroValue& value) {
    static const std::function<bool(const std::monostate&)> EmptyValidator = [](const std::monostate&){return true;};

    // Always returns false. Used for deprecated macros that must not be used
    //static const std::function<bool(const std::monostate&)> FailValidator = [](const std::monostate&){return false;};

    switch (macro) {
        case ShaderMacro::VertexDataLayout: // Need a valid vertex data layout here
            static_assert(static_cast<std::size_t>(VertexDataLayout::MeshVertex) == 0);
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(VertexDataLayout::MeshVertex, VertexDataLayout::COUNT, false));
        case ShaderMacro::NormalSetByMaterialGraph: // Must be defined or not.
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::NormalMappingMode: // Must be equal to 0, 1 or 2.
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(0, 2));
        case ShaderMacro::NormalTextureChannelCount:
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator(2, 3));
        case ShaderMacro::WorldSpacePositionAvailable:
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::NormalAvailable:
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::VertexColorAvailable: // Must be defined or not.
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::TextureCoordinatesAvailable: // Must be defined or not.
            return ValidateValueImpl<std::monostate>(value, EmptyValidator);
        case ShaderMacro::Renderer:
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator<std::int64_t>(0, static_cast<std::int64_t>(RendererType::COUNT), false));
        case ShaderMacro::ShadowMode:
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator<std::int64_t>(0, static_cast<std::int64_t>(ShadowMode::COUNT), false));
        case ShaderMacro::FogMode:
            return ValidateValueImpl<std::int64_t>(value, MakeIntervalValidator<std::int64_t>(0, static_cast<std::int64_t>(FogMode::COUNT), false));
        case ShaderMacro::Custom:
            throw std::logic_error("Custom or COUNT must not be used in GetShaderMacroName()");
    }
    
    throw std::logic_error("Invalid macro ID");
}

ShaderMacroWithValue::ShaderMacroWithValue(ShaderMacro macro, ShaderMacroValue value) : macro(macro), name(GetShaderMacroName(macro)), value(value) {
    if (macro >= ShaderMacro::Custom) {
        throw std::logic_error("Invalid shader macro value.");
    }
    
    if (!ValidateShaderMacroValue(macro, value)) {
        throw std::invalid_argument("Invalid shader macro value.");
    }
    
    nameLength = std::strlen(name);
}

ShaderMacroWithValue::ShaderMacroWithValue(const std::string& nameString, ShaderMacroValue value) : ShaderMacroWithValue(nameString.c_str(), nameString.length(), value) {}

ShaderMacroWithValue::ShaderMacroWithValue(const char* nameString, std::size_t length, ShaderMacroValue value) : macro(ShaderMacro::Custom), value(value) {
    char* newName = new char[length + 1];
    
    std::strncpy(newName, nameString, length);
    newName[length] = '\0';
    
    name = newName;
}

ShaderMacroWithValue::~ShaderMacroWithValue() {
    if (macro == ShaderMacro::Custom) {
        delete[] name;
    }
}

}
