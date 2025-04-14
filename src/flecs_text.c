#include "flecs_text.h"
#include <flecs.h>
#include <string.h>
#include "shaders/text_vert.spv.h"
#include "shaders/text_frag.spv.h"
#include "flecs_utils.h" // createShaderModuleLen(v_ctx->device, text_vert_spv)
#include "flecs_sdl.h"
#include "flecs_vulkan.h"

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

// static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
//   VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
//   createInfo.codeSize = codeSize;
//   createInfo.pCode = code;
//   VkShaderModule module;
//   if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
//       ecs_err("Failed to create shader module");
//       return VK_NULL_HANDLE;
//   }
//   return module;
// }

// Unchanged: transitionImageLayout, createShaderModule
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

static void createBuffer(VulkanContext *v_ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(v_ctx->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        ecs_err("Failed to create buffer");
        v_ctx->hasError = true;
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(v_ctx->device, *buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memReqs.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(v_ctx->device, &allocInfo, NULL, memory) != VK_SUCCESS) {
        ecs_err("Failed to allocate buffer memory");
        v_ctx->hasError = true;
        return;
    }

    vkBindBufferMemory(v_ctx->device, *buffer, *memory, 0);
}

static void updateBuffer(VulkanContext *v_ctx, VkDeviceMemory memory, VkDeviceSize size, void *data) {
    void *mapped;
    vkMapMemory(v_ctx->device, memory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(v_ctx->device, memory);
}

static void createFontAtlas(VulkanContext *v_ctx, Text2DContext *text_ctx) {
    FT_Library ft;
    FT_Face face;
    if (FT_Init_FreeType(&ft)) {
        ecs_err("Failed to initialize FreeType");
        v_ctx->hasError = true;
        v_ctx->errorMessage = "FreeType init failed";
        return;
    }

    if (FT_New_Face(ft, "assets/fonts/Kenney Mini.ttf", 0, &face)) {
        ecs_err("Failed to load Kenney Mini.ttf");
        FT_Done_FreeType(ft);
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Font load failed";
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    const int textAtlasWidth = 512;
    const int textAtlasHeight = 512;
    unsigned char *atlasData = calloc(textAtlasWidth * textAtlasHeight, sizeof(unsigned char));
    int x = 0, y = 0;
    unsigned int maxHeight = 0;

    GlyphInfo textGlyphs[95];
    memset(textGlyphs, 0, sizeof(textGlyphs));

    for (unsigned char c = 32; c < 127; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        if (x + (int)face->glyph->bitmap.width >= textAtlasWidth) {
            x = 0;
            y += maxHeight + 1;
            maxHeight = 0;
        }

        for (unsigned int i = 0; i < face->glyph->bitmap.rows; i++) {
            for (unsigned int j = 0; j < face->glyph->bitmap.width; j++) {
                int atlasX = x + j;
                int atlasY = y + i;
                if (atlasX < textAtlasWidth && atlasY < textAtlasHeight) {
                    atlasData[atlasY * textAtlasWidth + atlasX] = face->glyph->bitmap.buffer[i * face->glyph->bitmap.width + j];
                }
            }
        }

        textGlyphs[c - 32].u0 = (float)x / textAtlasWidth;
        textGlyphs[c - 32].v0 = (float)y / textAtlasHeight;
        textGlyphs[c - 32].u1 = (float)(x + face->glyph->bitmap.width) / textAtlasWidth;
        textGlyphs[c - 32].v1 = (float)(y + face->glyph->bitmap.rows) / textAtlasHeight;
        textGlyphs[c - 32].width = face->glyph->bitmap.width;
        textGlyphs[c - 32].height = face->glyph->bitmap.rows;
        textGlyphs[c - 32].advanceX = face->glyph->advance.x >> 6;
        textGlyphs[c - 32].bearingX = face->glyph->bitmap_left;
        textGlyphs[c - 32].bearingY = face->glyph->bitmap_top;

        x += face->glyph->bitmap.width + 1;
        maxHeight = face->glyph->bitmap.rows > maxHeight ? face->glyph->bitmap.rows : maxHeight;
    }

    text_ctx->textGlyphs = malloc(sizeof(textGlyphs));
    memcpy(text_ctx->textGlyphs, textGlyphs, sizeof(textGlyphs));
    text_ctx->textAtlasWidth = textAtlasWidth;
    text_ctx->textAtlasHeight = textAtlasHeight;

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.extent.width = textAtlasWidth;
    imageInfo.extent.height = textAtlasHeight;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(v_ctx->device, &imageInfo, NULL, &text_ctx->textFontImage) != VK_SUCCESS) {
        ecs_err("Failed to create font atlas image");
        free(atlasData);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Font atlas image creation failed";
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(v_ctx->device, text_ctx->textFontImage, &memReqs);
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (memReqs.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(v_ctx->device, &allocInfo, NULL, &text_ctx->textFontImageMemory) != VK_SUCCESS) {
        vkDestroyImage(v_ctx->device, text_ctx->textFontImage, NULL);
        free(atlasData);
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Font atlas memory allocation failed";
        return;
    }

    vkBindImageMemory(v_ctx->device, text_ctx->textFontImage, text_ctx->textFontImageMemory, 0);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(v_ctx, textAtlasWidth * textAtlasHeight, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);
    updateBuffer(v_ctx, stagingMemory, textAtlasWidth * textAtlasHeight, atlasData);

    VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = v_ctx->commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(v_ctx->device, &cmdAllocInfo, &commandBuffer) != VK_SUCCESS) {
        ecs_err("Failed to allocate one-time command buffer");
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Command buffer allocation failed";
        return;
    }

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        ecs_err("Failed to begin one-time command buffer");
        vkFreeCommandBuffers(v_ctx->device, v_ctx->commandPool, 1, &commandBuffer);
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Command buffer begin failed";
        return;
    }

    transitionImageLayout(commandBuffer, text_ctx->textFontImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = textAtlasWidth;
    region.imageExtent.height = textAtlasHeight;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, text_ctx->textFontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    transitionImageLayout(commandBuffer, text_ctx->textFontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        ecs_err("Failed to end one-time command buffer");
        vkFreeCommandBuffers(v_ctx->device, v_ctx->commandPool, 1, &commandBuffer);
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Command buffer end failed";
        return;
    }

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (vkQueueSubmit(v_ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        ecs_err("Failed to submit one-time command buffer");
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Command buffer submit failed";
    }
    vkQueueWaitIdle(v_ctx->graphicsQueue);

    vkFreeCommandBuffers(v_ctx->device, v_ctx->commandPool, 1, &commandBuffer);
    vkFreeMemory(v_ctx->device, stagingMemory, NULL);
    vkDestroyBuffer(v_ctx->device, stagingBuffer, NULL);

    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = text_ctx->textFontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(v_ctx->device, &viewInfo, NULL, &text_ctx->textFontImageView) != VK_SUCCESS) {
        ecs_err("Failed to create font image view");
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Font image view creation failed";
    }

    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(v_ctx->device, &samplerInfo, NULL, &text_ctx->textFontSampler) != VK_SUCCESS) {
        ecs_err("Failed to create font sampler");
        v_ctx->hasError = true;
        v_ctx->errorMessage = "Font sampler creation failed";
    }

    free(atlasData);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void TextSetupSystem(ecs_iter_t *it) {
    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError) return;
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    Text2DContext *text_ctx = ecs_singleton_ensure(it->world, Text2DContext);
    if (!text_ctx) return;

    ecs_log(1, "TextSetupSystem starting...");

    // Create text-specific descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(v_ctx->device, &poolInfo, NULL, &text_ctx->textDescriptorPool) != VK_SUCCESS) {
        ecs_err("Failed to create text descriptor pool");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create text descriptor pool";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    // Create descriptor set layout
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(v_ctx->device, &layoutInfo, NULL, &text_ctx->textDescriptorSetLayout) != VK_SUCCESS) {
        ecs_err("Failed to create text descriptor set layout");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create text descriptor set layout";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = text_ctx->textDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &text_ctx->textDescriptorSetLayout;
    if (vkAllocateDescriptorSets(v_ctx->device, &allocInfo, &text_ctx->textDescriptorSet) != VK_SUCCESS) {
        ecs_err("Failed to allocate text descriptor set");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to allocate text descriptor set";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    // Create font atlas
    createFontAtlas(v_ctx, text_ctx);
    if (text_ctx->textFontImageView == VK_NULL_HANDLE || text_ctx->textFontSampler == VK_NULL_HANDLE) {
        ecs_err("Font atlas creation failed: ImageView=%p, Sampler=%p", (void*)text_ctx->textFontImageView, (void*)text_ctx->textFontSampler);
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Font atlas creation failed";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    // Update descriptor set
    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = text_ctx->textFontImageView;
    imageInfo.sampler = text_ctx->textFontSampler;

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = text_ctx->textDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(v_ctx->device, 1, &descriptorWrite, 0, NULL);

    // Create buffers
    if (!text_ctx->textVertexBuffer) {
        createBuffer(v_ctx, 44 * sizeof(TextVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &text_ctx->textVertexBuffer, &text_ctx->textVertexBufferMemory);
    }
    if (!text_ctx->textIndexBuffer) {
        createBuffer(v_ctx, 66 * sizeof(uint32_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &text_ctx->textIndexBuffer, &text_ctx->textIndexBufferMemory);
    }

    // Shader and pipeline setup
    // VkShaderModule vertShaderModule = createShaderModule(v_ctx->device, text_vert_spv, sizeof(text_vert_spv));
    // VkShaderModule fragShaderModule = createShaderModule(v_ctx->device, text_frag_spv, sizeof(text_frag_spv));
    VkShaderModule vertShaderModule = createShaderModuleH(v_ctx->device, text_vert_spv, sizeof(text_vert_spv));
    VkShaderModule fragShaderModule = createShaderModuleH(v_ctx->device, text_frag_spv, sizeof(text_frag_spv));


    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        ecs_err("Failed to create text shader modules");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create text shader modules";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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

    VkViewport viewport = {0.0f, 0.0f, (float)sdl_ctx->width, (float)sdl_ctx->height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {sdl_ctx->width, sdl_ctx->height}};

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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &text_ctx->textDescriptorSetLayout;
    if (vkCreatePipelineLayout(v_ctx->device, &pipelineLayoutInfo, NULL, &text_ctx->textPipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create text pipeline layout");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create text pipeline layout";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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
    pipelineInfo.layout = text_ctx->textPipelineLayout;
    pipelineInfo.renderPass = v_ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(v_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &text_ctx->textPipeline) != VK_SUCCESS) {
        ecs_err("Failed to create text graphics pipeline");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create text graphics pipeline";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    vkDestroyShaderModule(v_ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(v_ctx->device, vertShaderModule, NULL);

    ecs_log(1, "TextSetupSystem completed");
}

void TextRenderSystem(ecs_iter_t *it) {
    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    Text2DContext *text_ctx = ecs_singleton_ensure(it->world, Text2DContext);
    if (!text_ctx || !text_ctx->textGlyphs) return;

    const char *text = "Hello World";
    size_t textLen = strlen(text);
    TextVertex vertices[44];
    uint32_t indices[66];
    int vertexCount = 0, indexCount = 0;

    GlyphInfo *textGlyphs = text_ctx->textGlyphs;
    float totalWidth = 0.0f;
    for (size_t i = 0; i < textLen; i++) {
        if (text[i] < 32 || text[i] > 126) continue;
        int glyphIdx = text[i] - 32;
        totalWidth += textGlyphs[glyphIdx].advanceX;
    }

    float screenWidth = (float)sdl_ctx->width;
    float screenHeight = (float)sdl_ctx->height;
    float x = (screenWidth - totalWidth) / 2.0f;
    float y = screenHeight / 2.0f;
    float ndcX = (2.0f * x / screenWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / screenHeight);

    for (size_t i = 0; i < textLen; i++) {
        char c = text[i];
        if (c < 32 || c > 126) continue;

        int glyphIdx = c - 32;
        float x0 = ndcX + (2.0f * textGlyphs[glyphIdx].bearingX / screenWidth);
        float y0 = ndcY - (2.0f * textGlyphs[glyphIdx].bearingY / screenHeight);
        float x1 = x0 + (2.0f * textGlyphs[glyphIdx].width / screenWidth);
        float y1 = y0 + (2.0f * textGlyphs[glyphIdx].height / screenHeight);

        vertices[vertexCount + 0] = (TextVertex){{x0, y0}, {textGlyphs[glyphIdx].u0, textGlyphs[glyphIdx].v0}};
        vertices[vertexCount + 1] = (TextVertex){{x1, y0}, {textGlyphs[glyphIdx].u1, textGlyphs[glyphIdx].v0}};
        vertices[vertexCount + 2] = (TextVertex){{x1, y1}, {textGlyphs[glyphIdx].u1, textGlyphs[glyphIdx].v1}};
        vertices[vertexCount + 3] = (TextVertex){{x0, y1}, {textGlyphs[glyphIdx].u0, textGlyphs[glyphIdx].v1}};

        indices[indexCount + 0] = vertexCount + 0;
        indices[indexCount + 1] = vertexCount + 1;
        indices[indexCount + 2] = vertexCount + 2;
        indices[indexCount + 3] = vertexCount + 2;
        indices[indexCount + 4] = vertexCount + 3;
        indices[indexCount + 5] = vertexCount + 0;

        ndcX += (2.0f * textGlyphs[glyphIdx].advanceX / screenWidth);
        vertexCount += 4;
        indexCount += 6;
    }

    updateBuffer(v_ctx, text_ctx->textVertexBufferMemory, vertexCount * sizeof(TextVertex), vertices);
    updateBuffer(v_ctx, text_ctx->textIndexBufferMemory, indexCount * sizeof(uint32_t), indices);

    vkCmdBindPipeline(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, text_ctx->textPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(v_ctx->commandBuffer, 0, 1, &text_ctx->textVertexBuffer, offsets);
    vkCmdBindIndexBuffer(v_ctx->commandBuffer, text_ctx->textIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, text_ctx->textPipelineLayout, 0, 1, &text_ctx->textDescriptorSet, 0, NULL);
    vkCmdDrawIndexed(v_ctx->commandBuffer, indexCount, 1, 0, 0, 0);
}

void text_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] text_cleanup_event_system");
  flecs_text_cleanup(it->world);

  module_break_name(it, "text_module");

  // ecs_query_t *q = ecs_query(it->world, {
  //   .terms = {
  //     { .id = ecs_id(PluginModule) },
  //   }
  // });

  // ecs_iter_t s_it = ecs_query_iter(it->world, q);

  // while (ecs_query_next(&s_it)) {
  //   PluginModule *p = ecs_field(&s_it, PluginModule, 0);
  //   for (int i = 0; i < s_it.count; i ++) {
  //     ecs_print(1,"TEXT CHECK... Module Name : %s", p[i].name);
  //     if(strcmp(p[i].name, "text_module") == 0){
  //       p[i].isCleanUp = true;
  //       ecs_print(1,"text_module XXXXXXXXXX");
  //       break;
  //     }
  //   }
  // }

}

void flecs_text_cleanup(ecs_world_t *world) {
  VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
  if (!v_ctx || !v_ctx->device) return;
  Text2DContext *text_ctx = ecs_singleton_ensure(world, Text2DContext);
  if (!text_ctx) return;

  ecs_log(1, "Text cleanup starting...");
  vkDeviceWaitIdle(v_ctx->device);

  if (text_ctx->textPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(v_ctx->device, text_ctx->textPipeline, NULL);
      text_ctx->textPipeline = VK_NULL_HANDLE;
  }
  if (text_ctx->textPipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(v_ctx->device, text_ctx->textPipelineLayout, NULL);
      text_ctx->textPipelineLayout = VK_NULL_HANDLE;
  }
  if (text_ctx->textDescriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(v_ctx->device, text_ctx->textDescriptorSetLayout, NULL);
      text_ctx->textDescriptorSetLayout = VK_NULL_HANDLE;
  }
  if (text_ctx->textDescriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(v_ctx->device, text_ctx->textDescriptorPool, NULL);
      text_ctx->textDescriptorPool = VK_NULL_HANDLE;
  }
  if (text_ctx->textFontSampler != VK_NULL_HANDLE) {
      vkDestroySampler(v_ctx->device, text_ctx->textFontSampler, NULL);
      text_ctx->textFontSampler = VK_NULL_HANDLE;
  }
  if (text_ctx->textFontImageView != VK_NULL_HANDLE) {
      vkDestroyImageView(v_ctx->device, text_ctx->textFontImageView, NULL);
      text_ctx->textFontImageView = VK_NULL_HANDLE;
  }
  if (text_ctx->textFontImageMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, text_ctx->textFontImageMemory, NULL);
      text_ctx->textFontImageMemory = VK_NULL_HANDLE;
  }
  if (text_ctx->textFontImage != VK_NULL_HANDLE) {
      vkDestroyImage(v_ctx->device, text_ctx->textFontImage, NULL);
      text_ctx->textFontImage = VK_NULL_HANDLE;
  }
  if (text_ctx->textVertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, text_ctx->textVertexBufferMemory, NULL);
      text_ctx->textVertexBufferMemory = VK_NULL_HANDLE;
  }
  if (text_ctx->textVertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(v_ctx->device, text_ctx->textVertexBuffer, NULL);
      text_ctx->textVertexBuffer = VK_NULL_HANDLE;
  }
  if (text_ctx->textIndexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, text_ctx->textIndexBufferMemory, NULL);
      text_ctx->textIndexBufferMemory = VK_NULL_HANDLE;
  }
  if (text_ctx->textIndexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(v_ctx->device, text_ctx->textIndexBuffer, NULL);
      text_ctx->textIndexBuffer = VK_NULL_HANDLE;
  }
  if (text_ctx->textGlyphs) {
      free(text_ctx->textGlyphs);
      text_ctx->textGlyphs = NULL;
  }

  ecs_log(1, "Text cleanup completed");
}

void text2d_register_components(ecs_world_t *world){
  ECS_COMPONENT_DEFINE(world, Text2DContext);
}

void text2d_register_systems(ecs_world_t *world){

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = text_cleanup_event_system
  });

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
        .name = "TextSetupSystem", 
        .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) 
    }),
    .callback = TextSetupSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { 
          .name = "TextRenderSystem", 
          .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) 
      }),
      .callback = TextRenderSystem
  });
}

void flecs_text_module_init(ecs_world_t *world) {
  ecs_log(1, "Initializing text module...");

  text2d_register_components(world);

  ecs_singleton_set(world, Text2DContext, {0});

  ecs_entity_t e = ecs_new(world);
  ecs_set(world, e, PluginModule, { .name = "text_module", .isCleanUp = false });

  text2d_register_systems(world);

  ecs_log(1, "Text module initialized");
}