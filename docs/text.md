


```c
void TextSetupSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    ecs_print(1, "TextSetupSystem starting...");
    createFontAtlas(ctx);

    // Create buffers with enough space for "Hello World" (11 chars * 4 vertices + 6 indices per char)
    createBuffer(ctx, 44 * sizeof(TextVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->vertexBuffer, &ctx->vertexBufferMemory);
    createBuffer(ctx, 66 * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->triIndexBuffer, &ctx->triIndexBufferMemory);

    ecs_print(1, "TextSetupSystem completed");
}
```