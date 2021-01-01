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

#include "graphics/shaderGeneration/ShaderMacroCombiner.hpp"
#include "graphics/Renderer.hpp"

#include "logging/Logger.hpp"

#include <unordered_set>

namespace iyf::ShaderMacroCombiner {
std::vector<std::vector<ShaderMacroWithValue>> MakeMacroAndValueVectors() {
    // WARNING the variant generation system doesn't know how to work with macros without values (represented by the monostate variant)
    // don't use them.
    
    std::vector<std::vector<ShaderMacroWithValue>> results = {
        MakeMacroAndValueVector(ShaderMacro::Renderer, Renderer::GetAvailableRenderers()),
        MakeMacroAndValueVector<ShadowMode>(ShaderMacro::ShadowMode, {ShadowMode::NoShadows, ShadowMode::CascadedShadowMap}),
        MakeMacroAndValueVector<FogMode>(ShaderMacro::FogMode, {FogMode::NoFog, FogMode::Linear, FogMode::Exponential, FogMode::ExponentialSquared}),
    };
    
    for (const auto& r : results) {
        if (r.empty()) {
            throw std::runtime_error("ShaderMacroWithValue vector not be empty.");
        }
    }
    
    std::sort(results.begin(), results.end(), [](const std::vector<ShaderMacroWithValue>& a, const std::vector<ShaderMacroWithValue>& b) {
        return a[0].getNameHash() < b[0].getNameHash();
    });
    
    return results;
}

std::size_t DetermineCombinationCount(const std::vector<std::vector<ShaderMacroWithValue>>& input) {
    int size = 1;
    
    for (const auto& i : input) {
        size *= i.size();
    }
    
    return size;
}

void RecursiveCombine(const std::vector<std::vector<ShaderMacroWithValue>>& input, std::vector<std::pair<StringHash, std::vector<ShaderMacroWithValue>>>& output, std::vector<ShaderMacroWithValue> current, std::size_t depth) {
    if (depth == input.size()) {
        output.emplace_back(StringHash(0), current);
        return;
    }
    
    for (const auto& i : input[depth]) {
        std::vector<ShaderMacroWithValue> next = current;
        next.emplace_back(i);
        
        RecursiveCombine(input, output, next, depth + 1);
    }
}

MacroCombos MakeAllCombinations(std::vector<std::vector<ShaderMacroWithValue>>& macroAndValueVectors) {
    MacroCombos result;
    
    result.combos.reserve(DetermineCombinationCount(macroAndValueVectors));
    
    std::vector<ShaderMacroWithValue> current;
    RecursiveCombine(macroAndValueVectors, result.combos, current, 0);
    
    std::unordered_set<StringHash> testSet;
    
    StringHash versionHash(0);
    for (auto& combo : result.combos) {
        StringHash comboHash(0);
        for (const auto& m : combo.second) {
            util::HashCombineImpl(comboHash, m.getNameHash());
            util::HashCombineImpl(comboHash, m.getValueHash());
        }
        combo.first = comboHash;
        
        if (!testSet.emplace(comboHash).second) {
            throw std::runtime_error("Hash collision in shader macro combo generator.");
        }
        
        util::HashCombine(versionHash, comboHash);
    }
    result.versionHash = versionHash;
    
    return result;
}
}
