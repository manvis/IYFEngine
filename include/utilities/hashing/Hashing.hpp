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

#ifndef IYF_HASHING_HPP
#define IYF_HASHING_HPP

// #include "utilities/hashing/crc32.hpp"
// #include "utilities/hashing/crc64.hpp"
// WARNING Murmur's borked
// #include "utilities/hashing/murmur3.hpp"

#include <string>
#include <cstring>

#include "utilities/ForceInline.hpp"
#include "utilities/hashing/HashUtils.hpp"

// The settings are defined in CMake
#include "xxhash.h"
#define IYF_HASH_32_NAME "xxhash32"
#define IYF_HASH_64_NAME "xxhash64"

namespace iyf {
    
template<typename T>
class HashedValue {
public:
    using ValueType = T;
    
    IYF_FORCE_INLINE constexpr HashedValue<T>() : hashValue(0) {}
    IYF_FORCE_INLINE constexpr explicit HashedValue<T>(std::size_t v) : hashValue(v) {}
    
    IYF_FORCE_INLINE constexpr operator std::size_t() const {
        return static_cast<std::size_t>(hashValue);
    }
    
    IYF_FORCE_INLINE constexpr HashedValue<T>& operator=(const HashedValue<T>& right) {
        if (&right == this) {
            return *this;
        }
        
        hashValue = right.hashValue;
        return *this;
    }
    
    IYF_FORCE_INLINE constexpr bool operator==(const HashedValue<T>& right) const {
        return right.hashValue == hashValue;
    }
    
    IYF_FORCE_INLINE constexpr T value() const {
        return hashValue;
    }
private:
    T hashValue;
};

class StringHash : public HashedValue<std::uint64_t> {
public:
    IYF_FORCE_INLINE constexpr StringHash() : HashedValue() {}
    IYF_FORCE_INLINE constexpr explicit StringHash(std::size_t v) : HashedValue(v) {}
};

class FileHash : public HashedValue<std::uint64_t> {
public:
    IYF_FORCE_INLINE constexpr FileHash() : HashedValue() {}
    IYF_FORCE_INLINE constexpr explicit FileHash(std::size_t v) : HashedValue(v) {}
};

// String hashing. HS stands for "hash string".

IYF_FORCE_INLINE StringHash HS(const char* str, std::size_t length, std::uint32_t seed) {
    return StringHash(XXH64(str, length, seed));
}

IYF_FORCE_INLINE StringHash HS(const char* str, std::size_t length) {
    return HS(str, length, 0);
}

IYF_FORCE_INLINE StringHash HS(const char* str) {
    return HS(str, ConstexprStrlen(str), 0);
}

IYF_FORCE_INLINE StringHash HS(const std::string& str, std::uint32_t seed) {
    return HS(str.c_str(), str.length(), seed);
}

IYF_FORCE_INLINE StringHash HS(const std::string& str) {
    return HS(str.c_str(), str.length(), 0);
}

// File (or other long data) hashing. HF stands for "hash file"

IYF_FORCE_INLINE FileHash HF(const char* str, std::size_t length, std::uint64_t seed) {
    return FileHash(XXH64(str, length, seed));
}

IYF_FORCE_INLINE FileHash HF(const char* str, std::size_t length) {
    return HF(str, length, 0);
}

IYF_FORCE_INLINE FileHash HF(const char* str) {
    return HF(str, ConstexprStrlen(str), 0);
}

IYF_FORCE_INLINE FileHash HF(const std::string& str, std::uint64_t seed) {
    return HF(str.c_str(), str.length(), seed);
}

IYF_FORCE_INLINE FileHash HF(const std::string& str) {
    return HF(str.c_str(), str.length(), 0);
}

}

namespace std {
    template <>
    struct hash<iyf::StringHash> {
        std::size_t operator()(const iyf::StringHash& k) const { 
            return std::hash<iyf::StringHash::ValueType>{}(k);
        }
    };
    
    template <>
    struct hash<iyf::FileHash> {
        std::size_t operator()(const iyf::FileHash& k) const { 
            return std::hash<iyf::FileHash::ValueType>{}(k);
        }
    };
}

#endif /* IYF_HASHING_HPP */

