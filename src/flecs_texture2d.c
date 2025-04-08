#include "flecs_texture2d.h"
#include <flecs.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include "texture2d_vert.spv.h"
#include "texture2d_frag.spv.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    float pos[2];    // 2D position
    float uv[2];     // Texture coordinates
} Texture2DVertex;

static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
  VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = codeSize; // Size in bytes
  createInfo.pCode = code;        // Already uint32_t*, no cast needed
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

  if (vkCreateImage(ctx->device, &imageInfo, NULL, &ctx->texture2dImage) != VK_SUCCESS) {
      ecs_err("Failed to create texture image");
      stbi_image_free(imageData);
      return;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(ctx->device, ctx->texture2dImage, &memReqs);
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

  if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->texture2dImageMemory) != VK_SUCCESS) {
      vkDestroyImage(ctx->device, ctx->texture2dImage, NULL);
      stbi_image_free(imageData);
      return;
  }

  vkBindImageMemory(ctx->device, ctx->texture2dImage, ctx->texture2dImageMemory, 0);

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
  barrier.image = ctx->texture2dImage;
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
  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, ctx->texture2dImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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
  viewInfo.image = ctx->texture2dImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;
  if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->texture2dImageView) != VK_SUCCESS) {
      ecs_err("Failed to create texture image view");
  }

  VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  if (vkCreateSampler(ctx->device, &samplerInfo, NULL, &ctx->texture2dSampler) != VK_SUCCESS) {
      ecs_err("Failed to create texture sampler");
  }
}



void Texture2DSetupSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "Texture2DSetupSystem starting...");

  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
  };
  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = poolSizes;
  if (vkCreateDescriptorPool(ctx->device, &poolInfo, NULL, &ctx->texture2dDescriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create texture2d descriptor pool");
      ctx->hasError = true;
      return;
  }

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerLayoutBinding;
  if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL, &ctx->texture2dDescriptorSetLayout) != VK_SUCCESS) {
      ecs_err("Failed to create texture2d descriptor set layout");
      ctx->hasError = true;
      return;
  }

  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = ctx->texture2dDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &ctx->texture2dDescriptorSetLayout;
  if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &ctx->texture2dDescriptorSet) != VK_SUCCESS) {
      ecs_err("Failed to allocate texture2d descriptor set");
      ctx->hasError = true;
      return;
  }

  createTextureImage(ctx);
  if (ctx->hasError) return;

  VkDescriptorImageInfo imageInfo = {0};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = ctx->texture2dImageView;
  imageInfo.sampler = ctx->texture2dSampler;

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = ctx->texture2dDescriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, NULL);

  Texture2DVertex vertices[] = {
      {{-1.0f, -0.5f}, {0.0f, 1.0f}}, // Bottom-left
      {{ 0.0f, -0.5f}, {1.0f, 1.0f}}, // Bottom-right
      {{ 0.0f,  0.5f}, {1.0f, 0.0f}}, // Top-right
      {{-1.0f,  0.5f}, {0.0f, 0.0f}}  // Top-left
  };
  uint32_t indices[] = {0, 1, 2, 2, 3, 0};

  createBuffer(ctx, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->texture2dVertexBuffer, &ctx->texture2dVertexBufferMemory);
  createBuffer(ctx, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ctx->texture2dIndexBuffer, &ctx->texture2dIndexBufferMemory);
  updateBuffer(ctx, ctx->texture2dVertexBufferMemory, sizeof(vertices), vertices);
  updateBuffer(ctx, ctx->texture2dIndexBufferMemory, sizeof(indices), indices);

  VkShaderModule vertShaderModule = createShaderModule(ctx->device, texture2d_vert_spv, sizeof(texture2d_vert_spv));
  VkShaderModule fragShaderModule = createShaderModule(ctx->device, texture2d_frag_spv, sizeof(texture2d_frag_spv));

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
  bindingDesc.stride = sizeof(Texture2DVertex);
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescs[2] = {0};
  attributeDescs[0].binding = 0;
  attributeDescs[0].location = 0;
  attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescs[0].offset = offsetof(Texture2DVertex, pos);
  attributeDescs[1].binding = 0;
  attributeDescs[1].location = 1;
  attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescs[1].offset = offsetof(Texture2DVertex, uv);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

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
  rasterizer.cullMode = VK_CULL_MODE_NONE;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
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
  pipelineLayoutInfo.pSetLayouts = &ctx->texture2dDescriptorSetLayout;
  if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->texture2dPipelineLayout) != VK_SUCCESS) {
      ecs_err("Failed to create texture2d pipeline layout");
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
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = ctx->texture2dPipelineLayout;
  pipelineInfo.renderPass = ctx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->texture2dPipeline) != VK_SUCCESS) {
      ecs_err("Failed to create texture2d graphics pipeline");
      ctx->hasError = true;
      return;
  }

  vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);

  ecs_print(1, "Texture2DSetupSystem completed");
}


void Texture2DRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "Rendering texture2d on left side");
  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->texture2dPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->texture2dVertexBuffer, offsets);
  vkCmdBindIndexBuffer(ctx->commandBuffer, ctx->texture2dIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->texture2dPipelineLayout, 0, 1, &ctx->texture2dDescriptorSet, 0, NULL);
  vkCmdDrawIndexed(ctx->commandBuffer, 6, 1, 0, 0, 0);
}


void flecs_texture2d_cleanup(WorldContext *ctx) {
  if (!ctx || !ctx->device) return;

  ecs_print(1, "Texture2D cleanup starting...");
  vkDeviceWaitIdle(ctx->device);

  if (ctx->texture2dPipeline != VK_NULL_HANDLE) vkDestroyPipeline(ctx->device, ctx->texture2dPipeline, NULL);
  if (ctx->texture2dPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(ctx->device, ctx->texture2dPipelineLayout, NULL);
  if (ctx->texture2dDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(ctx->device, ctx->texture2dDescriptorSetLayout, NULL);
  if (ctx->texture2dDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(ctx->device, ctx->texture2dDescriptorPool, NULL);
  if (ctx->texture2dSampler != VK_NULL_HANDLE) vkDestroySampler(ctx->device, ctx->texture2dSampler, NULL);
  if (ctx->texture2dImageView != VK_NULL_HANDLE) vkDestroyImageView(ctx->device, ctx->texture2dImageView, NULL);
  if (ctx->texture2dImageMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->texture2dImageMemory, NULL);
  if (ctx->texture2dImage != VK_NULL_HANDLE) vkDestroyImage(ctx->device, ctx->texture2dImage, NULL);
  if (ctx->texture2dVertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->texture2dVertexBufferMemory, NULL);
  if (ctx->texture2dVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->texture2dVertexBuffer, NULL);
  if (ctx->texture2dIndexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->texture2dIndexBufferMemory, NULL);
  if (ctx->texture2dIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->texture2dIndexBuffer, NULL);

  ecs_print(1, "Texture2D cleanup completed");
}



void flecs_texture2d_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "Initializing texture2d module...");

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "Texture2DSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
      .callback = Texture2DSetupSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "Texture2DRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
      .callback = Texture2DRenderSystem
  });

  ecs_print(1, "Texture2d module initialized");
}
