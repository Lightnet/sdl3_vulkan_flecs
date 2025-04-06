

```c
static const Vertex vertices[] = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};

const char* vertexShaderCode = 
    "#version 450\n"
    "layout(location = 0) in vec2 inPosition;\n"
    "layout(location = 1) in vec3 inColor;\n"
    "layout(location = 0) out vec3 fragColor;\n"
    "void main() {\n"
    "    gl_Position = vec4(inPosition, 0.0, 1.0);\n"
    "    fragColor = inColor;\n"
    "}\0";

const char* fragmentShaderCode = 
    "#version 450\n"
    "layout(location = 0) in vec3 fragColor;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "    outColor = vec4(fragColor, 1.0);\n"
    "}\0";
```