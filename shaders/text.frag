#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    float alpha = texture(texSampler, fragUV).r;
    outColor = vec4(1.0, 1.0, 1.0, alpha); // White text, alpha from atlas
}