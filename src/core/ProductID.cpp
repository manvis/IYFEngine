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

#include "core/ProductID.hpp"
#include <fstream>
#include <array>
#include <stdexcept>

#include "SDL_endian.h"

namespace iyf {
const int MAX_STR_LENGTH = 64;
std::array<char, 4> MAGIC = {'I', 'Y', 'F', 'I'};

ProductID::ProductID(const std::string& companyName, const std::string& gameName, const std::uint32_t version) : companyName(companyName), gameName(gameName), version(version) {
    if (companyName.size() > 64 || gameName.size() > 64) {
        throw std::runtime_error("Strings passed to ProductID are too long.");
    }
}

bool ProductID::serialize(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }
    
    out.write(MAGIC.data(), 4);
    
    std::uint8_t size = companyName.size();
    out.write(reinterpret_cast<char*>(&size), sizeof(size));
    out.write(companyName.data(), companyName.size());
    
    size = gameName.size();
    out.write(reinterpret_cast<char*>(&size), sizeof(size));
    out.write(gameName.data(), gameName.size());
    
    std::uint32_t temp = version;
    
    // File stores version in a little endian format
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        temp = SDL_Swap32(temp);
    }
    
    out.write(reinterpret_cast<char*>(&temp), sizeof(temp));
    
    return true;
}

bool ProductID::deserialize(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    
    std::array<char, 4> magicTest;
    in.read(magicTest.data(), 4);
    
    if (magicTest != MAGIC) {
        return false;
    }
    
    std::uint8_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));

    std::array<char, MAX_STR_LENGTH + 1> temp;
    in.read(temp.data(), size);
    temp[size] = '\0';
    
    companyName = temp.data();
    
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    in.read(temp.data(), size);
    temp[size] = '\0';
    
    gameName = temp.data();
    
    std::uint32_t tempVersion;
    in.read(reinterpret_cast<char*>(&tempVersion), sizeof(tempVersion));
    
    // File stores version in a little endian format
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        tempVersion = SDL_Swap32(tempVersion);
    }
    
    version = tempVersion;
    
    return true;
}
}
