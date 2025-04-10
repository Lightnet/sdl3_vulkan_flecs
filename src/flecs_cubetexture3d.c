#include "flecs_cubetexture3d.h"
#include <string.h>
#include "shaders/cubetexture3d_vert.spv.h" // You'll need to create this
#include "shaders/cubetexture3d_frag.spv.h" // You'll need to create this

//#define STB_IMAGE_IMPLEMENTATION //might have already define other module need work.
#include "stb_image.h"

typedef struct {
    float pos[3];    // 3D position
    float uv[2];     // Texture coordinates
} CubeTextureVertex;

typedef struct {
    float model[16];
    float view[16];
    float proj[16];
} UniformBufferObject;

static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = codeSize;
    createInfo.pCode = code;
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

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProps);
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memReqs.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
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

static void createTextureImage(WorldContext *ctx) {
    int width, height, channels;
    unsigned char *imageData = stbi_load("assets/textures/light/texture_08.png", &width, &height, &channels, STBI_rgb_alpha);
    if (!imageData) {
        ecs_err("Failed to load texture_08.png: %s", stbi_failure_reason());
        ctx->hasError = true;
        return;
    }

    VkDeviceSize imageSize = width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(ctx, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);
    updateBuffer(ctx, stagingMemory, imageSize, imageData);

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(ctx->device, &imageInfo, NULL, &ctx->cubetexture3dImage) != VK_SUCCESS) {
        ecs_err("Failed to create texture image");
        stbi_image_free(imageData);
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(ctx->device, ctx->cubetexture3dImage, &memReqs);
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

    if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->cubetexture3dImageMemory) != VK_SUCCESS) {
        vkDestroyImage(ctx->device, ctx->cubetexture3dImage, NULL);
        stbi_image_free(imageData);
        return;
    }

    vkBindImageMemory(ctx->device, ctx->cubetexture3dImage, ctx->cubetexture3dImageMemory, 0);

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = ctx->commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = ctx->cubetexture3dImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    VkBufferImageCopy region = {0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, ctx->cubetexture3dImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphicsQueue);

    vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &commandBuffer);
    vkFreeMemory(ctx->device, stagingMemory, NULL);
    vkDestroyBuffer(ctx->device, stagingBuffer, NULL);
    stbi_image_free(imageData);

    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = ctx->cubetexture3dImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->cubetexture3dImageView) != VK_SUCCESS) {
        ecs_err("Failed to create texture image view");
    }

    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(ctx->device, &samplerInfo, NULL, &ctx->cubetexture3dSampler) != VK_SUCCESS) {
        ecs_err("Failed to create texture sampler");
    }
}

static void createUniformBuffer(WorldContext *ctx) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &ctx->cubetexture3dUniformBuffer, &ctx->cubetexture3dUniformBufferMemory);
}

void CubeTexture3DSetupSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    ecs_print(1, "CubeTexture3DSetupSystem starting...");

    // Descriptor Pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    if (vkCreateDescriptorPool(ctx->device, &poolInfo, NULL, &ctx->cubetexture3dDescriptorPool) != VK_SUCCESS) {
        ecs_err("Failed to create cubetexture3d descriptor pool");
        ctx->hasError = true;
        return;
    }

    // Descriptor Set Layout
    VkDescriptorSetLayoutBinding bindings[2] = {0};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;
    if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL, &ctx->cubetexture3dDescriptorSetLayout) != VK_SUCCESS) {
        ecs_err("Failed to create cubetexture3d descriptor set layout");
        ctx->hasError = true;
        return;
    }

    // Allocate Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = ctx->cubetexture3dDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &ctx->cubetexture3dDescriptorSetLayout;
    if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &ctx->cubetexture3dDescriptorSet) != VK_SUCCESS) {
        ecs_err("Failed to allocate cubetexture3d descriptor set");
        ctx->hasError = true;
        return;
    }

    // Create Texture
    createTextureImage(ctx);
    if (ctx->hasError) return;

    // Create Buffers
    CubeTextureVertex vertices[] = {
      // Front face
      {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}}, // 0: Bottom-left
      {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}}, // 1: Bottom-right
      {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}}, // 2: Top-right
      {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}}, // 3: Top-left
      // Back face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}}, // 4: Bottom-left
      {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}}, // 5: Bottom-right
      {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}}, // 6: Top-right
      {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}}  // 7: Top-left
  };
  
  uint32_t indices[] = {
    0, 1, 2, 2, 3, 0,  // Front
    4, 7, 6, 6, 5, 4,  // Back (fixed)
    1, 5, 6, 6, 2, 1,  // Right
    0, 3, 7, 7, 4, 0,  // Left (fixed)
    3, 2, 6, 6, 7, 3,  // Top
    0, 4, 5, 5, 1, 0   // Bottom (fixed)
};

    createBuffer(ctx, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &ctx->cubetexture3dVertexBuffer, &ctx->cubetexture3dVertexBufferMemory);
    createBuffer(ctx, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &ctx->cubetexture3dIndexBuffer, &ctx->cubetexture3dIndexBufferMemory);
    createUniformBuffer(ctx);

    updateBuffer(ctx, ctx->cubetexture3dVertexBufferMemory, sizeof(vertices), vertices);
    updateBuffer(ctx, ctx->cubetexture3dIndexBufferMemory, sizeof(indices), indices);

    // Update Descriptor Set
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = ctx->cubetexture3dUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {0};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = ctx->cubetexture3dImageView;
    imageInfo.sampler = ctx->cubetexture3dSampler;

    VkWriteDescriptorSet descriptorWrites[2] = {
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
        {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}
    };
    descriptorWrites[0].dstSet = ctx->cubetexture3dDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].pBufferInfo = &bufferInfo;
    descriptorWrites[1].dstSet = ctx->cubetexture3dDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(ctx->device, 2, descriptorWrites, 0, NULL);

    // Pipeline Setup
    VkShaderModule vertShaderModule = createShaderModule(ctx->device, cubetexture3d_vert_spv, cubetexture3d_vert_spv_size);
    VkShaderModule fragShaderModule = createShaderModule(ctx->device, cubetexture3d_frag_spv, cubetexture3d_frag_spv_size);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", NULL},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", NULL}
    };

    VkVertexInputBindingDescription bindingDesc = {0, sizeof(CubeTextureVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributeDescs[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeTextureVertex, pos)},
        {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(CubeTextureVertex, uv)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0.0f, 0.0f, (float)ctx->width, (float)ctx->height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {ctx->width, ctx->height}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    // rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &ctx->cubetexture3dDescriptorSetLayout;
    if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->cubetexture3dPipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create cubetexture3d pipeline layout");
        ctx->hasError = true;
        return;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = ctx->cubetexture3dPipelineLayout;
    pipelineInfo.renderPass = ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->cubetexture3dPipeline) != VK_SUCCESS) {
        ecs_err("Failed to create cubetexture3d graphics pipeline");
        ctx->hasError = true;
        return;
    }

    vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);

    ecs_print(1, "CubeTexture3DSetupSystem completed");
}

