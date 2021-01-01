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

#ifndef IYF_FLAGS_HPP
#define IYF_FLAGS_HPP

#include <cstdint>
#include <type_traits>

namespace iyf {
    
/// A wrapper class that is used to implement flags based on enum class flag constants
/// \tparam T enum class to use
/// \tparam U the storage type to use (e.g., uint64_t, uint32_t...)
template <typename T, typename UT = std::underlying_type_t<T>>// TODO operator U(), default value (once all changes are done)
class Flags {
public:
    using U = UT;
    
    static_assert(std::is_enum<T>::value, "Parameter T used in Flags template must be an enum");
    static_assert(std::is_integral<U>::value, "U must be an integral type");
    static_assert(sizeof(U) >= sizeof(std::underlying_type_t<T>), "std::underlying_type_t<T> doesn't fit in U");

    Flags() : flags(0) {}
    Flags(T flagBits) : flags(static_cast<U>(flagBits)) {}
    Flags(Flags<T, U> const& right) : flags(right.flags) {}
    Flags<T, U>& operator=(Flags<T, U> const& right) {
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
    
    Flags<T, U> operator&(const Flags<T, U>& right) const {
        Flags<T, U> result(*this);
        result &= right;
        return result;
    }

    Flags<T, U>& operator&=(const Flags<T, U>& right) {
        flags &= right.flags;
        return *this;
    }

    Flags<T, U> operator^(const Flags<T, U>& right) const {
        Flags<T, U> result(*this);
        result ^= right;
        return result;
    }

    Flags<T, U>& operator^=(const Flags<T, U>& right) {
        flags ^= right.flags;
        return *this;
    }

    Flags<T, U> operator|(const Flags<T, U>& right) const {
        Flags<T, U> result(*this);
        result |= right;
        return result;
    }

    Flags<T, U>& operator|=(const Flags<T, U>& right) {
        flags |= right.flags;
        return *this;
    }

    Flags<T, U> operator~() const {
        Flags<T, U> result(*this);
        result.flags ^= static_cast<U>(0);
        return result;
    }

    bool operator!=(const Flags<T, U>& right) const {
        return flags != right.flags;
    }

    bool operator==(const Flags<T, U>& right) const {
        return flags == right.flags;
    }
private:
    U flags;
};
}

#endif // IYF_FLAGS_HPP

