#version 450

layout(location = 0) in vec3 inPosition;   // 3D position from OBJ
layout(location = 1) in vec3 inColor;      // Vertex color
layout(location = 2) in vec2 inTexCoord;   // Texture coordinates

layout(location = 0) out vec3 fragColor;   // Output color to fragment shader
layout(location = 1) out vec2 fragTexCoord; // Output texture coordinates

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}