void CubeTexture3DRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  static float angleY = 0.0f;
  static float angleX = 0.0f;
  angleY += 0.02f;  // Y-axis rotation (side-to-side)
  angleX += 0.015f; // X-axis rotation (tilt up/down, slightly slower)

  UniformBufferObject ubo = {0};

  // Compute rotation matrices
  float cosY = cosf(angleY), sinY = sinf(angleY);
  float cosX = cosf(angleX), sinX = sinf(angleX);

  // Y-axis rotation matrix (around Y)
  float rotY[16] = {
      cosY,  0.0f, sinY,  0.0f,
      0.0f,  1.0f, 0.0f,  0.0f,
     -sinY,  0.0f, cosY,  0.0f,
      0.0f,  0.0f, 0.0f,  1.0f
  };

  // X-axis rotation matrix (around X)
  float rotX[16] = {
      1.0f,  0.0f,  0.0f,  0.0f,
      0.0f,  cosX, -sinX,  0.0f,
      0.0f,  sinX,  cosX,  0.0f,
      0.0f,  0.0f,  0.0f,  1.0f
  };

  // Multiply matrices: model = rotY * rotX
  // (Apply X rotation first, then Y rotation)
  ubo.model[0] = rotY[0] * rotX[0] + rotY[1] * rotX[4] + rotY[2] * rotX[8] + rotY[3] * rotX[12];
  ubo.model[1] = rotY[0] * rotX[1] + rotY[1] * rotX[5] + rotY[2] * rotX[9] + rotY[3] * rotX[13];
  ubo.model[2] = rotY[0] * rotX[2] + rotY[1] * rotX[6] + rotY[2] * rotX[10] + rotY[3] * rotX[14];
  ubo.model[3] = rotY[0] * rotX[3] + rotY[1] * rotX[7] + rotY[2] * rotX[11] + rotY[3] * rotX[15];

  ubo.model[4] = rotY[4] * rotX[0] + rotY[5] * rotX[4] + rotY[6] * rotX[8] + rotY[7] * rotX[12];
  ubo.model[5] = rotY[4] * rotX[1] + rotY[5] * rotX[5] + rotY[6] * rotX[9] + rotY[7] * rotX[13];
  ubo.model[6] = rotY[4] * rotX[2] + rotY[5] * rotX[6] + rotY[6] * rotX[10] + rotY[7] * rotX[14];
  ubo.model[7] = rotY[4] * rotX[3] + rotY[5] * rotX[7] + rotY[6] * rotX[11] + rotY[7] * rotX[15];

  ubo.model[8] = rotY[8] * rotX[0] + rotY[9] * rotX[4] + rotY[10] * rotX[8] + rotY[11] * rotX[12];
  ubo.model[9] = rotY[8] * rotX[1] + rotY[9] * rotX[5] + rotY[10] * rotX[9] + rotY[11] * rotX[13];
  ubo.model[10] = rotY[8] * rotX[2] + rotY[9] * rotX[6] + rotY[10] * rotX[10] + rotY[11] * rotX[14];
  ubo.model[11] = rotY[8] * rotX[3] + rotY[9] * rotX[7] + rotY[10] * rotX[11] + rotY[11] * rotX[15];

  ubo.model[12] = rotY[12] * rotX[0] + rotY[13] * rotX[4] + rotY[14] * rotX[8] + rotY[15] * rotX[12];
  ubo.model[13] = rotY[12] * rotX[1] + rotY[13] * rotX[5] + rotY[14] * rotX[9] + rotY[15] * rotX[13];
  ubo.model[14] = rotY[12] * rotX[2] + rotY[13] * rotX[6] + rotY[14] * rotX[10] + rotY[15] * rotX[14];
  ubo.model[15] = rotY[12] * rotX[3] + rotY[13] * rotX[7] + rotY[14] * rotX[11] + rotY[15] * rotX[15];

  // View matrix (unchanged)
  ubo.view[0] = 1.0f;
  ubo.view[5] = 1.0f;
  ubo.view[10] = 1.0f;
  ubo.view[14] = -5.0f;
  ubo.view[15] = 1.0f;

  // Projection matrix (unchanged)
  float aspect = (float)ctx->width / (float)ctx->height;
  float fov = 45.0f * 3.14159f / 180.0f;
  float near = 0.1f, far = 100.0f;
  float tanHalfFov = tanf(fov / 2.0f);
  ubo.proj[0] = 1.0f / (aspect * tanHalfFov);
  ubo.proj[5] = -1.0f / tanHalfFov;
  ubo.proj[10] = -(far + near) / (far - near);
  ubo.proj[11] = -1.0f;
  ubo.proj[14] = -(2.0f * far * near) / (far - near);
  ubo.proj[15] = 0.0f;

  updateBuffer(ctx, ctx->cubetexture3dUniformBufferMemory, sizeof(ubo), &ubo);

  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->cubetexture3dPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->cubetexture3dVertexBuffer, offsets);
  vkCmdBindIndexBuffer(ctx->commandBuffer, ctx->cubetexture3dIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->cubetexture3dPipelineLayout, 0, 1, &ctx->cubetexture3dDescriptorSet, 0, NULL);
  //vkCmdDrawIndexed(ctx->commandBuffer, 36, 1, 0, 0, 0);

  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 0, 0, 0);   // Front
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 6, 0, 0);   // Back
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 12, 0, 0);  // Right
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 18, 0, 0);  // Left
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 24, 0, 0);  // Top
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 30, 0, 0);  // Bottom
}
// front okay but not correct
// back texture zip 
// right correct
// left one side is triangle and other zip
// top zig
// bottom one side triangle black and other gray




