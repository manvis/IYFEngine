#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec4 vertexTangent;
layout(location = 2) in vec4 vertexBitangent;
layout(location = 3) in vec4 vertexNormal;
layout(location = 4) in vec2 vertexUV;

layout (location = 0) out VertexData {
    vec3 positionWorld;
    vec2 UV;
    mat3 TBN;
} vertOut;

#ifdef VULKAN
layout(std140, push_constant) uniform matrixPushConstants {
	mat4 MVP;
    mat4 M;
} matrices;
#else
layout (std140, binding = 1) uniform matrixBuffer {
    mat4 MVP;
    mat4 M;
} matrices;
#endif

out gl_PerVertex {
    vec4 gl_Position;
};

void main () {
    gl_Position = matrices.MVP * vec4(vertexPosition, 1.0f);
    vertOut.positionWorld = (matrices.M * vec4(vertexPosition, 1.0f)).xyz;

    vertOut.UV = vertexUV.xy;
	
    vec3 normalWorld = mat3(matrices.M) * vertexNormal.xyz;
    vec3 tangentWorld = mat3(matrices.M) * vertexTangent.xyz;
    vec3 bitnagentWorld = mat3(matrices.M) * vertexBitangent.xyz;
    
    vec3 normalizedTangent = normalize(tangentWorld);
    vec3 normalizedBitangent = normalize(bitnagentWorld);
    vec3 normalizedNormal = normalize(normalWorld);
    
    vertOut.TBN = mat3(normalizedTangent, normalizedBitangent, normalizedNormal);
}
