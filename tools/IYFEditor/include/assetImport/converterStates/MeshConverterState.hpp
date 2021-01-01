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

#ifndef IYF_MESH_CONVERTER_STATE_HPP
#define IYF_MESH_CONVERTER_STATE_HPP

#include "assetImport/ConverterState.hpp"

namespace iyf::editor {
class MeshConverterState : public ConverterState {
public:
    // TODO FIXME these parameters do nothing at the moment
    bool convertAnimations;
    bool use32bitIndices;
    float scale;
    
    // TODO expose Assimp optimization options instead of going with the default
    // TODO allow to only import certain animations
    // TODO generate materials based on data retrieved from file
    
    virtual AssetType getType() const final override {
        return AssetType::Mesh;
    }
    
    virtual std::uint64_t getLatestSerializedDataVersion() const final override;
private:
    friend class MeshConverter;
    
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const final override;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint64_t version) final override;
    
    MeshConverterState(PlatformIdentifier platformID, std::unique_ptr<InternalConverterState> internalState, const Path& sourcePath, FileHash sourceFileHash)
        : ConverterState(platformID, std::move(internalState), sourcePath, sourceFileHash), convertAnimations(true), scale(1.0f) {}
};

}

#endif // IYF_MESH_CONVERTER_STATE_HPP
