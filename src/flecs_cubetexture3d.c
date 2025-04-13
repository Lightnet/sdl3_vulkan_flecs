#include "flecs_cubetexture3d.h"
#include <string.h>
#include "shaders/cubetexture3d_vert.spv.h" // You'll need to create this
#include "shaders/cubetexture3d_frag.spv.h" // You'll need to create this
#include "flecs_utils.h" // createShaderModuleLen(v_ctx->device, text_vert_spv)
#include "flecs_sdl.h"
#include "flecs_vulkan.h"

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

// static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
//     VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
//     createInfo.codeSize = codeSize;
//     createInfo.pCode = code;
//     VkShaderModule module;
//     if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
//         ecs_err("Failed to create shader module");
//         return VK_NULL_HANDLE;
//     }
//     return module;
// }

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

  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProps);
  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memReqs.size;
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
      if ((memReqs.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
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
  if (vkMapMemory(v_ctx->device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
      ecs_err("Failed to map buffer memory");
      v_ctx->hasError = true;
      return;
  }
  memcpy(mapped, data, (size_t)size);
  vkUnmapMemory(v_ctx->device, memory);
}

static void createTextureImage(VulkanContext *v_ctx, CubeText3DContext *cubetext3d_ctx) {
  int width, height, channels;
  unsigned char *imageData = stbi_load("assets/textures/light/texture_08.png", &width, &height, &channels, STBI_rgb_alpha);
  if (!imageData) {
      ecs_err("Failed to load texture_08.png: %s", stbi_failure_reason());
      v_ctx->hasError = true;
      return;
  }

  VkDeviceSize imageSize = width * height * 4;
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  createBuffer(v_ctx, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingMemory);
  updateBuffer(v_ctx, stagingMemory, imageSize, imageData);

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

  if (vkCreateImage(v_ctx->device, &imageInfo, NULL, &cubetext3d_ctx->cubetexture3dImage) != VK_SUCCESS) {
      ecs_err("Failed to create texture image");
      stbi_image_free(imageData);
      v_ctx->hasError = true;
      return;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(v_ctx->device, cubetext3d_ctx->cubetexture3dImage, &memReqs);
  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memReqs.size;
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
      if ((memReqs.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
          allocInfo.memoryTypeIndex = i;
          break;
      }
  }

  if (vkAllocateMemory(v_ctx->device, &allocInfo, NULL, &cubetext3d_ctx->cubetexture3dImageMemory) != VK_SUCCESS) {
      vkDestroyImage(v_ctx->device, cubetext3d_ctx->cubetexture3dImage, NULL);
      stbi_image_free(imageData);
      v_ctx->hasError = true;
      return;
  }

  vkBindImageMemory(v_ctx->device, cubetext3d_ctx->cubetexture3dImage, cubetext3d_ctx->cubetexture3dImageMemory, 0);

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = v_ctx->commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  vkAllocateCommandBuffers(v_ctx->device, &cmdAllocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = cubetext3d_ctx->cubetexture3dImage;
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
  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, cubetext3d_ctx->cubetexture3dImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(v_ctx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(v_ctx->graphicsQueue);

  vkFreeCommandBuffers(v_ctx->device, v_ctx->commandPool, 1, &commandBuffer);
  vkFreeMemory(v_ctx->device, stagingMemory, NULL);
  vkDestroyBuffer(v_ctx->device, stagingBuffer, NULL);
  stbi_image_free(imageData);

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = cubetext3d_ctx->cubetexture3dImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.layerCount = 1;
  if (vkCreateImageView(v_ctx->device, &viewInfo, NULL, &cubetext3d_ctx->cubetexture3dImageView) != VK_SUCCESS) {
      ecs_err("Failed to create texture image view");
      v_ctx->hasError = true;
      return;
  }

  VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  if (vkCreateSampler(v_ctx->device, &samplerInfo, NULL, &cubetext3d_ctx->cubetexture3dSampler) != VK_SUCCESS) {
      ecs_err("Failed to create texture sampler");
      v_ctx->hasError = true;
      return;
  }
}

static void createUniformBuffer(VulkanContext *v_ctx, CubeText3DContext *cubetext3d_ctx) {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  createBuffer(v_ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &cubetext3d_ctx->cubetexture3dUniformBuffer, &cubetext3d_ctx->cubetexture3dUniformBufferMemory);
}

void CubeTexture3DSetupSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) {
      ecs_err("SDLContext not available or has error");
      return;
  }

  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx || !v_ctx->device) {
      ecs_err("VulkanContext not available or device not initialized");
      return;
  }

  CubeText3DContext *cubetext3d_ctx = ecs_singleton_ensure(it->world, CubeText3DContext);
  if (!cubetext3d_ctx) {
      ecs_err("CubeText3DContext not available");
      return;
  }

  ecs_log(1, "CubeTexture3DSetupSystem starting...");

  // Descriptor Pool
  VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
  };
  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 2;
  poolInfo.pPoolSizes = poolSizes;
  if (vkCreateDescriptorPool(v_ctx->device, &poolInfo, NULL, &cubetext3d_ctx->cubetexture3dDescriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create cubetexture3d descriptor pool");
      sdl_ctx->hasError = true;
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
  if (vkCreateDescriptorSetLayout(v_ctx->device, &layoutInfo, NULL, &cubetext3d_ctx->cubetexture3dDescriptorSetLayout) != VK_SUCCESS) {
      ecs_err("Failed to create cubetexture3d descriptor set layout");
      sdl_ctx->hasError = true;
      return;
  }

  // Allocate Descriptor Set
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = cubetext3d_ctx->cubetexture3dDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &cubetext3d_ctx->cubetexture3dDescriptorSetLayout;
  if (vkAllocateDescriptorSets(v_ctx->device, &allocInfo, &cubetext3d_ctx->cubetexture3dDescriptorSet) != VK_SUCCESS) {
      ecs_err("Failed to allocate cubetexture3d descriptor set");
      sdl_ctx->hasError = true;
      return;
  }

  // Create Texture
  createTextureImage(v_ctx, cubetext3d_ctx);
  if (v_ctx->hasError) return;

  // Create Buffers
  CubeTextureVertex vertices[] = {
      // Front face
      {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}}, // 0
      {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}}, // 1
      {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}}, // 2
      {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}}, // 3
      // Back face
      {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}}, // 4
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}}, // 5
      {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}}, // 6
      {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}}, // 7
      // Right face
      {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}}, // 8
      {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}}, // 9
      {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}}, // 10
      {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}}, // 11
      // Left face
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}}, // 12
      {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}}, // 13
      {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}}, // 14
      {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}}, // 15
      // Top face
      {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}}, // 16
      {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}}, // 17
      {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}}, // 18
      {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}}, // 19
      // Bottom face
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}}, // 20
      {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}}, // 21
      {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}}, // 22
      {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}}, // 23
  };

  uint32_t indices[] = {
      // Front
      0, 1, 2, 2, 3, 0,
      // Back
      4, 5, 6, 6, 7, 4,
      // Right
      8, 9, 10, 10, 11, 8,
      // Left
      12, 13, 14, 14, 15, 12,
      // Top
      16, 17, 18, 18, 19, 16,
      // Bottom
      20, 21, 22, 22, 23, 20
  };

  createBuffer(v_ctx, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &cubetext3d_ctx->cubetexture3dVertexBuffer, &cubetext3d_ctx->cubetexture3dVertexBufferMemory);
  createBuffer(v_ctx, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &cubetext3d_ctx->cubetexture3dIndexBuffer, &cubetext3d_ctx->cubetexture3dIndexBufferMemory);
  createUniformBuffer(v_ctx, cubetext3d_ctx);

  updateBuffer(v_ctx, cubetext3d_ctx->cubetexture3dVertexBufferMemory, sizeof(vertices), vertices);
  updateBuffer(v_ctx, cubetext3d_ctx->cubetexture3dIndexBufferMemory, sizeof(indices), indices);

  // Update Descriptor Set
  VkDescriptorBufferInfo bufferInfo = {0};
  bufferInfo.buffer = cubetext3d_ctx->cubetexture3dUniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UniformBufferObject);

  VkDescriptorImageInfo imageInfo = {0};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = cubetext3d_ctx->cubetexture3dImageView;
  imageInfo.sampler = cubetext3d_ctx->cubetexture3dSampler;

  VkWriteDescriptorSet descriptorWrites[2] = {
      {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET},
      {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}
  };
  descriptorWrites[0].dstSet = cubetext3d_ctx->cubetexture3dDescriptorSet;
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].pBufferInfo = &bufferInfo;
  descriptorWrites[1].dstSet = cubetext3d_ctx->cubetexture3dDescriptorSet;
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[1].pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(v_ctx->device, 2, descriptorWrites, 0, NULL);

  // VkShaderModule vertShaderModule = createShaderModule(v_ctx->device, cubetexture3d_vert_spv, sizeof(cubetexture3d_vert_spv));
  // VkShaderModule fragShaderModule = createShaderModule(v_ctx->device, cubetexture3d_frag_spv, sizeof(cubetexture3d_frag_spv));
  VkShaderModule vertShaderModule = createShaderModuleH(v_ctx->device, cubetexture3d_vert_spv, sizeof(cubetexture3d_vert_spv));
  VkShaderModule fragShaderModule = createShaderModuleH(v_ctx->device, cubetexture3d_frag_spv, sizeof(cubetexture3d_frag_spv));
  
  if (!vertShaderModule || !fragShaderModule) {
      ecs_err("Failed to create shader modules");
      sdl_ctx->hasError = true;
      return;
  }

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
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
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
  pipelineLayoutInfo.pSetLayouts = &cubetext3d_ctx->cubetexture3dDescriptorSetLayout;
  if (vkCreatePipelineLayout(v_ctx->device, &pipelineLayoutInfo, NULL, &cubetext3d_ctx->cubetexture3dPipelineLayout) != VK_SUCCESS) {
      ecs_err("Failed to create cubetexture3d pipeline layout");
      sdl_ctx->hasError = true;
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
  pipelineInfo.layout = cubetext3d_ctx->cubetexture3dPipelineLayout;
  pipelineInfo.renderPass = v_ctx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(v_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &cubetext3d_ctx->cubetexture3dPipeline) != VK_SUCCESS) {
      ecs_err("Failed to create cubetexture3d graphics pipeline");
      sdl_ctx->hasError = true;
      return;
  }

  vkDestroyShaderModule(v_ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(v_ctx->device, vertShaderModule, NULL);

  ecs_log(1, "CubeTexture3DSetupSystem completed");
}

