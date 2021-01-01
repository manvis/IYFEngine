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

#ifndef IYF_ENDIANESS_HPP
#define IYF_ENDIANESS_HPP

#include <cstdint>

// TODO use this once C++20 is available #include <bit>

#ifndef __linux__
#include <intrin.h> // Visual Studio 
#endif

namespace iyf::util {

// constexpr bool IsLittleEndian() {
//     if constexpr (std::endian::native == std::endian::little) {
//         return true;
//     } else {
//         return false;
//     }
// }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
constexpr bool IsLittleEndian = true;
#else
constexpr bool IsLittleEndian = false;
#endif

constexpr inline std::uint16_t Swap(std::uint16_t i) {
#ifdef __linux__
    return __builtin_bswap16(i);
#else
    return _byteswap_ushort(i);
#endif
}

constexpr inline std::uint32_t Swap(std::uint32_t i) {
#ifdef __linux__
    return __builtin_bswap32(i);
#else
    return _byteswap_ulong(i);
#endif
}

constexpr inline std::uint64_t Swap(std::uint64_t i) {
#ifdef __linux__
    return __builtin_bswap64(i);
#else
    return _byteswap_uint64(i);
#endif
}

constexpr inline float Swap(float i) {
//     auto iInt = std::bit_cast<std::uint32_t>(i);
// #ifdef __linux__
//     iInt = __builtin_bswap32(iInt);
// #else
//     iInt = _byteswap_ulong(iInt);
// #endif
//     return std::bit_cast<float>(iInt);

    union {
        float f;
        uint32_t i;
    } u = {i};

#ifdef __linux__
    u.i = __builtin_bswap32(u.i);
#else
    u.i = _byteswap_ulong(u.i);
#endif

    return u.f;
}

constexpr inline double Swap(double i) {
//     auto iInt = std::bit_cast<std::uint64_t>(i);
// #ifdef __linux__
//     iInt = __builtin_bswap64(iInt);
// #else
//     iInt = _byteswap_uint64(iInt);
// #endif
//     return std::bit_cast<double>(iInt);

    union {
        double d;
        uint64_t i;
    } u = {i};

#ifdef __linux__
    u.i = __builtin_bswap64(u.i);
#else
    u.i = _byteswap_uint64(u.i);
#endif

    return u.d;
}

template <typename T>
constexpr inline T SwapLE(T i) {
    if constexpr (IsLittleEndian) {
        return i;
    } else {
        return Swap(i);
    }
}

template <typename T>
constexpr inline T SwapBE(T i) {
    if constexpr (IsLittleEndian) {
        return Swap(i);
    } else {
        return i;
    }
}
}

#endif // IYF_ENDIANESS_HPP
