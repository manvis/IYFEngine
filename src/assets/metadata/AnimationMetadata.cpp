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

#include "assets/metadata/AnimationMetadata.hpp"
#include "localization/TextLocalization.hpp"
#include "graphics/imgui/imgui.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"

#include <stdexcept>

namespace iyf {
std::uint16_t AnimationMetadata::getLatestSerializedDataVersion() const {
    return 1;
}

void AnimationMetadata::serializeImpl(Serializer& fw, std::uint16_t version) const {
    assert(version == 1);
    
    fw.writeFloat(duration);
    fw.writeFloat(ticksPerSecond);
    fw.writeUInt16(animationFormatVersion);
}

void AnimationMetadata::deserializeImpl(Serializer& fr, std::uint16_t version) {
    assert(version == 1);
    
    duration = fr.readFloat();
    ticksPerSecond = fr.readFloat();
    animationFormatVersion = fr.readUInt16();
}

constexpr const char* DURATION_FIELD_NAME = "duration";
constexpr const char* TICKS_PER_SECOND_FIELD_NAME = "ticksPerSecond";
constexpr const char* FORMAT_VERSION_FIELD_NAME = "formatVersion";

void AnimationMetadata::serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const {
    assert(version == 1);
    
    pw.Key(DURATION_FIELD_NAME);
    pw.Double(duration);
    
    pw.Key(TICKS_PER_SECOND_FIELD_NAME);
    pw.Double(ticksPerSecond);
    
    pw.Key(FORMAT_VERSION_FIELD_NAME);
    pw.Uint(animationFormatVersion);
}

void AnimationMetadata::deserializeJSONImpl(JSONObject& jo, std::uint16_t version) {
    assert(version == 1);
    
    duration = jo[DURATION_FIELD_NAME].GetFloat();
    ticksPerSecond = jo[TICKS_PER_SECOND_FIELD_NAME].GetFloat();
    animationFormatVersion = jo[FORMAT_VERSION_FIELD_NAME].GetUint();
}

void AnimationMetadata::displayInImGui() const {
    throw std::runtime_error("Method not yet implemented");
}
}
