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

#ifndef MATERIALDEFINITION_HPP
#define MATERIALDEFINITION_HPP

#include "utilities/hashing/Hashing.hpp"
#include "core/Constants.hpp"
#include "core/interfaces/Serializable.hpp"
#include "localization/TextLocalization.hpp"

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

extern const std::array<LocalizationHandle, static_cast<std::size_t>(MaterialRenderMode::COUNT)> MaterialRenderModeNames;

/// This class stores material data and metadata for editing, serializes it into files and is used by the World objects to 
/// instantiate Material objects that contain data used by the GPU during rendering.
/// \todo this class uses the word pipeline many times in documentation, but it's an INCORRECT use of this term. Fix it
class MaterialDefinition : public Serializable {
public:
    enum class DataType : std::uint8_t {
        TextureID = 1,
        ColorData = 2
    };
    
    /// Default constructor that initializes all hashes to 0
    MaterialDefinition() : pipelineID(0), pipelineVariant(0), renderMode(MaterialRenderMode::Opaque), name("NewMaterial"), id(0), idNeedsRecompute(true) {}
    
    /// \brief Compute and retrieve a hash that uniquely identifies this MaterialDefinition.
    ///
    /// \warning This value is a combination of other fields (except for the name) and IT WILL CHANGE if any field is changed, so make sure
    /// to only call this function after all other fields are set and you are sure that they won't change
    /// 
    /// This function is fairly cheap to call since the class keeps track of changes and only re-computes the ID if any setters or non-const ???
    /// 
    /// \return Unique identifier for this MaterialDefinition
    inline hash32_t getID() {
        if (idNeedsRecompute) {
            recomputeID();
        }
        
        return id;
    }
    
    /// \brief Get a human readable name assigned to this MaterialDefinition
    inline std::string getName() const {
        return name;
    }

    /// \brief Assign a new human readable name to this MaterialDefinition
    inline void setName(std::string newName) {
        name = newName;
    }

    /// \brief Get an ID of a pipeline that's currently associated with this MaterialDefinition
    inline hash32_t getPipelineID() const {
        return pipelineID;
    }

    /// \brief Associate a new pipeline with this MaterialDefinition
    ///
    /// \warning Calling this will invalidate and potentially change the ID. It will be recomputed the next time getID() is called.
    inline void setPipelineID(hash32_t newPipelineID) {
        pipelineID = newPipelineID;
        idNeedsRecompute = true;
    }

    /// \brief Get a pipeline variant that is associated with this MaterialDefinition
    inline hash32_t getPipelineVariant() const {
        return pipelineVariant;
    }

    /// \brief Associate a new pipeline variant with this Material definition
    ///
    /// \warning Calling this will invalidate and potentially change the ID. It will be recomputed the next time getID() is called.
    inline void setPipelineVariant(hash32_t newPipelineVariant) {
        pipelineVariant = newPipelineVariant;
        idNeedsRecompute = true;
    }

    /// \brief Get a MaterialRenderMode that is used by Materials created from this MaterialDefinition
    inline MaterialRenderMode getRenderMode() const {
        return renderMode;
    }

    /// \brief Set a new MaterialRenderMode that will be used by Materials created from this MaterialDefinition
    ///
    /// \warning Calling this will invalidate and potentially change the ID. It will be recomputed the next time getID() is called.
    ///
    /// \todo since I'm planning to make pipelineVariant carry more data, the render mode should PROBABLY be a part of it
    inline void setRenderMode(MaterialRenderMode newRenderMode) {
        renderMode = newRenderMode;
        idNeedsRecompute = true;
    }
    
    /// \brief Get a modifiable reference to a vector of material components
    ///
    /// \warning Calling this will invalidate the ID. It will be recomputed the next time getID() is called.
    std::vector<std::pair<DataType, std::uint32_t>>& getComponents() {
        idNeedsRecompute = true;
        return components;
    }
    
    /// \brief Get a const vector of material components for reading. Does not invalidate the ID
    const std::vector<std::pair<DataType, std::uint32_t>>& getComponents() const {
        return components;
    }
    
    // Serializable interface
    /// \warning Make sure to ALWAYS call getID before calling this to get the class finalized
    /// \todo maybe use a finalize method instead of relying on getID?
    virtual void serialize(Serializer& fw) const override;
    virtual void deserialize(Serializer& fr) override;
    
    // Separate serialization of name string
    virtual void serializeName(Serializer& fw) const;
    virtual void deserializeName(Serializer& fr);
protected:
    void recomputeID();
    
    /// ID of a Pipeline that's used by this material
    hash32_t pipelineID;
    
    /// ID of a variant of the said Pipeline. These typically indicate permutations of shaders (e.g. one may use a flat color albedo while another uses a texture) or their
    /// combinations (e.g. one has a tessellation shader while another does not)
    /// \todo replace with something that carries more information about the variant
    hash32_t pipelineVariant;
    
    /// Used to determine if an Entity object that uses a Material created from this definition should be stored in "opaque objects" draw list or in "transparent objects" draw list
    MaterialRenderMode renderMode;
    
    /// This vector stores material components. The first element indicates if a color (packed into std::uint32_t) or a texture ID is stored in the second element. The required size 
    /// of this vector, the order of its elements and requirements for them (e.g., the number of color channels) depend on the associated pipeline and its variant. The 
    /// appropriate pipeline also stores the human readable names for these elements.
    ///
    /// \warning Make sure to always have exactly as many material components as the pipeline requires and NEVER create a MaterialDefinition with more than MaxMaterialComponents 
    /// components or exceptions will be thrown at various different stages
    std::vector<std::pair<DataType, std::uint32_t>> components;
    
    /// Human readable name of the material. Stored separately (check serializeName()) so that it could be stripped from release builds
    std::string name;
    
    hash32_t id;
    bool idNeedsRecompute;
};
}

#endif /* MATERIALDEFINITION_HPP */

