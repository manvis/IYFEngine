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

#ifndef ASSET_HPP
#define ASSET_HPP

#include "utilities/hashing/Hashing.hpp"

namespace iyf {

/// Base class for all assets that can be managed by TypeManager objects.
///
/// You should treat this class and all of its derivatives as dumb containers that contain all data that
/// that a System may need in order to use this Asset. The data MUST be set by TypeManager::performLoad() overrides, 
/// cleared by TypeManager::performFree() overrides and stay constant otherwise. It must also be possible to
/// reuse a single Asset in many different Entity Component objects, that is, any data, specific to game logic
/// should be stored in Component objects and not here.
///
/// All Asset derivatives must also be default constructible.
class Asset {
public:
    /// Sets the name hash of this Asset to a new value.
    ///
    /// \warning This should only be called inside TypeManager::performLoad() overrides. Calling it anyhwere else
    /// may cause resource leaks, incorrect asset data being passed to various systems and other nasty bugs.
    void setNameHash(hash32_t newNameHash) {
        nameHash = newNameHash;
    }
    
    hash32_t getNameHash() const {
        return nameHash;
    }
    
private:
    hash32_t nameHash;
};

}

#endif //ASSET_HPP
