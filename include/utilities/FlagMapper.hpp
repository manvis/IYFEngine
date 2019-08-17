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

#ifndef IYF_FLAG_MAPPER_HPP
#define IYF_FLAG_MAPPER_HPP

namespace iyf {
/// Template class used to map engine native flags to API native flags. When assigning mappings, note that the 0th mapping
/// is for a no flags set value.
/// \tparam I input type, which must be an engine native enumerator wrapper iyf::Flag<wrappeddEnumType>,
/// start at 1 and increase by a left bit shift by 1 (not checked), e.g. (assume bits, not octal numbers): 0001, 0010, 0100, 1000...
/// \tparam O output type, which must be a graphics API native type. Used as the type for std::array
/// \tparam N number of single bit flags defined in I. Used as the size for the mappinng std::array.
/// In case you're using a COUNT flag in enum, make sure it's computed by hand and is NOT an automatically
/// incremented value of the last flag or things may explode ;). Moreover, if you have helper flags defined which
/// are a combination of single bit flags, these combinations DO NOT count into the total, e.g., the N of enum
/// class {A = 0x01, B = 0x02, C = 0x04, D = 0x03} is 3 because D = A | B
template <typename I, typename O, size_t N>
class FlagMapper {
public:
    using U = typename I::U;
    
    inline O operator()(I engineFlag) {
        if (static_cast<U>(engineFlag) == 0) {
            return flagMap[0];
        } else {
            U gather = 0;

            for (U i = 1, c = 1; c <= N; i <<= 1, ++c) {
                if (i & static_cast<U>(engineFlag)) {
                    gather |= flagMap[c];
                }
            }
            
            return gather;
        }
    }

    // Keeping this public for the sake of easy initialization
    // N + 1 is here because we store flags for 0 as well
    const std::array<O, N + 1> flagMap;
};

/// In case engine and API uses the same bits for flags, this simply performs a static_cast
/// \tparam I input type, which must be an engine native enumerator wrapper iyf::Flag<wrappeddEnumType>,
/// start at 1 and increase by a left bit shift by 1 (not checked), e.g. (assume bits, not octal numbers): 0001, 0010, 0100, 1000...
/// \tparam O output type, which must be a graphics API native type. Used as the type for std::array
template <typename I, typename O>
class IdentityFlagMapper {
public:
    using U = typename I::U;
    
    inline O operator()(I engineFlag) {
        U underlyingType = static_cast<U>(engineFlag);
        return static_cast<O>(underlyingType);
    }
};
}

#endif // IYF_FLAG_MAPPER_HPP

