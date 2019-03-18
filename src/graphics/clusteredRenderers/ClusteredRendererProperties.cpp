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

#include "graphics/clusteredRenderers/ClusteredRendererProperties.hpp"
#include "graphics/clusteredRenderers/ClusteredRendererConstants.hpp"

#include <sstream>
#include <vector>

namespace iyf {
ClusteredRendererProperties::ClusteredRendererProperties()
 : RendererProperties("ForwardClustered", HS("forward_clustered_renderer"), false, true) {}

std::string ClusteredRendererProperties::makeRenderDataSet(ShaderLanguage language) const {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Only GLSLVulkan is supported by this renderer");
    }
    
    std::stringstream ss;
    ss << "struct Cluster {\n";
    ss << "    uint offset;\n";
    ss << "    uint lightCount;\n";
    ss << "};\n\n";
    
    ss << "layout(std" << "430" << ", set = " << con::RendererDataBuffer.set << ", binding = " << con::RendererDataBuffer.binding << ") buffer ClusterDataBuffer {\n";
    ss << "    vec4 gridParameters;\n";
    ss << "    Cluster clusters[" << MaxClustersName << "];\n";
    ss << "    uint lightIDs[" << MaxLightIDsName << "];\n";
    ss << "} clusterData;\n";
    
    return ss.str();
}

std::string ClusteredRendererProperties::makeLightLoops(ShaderLanguage language, const std::string& lightingFunction) const {
    if (language != ShaderLanguage::GLSLVulkan) {
        throw std::runtime_error("Only GLSLVulkan is supported by this renderer");
    }
    
    std::stringstream ss;
    
    ss << "    // WARNING - TEST ONLY!!! These are poorly performing regular forward rendering loops.\n";
    ss << "    // Directional lights\n";
    ss << "    for (int i = 0; i < cameraAndLights.directionalLightCount; ++i) {\n";
    ss << "        vec3 lightDirection = cameraAndLights.directionalLights[i].direction;\n";
    ss << "        vec3 lightColor = cameraAndLights.directionalLights[i].color;\n";
    ss << "        float lightIntensity = cameraAndLights.directionalLights[i].intensity;\n";
    ss << "\n";
    ss << "        " << lightingFunction << "\n";
    ss << "    }\n\n";
    
//         float falloff = clamp(1.0f - (DdivR * DdivR * DdivR * DdivR), 0.0f, 1.0f);
//         falloff *= falloff;
//         falloff = falloff / (lightDistance * lightDistance + 1);
// 
//         if(falloff > 0.0f) {
//             color.rgb += falloff * cookTorranceGGX(normal, viewDirection, lightDirection, lightsAndCamera.lightData[i].color.xyz, diffuseColor, 1.0f, 0.1f, 0.3f); // Point specular + diffuse
//         }
    ss << "    // Point lights\n";
    ss << "    for (int i = 0; i < cameraAndLights.pointLightCount; ++i) {\n";
    ss << "        vec3 lightDirection = normalize(cameraAndLights.pointLights[i].position - fragmentInput.positionWS);\n";
    ss << "        float lightDistance = length(cameraAndLights.pointLights[i].position - fragmentInput.positionWS);\n\n";
    ss << "        float DdivR = lightDistance / cameraAndLights.pointLights[i].radius;\n";
    ss << "        float falloff = clamp(1.0f - (DdivR * DdivR * DdivR * DdivR), 0.0f, 1.0f);\n";
    ss << "        falloff *= falloff;\n";
    ss << "        falloff = falloff / (lightDistance * lightDistance + 1);\n\n";
    ss << "        vec3 lightColor = cameraAndLights.pointLights[i].color;\n";
    ss << "        float lightIntensity = cameraAndLights.pointLights[i].intensity * falloff;\n\n";
    ss << "        if (falloff > 0.0f) {\n";
    ss << "            " << lightingFunction << "\n";
    ss << "        }\n";
    ss << "    }\n\n";
    
    // TODO implement spotlights
    ss << "   // TODO implement spotlights\n\n";
    
    return ss.str();
}

inline std::vector<SpecializationConstant> MakeClusteredRendererSpecializationConstants() {
    std::vector<SpecializationConstant> specializationConstants(con::DefaultSpecializationConstants.begin(), con::DefaultSpecializationConstants.end());
    specializationConstants.emplace_back(MaxClustersName, Format::R32_uInt, MaxClusters);
    specializationConstants.emplace_back(MaxLightIDsName, Format::R32_uInt, MaxLightIDs);
    
    return specializationConstants;
}

const std::vector<SpecializationConstant>& ClusteredRendererProperties::getShaderSpecializationConstants() const {
    static std::vector<SpecializationConstant> specializationConstants = MakeClusteredRendererSpecializationConstants();
    
    return specializationConstants;
}

}
