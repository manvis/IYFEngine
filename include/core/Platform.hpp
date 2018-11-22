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

#ifndef IYF_PLATFORM_HPP
#define IYF_PLATFORM_HPP

#include "core/Constants.hpp"

#include <string>
#include <array>

namespace iyf {

/// \brief A list of all platforms that the Engine can run on.
/// 
/// \warning Always use explicit and unchanging platform IDs because they are written to many important files.
/// 
/// \warning Update PlatformIdentifierToName when changing this
enum class PlatformIdentifier : std::uint32_t {
    Linux_Desktop_x86_64 = 0,
    Windows_Desktop_x86_64 = 1,
    Android_Handheld_ARM_v7a = 2,
    Android_Handheld_ARM64_v8a = 3,
    Android_Handheld_x86 = 4,
    Android_Handheld_x86_64 = 5,
    COUNT
};

struct PlatformInfo {
    PlatformInfo(std::string name, CompressionFormatFamily preferredTextureFormat, int normalTextureChannelCount);
    
    std::string name;
    CompressionFormatFamily preferredTextureFormat;
    int normalTextureChannelCount;
};

namespace con {
// -----------------------------------------------------------------------------
// Platform data mappings
// -----------------------------------------------------------------------------

/// \return an English name of the platform that can be used in directory creation, lookup and as a translation string
extern const std::string& PlatformIdentifierToName(PlatformIdentifier platformID);

extern const PlatformInfo& PlatformIdentifierToInfo(PlatformIdentifier platformID);

extern PlatformIdentifier GetCurrentPlatform();
}

}

#endif // IYF_PLATFORM_HPP
