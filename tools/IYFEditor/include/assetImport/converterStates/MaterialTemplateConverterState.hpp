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

#ifndef IYF_MATERIAL_TEMPLATE_CONVERTER_STATE_HPP
#define IYF_MATERIAL_TEMPLATE_CONVERTER_STATE_HPP

#include "assetImport/ConverterState.hpp"

namespace iyf::editor {
class MaterialTemplateConverterState : public ConverterState {
public:
    virtual std::uint64_t getLatestSerializedDataVersion() const final override;
    
    virtual AssetType getType() const final override {
        return AssetType::MaterialTemplate;
    }
private:
    friend class MaterialTemplateConverter;
    
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint64_t version) const final override;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint64_t version) final override;
    
    MaterialTemplateConverterState(PlatformIdentifier platformID, std::unique_ptr<InternalConverterState> internalState, const fs::path& sourcePath, FileHash sourceFileHash) :
        ConverterState(platformID, std::move(internalState), sourcePath, sourceFileHash) {}
};
}

#endif // IYF_MATERIAL_TEMPLATE_CONVERTER_STATE_HPP


