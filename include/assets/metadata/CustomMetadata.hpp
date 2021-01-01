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

#ifndef IYF_CUSTOM_METADATA_HPP
#define IYF_CUSTOM_METADATA_HPP

#include "assets/metadata/MetadataBase.hpp"

namespace iyf {
class CustomMetadata : public MetadataBase {
public:
    inline CustomMetadata() : MetadataBase(AssetType::Custom) {}
    
    inline CustomMetadata(FileHash fileHash,
                          const Path& sourceAsset,
                          FileHash sourceFileHash,
                          bool systemAsset,
                          const std::vector<std::string>& tags)
        : MetadataBase(AssetType::Custom, fileHash, sourceAsset, sourceFileHash, systemAsset, tags, true) {}
    
    virtual std::uint16_t getLatestSerializedDataVersion() const final override;
    
    virtual void displayInImGui() const final override;
    
    friend bool operator!=(const CustomMetadata& a, const CustomMetadata& b) {
        return !(a == b);
    }
    
    friend bool operator==(const CustomMetadata& a, const CustomMetadata& b) {
        return a.equals(b);
    }
private:
    virtual void serializeImpl(Serializer& fw, std::uint16_t version) const final override;
    virtual void deserializeImpl(Serializer& fr, std::uint16_t version) final override;
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const final override;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint16_t version) final override;
};
}

#endif // IYF_CUSTOM_METADATA_HPP