void CubeTexture3DRenderSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;

  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx || !v_ctx->device) {
      ecs_err("VulkanContext not available or device not initialized");
      return;
  }

  CubeText3DContext *cubetext3d_ctx = ecs_singleton_ensure(it->world, CubeText3DContext);
  if (!cubetext3d_ctx) {
      ecs_err("CubeText3DContext not available");
      return;
  }

  static float angleY = 0.0f;
  static float angleX = 0.0f;
  angleY += 0.02f;
  angleX += 0.015f;

  UniformBufferObject ubo = {0};

  // Compute rotation matrices
  float cosY = cosf(angleY), sinY = sinf(angleY);
  float cosX = cosf(angleX), sinX = sinf(angleX);

  // Y-axis rotation matrix
  float rotY[16] = {
      cosY,  0.0f, sinY,  0.0f,
      0.0f,  1.0f, 0.0f,  0.0f,
     -sinY,  0.0f, cosY,  0.0f,
      0.0f,  0.0f, 0.0f,  1.0f
  };

  // X-axis rotation matrix
  float rotX[16] = {
      1.0f,  0.0f,  0.0f,  0.0f,
      0.0f,  cosX, -sinX,  0.0f,
      0.0f,  sinX,  cosX,  0.0f,
      0.0f,  0.0f,  0.0f,  1.0f
  };

  // Model matrix: rotY * rotX
  for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
          ubo.model[i * 4 + j] = 0.0f;
          for (int k = 0; k < 4; k++) {
              ubo.model[i * 4 + j] += rotY[i * 4 + k] * rotX[k * 4 + j];
          }
      }
  }

  // View matrix
  ubo.view[0] = 1.0f;
  ubo.view[5] = 1.0f;
  ubo.view[10] = 1.0f;
  ubo.view[14] = -5.0f;
  ubo.view[15] = 1.0f;

  // Projection matrix
  float aspect = (float)sdl_ctx->width / (float)sdl_ctx->height;
  float fov = 45.0f * 3.14159f / 180.0f;
  float near = 0.1f, far = 100.0f;
  float tanHalfFov = tanf(fov / 2.0f);
  ubo.proj[0] = 1.0f / (aspect * tanHalfFov);
  ubo.proj[5] = -1.0f / tanHalfFov;
  ubo.proj[10] = -(far + near) / (far - near);
  ubo.proj[11] = -1.0f;
  ubo.proj[14] = -(2.0f * far * near) / (far - near);
  ubo.proj[15] = 0.0f;

  updateBuffer(v_ctx, cubetext3d_ctx->cubetexture3dUniformBufferMemory, sizeof(ubo), &ubo);

  vkCmdBindPipeline(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cubetext3d_ctx->cubetexture3dPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(v_ctx->commandBuffer, 0, 1, &cubetext3d_ctx->cubetexture3dVertexBuffer, offsets);
  vkCmdBindIndexBuffer(v_ctx->commandBuffer, cubetext3d_ctx->cubetexture3dIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cubetext3d_ctx->cubetexture3dPipelineLayout, 0, 1, &cubetext3d_ctx->cubetexture3dDescriptorSet, 0, NULL);
  vkCmdDrawIndexed(v_ctx->commandBuffer, 36, 1, 0, 0, 0);
}

