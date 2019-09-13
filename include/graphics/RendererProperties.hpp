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

#ifndef IYF_RENDERER_PROPERTIES_HPP
#define IYF_RENDERER_PROPERTIES_HPP

#include "graphics/RendererConstants.hpp"
#include "graphics/ShaderConstants.hpp"
#include "utilities/hashing/Hashing.hpp"
#include "utilities/NonCopyable.hpp"

#include <string>
#include <vector>

namespace iyf {
class RendererProperties : private NonCopyable {
public:
    virtual ~RendererProperties() {}
    
    /// A simple, filename friendly string (no spaces or special characters) that can be used by the shader generator or in logs.
    const std::string name;
    
    /// A localization handle that can be used to retrieve a "pretty" name 
    const StringHash localizationHandle;
    
    /// Indicates if this renderer uses one pass to build G-Buffers and another to compute the final
    /// shading of the surfaces or not. If this bool is true, compatible ShadingPipeline objects
    /// must have Shaders for both passes defined.
    ///
    /// NOTE: this does NOT imply that only two passes will be used when this is set and one when
    /// it isn't. E.g., some renderers may use an early Z pass.
    const bool usesSeparateShadingPass;
    
    /// Indicates if it's possible to use multiple lighting models in a compatible ShadingPipeline.
    /// Forward renderers typically can do this without any modifications, while deferred ones can
    /// achieve similar results by writing the material IDs to their G-buffers.
    const bool canUseMultipleLightingModels;
    
    /// The returned vector contains a copy of con::DefaultSpecializationConstants array with Renderer specific
    /// constants (if any) appended at the end.
    virtual const std::vector<SpecializationConstant>& getShaderSpecializationConstants() const = 0;
    
    /// Used by the shader generator. Returns a string that contains all structs and buffer layout definitions
    /// that can be used to store data specific to a Renderer (e.g., various acceleration structs).
    virtual std::string makeRenderDataSet(ShaderLanguage language) const = 0;
    
    /// Used by the shader generator. Returns the loops that process visible lights.
    virtual std::string makeLightLoops(ShaderLanguage language, const std::string& lightingFunction) const = 0;
protected:
    friend class Renderer;
    
    RendererProperties(std::string name, StringHash localizationHandle,
                       bool usesSeparateShadingPass, bool canUseMultipleLightingModels);
};
}

#endif // IYF_RENDERER_PROPERTIES_HPP
