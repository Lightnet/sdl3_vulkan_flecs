#version 450
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 fragUV;

void main() {
    vec2 ndc = (inPosition / vec2(800.0, 600.0)) * 2.0 - 1.0; // Adjust to WIDTH, HEIGHT
    gl_Position = vec4(ndc, 0.0, 1.0);
    fragUV = inUV;
}