void cubetexture3d_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] cubetexture3d_cleanup_event_system");
}

void flecs_cubetexture3d_cleanup(ecs_world_t *world) {
    VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
    if (!v_ctx || !v_ctx->device) {
        ecs_err("VulkanContext not available for cleanup");
        return;
    }

    CubeText3DContext *cubetext3d_ctx = ecs_singleton_ensure(world, CubeText3DContext);
    if (!cubetext3d_ctx) {
        ecs_err("CubeText3DContext not available for cleanup");
        return;
    }

    ecs_log(1, "CubeTexture3D cleanup starting...");
    vkDeviceWaitIdle(v_ctx->device);

    if (cubetext3d_ctx->cubetexture3dPipeline != VK_NULL_HANDLE) vkDestroyPipeline(v_ctx->device, cubetext3d_ctx->cubetexture3dPipeline, NULL);
    if (cubetext3d_ctx->cubetexture3dPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(v_ctx->device, cubetext3d_ctx->cubetexture3dPipelineLayout, NULL);
    if (cubetext3d_ctx->cubetexture3dDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(v_ctx->device, cubetext3d_ctx->cubetexture3dDescriptorSetLayout, NULL);
    if (cubetext3d_ctx->cubetexture3dDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(v_ctx->device, cubetext3d_ctx->cubetexture3dDescriptorPool, NULL);
    if (cubetext3d_ctx->cubetexture3dSampler != VK_NULL_HANDLE) vkDestroySampler(v_ctx->device, cubetext3d_ctx->cubetexture3dSampler, NULL);
    if (cubetext3d_ctx->cubetexture3dImageView != VK_NULL_HANDLE) vkDestroyImageView(v_ctx->device, cubetext3d_ctx->cubetexture3dImageView, NULL);
    if (cubetext3d_ctx->cubetexture3dImageMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cubetext3d_ctx->cubetexture3dImageMemory, NULL);
    if (cubetext3d_ctx->cubetexture3dImage != VK_NULL_HANDLE) vkDestroyImage(v_ctx->device, cubetext3d_ctx->cubetexture3dImage, NULL);
    if (cubetext3d_ctx->cubetexture3dVertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cubetext3d_ctx->cubetexture3dVertexBufferMemory, NULL);
    if (cubetext3d_ctx->cubetexture3dVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cubetext3d_ctx->cubetexture3dVertexBuffer, NULL);
    if (cubetext3d_ctx->cubetexture3dIndexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cubetext3d_ctx->cubetexture3dIndexBufferMemory, NULL);
    if (cubetext3d_ctx->cubetexture3dIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cubetext3d_ctx->cubetexture3dIndexBuffer, NULL);
    if (cubetext3d_ctx->cubetexture3dUniformBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cubetext3d_ctx->cubetexture3dUniformBufferMemory, NULL);
    if (cubetext3d_ctx->cubetexture3dUniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cubetext3d_ctx->cubetexture3dUniformBuffer, NULL);

    ecs_log(1, "CubeTexture3D cleanup completed");
}

// CubeText3DContext
void cubetext3d_register_components(ecs_world_t *world){
  ECS_COMPONENT_DEFINE(world, CubeText3DContext);
}

void cubetext3d_register_systems(ecs_world_t *world){

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = cubetexture3d_cleanup_event_system
  });

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "CubeTexture3DSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
    .callback = CubeTexture3DSetupSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "CubeTexture3DRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
      .callback = CubeTexture3DRenderSystem
  });
}

void flecs_cubetexture3d_module_init(ecs_world_t *world) {
    ecs_log(1, "Initializing cubetexture3d module...");

    cubetext3d_register_components(world);

    ecs_singleton_set(world, CubeText3DContext, {0});

    cubetext3d_register_systems(world);

    ecs_log(1, "CubeTexture3d module initialized");
}

