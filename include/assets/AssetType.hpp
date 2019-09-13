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

#ifndef IYF_ASSET_TYPE_HPP
#define IYF_ASSET_TYPE_HPP

#include <cstdint>

namespace iyf {

/// \brief Identifiers for different asset types.
///
/// \warning AssetType::ANY and AssetType::COUNT MUST NEVER BE USED IN THE ASSET DATABASE. Even if you add a custom asset type and then
/// decide to stop using it, keep it in the enum to avoid moving AssetType::ANY and breaking your Project
///
/// \warning When updating or changing this:
///   - update AssetTypeToPath;
///   - update AssetTypeToTranslationString;
///   - update the names of asset specific directories;
///   - update the extensions
///   - update the importers.
///   - update the Metadata.cpp (include the changes in the switch)
///
/// \warning Updating these values may break existing projects.
enum class AssetType : std::uint8_t {
    Animation = 0,
    Mesh = 1,
    Texture = 2,
    Font = 3,
    Audio = 4,
    Video = 5,
    Script = 6,
    Shader = 7,
    Strings = 8,
    Custom = 9,
    MaterialTemplate = 10,
    MaterialInstance = 11,
    COUNT,
    /// Any is equal to COUNT
    ANY = COUNT
};

}

#endif // IYF_ASSET_TYPE_HPP
