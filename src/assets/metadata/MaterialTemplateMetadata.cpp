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

namespace iyf {
std::uint16_t MaterialTemplateMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

static const char* MATERIAL_FAMILY_FIELD_NAME = "materialFamily";
static const char* MATERIAL_FAMILY_HASH_FIELD_NAME = "materialFamilyHash";

void MaterialTemplateMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt32(static_cast<std::uint32_t>(family));
    fw.writeUInt64(materialFamilyHash);
}

void MaterialTemplateMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(fr.readUInt32());
    materialFamilyHash = FileHash(fr.readUInt64());
}

void MaterialTemplateMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.String(MATERIAL_FAMILY_FIELD_NAME);
    pw.Uint(static_cast<std::uint32_t>(family));
    
    pw.String(MATERIAL_FAMILY_HASH_FIELD_NAME);
    pw.Uint64(materialFamilyHash);
}

void MaterialTemplateMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    family = static_cast<MaterialFamily>(jo[MATERIAL_FAMILY_FIELD_NAME].GetUint());
    materialFamilyHash = FileHash(jo[MATERIAL_FAMILY_HASH_FIELD_NAME].GetUint64());
}

void MaterialTemplateMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
