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

#ifndef IYF_MATERIAL_INSTANCE_DEFINITION_HPP
#define IYF_MATERIAL_INSTANCE_DEFINITION_HPP

#include "utilities/hashing/Hashing.hpp"
#include "core/Constants.hpp"
#include "core/interfaces/Serializable.hpp"
#include "core/interfaces/TextSerializable.hpp"
#include "localization/TextLocalization.hpp"

#include "glm/vec4.hpp"

#include <vector>

namespace iyf {
    
/// \brief List of all supported ways to render materials
/// \warning Each mode must have a corresponding entry in MaterialRenderModeNames
enum class MaterialRenderMode : std::uint8_t {
    Opaque      = 0,
    Transparent = 1,
    // TODO cutout, wireframe
    COUNT
};
namespace con {
/// \remark This function does not return a user friendly name. Use GetMaterialRenderModeLocalizationHandle() and the localization DB if you
/// need one.
/// \return the name of the MaterialRenderMode.
const std::string& GetMaterialRenderModeName(MaterialRenderMode renderMode);

const LocalizationHandle& GetMaterialRenderModeLocalizationHandle(MaterialRenderMode renderMode);
}

/// This class stores material data and metadata for editing, serializes it into files and is used by the World objects to 
/// instantiate Material objects that contain data used by the GPU during rendering.
class MaterialInstanceDefinition : public Serializable, public TextSerializable {
public:
    MaterialInstanceDefinition() : materialTemplateDefinition(0), renderMode(MaterialRenderMode::Opaque) {}

    /// \brief Get a StringHash of a MaterialTemplateDefinition that this MaterialInstanceDefinition is based on.
    inline StringHash getMaterialTemplateDefinition() const {
        return materialTemplateDefinition;
    }

    /// \brief Associate a new MaterialTemplateDefinition with this MaterialInstanceDefinition.
    inline void setMaterialTemplateDefinition(StringHash newMaterialTemplateDefinition,
                                              std::vector<std::pair<StringHash, glm::vec4>> variables,
                                              std::vector<std::pair<StringHash, StringHash>> textures) {
        materialTemplateDefinition = newMaterialTemplateDefinition;
        this->variables = std::move(variables);
        this->textures = std::move(textures);
    }

    /// \brief Get a MaterialRenderMode that is used by MaterialInstances created from this MaterialInstanceDefinition
    inline MaterialRenderMode getRenderMode() const {
        return renderMode;
    }

    /// \brief Set a new MaterialRenderMode that will be used by MaterialInstances created from this MaterialInstanceDefinition
    ///
    /// \warning Calling this will invalidate and potentially change the ID. It will be recomputed the next time getID() is called.
    ///
    /// \todo since I'm planning to make familyVariant carry more data, the render mode should PROBABLY be a part of it
    inline void setRenderMode(MaterialRenderMode newRenderMode) {
        renderMode = newRenderMode;
    }
    
    inline const std::vector<std::pair<StringHash, glm::vec4>>& getVariables() const {
        return variables;
    }
    
    inline const std::vector<std::pair<StringHash, StringHash>>& getTextures() const {
        return textures;
    }
    
    virtual void serialize(Serializer& fw) const final override;
    virtual void deserialize(Serializer& fr) final override;
    
    virtual void serializeJSON(PrettyStringWriter& pw) const final override;
    virtual void deserializeJSON(JSONObject& jo) final override;
    
    virtual bool makesJSONRoot() const final override {
        return true;
    }
protected:
    StringHash materialTemplateDefinition;
    
    std::vector<std::pair<StringHash, glm::vec4>> variables;
    std::vector<std::pair<StringHash, StringHash>> textures;
    
    /// Used to determine if an Entity object that uses a Material created from this definition should be stored in "opaque objects" draw list or in "transparent objects" draw list
    MaterialRenderMode renderMode;
};
}

#endif // IYF_MATERIAL_INSTANCE_DEFINITION_HPP

