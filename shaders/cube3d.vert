#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(binding = 0) uniform Matrices {
    mat4 model;
    mat4 view;
    mat4 proj;
} matrices;

void main() {
    gl_Position = matrices.proj * matrices.view * matrices.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}