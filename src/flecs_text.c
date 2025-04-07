#include "flecs_text.h"
#include <flecs.h>
#include <string.h>
#include "text_vert.spv.h"
#include "text_frag.spv.h"

typedef struct {
    float pos[2];    // 2D position
    float uv[2];     // Texture coordinates
} TextVertex;

typedef struct {
    float u0, v0, u1, v1; // UV coordinates
    int width, height;    // Glyph size
    int advanceX;         // Advance to next glyph
    int bearingX, bearingY; // Offset from baseline
} GlyphInfo;

static void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

static VkShaderModule createShaderModule(VkDevice device, const unsigned char *code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = codeSize;
    createInfo.pCode = (const uint32_t *)code;
    VkShaderModule module;
    if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
        ecs_err("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    return module;
}

static void createBuffer(WorldContext *ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        ecs_err("Failed to create buffer");
        ctx->hasError = true;
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(ctx->device, *buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memReqs.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(ctx->device, &allocInfo, NULL, memory) != VK_SUCCESS) {
        ecs_err("Failed to allocate buffer memory");
        ctx->hasError = true;
        return;
    }

    vkBindBufferMemory(ctx->device, *buffer, *memory, 0);
}

static void updateBuffer(WorldContext *ctx, VkDeviceMemory memory, VkDeviceSize size, void *data) {
    void *mapped;
    vkMapMemory(ctx->device, memory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(ctx->device, memory);
}

static void createFontAtlas(WorldContext *ctx) {
    FT_Library ft;
    FT_Face face;
    if (FT_Init_FreeType(&ft)) {
        ecs_err("Failed to initialize FreeType");
        ctx->hasError = true;
        ctx->errorMessage = "FreeType init failed";
        return;
    }

    if (FT_New_Face(ft, "assets/fonts/Kenney Mini.ttf", 0, &face)) {
        ecs_err("Failed to load Kenney Mini.ttf");
        FT_Done_FreeType(ft);
        ctx->hasError = true;
        ctx->errorMessage = "Font load failed";
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    const int atlasWidth = 512; // Increased size to fit more glyphs
    const int atlasHeight = 512;
    unsigned char *atlasData = calloc(atlasWidth * atlasHeight, sizeof(unsigned char));
    int x = 0, y = 0, maxHeight = 0;

    GlyphInfo glyphs[95];
    memset(glyphs, 0, sizeof(glyphs));

    for (unsigned char c = 32; c < 127; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        if (x + (int)face->glyph->bitmap.width >= atlasWidth) {
            x = 0;
            y += maxHeight + 1;
            maxHeight = 0;
        }

        for (unsigned int i = 0; i < face->glyph->bitmap.rows; i++) {
            for (unsigned int j = 0; j < face->glyph->bitmap.width; j++) {
                int atlasX = x + j;
                int atlasY = y + i;
                if (atlasX < atlasWidth && atlasY < atlasHeight) {
                    atlasData[atlasY * atlasWidth + atlasX] = face->glyph->bitmap.buffer[i * face->glyph->bitmap.width + j];
                }
            }
        }

        glyphs[c - 32].u0 = (float)x / atlasWidth;
        glyphs[c - 32].v0 = (float)y / atlasHeight;
        glyphs[c - 32].u1 = (float)(x + face->glyph->bitmap.width) / atlasWidth;
        glyphs[c - 32].v1 = (float)(y + face->glyph->bitmap.rows) / atlasHeight;
        glyphs[c - 32].width = face->glyph->bitmap.width;
        glyphs[c - 32].height = face->glyph->bitmap.rows;
        glyphs[c - 32].advanceX = face->glyph->advance.x >> 6;
        glyphs[c - 32].bearingX = face->glyph->bitmap_left;
        glyphs[c - 32].bearingY = face->glyph->bitmap_top;

        x += face->glyph->bitmap.width + 1;
        maxHeight = face->glyph->bitmap.rows > maxHeight ? face->glyph->bitmap.rows : maxHeight;
    }

    ctx->glyphs = malloc(sizeof(glyphs));
    memcpy(ctx->glyphs, glyphs, sizeof(glyphs));
    ctx->atlasWidth = atlasWidth;
    ctx->atlasHeight = atlasHeight;

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.extent.width = atlasWidth;
    imageInfo.extent.height = atlasHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(ctx->device, &imageInfo, NULL, &ctx->fontImage) != VK_SUCCESS) {
        ecs_err("Failed to create font atlas image");
        free(atlasData);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(ctx->device, ctx->fontImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memReqs.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->fontImageMemory) != VK_SUCCESS) {
        vkDestroyImage(ctx->device, ctx->fontImage, NULL);
        free(atlasData);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        return;
    }

    vkBindImageMemory(ctx->device, ctx->fontImage, ctx->fontImageMemory, 0);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(ctx, atlasWidth * atlasHeight, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);
    updateBuffer(ctx, stagingMemory, atlasWidth * atlasHeight, atlasData);

    VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = ctx->commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &commandBuffer) != VK_SUCCESS) {
        ecs_err("Failed to allocate one-time command buffer");
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        ecs_err("Failed to begin one-time command buffer");
        vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
        return;
    }

    transitionImageLayout(commandBuffer, ctx->fontImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = atlasWidth;
    region.imageExtent.height = atlasHeight;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, ctx->fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImageLayout(commandBuffer, ctx->fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        ecs_err("Failed to end one-time command buffer");
        vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
        return;
    }

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        ecs_err("Failed to submit one-time command buffer");
    }
    vkQueueWaitIdle(ctx->graphicsQueue);

    vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
    vkFreeMemory(ctx->device, stagingMemory, NULL);
    vkDestroyBuffer(ctx->device, stagingBuffer, NULL);

    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = ctx->fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->fontImageView) != VK_SUCCESS) {
        ecs_err("Failed to create font image view");
    }

    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(ctx->device, &samplerInfo, NULL, &ctx->fontSampler) != VK_SUCCESS) {
        ecs_err("Failed to create font sampler");
    }

    free(atlasData);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}



void TextSetupSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "TextSetupSystem starting...");
  createFontAtlas(ctx);

  // Only create text buffers if they don’t already exist
  if (!ctx->textVertexBuffer) {
      createBuffer(ctx, 44 * sizeof(TextVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->textVertexBuffer, &ctx->textVertexBufferMemory);
  }
  if (!ctx->textIndexBuffer) {
      createBuffer(ctx, 66 * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->textIndexBuffer, &ctx->textIndexBufferMemory);
  }

  ecs_print(1, "TextSetupSystem completed");
}


void TextPipelineSetupSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    ecs_print(1, "TextPipelineSetupSystem starting...");

    VkShaderModule vertShaderModule = createShaderModule(ctx->device, (const unsigned char *)text_vert_spv, sizeof(text_vert_spv));
    VkShaderModule fragShaderModule = createShaderModule(ctx->device, (const unsigned char *)text_frag_spv, sizeof(text_frag_spv));
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        ecs_err("Failed to create text shader modules");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create text shader modules";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    VkPipelineShaderStageCreateInfo vertStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module = vertShaderModule;
    vertStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module = fragShaderModule;
    fragStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    VkVertexInputBindingDescription bindingDesc = {0};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(TextVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[2] = {0};
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[0].offset = offsetof(TextVertex, pos);
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = offsetof(TextVertex, uv);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL, &ctx->textDescriptorSetLayout) != VK_SUCCESS) {
        ecs_err("Failed to create text descriptor set layout");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create text descriptor set layout";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &ctx->textDescriptorSetLayout;
    if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->textPipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create text pipeline layout");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create text pipeline layout";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = ctx->textPipelineLayout;
    pipelineInfo.renderPass = ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->textPipeline) != VK_SUCCESS) {
        ecs_err("Failed to create text graphics pipeline");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create text graphics pipeline";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);

    ecs_print(1, "TextPipelineSetupSystem completed");
}


void TextRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError || !ctx->glyphs) return;

  ecs_print(1, "TextRenderSystem starting...");

  const char *text = "Hello World";
  size_t textLen = strlen(text);
  TextVertex vertices[44];
  uint32_t indices[66];
  int vertexCount = 0, indexCount = 0;

  GlyphInfo *glyphs = (GlyphInfo *)ctx->glyphs;
  float totalWidth = 0.0f;
  for (size_t i = 0; i < textLen; i++) {
      if (text[i] < 32 || text[i] > 126) continue;
      int glyphIdx = text[i] - 32;
      totalWidth += glyphs[glyphIdx].advanceX;
  }
  float x = (800.0f - totalWidth) / 2.0f; // Center for 800x600
  float y = 600.0f / 2.0f;

  for (size_t i = 0; i < textLen; i++) {
      char c = text[i];
      if (c < 32 || c > 126) continue;

      int glyphIdx = c - 32;
      float x0 = x + glyphs[glyphIdx].bearingX;
      float y0 = y - glyphs[glyphIdx].bearingY;
      float x1 = x0 + glyphs[glyphIdx].width;
      float y1 = y0 + glyphs[glyphIdx].height;

      vertices[vertexCount + 0] = (TextVertex){{x0, y0}, {glyphs[glyphIdx].u0, glyphs[glyphIdx].v0}};
      vertices[vertexCount + 1] = (TextVertex){{x1, y0}, {glyphs[glyphIdx].u1, glyphs[glyphIdx].v0}};
      vertices[vertexCount + 2] = (TextVertex){{x1, y1}, {glyphs[glyphIdx].u1, glyphs[glyphIdx].v1}};
      vertices[vertexCount + 3] = (TextVertex){{x0, y1}, {glyphs[glyphIdx].u0, glyphs[glyphIdx].v1}};

      indices[indexCount + 0] = vertexCount + 0;
      indices[indexCount + 1] = vertexCount + 1;
      indices[indexCount + 2] = vertexCount + 2;
      indices[indexCount + 3] = vertexCount + 2;
      indices[indexCount + 4] = vertexCount + 3;
      indices[indexCount + 5] = vertexCount + 0;

      x += glyphs[glyphIdx].advanceX;
      vertexCount += 4;
      indexCount += 6;
  }

  updateBuffer(ctx, ctx->textVertexBufferMemory, vertexCount * sizeof(TextVertex), vertices);
  updateBuffer(ctx, ctx->textIndexBufferMemory, indexCount * sizeof(uint32_t), indices);

  VkDescriptorSet descriptorSet;
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = ctx->descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &ctx->textDescriptorSetLayout;
  if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
      ecs_err("Failed to allocate text descriptor set");
      return;
  }

  VkDescriptorImageInfo imageInfo = {0};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = ctx->fontImageView;
  imageInfo.sampler = ctx->fontSampler;

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, NULL);

  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->textPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->textVertexBuffer, offsets);
  vkCmdBindIndexBuffer(ctx->commandBuffer, ctx->textIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->textPipelineLayout, 0, 1, &descriptorSet, 0, NULL);
  vkCmdDrawIndexed(ctx->commandBuffer, indexCount, 1, 0, 0, 0);

  ecs_print(1, "TextRenderSystem completed");
}



void flecs_text_cleanup(WorldContext *ctx) {
  if (!ctx || !ctx->device) return;

  ecs_print(1, "Text cleanup starting...");
  vkDeviceWaitIdle(ctx->device);

  if (ctx->textPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(ctx->device, ctx->textPipeline, NULL);
      ctx->textPipeline = VK_NULL_HANDLE;
  }
  if (ctx->textPipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(ctx->device, ctx->textPipelineLayout, NULL);
      ctx->textPipelineLayout = VK_NULL_HANDLE;
  }
  if (ctx->textDescriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(ctx->device, ctx->textDescriptorSetLayout, NULL);
      ctx->textDescriptorSetLayout = VK_NULL_HANDLE;
  }
  if (ctx->fontSampler != VK_NULL_HANDLE) {
      vkDestroySampler(ctx->device, ctx->fontSampler, NULL);
      ctx->fontSampler = VK_NULL_HANDLE;
  }
  if (ctx->fontImageView != VK_NULL_HANDLE) {
      vkDestroyImageView(ctx->device, ctx->fontImageView, NULL);
      ctx->fontImageView = VK_NULL_HANDLE;
  }
  if (ctx->fontImageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(ctx->device, ctx->fontImageMemory, NULL);
      ctx->fontImageMemory = VK_NULL_HANDLE;
  }
  if (ctx->fontImage != VK_NULL_HANDLE) {
      vkDestroyImage(ctx->device, ctx->fontImage, NULL);
      ctx->fontImage = VK_NULL_HANDLE;
  }
  if (ctx->textVertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(ctx->device, ctx->textVertexBufferMemory, NULL);
      ctx->textVertexBufferMemory = VK_NULL_HANDLE;
  }
  if (ctx->textVertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(ctx->device, ctx->textVertexBuffer, NULL);
      ctx->textVertexBuffer = VK_NULL_HANDLE;
  }
  if (ctx->textIndexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(ctx->device, ctx->textIndexBufferMemory, NULL);
      ctx->textIndexBufferMemory = VK_NULL_HANDLE;
  }
  if (ctx->textIndexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(ctx->device, ctx->textIndexBuffer, NULL);
      ctx->textIndexBuffer = VK_NULL_HANDLE;
  }
  if (ctx->glyphs) {
      free(ctx->glyphs);
      ctx->glyphs = NULL;
  }

  ecs_print(1, "Text cleanup completed");
}




void flecs_text_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing text module...");

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "TextSetupSystem", 
            .add = ecs_ids(ecs_dependson(GlobalPhases.CommandPoolSetupPhase)) 
        }),
        .callback = TextSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "TextPipelineSetupSystem", 
            .add = ecs_ids(ecs_dependson(GlobalPhases.PipelineSetupPhase)) 
        }),
        .callback = TextPipelineSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "TextRenderSystem", 
            .add = ecs_ids(ecs_dependson(GlobalPhases.BeginCMDBufferPhase)) 
        }),
        .callback = TextRenderSystem
    });

    ecs_print(1, "Text module initialized");
}