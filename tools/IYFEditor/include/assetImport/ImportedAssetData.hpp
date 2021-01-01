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

#ifndef IYF_IMPORTED_ASSET_DATA_HPP
#define IYF_IMPORTED_ASSET_DATA_HPP

#include "core/Constants.hpp"
#include "assets/metadata/Metadata.hpp"

namespace iyf::editor {
class ImportedAssetData {
public:
    ImportedAssetData(AssetType type, Metadata metadata, Path destinationPath) : type(type), metadata(std::move(metadata)), destinationPath(std::move(destinationPath)) {
        destinationPathHash = HS(this->destinationPath.getGenericString());
    }
    
    AssetType getType() const {
        return type;
    }
    
    const Metadata& getMetadata() const {
        return metadata;
    }
    
    const Path& getDestinationPath() const {
        return destinationPath;
    }
    
    StringHash getDestinationPathHash() const {
        return destinationPathHash;
    }
private:
    AssetType type;
    Metadata metadata;
    StringHash destinationPathHash;
    Path destinationPath;
};

}
#endif // IYF_IMPORTED_ASSET_DATA_HPP
