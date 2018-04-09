#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define MAX_LIGHTS 10000

layout (location = 0) in VertexData {
    vec3 positionWorld;
    vec2 UV;
    mat3 TBN;
} inData;

layout (location = 0) out vec4 color;

struct Light {
    vec4 positionRadius;
    vec4 color;
};

#ifdef VULKAN
layout(set = 0, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 0, binding = 1) uniform sampler2D normalTexture;

layout (std140, set = 1, binding = 0) buffer lightAndCameraDataBuffer {
    vec3 sunDirection;
    float padding1; // TODO vieta lights masyve, kur prasideda spot?
    vec3 sunColor;
    uint activeLightCount;
    vec3 cameraPos;
    float padding2;
    
    Light lightData[MAX_LIGHTS];
} lightsAndCamera;

#else
    layout (binding = 1) uniform sampler2D diffuseTexture;
#endif

// LT - Pagal
// EN - Based on
// http://www.filmicworlds.com/2014/04/21/optimizing-ggx-shaders-with-dotlh/
float G1V(float v, float k) {
    return v / (v * (1.0f - k) + k);
}

vec3 cookTorranceGGX(vec3 normal, vec3 viewDirection, vec3 lightDirection, vec3 lightColorIntensity, vec3 diffuseColor, float specularity, float F0, float roughnessValue) {
	vec3 halfVector = normalize(lightDirection + viewDirection);

    float NdotL = clamp(dot(lightDirection, normal), 0.0f, 1.0f);
    float LdotH = clamp(dot(lightDirection, halfVector), 0.0f, 1.0f);
    float NdotH = clamp(dot(normal, halfVector), 0.0f, 1.0f);
    float NdotV = clamp(dot(normal, viewDirection), 0.0f, 1.0f);
    float VdotH = clamp(dot(halfVector, viewDirection), 0.0f, 1.0f);

    float alpha = roughnessValue * roughnessValue;

    float F, D, V;

    float alphaSqr = alpha * alpha;
    float pi = 3.14159f;
    float denom = NdotH * NdotH * (alphaSqr - 1.0f) + 1.0f;
    D = alphaSqr / (pi * denom * denom);

    float LdotH5 = pow(1.0f - LdotH, 5);
    F = F0 + (1.0f - F0) * LdotH5;

    float k = (roughnessValue + 1) * (roughnessValue + 1) / 8.0f;
    V = G1V(NdotL, k) * G1V(NdotV, k);

    vec3 specular = vec3(NdotL * D * F * V);

    return lightColorIntensity * (specularity * specular + NdotL * diffuseColor);//specular;
}

void main () {
    // Skaitom paviršiaus spalvą
    vec3 diffuseColor = texture(diffuseTexture, inData.UV).rgb;
    
    // Skaitom iš normalmap ir atstatom į intervalą [-1; 1];
    vec2 normalXY = texture(normalTexture, inData.UV).xy;
    normalXY = 2.0f * normalXY - vec2(1.0f, 1.0f);
    
    // Atstatom paskutinę ašį pasinaudodami jų ortogonalumu. To reikia, nes duomenys šešėliuokliui paduodami BC5 formatu, kuris palaiko tik 2 kanalus.
    float normalZ = sqrt(1 - (normalXY.x * normalXY.x) - (normalXY.y * normalXY.y));
    vec3 normal = normalize(vec3(normalXY, normalZ));
    normal = normalize(inData.TBN * normal);
    
    vec3 viewDirection = normalize(lightsAndCamera.cameraPos - inData.positionWorld);
    
    color.rgb = vec3(0.05f, 0.05f, 0.05f) * diffuseColor; // ambient
    color.rgb += cookTorranceGGX(normal, viewDirection, lightsAndCamera.sunDirection, lightsAndCamera.sunColor, diffuseColor, 1.0f, 0.1f, 0.3f); // Directional specular + diffuse //TODO remove specularity
    
    for(int i = 0; i < lightsAndCamera.activeLightCount; ++i) {
        vec3 lightDirection = normalize(lightsAndCamera.lightData[i].positionRadius.xyz - inData.positionWorld);
        float lightDistance = length(lightsAndCamera.lightData[i].positionRadius.xyz - inData.positionWorld);
        
        //"Real Shading in Unreal Engine 4"
        // http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
        float DdivR = lightDistance / lightsAndCamera.lightData[i].positionRadius.w;
        float falloff = clamp(1.0f - (DdivR * DdivR * DdivR * DdivR), 0.0f, 1.0f);
        falloff *= falloff;
        falloff = falloff / (lightDistance * lightDistance + 1);

        if(falloff > 0.0f) {
            color.rgb += falloff * cookTorranceGGX(normal, viewDirection, lightDirection, lightsAndCamera.lightData[i].color.xyz, diffuseColor, 1.0f, 0.1f, 0.3f); // Point specular + diffuse
        }
    }

    color.a = 1.0f;
}
