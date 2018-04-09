#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 position;

layout(std140, push_constant) uniform matrixPushConstants {
	mat4 MVP;
} matrices;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out VertexData {
    vec3 UVW;
} vertOut;

void main() {
    vec4 finalPosition = matrices.MVP * vec4(position, 1.0f);
    
    // This is for normal Z
    //gl_Position = finalPosition.xyww;
    // This is for a reversed Z buffer that we use
    gl_Position = vec4(finalPosition.xy, 0.0f, finalPosition.w);
    
    vertOut.UVW = position;
}
