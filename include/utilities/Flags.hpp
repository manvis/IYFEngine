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

#ifndef FLAGS_HPP
#define FLAGS_HPP

#include <cstdint>
#include <type_traits>

namespace iyf {
    
/// A wrapper class that is used to implement flags based on enum class flag constants
/// \tparam T enum class to use
template <typename T>
class Flags {
public:
    using U = typename std::underlying_type<T>::type;
    
    static_assert(std::is_enum<T>::value, "Parameter used in Flags template is not an enum");
    // TODO 32 bit flags match what Vulkan uses, but what about other APIs?
    static_assert(std::is_same<U, std::uint32_t>::value, "Parameter used in Flags template must have a 32 bit unsigned integer underlying type");

    Flags() : flags(0) {}
    Flags(T flagBits) : flags(static_cast<U>(flagBits)) {}
    Flags(Flags<T> const& right) : flags(right.flags) {}
    Flags<T>& operator=(Flags<T> const& right) {
        flags = right.flags;
        return *this;
    }
    
    std::uint64_t uint64() const {
        return flags;
    }
    
    explicit operator U() const {
        return flags;
    }
    
    explicit operator bool() const {
        return !!flags;
    }
    
    Flags<T> operator&(const Flags<T>& right) const {
        Flags<T> result(*this);
        result &= right;
        return result;
    }

    Flags<T>& operator&=(const Flags<T>& right) {
        flags &= right.flags;
        return *this;
    }

    Flags<T> operator^(const Flags<T>& right) const {
        Flags<T> result(*this);
        result ^= right;
        return result;
    }

    Flags<T>& operator^=(const Flags<T>& right) {
        flags ^= right.flags;
        return *this;
    }

    Flags<T> operator|(const Flags<T>& right) const {
        Flags<T> result(*this);
        result |= right;
        return result;
    }

    Flags<T>& operator|=(const Flags<T>& right) {
        flags |= right.flags;
        return *this;
    }

    Flags<T> operator~() const {
        Flags<T> result(*this);
        result.flags ^= static_cast<U>(0);
        return result;
    }

    bool operator!=(const Flags<T>& right) const {
        return flags != right.flags;
    }

    bool operator==(const Flags<T>& right) const {
        return flags == right.flags;
    }
private:
    U flags;
};
}

#endif /* FLAGS_HPP */

