#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec4 color;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(std140, push_constant) uniform matrixPushConstants {
	mat4 P;
} matrix;

layout (location = 0) out VertexData {
    out vec2 UV;
    out vec4 color;
} vertexData;

void main() {
    vertexData.UV = UV;
	vertexData.color = color;
	gl_Position = matrix.P * vec4(position.xy, 0.0f, 1.0f);
}
