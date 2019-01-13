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

#ifndef IYF_SHADER_MACRO_COMBINER_HPP
#define IYF_SHADER_MACRO_COMBINER_HPP

#include "graphics/ShaderMacros.hpp"

#include <vector>
#include <type_traits>
#include <stdexcept>

namespace iyf::ShaderMacroCombiner {
template <typename T>
std::vector<ShaderMacroWithValue> MakeMacroAndValueVector(ShaderMacro macro, const std::vector<T>& allowedValues) {
    if (macro == ShaderMacro::Custom || macro == ShaderMacro::COUNT) {
        throw std::logic_error("ShaderMacro::Custom and ShaderMacro::COUNT are not supported");
    }
    
    if (allowedValues.empty()) {
        throw std::runtime_error("allowedValues must not be empty");
    }
    
    std::vector<ShaderMacroWithValue> result;
    result.reserve(allowedValues.size());
    
    for (const T& v : allowedValues) {
        if constexpr (std::is_enum_v<T>) {
            result.emplace_back(macro, static_cast<std::int64_t>(v));
        } else {
            result.emplace_back(macro, v);
        }
    }
    
    return result;
}

/// Each child std::vector contains all allowed shader macro values.
///
/// The parent vector is sorted based on the hash of the macro name. This is very important for correct shader retrieval because
/// shader variants are uniquely identified by a combined hash of macro names and their parameters and the hash combination
/// operation is not commutative.
///
/// The first available value in each macro vector is supposed to be treated as a default that's used when an macro is not requested.
///
/// \remark This function is fairly expensive and it's going to become even more expensive once more macros are added. Since the
/// results won't change without recompilation, it would be wise to call it once and cache the result.
std::vector<std::vector<ShaderMacroWithValue>> MakeMacroAndValueVectors();

struct MacroCombos {
    /// Combined hash of all combo hashes. Used to uniquely identify the version.
    StringHash versionHash;
    
    std::vector<std::pair<StringHash, std::vector<ShaderMacroWithValue>>> combos;
};

/// Creates all possible unique combinations of vectors retuned by MakeMacroAndValueVectors();
MacroCombos MakeAllCombinations(std::vector<std::vector<ShaderMacroWithValue>>& macroAndValueVectors);

}

#endif // IYF_SHADER_MACRO_COMBINER_HPP
