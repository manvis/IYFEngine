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

#ifndef INTEGERPACKING_HPP
#define INTEGERPACKING_HPP

#include <array>

namespace iyf {
namespace util {
inline constexpr std::uint32_t BytesToInt32(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) {
    return (std::uint32_t(a) << 24 | std::uint32_t(b) << 16 | std::uint32_t(c) << 8 | d);
}

inline constexpr void Int32ToBytes(std::uint32_t in, std::uint8_t& a, std::uint8_t& b, std::uint8_t& c, std::uint8_t& d) {
    a = (in >> 24) & 0xFF;
    b = (in >> 16) & 0xFF;
    c = (in >>  8) & 0xFF;
    d =  in        & 0xFF;
}

inline std::uint32_t BytesToInt32(std::array<std::uint8_t, 4> in) {
    return BytesToInt32(in[0], in[1], in[2], in[3]);
}

inline void Int32ToBytes(std::uint32_t in, std::array<std::uint8_t, 4>& out) {
    out[0] = (in >> 24) & 0xFF;
    out[1] = (in >> 16) & 0xFF;
    out[2] = (in >>  8) & 0xFF;
    out[3] =  in        & 0xFF;
}

inline constexpr std::uint64_t Int32sToInt64(std::uint32_t a, std::uint32_t b) {
    return std::uint64_t(a) << 32 | b;
}

inline constexpr std::uint32_t Int64ToInt32a(std::uint64_t in) {
    return (std::uint32_t)(in >> 32);
}

inline constexpr std::uint32_t Int64ToInt32b(std::uint64_t in) {
    return (std::uint32_t)(in);
}

}
}

#endif /* INTEGERPACKING_HPP */

