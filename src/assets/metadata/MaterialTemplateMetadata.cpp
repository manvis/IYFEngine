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
static const char* SUPPORTED_MACROS_FIELD_NAME = "supportedMacros";
static const char* SUPPORTED_VALUES_FIELD_NAME = "supportedValues";
static const char* MACRO_ID_FIELD_NAME = "macroID";
static const char* MACRO_DEFAULT_VALUE_FIELD_NAME = "macroDefault";

void MaterialTemplateMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt32(static_cast<std::uint32_t>(family));
    fw.writeUInt64(materialFamilyHash);
    fw.writeUInt64(macroComboHash);
    
    fw.writeUInt64(supportedMacrosAndValues.size());
    for (const auto& v : supportedMacrosAndValues) {
        fw.writeUInt16(static_cast<std::uint16_t>(v.first));
        
        SerializeMacroValue(fw, v.second.defaultValue);
        
        fw.writeUInt16(v.second.supportedValues.size());
        for (const auto& sv : v.second.supportedValues) {
            SerializeMacroValue(fw, sv);
        }
    }
}

void MaterialTemplateMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(fr.readUInt32());
    materialFamilyHash = FileHash(fr.readUInt64());
    macroComboHash = StringHash(fr.readUInt64());
    
    std::uint64_t supportedValueCount = fr.readUInt64();
    supportedMacrosAndValues.clear();
    supportedMacrosAndValues.reserve(supportedValueCount);
    
    for (std::uint64_t i = 0; i < supportedValueCount; ++i) {
        const ShaderMacro macro = static_cast<ShaderMacro>(fr.readUInt16());
        
        auto insertionResult = supportedMacrosAndValues.emplace(macro, SupportedMacroValues());
        assert(insertionResult.second);
        
        SupportedMacroValues& values = insertionResult.first->second;
        values.defaultValue = DeserializeMacroValue(fr);
        
        std::uint16_t valueCount = fr.readUInt16();
        values.supportedValues.reserve(valueCount);
        for (std::size_t v = 0; v < valueCount; ++v) {
            values.supportedValues.emplace_back(DeserializeMacroValue(fr));
        }
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
    
    pw.String(SUPPORTED_MACROS_FIELD_NAME);
    pw.StartArray();
    
    for (const auto& v : supportedMacrosAndValues) {
        pw.StartObject();
        
        pw.String(MACRO_ID_FIELD_NAME);
        pw.Uint64(static_cast<std::uint64_t>(v.first));
        
        pw.String(MACRO_DEFAULT_VALUE_FIELD_NAME);
        SerializeMacroValueJSON(pw, v.second.defaultValue);
        
        pw.String(SUPPORTED_VALUES_FIELD_NAME);
        pw.StartArray();
        
        for (const auto& sv : v.second.supportedValues) {
            SerializeMacroValueJSON(pw, sv);
        }
        
        pw.EndArray();
        
        pw.EndObject();
    }
    pw.EndArray();
}

void MaterialTemplateMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(jo[MATERIAL_FAMILY_FIELD_NAME].GetUint());
    materialFamilyHash = FileHash(jo[MATERIAL_FAMILY_HASH_FIELD_NAME].GetUint64());
    macroComboHash = StringHash(jo[MACRO_COMBO_HASH_FIELD_NAME].GetUint64());
    
    supportedMacrosAndValues.clear();
    
    rj::GenericArray macroArray = jo[SUPPORTED_MACROS_FIELD_NAME].GetArray();
    supportedMacrosAndValues.reserve(macroArray.Size());
    
    for (const auto& v : macroArray) {
        const ShaderMacro macro = static_cast<ShaderMacro>(v[MACRO_ID_FIELD_NAME].GetUint64());
        
        auto insertionResult = supportedMacrosAndValues.emplace(macro, SupportedMacroValues());
        assert(insertionResult.second);
        
        SupportedMacroValues& values = insertionResult.first->second;
        values.defaultValue = DeserializeMacroValueJSON(v[MACRO_DEFAULT_VALUE_FIELD_NAME]);
        
        rj::GenericArray valueArray = v[SUPPORTED_VALUES_FIELD_NAME].GetArray();
        std::uint16_t valueCount = valueArray.Size();
        
        values.supportedValues.reserve(valueCount);
        for (const auto& sv : valueArray) {
            values.supportedValues.emplace_back(DeserializeMacroValueJSON(sv));
        }
    }
}

void MaterialTemplateMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
