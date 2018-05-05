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

#ifndef METADATA_HPP
#define METADATA_HPP

#include "assets/metadata/AnimationMetadata.hpp"
#include "assets/metadata/MeshMetadata.hpp"
#include "assets/metadata/TextureMetadata.hpp"
#include "assets/metadata/FontMetadata.hpp"
#include "assets/metadata/AudioMetadata.hpp"
#include "assets/metadata/VideoMetadata.hpp"
#include "assets/metadata/WorldMetadata.hpp"
#include "assets/metadata/ScriptMetadata.hpp"
#include "assets/metadata/ShaderMetadata.hpp"
#include "assets/metadata/PipelineMetadata.hpp"
#include "assets/metadata/StringMetadata.hpp"
#include "assets/metadata/CustomMetadata.hpp"
#include "assets/metadata/MaterialMetadata.hpp"
#include "utilities/ForceInline.hpp"

#include <variant>

namespace iyf {
/// We use a variant here because the Metadata is stored in a frequently accessed std::unordered_map. Using polymorphism would require pointers and, by
/// extension, an extra layer of indirection.
/// 
/// \warning The order of metadata elements must match con::AssetType
using Metadata = std::variant<AnimationMetadata, MeshMetadata, TextureMetadata, FontMetadata, AudioMetadata, VideoMetadata, WorldMetadata, ScriptMetadata,
                              ShaderMetadata, PipelineMetadata, StringMetadata, CustomMetadata, MaterialMetadata>;

IYF_FORCE_INLINE bool MetadataCorrespondsToAssetType(const Metadata& metadata, AssetType type) {
    return static_cast<std::size_t>(type) == metadata.index();
}
}

#endif /* METADATA_HPP */

