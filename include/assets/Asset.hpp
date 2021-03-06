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
#include "assets/AssetConstants.hpp"
#include <bitset>

namespace iyf {

/// \brief Base class for all assets that can be managed by TypeManager objects.
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
    virtual ~Asset() {}
    
    /// \brief Sets the name hash of this Asset to a new value.
    ///
    /// \warning This should only be called inside TypeManager::load(). Calling it anyhwere else may cause 
    /// resource leaks, incorrect asset data being passed to various systems and other nasty bugs.
    inline void setNameHash(StringHash newNameHash) {
        nameHash = newNameHash;
    }
    
    /// \returns The name hash of this Asset
    inline StringHash getNameHash() const {
        return nameHash;
    }
    
    /// \brief Marks this asset as loaded (true) or loading (false)
    ///
    /// \warning This should only be called inside the TypeManager.
    inline void setLoaded(bool loaded) {
        this->loaded = loaded;
    }
    
    /// If this is true, the asset is loaded and can safely be used. If it's false, the asset is being loaded
    /// asynchronously (or there's a horrible bug somewhere and you're accessing a stale handle of a long
    /// dead asset) and you MUSTN'T use any data contained within it.
    inline bool isLoaded() const {
        return loaded;
    }
    
    virtual AssetType getType() const = 0;
protected:
    Asset() : loaded(false) {}
private:
    StringHash nameHash;
    bool loaded;
    // What should I do with the 3 bytes here?
};

}

#endif //ASSET_HPP
