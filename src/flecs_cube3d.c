#include "flecs_cube3d.h"
#include <flecs.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include "shaders/cube3d_vert.spv.h"
#include "shaders/cube3d_frag.spv.h"

typedef struct {
    float pos[3];    // 3D position
    float color[3];  // RGB color
} CubeVertex;

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

static void createUniformBuffer(WorldContext *ctx) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &ctx->cubeUniformBuffer, &ctx->cubeUniformBufferMemory);
}



void Cube3DSetupSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "Cube3DSetupSystem starting...");

  // Descriptor Pool
  VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = 1;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  if (vkCreateDescriptorPool(ctx->device, &poolInfo, NULL, &ctx->cubeDescriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create cube descriptor pool");
      ctx->hasError = true;
      return;
  }

  // Descriptor Set Layout
  VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;
  if (vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, NULL, &ctx->cubeDescriptorSetLayout) != VK_SUCCESS) {
      ecs_err("Failed to create cube descriptor set layout");
      ctx->hasError = true;
      return;
  }

  // Allocate Descriptor Set
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = ctx->cubeDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &ctx->cubeDescriptorSetLayout;
  if (vkAllocateDescriptorSets(ctx->device, &allocInfo, &ctx->cubeDescriptorSet) != VK_SUCCESS) {
      ecs_err("Failed to allocate cube descriptor set");
      ctx->hasError = true;
      return;
  }

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

  createBuffer(ctx, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
               &ctx->cubeVertexBuffer, &ctx->cubeVertexBufferMemory);
  createBuffer(ctx, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
               &ctx->cubeIndexBuffer, &ctx->cubeIndexBufferMemory);
  createUniformBuffer(ctx);

  updateBuffer(ctx, ctx->cubeVertexBufferMemory, sizeof(vertices), vertices);
  updateBuffer(ctx, ctx->cubeIndexBufferMemory, sizeof(indices), indices);

  // Update Descriptor Set
  VkDescriptorBufferInfo bufferInfo = {0};
  bufferInfo.buffer = ctx->cubeUniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UniformBufferObject);

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = ctx->cubeDescriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(ctx->device, 1, &descriptorWrite, 0, NULL);

  // Pipeline Setup
  VkShaderModule vertShaderModule = createShaderModule(ctx->device, cube3d_vert_spv, cube3d_vert_spv_size);
  VkShaderModule fragShaderModule = createShaderModule(ctx->device, cube3d_frag_spv, cube3d_frag_spv_size);

  VkPipelineShaderStageCreateInfo shaderStages[] = {
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", NULL},
      {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", NULL}
  };

  VkVertexInputBindingDescription bindingDesc = {0, sizeof(CubeVertex), VK_VERTEX_INPUT_RATE_VERTEX};
  VkVertexInputAttributeDescription attributeDescs[] = {
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, pos)},
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, color)}
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
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &ctx->cubeDescriptorSetLayout;
  if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->cubePipelineLayout) != VK_SUCCESS) {
      ecs_err("Failed to create cube pipeline layout");
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
  pipelineInfo.layout = ctx->cubePipelineLayout;
  pipelineInfo.renderPass = ctx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->cubePipeline) != VK_SUCCESS) {
      ecs_err("Failed to create cube graphics pipeline");
      ctx->hasError = true;
      return;
  }

  vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);

  ecs_print(1, "Cube3DSetupSystem completed");
}




void Cube3DRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // Update uniform buffer with spinning cube
  static float angleX = 0.0f; // Rotation around X-axis (tilts top/bottom)
  static float angleY = 0.0f; // Rotation around Y-axis (spins left/right)
  angleX += 0.01f; // Adjust speed as needed
  angleY += 0.02f; // Different speed for variety

  UniformBufferObject ubo = {0};

  // Model matrix: Combine X and Y rotations
  float cosX = cosf(angleX), sinX = sinf(angleX);
  float cosY = cosf(angleY), sinY = sinf(angleY);

  // Rotation around X-axis (tilts top/bottom)
  float rotX[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, cosX, -sinX, 0.0f,
      0.0f, sinX, cosX, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  };

  // Rotation around Y-axis (spins left/right)
  float rotY[16] = {
      cosY, 0.0f, sinY, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      -sinY, 0.0f, cosY, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
  };

  // Multiply rotY * rotX to get combined rotation
  for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
          ubo.model[i * 4 + j] = 0.0f;
          for (int k = 0; k < 4; k++) {
              ubo.model[i * 4 + j] += rotY[i * 4 + k] * rotX[k * 4 + j];
          }
      }
  }

  // View matrix (camera at (0, 0, -5) looking at origin)
  ubo.view[0] = 1.0f; ubo.view[5] = 1.0f; ubo.view[10] = 1.0f;
  ubo.view[14] = -5.0f; // Move camera back to z = -5 (was -2)
  ubo.view[15] = 1.0f;

  // Projection matrix
  float aspect = (float)ctx->width / (float)ctx->height;
  float fov = 45.0f * 3.14159f / 180.0f;
  float near = 0.1f, far = 100.0f;
  float tanHalfFov = tanf(fov / 2.0f);
  ubo.proj[0] = 1.0f / (aspect * tanHalfFov);
  ubo.proj[5] = 1.0f / tanHalfFov;
  ubo.proj[10] = -(far + near) / (far - near);
  ubo.proj[11] = -1.0f;
  ubo.proj[14] = -(2.0f * far * near) / (far - near);

  updateBuffer(ctx, ctx->cubeUniformBufferMemory, sizeof(ubo), &ubo);

  // Render commands
  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->cubePipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->cubeVertexBuffer, offsets);
  vkCmdBindIndexBuffer(ctx->commandBuffer, ctx->cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->cubePipelineLayout, 0, 1, &ctx->cubeDescriptorSet, 0, NULL);

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
}




void flecs_cube3d_cleanup(WorldContext *ctx) {
    if (!ctx || !ctx->device) return;

    ecs_print(1, "Cube3D cleanup starting...");
    vkDeviceWaitIdle(ctx->device);

    if (ctx->cubePipeline != VK_NULL_HANDLE) vkDestroyPipeline(ctx->device, ctx->cubePipeline, NULL);
    if (ctx->cubePipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(ctx->device, ctx->cubePipelineLayout, NULL);
    if (ctx->cubeDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(ctx->device, ctx->cubeDescriptorSetLayout, NULL);
    if (ctx->cubeDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(ctx->device, ctx->cubeDescriptorPool, NULL);
    if (ctx->cubeVertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubeVertexBufferMemory, NULL);
    if (ctx->cubeVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubeVertexBuffer, NULL);
    if (ctx->cubeIndexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubeIndexBufferMemory, NULL);
    if (ctx->cubeIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubeIndexBuffer, NULL);
    if (ctx->cubeUniformBufferMemory != VK_NULL_HANDLE) vkFreeMemory(ctx->device, ctx->cubeUniformBufferMemory, NULL);
    if (ctx->cubeUniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(ctx->device, ctx->cubeUniformBuffer, NULL);

    ecs_print(1, "Cube3D cleanup completed");
}

void flecs_cube3d_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing cube3d module...");

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "Cube3DSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
        .callback = Cube3DSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "Cube3DRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
        .callback = Cube3DRenderSystem
    });

    ecs_print(1, "Cube3d module initialized");
}