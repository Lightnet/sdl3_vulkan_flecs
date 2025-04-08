
Center Screen
```c
Create vertex and index buffers
Texture2DVertex vertices[] = {
    {{-0.5f, -0.5f}, {0.0f, 1.0f}}, // Bottom-left
    {{ 0.5f, -0.5f}, {1.0f, 1.0f}}, // Bottom-right
    {{ 0.5f,  0.5f}, {1.0f, 0.0f}}, // Top-right
    {{-0.5f,  0.5f}, {0.0f, 0.0f}}  // Top-left
};
uint32_t indices[] = {0, 1, 2, 2, 3, 0};
```
Left Screen
```c
Texture2DVertex vertices[] = {
  {{-1.0f, -0.5f}, {0.0f, 1.0f}}, // Bottom-left (shifted to left edge)
  {{-0.0f, -0.5f}, {1.0f, 1.0f}}, // Bottom-right (center of screen)
  {{-0.0f,  0.5f}, {1.0f, 0.0f}}, // Top-right (center of screen)
  {{-1.0f,  0.5f}, {0.0f, 0.0f}}  // Top-left (shifted to left edge)
};
uint32_t indices[] = {0, 1, 2, 2, 3, 0};
```