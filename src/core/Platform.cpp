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

#include "core/Platform.hpp"

namespace iyf {
PlatformInfo::PlatformInfo(std::string name, CompressionFormatFamily preferredTextureFormat, int normalTextureChannelCount) 
    : name(std::move(name)), preferredTextureFormat(preferredTextureFormat), normalTextureChannelCount(normalTextureChannelCount) {}
namespace con {
static const std::array<std::string, static_cast<std::size_t>(PlatformIdentifier::COUNT)> PlatformIdentifierToNameMap = {
    "Linux_Desktop_x86_64",       // 0
    "Windows_Desktop_x86_64",     // 1
    "Android_Handheld_ARM_v7a",   // 2
    "Android_Handheld_ARM64_v8a", // 3
    "Android_Handheld_x86",       // 4
    "Android_Handheld_x86_64",    // 5
    
};

const std::string& PlatformIdentifierToName(PlatformIdentifier platformID) {
    return PlatformIdentifierToNameMap[static_cast<std::size_t>(platformID)];
}

static const std::array<PlatformInfo, static_cast<std::size_t>(PlatformIdentifier::COUNT)> PlatformIdentifierToInfoMap = {
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Linux_Desktop_x86_64), CompressionFormatFamily::BC, 2),        // 0
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Windows_Desktop_x86_64), CompressionFormatFamily::BC, 2),      // 1
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Android_Handheld_ARM_v7a), CompressionFormatFamily::ETC, 3),   // 2
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Android_Handheld_ARM64_v8a), CompressionFormatFamily::ETC, 3), // 3
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Android_Handheld_x86), CompressionFormatFamily::ETC, 3),       // 4
    PlatformInfo(PlatformIdentifierToName(PlatformIdentifier::Android_Handheld_x86_64), CompressionFormatFamily::ETC, 3),    // 5
};

const PlatformInfo& PlatformIdentifierToInfo(PlatformIdentifier platformID) {
    return PlatformIdentifierToInfoMap[static_cast<std::size_t>(platformID)];
}

PlatformIdentifier GetCurrentPlatform() {
#if defined(__ANDROID__)
    #error "Not implemented yet"
#elif defined(__linux__)
    #if defined(__x86_64__)
        return PlatformIdentifier::Linux_Desktop_x86_64;
    #else
        #error "Unknown or unsupported CPU architecture for Linux"
    #endif 
#elif defined(_WIN64)
    return PlatformIdentifier::Windows_Desktop_x86_64;
#endif
}
}
}
