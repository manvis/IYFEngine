#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in VertexData {
    vec3 positionWorld;
    vec2 UV;
    mat3 TBN;
} inData;

layout (location = 0) out vec4 color;

void main () {
    color = vec4(inData.positionWorld, 0.0f);
}

