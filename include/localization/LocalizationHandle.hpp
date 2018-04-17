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

#ifndef IYF_LOCALIZATION_HANDLE_HPP
#define IYF_LOCALIZATION_HANDLE_HPP

#include "utilities/hashing/HashCombine.hpp"

namespace iyf {
class LocalizationHandle {
public:
    explicit inline LocalizationHandle(const char* key) : LocalizationHandle(key, "") {}
    explicit inline LocalizationHandle(const char* key, const char* strNamespace) {
        hash32_t seed(0);
        
        const hash32_t keyHash = HS(key);
        util::HashCombine(seed, keyHash);
        
        std::size_t namespaceStringLength = std::strlen(strNamespace);
        if (namespaceStringLength != 0) {
            const hash32_t namespaceHash = HS(strNamespace, namespaceStringLength);
            util::HashCombine(seed, namespaceHash);
        }
        
        handle = seed;
    }
    
    explicit inline constexpr LocalizationHandle(hash32_t handle) : handle(handle) {}
    
    inline constexpr hash32_t getHashValue() const {
        return handle;
    }
private:
    hash32_t handle;
};

using LH = LocalizationHandle;
}

#endif // IYF_LOCALIZATION_HANDLE_HPP