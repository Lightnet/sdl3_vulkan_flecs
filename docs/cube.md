

# Cube vertex:

```c
// Create Buffers
  CubeVertex vertices[] = {
      // Front face
      {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left
      {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}}, // Bottom-right
      {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // Top-right
      {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // Top-left
      // Back face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
      {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
      {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
      {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}}
  };
  uint32_t indices[] = {
      0, 3, 2, 2, 1, 0,  // Front (counterclockwise)
      4, 5, 6, 6, 7, 4,  // Back (counterclockwise)
      1, 2, 6, 6, 5, 1,  // Right (counterclockwise)
      0, 4, 7, 7, 3, 0,  // Left (counterclockwise)
      3, 7, 6, 6, 2, 3,  // Top (counterclockwise)
      0, 1, 5, 5, 4, 0   // Bottom (counterclockwise)
  };
```

```c
// Full cube rendering (all faces working, no debug needed unless issues persist)
  vkCmdDrawIndexed(ctx->commandBuffer, 36, 1, 0, 0, 0);

  // Debug face rendering (commented out, uncomment if needed)
  /*
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 0, 0, 0);   // Front
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 6, 0, 0);   // Back
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 12, 0, 0);  // Right
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 18, 0, 0);  // Left
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 24, 0, 0);  // Top
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 30, 0, 0);  // Bottom
  */
```