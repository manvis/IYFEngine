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

#ifndef CONSTANTMAPPER_HPP
#define CONSTANTMAPPER_HPP

#include <array>
#include <type_traits>

namespace iyf {
/// Template class, typically used for easy mapping between engine native and
/// graphics API native constants
/// \tparam I input type, which must be an engine native enumerator (checked using std::is_enum during compilation),
/// start at 0 and increase by 1. It's important because the mapping table is an std::array
/// \tparam O output type, which must be a graphics API native type. Used as the type for std::array
/// \tparam N number of constants to be used in this mapper. Used as the size for the mappinng std::array
template <typename I, typename O, size_t N>
class ConstantMapper {
public:
    static_assert(std::is_enum<I>::value, "I parameter used in ConstantMapper template is not an enum");
    
    inline O operator()(I engineConstant) {
#ifndef ENG_DEBUG_MODE
        return constantMap[static_cast<size_t>(engineConstant)];
#else // Check bounds when debug mode is enabled
        return constantMap.at(static_cast<size_t>(engineConstant));
#endif
    }

    // Keeping this public for the sake of easy initialization
    const std::array<O, N> constantMap;
};
}


#endif /* CONSTANTMAPPER_HPP */

