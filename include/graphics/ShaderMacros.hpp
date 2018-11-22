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

#ifndef IYF_SHADER_MACROS_HPP
#define IYF_SHADER_MACROS_HPP

#include <variant>
#include <string>
#include <cstdint>

namespace iyf {
/// Identifiers for all shader macros used by the engine.
///
/// \remark While you may add custom macro definitions during compilation (they'll have ShaderMacroWithValue::getMacro() set to ShaderMacro::Custom).
/// it's preferable to delare them here and implement their validation in ValidateShaderMacroValue().
///
/// \warning Use explicit, sequential IDs. They're written to files and having them visible makes debugging easier. Adding IDs at the end (before Custom)
/// is safe. To reduce the chance of causing bugs, you should avoid removing, reusing or reordering these IDs.
/// 
/// \warning Do not change the underlying type
enum class ShaderMacro : std::uint16_t {
    VertexDataLayout            = 0, /// < DO NOT SET - set automatically by the ShaderGenerator. Must be an integer that corresponds to a VertexDataLayout used in the graphics pipeline.
    NormalSetByMaterialGraph    = 1, /// < DO NOT SET - set automatically by the MaterialLogicGraph. If defined, normal data is set by the MaterialLogicGraph (e.g., from normal map) and there's no need to set it again.
    NormalMappingMode           = 2, /// < DO NOT SET - set automatically by the ShaderGenerator. Integer. 0 - normal mapping disabled, 1 - all TBN components present, 2 - bitangent needs recovering. Depends on the VertexDataLayout
    NormalTextureChannelCount   = 3, /// < DO NOT SET - set automatically by the ShaderGenerator. Number of channels in normal textures. Depends on the platform
    WorldSpacePositionAvailable = 4, /// < DO NOT SET - set automatically by the ShaderGenerator. If defined, the vertex shader will compute a world space position and pass it to later stages.
    NormalAvailable             = 5, /// < DO NOT SET - set automatically by the ShaderGenerator. If defined, the vertex shader will pass the normal (or a TBN if NormalMapTextureAvailable is defined) to later stages. Depends on the VertexDataLayout.
    VertexColorAvailable        = 6, /// < DO NOT SET - set automatically by the ShaderGenerator. If defined and the vertices have colors, the vertex shader will pass them to later stages.
    TextureCoordinatesAvailable = 7, /// < DO NOT SET - set automatically by the ShaderGenerator. If defined and the vertices have texture coordinates, the vertex shader will pass them to later stages. Depends on the VertexDataLayout.
    Custom, /// < Special value, can be anything the user wants. Don't use in GetShaderMacroName(). Depends on the VertexDataLayout.
    COUNT = Custom
};

/// Returns a macro name that's used in the shader.
///
/// \warning Make sure to update shader helper includes if you change the names returned by this function
std::string GetShaderMacroName(ShaderMacro macro);

enum class ShaderInclude : std::uint16_t {
    CommonHelpers         = 0, /// < Common helper functions that get included by other helpers
    VertexShaderHelpers   = 1, /// < Helper functions that get included in the vertex shader
    FragmentShaderHelpers = 2, /// < Helper functions that get included in the fragment shader
};

std::string GetShaderIncludeName(ShaderInclude include);

using ShaderMacroValue = std::variant<std::monostate, double, std::int64_t, std::string>;
bool ValidateShaderMacroValue(ShaderMacro macro, const ShaderMacroValue& value);

class ShaderMacroWithValue {
public:
    /// Create a validated ShaderMacroWithValue.
    ///
    /// \throws std::logic_error if macro == ShaderMacro::Custom because it's impossible to know what custom values should contain.
    /// \throws std::invalid_argument if the value fails validation
    ShaderMacroWithValue(ShaderMacro macro, ShaderMacroValue value = std::monostate());
    
    /// Create a custom ShaderMacroWithValue.
    ShaderMacroWithValue(std::string name, ShaderMacroValue value = std::monostate());
    
    /// Empty constructor that set a special value CUSTOM
    ShaderMacroWithValue() : ShaderMacroWithValue("CUSTOM") {}
    
    inline ShaderMacro getMacroIdentifier() const {
        return macro;
    }
    
    inline const std::string& getName() const {
        return name;
    }
    
    inline const std::string getValue() const {
        return value;
    }
private:
    ShaderMacro macro;
    std::string name;
    std::string value;
};
}

#endif // IYF_SHADER_MACROS_HPP
