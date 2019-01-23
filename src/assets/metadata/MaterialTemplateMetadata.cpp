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

#include "assets/metadata/MaterialTemplateMetadata.hpp"
#include "core/Logger.hpp"

namespace iyf {
inline void SerializeMacroValue(Serializer& fw, const ShaderMacroValue& sv) {
    std::visit([&fw](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            throw std::runtime_error("Macros without parameters are not supported yet");
        } else if constexpr (std::is_same_v<T, double>) {
            fw.writeUInt8(1);
            fw.writeDouble(arg);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            fw.writeUInt8(2);
            fw.writeInt64(arg);
        } else {
            static_assert(util::always_false_type<T>::value, "Unhandled value");
        }
    }, sv);
}

inline ShaderMacroValue DeserializeMacroValue(Serializer& fr) {
    switch (fr.readUInt8()) {
        case 1:
            return fr.readDouble();
        case 2:
            return fr.readInt64();
        default:
            throw std::runtime_error("Unhandled macro value type");
    }
}

inline void SerializeMacroValueJSON(PrettyStringWriter& pw, const ShaderMacroValue& sv) {
    std::visit([&pw](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            throw std::runtime_error("Macros without parameters are not supported yet");
        } else if constexpr (std::is_same_v<T, double>) {
            pw.Double(arg);
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            pw.Int64(arg);
        } else {
            static_assert(util::always_false_type<T>::value, "Unhandled value");
        }
    }, sv);
}

inline ShaderMacroValue DeserializeMacroValueJSON(const JSONObject& jo) {
    if (jo.IsInt64()) {
        return jo.GetInt64();
    } else if (jo.IsDouble()) {
        return jo.GetDouble();
    } else {
        throw std::logic_error("A macro value must be a double or int");
    }
}

std::uint16_t MaterialTemplateMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

static const char* MATERIAL_FAMILY_FIELD_NAME = "materialFamily";
static const char* MATERIAL_FAMILY_HASH_FIELD_NAME = "materialFamilyHash";
static const char* MACRO_COMBO_HASH_FIELD_NAME = "macroComboHash";
static const char* SUPPORTED_VERTEX_DATA_LAYOUTS_FIELD_NAME = "supportedVertexLayouts";
static const char* REQUIRED_TEXTURES_FIELD_NAME = "requiredTextures";
static const char* REQUIRED_VARIABLES_FIELD_NAME = "requiredVariables";
static const char* NAME_FIELD_NAME = "name";
static const char* X_FIELD_NAME = "x";
static const char* Y_FIELD_NAME = "y";
static const char* Z_FIELD_NAME = "z";
static const char* W_FIELD_NAME = "w";
static const char* COMPONENT_COUNT_FIELD_NAME = "componentCount";
static const char* DEFAULT_VALUE_FIELD_NAME = "defaultValue";

void MaterialTemplateMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt32(static_cast<std::uint32_t>(family));
    fw.writeUInt64(materialFamilyHash);
    fw.writeUInt64(macroComboHash);
    fw.writeUInt64(vertexDataLayouts.to_ullong());
    
    fw.writeUInt16(requiredVariables.size());
    for (MaterialInputVariable miv : requiredVariables) {
        assert(miv.isValid());
        fw.writeString(miv.getName(), StringLengthIndicator::UInt8);
        fw.writeUInt8(miv.getComponentCount());
        
        fw.writeFloat(miv.getDefaultValue().x);
        fw.writeFloat(miv.getDefaultValue().y);
        fw.writeFloat(miv.getDefaultValue().z);
        fw.writeFloat(miv.getDefaultValue().w);
    }
    
    fw.writeUInt16(requiredTextures.size());
    for (MaterialInputTexture mit : requiredTextures) {
        assert(mit.isValid());
        fw.writeString(mit.getName(), StringLengthIndicator::UInt8);
        fw.writeUInt64(mit.getDefaultTexture());
    }
}

void MaterialTemplateMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(fr.readUInt32());
    materialFamilyHash = FileHash(fr.readUInt64());
    macroComboHash = StringHash(fr.readUInt64());
    vertexDataLayouts = std::bitset<64>(fr.readUInt64());
    
    const std::size_t inputVariableCount = fr.readUInt16();
    requiredVariables.clear();
    requiredVariables.reserve(inputVariableCount);
    
    for (std::size_t i = 0; i < inputVariableCount; ++i) {
        std::string name;
        fr.readString(name, StringLengthIndicator::UInt8);
        const std::uint8_t componentCount = fr.readUInt8();
        
        glm::vec4 defaultValue;
        defaultValue.x = fr.readFloat();
        defaultValue.y = fr.readFloat();
        defaultValue.z = fr.readFloat();
        defaultValue.w = fr.readFloat();
        
        requiredVariables.emplace_back(name, defaultValue, componentCount);
    }
    
    
    const std::size_t inputTextureCount = fr.readUInt16();
    requiredTextures.clear();
    requiredTextures.reserve(inputTextureCount);
    
    for (std::size_t i = 0; i < inputTextureCount; ++i) {
        std::string name;
        fr.readString(name, StringLengthIndicator::UInt8);
        const StringHash defaultTexture(fr.readUInt64());
        
        requiredTextures.emplace_back(name, defaultTexture);
    }
}

void MaterialTemplateMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.String(MATERIAL_FAMILY_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(family));
    
    pw.String(MATERIAL_FAMILY_HASH_FIELD_NAME);
    pw.Uint64(materialFamilyHash);
    
    pw.String(MACRO_COMBO_HASH_FIELD_NAME);
    pw.Uint64(macroComboHash);
    
    pw.String(SUPPORTED_VERTEX_DATA_LAYOUTS_FIELD_NAME);
    pw.Uint64(vertexDataLayouts.to_ullong());
    
    pw.String(REQUIRED_VARIABLES_FIELD_NAME);
    pw.StartArray();
    for (const auto& v : requiredVariables) {
        pw.StartObject();
        
        pw.String(NAME_FIELD_NAME);
        pw.String(v.getName().data(), v.getName().size(), true);
        
        pw.String(COMPONENT_COUNT_FIELD_NAME);
        pw.Uint(v.getComponentCount());
        
        pw.String(DEFAULT_VALUE_FIELD_NAME);
        pw.StartObject();
        pw.String(X_FIELD_NAME);
        pw.Double(v.getDefaultValue().x);
        
        pw.String(Y_FIELD_NAME);
        pw.Double(v.getDefaultValue().y);
        
        pw.String(Z_FIELD_NAME);
        pw.Double(v.getDefaultValue().z);
        
        pw.String(W_FIELD_NAME);
        pw.Double(v.getDefaultValue().w);
        pw.EndObject();
        
        pw.EndObject();
    }
    pw.EndArray();
    
    pw.String(REQUIRED_TEXTURES_FIELD_NAME);
    pw.StartArray();
    for (const auto& t : requiredTextures) {
        pw.StartObject();
        
        pw.String(NAME_FIELD_NAME);
        pw.String(t.getName().data(), t.getName().size(), true);
        
        pw.String(DEFAULT_VALUE_FIELD_NAME);
        pw.Uint64(t.getDefaultTexture());
        
        pw.EndObject();
    }
    pw.EndArray();
}

void MaterialTemplateMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(jo[MATERIAL_FAMILY_FIELD_NAME].GetUint());
    materialFamilyHash = FileHash(jo[MATERIAL_FAMILY_HASH_FIELD_NAME].GetUint64());
    macroComboHash = StringHash(jo[MACRO_COMBO_HASH_FIELD_NAME].GetUint64());
    vertexDataLayouts = std::bitset<64>(jo[SUPPORTED_VERTEX_DATA_LAYOUTS_FIELD_NAME].GetUint64());
    
    const auto inputVariableArray = jo[REQUIRED_VARIABLES_FIELD_NAME].GetArray();
    const std::size_t inputVariableCount = inputVariableArray.Size();
    requiredVariables.clear();
    requiredVariables.reserve(inputVariableCount);
    
    for (const auto& e : inputVariableArray) {
        glm::vec4 defaultValue;
        defaultValue.x = e[DEFAULT_VALUE_FIELD_NAME][X_FIELD_NAME].GetFloat();
        defaultValue.y = e[DEFAULT_VALUE_FIELD_NAME][Y_FIELD_NAME].GetFloat();
        defaultValue.z = e[DEFAULT_VALUE_FIELD_NAME][Z_FIELD_NAME].GetFloat();
        defaultValue.w = e[DEFAULT_VALUE_FIELD_NAME][W_FIELD_NAME].GetFloat();
        
        requiredVariables.emplace_back(e[NAME_FIELD_NAME].GetString(), defaultValue, e[COMPONENT_COUNT_FIELD_NAME].GetUint());
    }
    
    const auto inputTextureArray = jo[REQUIRED_TEXTURES_FIELD_NAME].GetArray();
    const std::size_t inputTextureCount = inputTextureArray.Size();
    requiredTextures.clear();
    requiredTextures.reserve(inputTextureCount);
    
    for (const auto& e : inputTextureArray) {
        requiredTextures.emplace_back(e[NAME_FIELD_NAME].GetString(), StringHash(e[DEFAULT_VALUE_FIELD_NAME].GetUint64()));
    }
}

void MaterialTemplateMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
