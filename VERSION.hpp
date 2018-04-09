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

#ifndef IYF_ENGINE_VERSION_HPP
#define IYF_ENGINE_VERSION_HPP

#include <cstdint>

namespace iyf {

class Version {
public:
    constexpr Version() : version(0) {}
    
    constexpr Version (std::uint16_t major, std::uint16_t minor, std::uint16_t patch) : version(((major) << 22) | ((minor) << 12) | (patch)) {
        if (patch > 4095) {
            throw std::invalid_argument("Patch version number can't be bigger than 4095");
        }
        
        if (minor > 1023) {
            throw std::invalid_argument("Minor version number can't be bigger than 1023");
        }
        
        if (major > 1023) {
            throw std::invalid_argument("Major version number can't be bigger than 1023");
        }
    }
    
    constexpr std::uint16_t getPatch() const {
        return version & 0xFFF;
    }
    
    constexpr std::uint16_t getMinor() const {
        return (version >> 12) & 0x3FF;
    }
    
    constexpr std::uint16_t getMajor() const {
        return (version >> 22) & 0x3FF;
    }
    
    constexpr std::uint32_t getPackedVersion() const {
        return version;
    }
private:
    std::uint32_t version;
};

namespace con {

constexpr std::uint16_t EngineVersionMajor = 0;
constexpr std::uint16_t EngineVersionMinor = 3;
constexpr std::uint16_t EngineVersionPatch = 14;

constexpr std::uint16_t EditorVersionMajor = 0;
constexpr std::uint16_t EditorVersionMinor = 0;
constexpr std::uint16_t EditorVersionPatch = 25;

constexpr Version EngineVersion(EngineVersionMajor, EngineVersionMinor, EngineVersionPatch);
constexpr Version EditorVersion(EditorVersionMajor, EditorVersionMinor, EditorVersionPatch);

}

}

#endif // IYF_ENGINE_VERSION_HPP
