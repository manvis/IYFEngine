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

#include "assetImport/converterStates/MeshConverterState.hpp"

namespace iyf::editor {
static const char* USE_32_BIT_INDICES_FIELD_NAME = "use32BitIndices";
static const char* CONVERT_ANIMATIONS_FIELD_NAME = "exportAnimations";
static const char* MESH_SCALE_FIELD_NAME = "scale";

std::uint64_t MeshConverterState::getLatestSerializedDataVersion() const {
    return 1;
}

void MeshConverterState::serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const {
    assert(version == 1);
    
    pw.Key(USE_32_BIT_INDICES_FIELD_NAME);
    pw.Bool(use32bitIndices);
    
    pw.Key(CONVERT_ANIMATIONS_FIELD_NAME);
    pw.Bool(convertAnimations);
    
    pw.Key(MESH_SCALE_FIELD_NAME);
    pw.Double(scale);
    
    // TODO serialize the not yet created fields
    // if (version == 2) {}
}

void MeshConverterState::deserializeJSONImpl(JSONObject& jo, std::uint64_t version) {
    assert(version == 1);
    
    use32bitIndices = jo[USE_32_BIT_INDICES_FIELD_NAME].GetBool();
    convertAnimations = jo[CONVERT_ANIMATIONS_FIELD_NAME].GetBool();
    scale = jo[MESH_SCALE_FIELD_NAME].GetFloat();
    
    // TODO deserialize the not yet created fields
    // if (version == 2) {}
}
}

