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

#ifndef IYF_MATERIAL_RENDER_MODE_HPP
#define IYF_MATERIAL_RENDER_MODE_HPP

#include "localization/TextLocalization.hpp"
#include <cstdint>

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


}

#endif // IYF_MATERIAL_RENDER_MODE_HPP
