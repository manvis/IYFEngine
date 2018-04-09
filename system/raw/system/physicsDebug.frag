#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in VertexData {
    vec4 color;
} vertexData;

layout (location = 0) out vec4 color;

void main() {
    color = vertexData.color;
}

