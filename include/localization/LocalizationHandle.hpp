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
#include "utilities/ForceInline.hpp"
#include "utilities/hashing/HashUtils.hpp"

namespace iyf {
class LocalizationHandle {
public:
    explicit IYF_FORCE_INLINE constexpr LocalizationHandle(StringHash handle) : handle(handle) {}
    
    IYF_FORCE_INLINE constexpr StringHash getHashValue() const {
        return handle;
    }
private:
    StringHash handle;
};

/// Hashes the key and the namespace and builds a LocalizationHandle that can be used for 
/// localized string lookups
IYF_FORCE_INLINE LocalizationHandle LH(const char* key, const char* strNamespace) {
    StringHash seed(0);
        
    const StringHash keyHash = HS(key);
    util::HashCombine(seed, keyHash);
    
    std::size_t namespaceStringLength = ConstexprStrlen(strNamespace);
    if (namespaceStringLength != 0) {
        const StringHash namespaceHash = HS(strNamespace, namespaceStringLength);
        util::HashCombine(seed, namespaceHash);
    }
    
    return LocalizationHandle(seed);
}

/// Hashes the key and builds a LocalizationHandle that can be used for localized string lookups.
/// This assumes an empty namespace
IYF_FORCE_INLINE LocalizationHandle LH(const char* key) {
    StringHash seed(0);
        
    const StringHash keyHash = HS(key);
    util::HashCombine(seed, keyHash);
    
    return LocalizationHandle(seed);
}

}

#endif // IYF_LOCALIZATION_HANDLE_HPP
