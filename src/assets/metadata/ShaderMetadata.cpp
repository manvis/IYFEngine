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

#include "assets/metadata/ShaderMetadata.hpp"
#include <stdexcept>

namespace iyf {
static const char* PURPOSE_FIELD_NAME = "purpose";
static const char* STAGE_FLAGS_FIELD_NAME = "stageFlags";

std::uint16_t ShaderMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

void ShaderMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeUInt32(std::uint32_t(stage));
    fw.writeUInt8(static_cast<std::uint8_t>(purpose));
}

void ShaderMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    stage = ShaderStageFlags(static_cast<ShaderStageFlagBits>(fr.readUInt32()));
    purpose = static_cast<ShaderPurpose>(fr.readUInt8());
}

void ShaderMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.String(PURPOSE_FIELD_NAME);
    pw.Uint(static_cast<std::uint8_t>(purpose));
    
    pw.String(STAGE_FLAGS_FIELD_NAME);
    pw.Uint(std::uint32_t(stage));
}

void ShaderMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    purpose = static_cast<ShaderPurpose>(jo[PURPOSE_FIELD_NAME].GetUint());
    stage = ShaderStageFlags(static_cast<ShaderStageFlagBits>(jo[STAGE_FLAGS_FIELD_NAME].GetUint()));
}

void ShaderMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
