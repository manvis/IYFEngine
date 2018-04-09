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

#ifndef DATASIZES_HPP
#define DATASIZES_HPP

#include "utilities/TypeHelpers.hpp"
#include <ratio>

/// \file a nice data size conversion header that works in a similar principle to std::chrono::duration

namespace iyf {
    
using kibi = std::ratio<1024L,          1>;
using mebi = std::ratio<1048576L,       1>;
using gibi = std::ratio<1073741824L,    1>;
using tebi = std::ratio<1099511627776L, 1>;

template<typename T>
struct is_data_size_type : util::is_type_one_of<T, std::ratio<1>, kibi, mebi, gibi, tebi> {};

template<typename T = std::ratio<1>>
class DataSize;

// The default case should be impossible for these ratios, since either the num or the den will always end up equal to 1
template<typename ToT, typename RD, bool NumOne = false, bool DenOne = false>
struct data_size_cast_impl {
    template<typename T>
    static constexpr ToT cast(const DataSize<T>& t) {
        return ToT(t.count() * RD::num / RD::den);
    }
};

template<typename ToT, typename RD>
struct data_size_cast_impl <ToT, RD, true, true> {
    template<typename T>
    static constexpr ToT cast(const DataSize<T>& t) {
        return ToT(t.count());
    }
};

template<typename ToT, typename RD>
struct data_size_cast_impl <ToT, RD, true, false> {
    template<typename T>
    static constexpr ToT cast(const DataSize<T>& t) {
        return ToT(t.count() / RD::den);
    }
};

template<typename ToT, typename RD>
struct data_size_cast_impl <ToT, RD, false, true> {
    template<typename T>
    static constexpr ToT cast(const DataSize<T>& t) {
        return ToT(t.count() * RD::num);
    }
};

template<typename ToT, typename T>
constexpr typename std::enable_if<(std::is_same<T, std::ratio<1>>::value || std::is_same<T, kibi>::value || std::is_same<T, mebi>::value || std::is_same<T, gibi>::value || std::is_same<T, tebi>::value), ToT>::type
datasize_cast(const DataSize<T> size) {
    using rd = std::ratio_divide<T, typename ToT::t>;
    
    using impl = data_size_cast_impl<ToT, rd, rd::num == 1, rd::den == 1>;
    
    return impl::cast(size);
}

template <typename T>
class DataSize {
public:
    using t = T;
    
    static_assert(std::is_same<T, std::ratio<1>>::value || std::is_same<T, kibi>::value || std::is_same<T, mebi>::value || std::is_same<T, gibi>::value || std::is_same<T, tebi>::value,
                  "If specified, template parameter T must be kibi, mebi, gibi or tebi");
    
    constexpr DataSize() : dataSize(0) { }
    DataSize(const DataSize&) = default;
    
    constexpr explicit DataSize(const std::uint64_t& size) : dataSize(size) {}
    
    template <typename T2>
    constexpr DataSize(const DataSize<T2>& size) : dataSize(datasize_cast<DataSize>(size).count()) {}
    
    std::uint64_t count() const {
        return dataSize;
    }
    
    operator std::uint64_t() const {
        return dataSize;
    }
    
    DataSize<T>& operator+=(const DataSize<T>& right) {
        dataSize += right.dataSize;
        
        return *this;
    }

    DataSize<T>& operator-=(const DataSize<T>& right) {
        dataSize -= right.dataSize;
        
        return *this;
    }
    
    DataSize<T> operator+(const DataSize<T>& right) const {
        DataSize<T> result(*this);
        result += right;
        return result;
    }

    DataSize<T>& operator++() {
        ++dataSize;
        return *this;
    }
    
    DataSize<T>& operator++(int) {
        dataSize++;
        return *this;
    }

    DataSize<T> operator-(const DataSize<T>& right) const {
        DataSize<T> result(*this);
        result -= right;
        return result;
    }

    DataSize<T>& operator--() {
        --dataSize;
        return *this;
    }
    
    DataSize<T>& operator--(int) {
        dataSize--;
        return *this;
    }
    
    friend inline bool operator<(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize < r.dataSize;
    }
    
    friend inline bool operator>(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize > r.dataSize;
    }
    
    friend inline bool operator==(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize == r.dataSize;
    }
    
    friend inline bool operator>=(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize >= r.dataSize;
    }
    
    friend inline bool operator<=(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize <= r.dataSize;
    }
    
    friend inline bool operator!=(const DataSize<T>& l, const DataSize<T>& r) {
        return l.dataSize != r.dataSize;
    }

protected:
    std::uint64_t dataSize;
};

using Bytes     = DataSize<>;
using Kibibytes = DataSize<kibi>;
using Mebibytes = DataSize<mebi>;
using Gibibytes = DataSize<gibi>;
using Tebibytes = DataSize<tebi>;

using KiB = Kibibytes;
using MiB = Mebibytes;
using GiB = Gibibytes;
using TiB = Tebibytes;

namespace literals {
constexpr Bytes operator ""_B(unsigned long long int size) {
    return Bytes(size);
}

constexpr KiB operator ""_KiB(unsigned long long int size) {
    return KiB(size);
}

constexpr MiB operator ""_MiB(unsigned long long int size) {
    return MiB(size);
}

constexpr GiB operator ""_GiB(unsigned long long int size) {
    return GiB(size);
}
}
}

#endif /* DATASIZES_HPP */