void flecs_cubetexture3d_cleanup(WorldContext *ctx) {
    if (!ctx || !ctx->device) return;

    ecs_print(1, "CubeTexture3D cleanup starting...");
    vkDeviceWaitIdle(ctx->device);

    if (ctx->cubetexture3dPipeline != VK_NULL_HANDLE) vkDestroyPipeline(ctx->device, ctx->cubetexture3dPipeline, NULL);
    if (ctx->cubetexture3dPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(ctx->device, ctx->cubetexture3dPipelineLayout, NULL);
    if (ctx->cubetexture3dDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(ctx->device, ctx->cubetexture3dDescriptorSetLayout, NULL);
    if (ctx->cubetexture3dDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(ctx->device, ctx->cubetexture3dDescriptorPool, NULL);
    if (ctx->cubetexture3dSampler != VK_NULL_HANDLE) vkDestroySampler(ctx->device, ctx->cubetexture3dSampler, NULL);
    if (ctx->cubetexture3dImageView != VK_NULL_HANDLE) vkDestroyImageView(ctx->device, ctx->cubetexture3dImageView, NULL);
    if (ctx->cubetexture3dImageMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubetexture3dImageMemory, NULL);
    if (ctx->cubetexture3dImage != VK_NULL_HANDLE) vkDestroyImage(ctx->device, ctx->cubetexture3dImage, NULL);
    if (ctx->cubetexture3dVertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubetexture3dVertexBufferMemory, NULL);
    if (ctx->cubetexture3dVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubetexture3dVertexBuffer, NULL);
    if (ctx->cubetexture3dIndexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubetexture3dIndexBufferMemory, NULL);
    if (ctx->cubetexture3dIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubetexture3dIndexBuffer, NULL);
    if (ctx->cubetexture3dUniformBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubetexture3dUniformBufferMemory, NULL);
    if (ctx->cubetexture3dUniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubetexture3dUniformBuffer, NULL);

    ecs_print(1, "CubeTexture3D cleanup completed");
}

void flecs_cubetexture3d_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing cubetexture3d module...");

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "CubeTexture3DSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
        .callback = CubeTexture3DSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "CubeTexture3DRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
        .callback = CubeTexture3DRenderSystem
    });

    ecs_print(1, "CubeTexture3d module initialized");
}


