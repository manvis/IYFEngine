#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in VertexData {
    vec3 UVW;
} inData;

layout (location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform samplerCube skyTexture;

void main() {
    color.rgb = texture(skyTexture, inData.UVW).rgb;
    color.a = 1.0f;
}
