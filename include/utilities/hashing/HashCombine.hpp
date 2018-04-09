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

#ifndef HASHCOMBINE_HPP
#define HASHCOMBINE_HPP

#include "utilities/hashing/Hashing.hpp"

/// \file
/// This file contains hash32_t and std::hash combination functions

namespace iyf {
namespace util {
    
/// \brief Combine several hash values into one. This function is based on boost::hash_combine(). It has been re-written to allow builds without a mandatory Boost dependency.
/// 
/// To use, call this function repeatedly, passing the same seed (it's an in-out parameter) and different hash values that you wish to combine into one.
///
/// \warning order of hashCombine calls matters, that is combining A with B will yield different results than combining B with A
/// 
/// \param[in,out] seed the results are accumulated into this value. It should typically start as hash32_t(0).
/// \param[in] value value of the hash to combine with seed
inline void HashCombine(hash32_t& seed, hash32_t value) {
    std::size_t temp = seed.value();
    temp ^= value.value() + 0x9e3779b9 + (seed.value() << 6) + (seed.value() >> 2);
    seed = hash32_t(temp);
}

inline void HashCombine(std::size_t& seed, std::size_t value) {
    std::size_t temp = seed;
    temp ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed = temp;
}

}
}

#endif /* HASHCOMBINE_HPP */

