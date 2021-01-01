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

#ifndef IYF_MATERIAL_TEMPLATE_METADATA_HPP
#define IYF_MATERIAL_TEMPLATE_METADATA_HPP

#include "assets/metadata/MetadataBase.hpp"
#include "graphics/materials/MaterialConstants.hpp"
#include "graphics/materials/MaterialInputs.hpp"
#include "graphics/ShaderMacros.hpp"
#include "graphics/VertexDataLayouts.hpp"

#include <bitset>
#include <unordered_map>

namespace iyf {
class MaterialTemplateMetadata : public MetadataBase {
public:
    struct SupportedMacroValues {
        ShaderMacroValue defaultValue;
        std::vector<ShaderMacroValue> supportedValues;
    };
    
    inline MaterialTemplateMetadata() : MetadataBase(AssetType::MaterialTemplate) {}
    
    inline MaterialTemplateMetadata(FileHash fileHash,
                        const Path& sourceAsset,
                        FileHash sourceFileHash,
                        bool systemAsset,
                        const std::vector<std::string>& tags,
                        MaterialFamily family,
                        FileHash materialFamilyHash,
                        StringHash macroComboHash,
                        std::bitset<64> vertexDataLayouts,
                        std::vector<MaterialInputVariable> requiredVariables,
                        std::vector<MaterialInputTexture> requiredTextures)
        : MetadataBase(AssetType::MaterialTemplate, fileHash, sourceAsset, sourceFileHash, systemAsset, tags, true), family(family),
          materialFamilyHash(materialFamilyHash), macroComboHash(macroComboHash), vertexDataLayouts(vertexDataLayouts),
          requiredVariables(std::move(requiredVariables)), requiredTextures(std::move(requiredTextures)) {
        }
    
    virtual std::uint16_t getLatestSerializedDataVersion() const final override;
    
    virtual void displayInImGui() const final override;
    
    inline MaterialFamily getMaterialFamily() const {
        return family;
    }
    
    /// Used to determine if the MaterialFamilyDefinition has changed and the MaterialTemplate needs to be regenerated
    inline FileHash getMaterialFamilyHash() const {
        return materialFamilyHash;
    }
    
    /// Used to determine if the macro combos supported by the engine changed and the MaterialTemplate needs to be regenerated
    inline StringHash getMaterialComboHash() const {
        return macroComboHash;
    }
    
    inline std::bitset<64> getSupportedVertexDataLayouts() const {
        return vertexDataLayouts;
    }
    
    inline const std::vector<MaterialInputVariable>& getRequiredVariables() const {
        return requiredVariables;
    }
    
    inline const std::vector<MaterialInputTexture>& getRequiredTextures() const {
        return requiredTextures;
    }
    
    std::unordered_map<StringHash, std::string> getNameMap() const;
    
    friend bool operator!=(const MaterialTemplateMetadata& a, const MaterialTemplateMetadata& b) {
        return !(a == b);
    }
    
    friend bool operator==(const MaterialTemplateMetadata& a, const MaterialTemplateMetadata& b) {
        return a.equals(b) &&
               (a.family == b.family) &&
               (a.materialFamilyHash == b.materialFamilyHash) &&
               (a.macroComboHash == b.macroComboHash) &&
               (a.vertexDataLayouts == b.vertexDataLayouts) &&
               (a.requiredVariables == b.requiredVariables) &&
               (a.requiredTextures == b.requiredTextures);
    }
private:
    virtual void serializeImpl(Serializer& fw, std::uint16_t version) const final override;
    virtual void deserializeImpl(Serializer& fr, std::uint16_t version) final override;
    virtual void serializeJSONImpl(PrettyStringWriter& pw, std::uint16_t version) const final override;
    virtual void deserializeJSONImpl(JSONObject& jo, std::uint16_t version) final override;
    
    void buildNameMap();
    
    MaterialFamily family;
    FileHash materialFamilyHash;
    
    StringHash macroComboHash;
    std::bitset<64> vertexDataLayouts;
    
    std::vector<MaterialInputVariable> requiredVariables;
    std::vector<MaterialInputTexture> requiredTextures;
    
    static_assert(static_cast<std::uint64_t>(VertexDataLayout::COUNT) <= 64, "Only 64 VertexDataLayouts are supported");
};

}

#endif // IYF_MATERIAL_TEMPLATE_METADATA_HPP

