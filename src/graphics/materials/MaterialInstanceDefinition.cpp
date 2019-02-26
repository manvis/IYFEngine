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

#include "graphics/materials/MaterialInstanceDefinition.hpp"
#include "core/filesystem/File.hpp"
#include "utilities/hashing/HashCombine.hpp"

#include <cassert>

namespace iyf {
namespace con {
const std::string& GetMaterialRenderModeName(MaterialRenderMode renderMode) {
    static const std::array<std::string, static_cast<std::size_t>(MaterialRenderMode::COUNT)> MaterialRenderModeNames = {
        "opaqueRenderMode",      // 0
        "transparentRenderMode", // 1
    };
    
    return MaterialRenderModeNames[static_cast<std::size_t>(renderMode)];
}

const LocalizationHandle& GetMaterialRenderModeLocalizationHandle(MaterialRenderMode renderMode) {
    static const std::array<LocalizationHandle, static_cast<std::size_t>(MaterialRenderMode::COUNT)> MaterialRenderModeHandles = {
        LH(GetMaterialRenderModeName(MaterialRenderMode::Opaque).c_str()),      // 0
        LH(GetMaterialRenderModeName(MaterialRenderMode::Transparent).c_str()), // 1
    };
    
    return MaterialRenderModeHandles[static_cast<std::size_t>(renderMode)];
}
}

void MaterialInstanceDefinition::serialize(Serializer& fw) const {
    fw.writeUInt64(materialTemplateDefinition);
    fw.writeUInt8(static_cast<std::uint8_t>(renderMode));
    
    fw.writeUInt16(variables.size());
    for (const auto& v : variables) {
        fw.writeUInt64(v.first);
        
        fw.writeFloat(v.second.x);
        fw.writeFloat(v.second.y);
        fw.writeFloat(v.second.z);
        fw.writeFloat(v.second.w);
    }
    
    fw.writeUInt16(textures.size());
    for (const auto& t : textures) {
        fw.writeUInt64(t.first);
        fw.writeUInt64(t.second);
    }
}

void MaterialInstanceDefinition::deserialize(Serializer& fr) {
    materialTemplateDefinition = StringHash(fr.readUInt64());
    renderMode = static_cast<MaterialRenderMode>(fr.readUInt8());
    
    const std::size_t variableCount = fr.readUInt16();
    variables.clear();
    variables.reserve(variableCount);
    
    for (std::size_t i = 0; i < variableCount; ++i) {
        const StringHash variableNameHash(fr.readUInt64());
        
        glm::vec4 value;
        value.x = fr.readFloat();
        value.y = fr.readFloat();
        value.z = fr.readFloat();
        value.w = fr.readFloat();
        
        variables.emplace_back(variableNameHash, value);
    }
    
    
    const std::size_t textureCount = fr.readUInt16();
    textures.clear();
    textures.reserve(textureCount);
    
    for (std::size_t i = 0; i < textureCount; ++i) {
        const StringHash textureVariableNameHash(fr.readUInt64());
        const StringHash textureNameHash(fr.readUInt64());
        
        textures.emplace_back(textureVariableNameHash, textureNameHash);
    }
}

constexpr const char* MATERIAL_TEMPLATE_DEFINITION_FIELD_NAME = "materialTemplate";
constexpr const char* RENDER_MODE_FIELD_NAME = "renderMode";
constexpr const char* VARIABLES_FIELD_NAME = "variables";
constexpr const char* TEXTURES_FIELD_NAME = "textures";
constexpr const char* NAME_HASH_FIELD_NAME = "nameHash";
constexpr const char* VALUE_FIELD_NAME = "value";
constexpr const char* X_FIELD_NAME = "x";
constexpr const char* Y_FIELD_NAME = "y";
constexpr const char* Z_FIELD_NAME = "z";
constexpr const char* W_FIELD_NAME = "w";

void MaterialInstanceDefinition::serializeJSON(PrettyStringWriter& pw) const {
    pw.StartObject();
    
    pw.String(MATERIAL_TEMPLATE_DEFINITION_FIELD_NAME);
    pw.Uint64(materialTemplateDefinition);
    
    pw.String(RENDER_MODE_FIELD_NAME);
    pw.Uint(static_cast<unsigned>(renderMode));
    
    pw.String(VARIABLES_FIELD_NAME);
    pw.StartArray();
    for (const auto& v : variables) {
        pw.StartObject();
        
        pw.String(NAME_HASH_FIELD_NAME);
        pw.Uint64(v.first);
        
        pw.String(VALUE_FIELD_NAME);
        pw.StartObject();
        
        pw.String(X_FIELD_NAME);
        pw.Double(v.second.x);
        pw.String(Y_FIELD_NAME);
        pw.Double(v.second.y);
        pw.String(Z_FIELD_NAME);
        pw.Double(v.second.z);
        pw.String(W_FIELD_NAME);
        pw.Double(v.second.w);
        pw.EndObject();
        
        pw.EndObject();
    }
    pw.EndArray();
    
    
    pw.String(TEXTURES_FIELD_NAME);
    pw.StartArray();
    for (const auto& t : textures) {
        pw.StartObject();
        
        pw.String(NAME_HASH_FIELD_NAME);
        pw.Uint64(t.first);
        
        pw.String(VALUE_FIELD_NAME);
        pw.Uint64(t.second);
        
        
        pw.EndObject();
    }
    pw.EndArray();
    
    pw.EndObject();
}

void MaterialInstanceDefinition::deserializeJSON(JSONObject& jo) {
    materialTemplateDefinition = StringHash(jo[MATERIAL_TEMPLATE_DEFINITION_FIELD_NAME].GetUint64());
    renderMode = static_cast<MaterialRenderMode>(jo[RENDER_MODE_FIELD_NAME].GetUint());
    
    rj::GenericArray varArray = jo[VARIABLES_FIELD_NAME].GetArray();
    variables.clear();
    variables.reserve(varArray.Size());
    
    for (const auto& v : varArray) {
        StringHash nameHash(v[NAME_HASH_FIELD_NAME].GetUint64());
        glm::vec4 defaultValue;
        defaultValue.x = v[VALUE_FIELD_NAME][X_FIELD_NAME].GetFloat();
        defaultValue.y = v[VALUE_FIELD_NAME][Y_FIELD_NAME].GetFloat();
        defaultValue.z = v[VALUE_FIELD_NAME][Z_FIELD_NAME].GetFloat();
        defaultValue.w = v[VALUE_FIELD_NAME][W_FIELD_NAME].GetFloat();
        
        variables.emplace_back(nameHash, defaultValue);
    }
    
    rj::GenericArray texArray = jo[TEXTURES_FIELD_NAME].GetArray();
    textures.clear();
    textures.reserve(texArray.Size());
    
    for (const auto& t : texArray) {
        StringHash nameHash(t[NAME_HASH_FIELD_NAME].GetUint64());
        StringHash textureNameHash(t[VALUE_FIELD_NAME].GetUint64());
        
        textures.emplace_back(nameHash, textureNameHash);
    }
}

}
