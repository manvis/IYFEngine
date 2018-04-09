#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(std140, push_constant) uniform matrixPushConstants {
    mat4 VP;
} matrix;

layout (location = 0) out VertexData {
    out vec4 color;
} vertexData;

void main() {
    vertexData.color = color;
    gl_Position = matrix.VP * vec4(position.xyz, 1.0f);
}

