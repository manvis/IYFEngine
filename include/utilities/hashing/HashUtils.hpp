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

#ifndef CONSTEXPR_STRLEN_HPP
#define CONSTEXPR_STRLEN_HPP

#include <cstdint>

#include "utilities/ForceInline.hpp"

namespace iyf {
/// \todo Recursive functions cannot be IYF_FORCE_INLINE'd. I still get the benefit of the constexpr, 
/// however, some hashes have to be computed during runtime and IYF_FORCE_INLINE would help. Do I still 
/// need it to be recursive? it was written for c++11, however, the engine is now c++14 minimum and
/// restrictions to constexpr functions have been considerably relaxed.
constexpr std::size_t constexprStrlen(const char* str) {
    return *str ? 1 + constexprStrlen(str + 1) : 0;
}

/// Used by murmur3 hash function, adapted from the public domain official implementation, available at
/// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
IYF_FORCE_INLINE constexpr std::uint32_t getblock32(const std::uint32_t* p, int i) {
    return p[i];
}

/// Used by murmur3 hash function, adapted from the public domain official implementation, available at
/// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
IYF_FORCE_INLINE constexpr std::uint32_t rotl32(std::uint32_t x, std::int8_t r) {
    return (x << r) | (x >> (32 - r));
}

/// Used by murmur3 hash function, adapted from the public domain official implementation, available at
/// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
IYF_FORCE_INLINE constexpr std::uint32_t fmix32(std::uint32_t h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}
}

#endif //CONSTEXPR_STRLEN_HPP
