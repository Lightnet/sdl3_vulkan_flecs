#include "flecs_assimp.h"
#include "flecs_types.h"
#include "flecs_sdl.h"
#include "flecs_vulkan.h"
#include "flecs_utils.h"
#include "shaders/assimp_shader3d_vert.spv.h"
#include "shaders/assimp_shader3d_frag.spv.h"
#include <cglm/cglm.h> // Include cglm

typedef struct {
  mat4 model; // cglm's mat4 is a float[4][4]
  mat4 view;
  mat4 proj;
} UniformBufferObject;

// Helper to load model using Assimp
static bool assimp_load_model(const char *filePath, Vertex3d **vertices, uint32_t *vertexCount, uint32_t **indices, uint32_t *indexCount) {
  const struct aiScene *scene = aiImportFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
      ecs_err("Assimp error: %s", aiGetErrorString());
      return false;
  }

  struct aiMesh *mesh = scene->mMeshes[0];
  *vertexCount = mesh->mNumVertices;
  *vertices = malloc(sizeof(Vertex3d) * (*vertexCount));
  *indexCount = mesh->mNumFaces * 3;
  *indices = malloc(sizeof(uint32_t) * (*indexCount));

  for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
      (*vertices)[i].pos[0] = mesh->mVertices[i].x;
      (*vertices)[i].pos[1] = mesh->mVertices[i].y;
      (*vertices)[i].pos[2] = mesh->mVertices[i].z;
      (*vertices)[i].color[0] = 1.0f;
      (*vertices)[i].color[1] = 1.0f;
      (*vertices)[i].color[2] = 1.0f;
      (*vertices)[i].texCoord[0] = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i].x : 0.0f;
      (*vertices)[i].texCoord[1] = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0][i].y : 0.0f;
  }

  for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
      struct aiFace face = mesh->mFaces[i];
      for (uint32_t j = 0; j < face.mNumIndices; j++) {
          (*indices)[i * 3 + j] = face.mIndices[j];
      }
  }

  aiReleaseImport(scene);
  return true;
}

// Create uniform buffer
static bool assimp_create_uniform_buffer(VulkanContext *v_ctx, AssimpModelContext *assimp_ctx, SDLContext *sdl_ctx) {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(v_ctx->device, &bufferInfo, NULL, &assimp_ctx->assimp_uniformBuffer) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp uniform buffer");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp uniform buffer";
      return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(v_ctx->device, assimp_ctx->assimp_uniformBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memRequirements.size;
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((memRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
          allocInfo.memoryTypeIndex = i;
          break;
      }
  }

  if (vkAllocateMemory(v_ctx->device, &allocInfo, NULL, &assimp_ctx->assimp_uniformBufferMemory) != VK_SUCCESS) {
      ecs_err("Failed to allocate Assimp uniform buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to allocate Assimp uniform buffer memory";
      return false;
  }

  if (vkBindBufferMemory(v_ctx->device, assimp_ctx->assimp_uniformBuffer, assimp_ctx->assimp_uniformBufferMemory, 0) != VK_SUCCESS) {
      ecs_err("Failed to bind Assimp uniform buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to bind Assimp uniform buffer memory";
      return false;
  }

  return true;
}

// Setup system
void AssimpModelSetupSystem(ecs_iter_t *it) {
  ecs_log(1, "AssimpModelSetupSystem");

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;
  AssimpModelContext *assimp_ctx = ecs_singleton_ensure(it->world, AssimpModelContext);
  if (!assimp_ctx) return;

  // Load model
  Vertex3d *vertices = NULL;
  uint32_t *indices = NULL;
  if (!assimp_load_model("assets/cube.obj", &vertices, &assimp_ctx->assimp_vertexCount, &indices, &assimp_ctx->assimp_indexCount)) {
      ecs_err("Failed to load model");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to load model";
      return;
  }

  // Vertex Buffer Setup
  VkDeviceSize vertexBufferSize = sizeof(Vertex3d) * assimp_ctx->assimp_vertexCount;
  VkBufferCreateInfo vertexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  vertexBufferInfo.size = vertexBufferSize;
  vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(v_ctx->device, &vertexBufferInfo, NULL, &assimp_ctx->assimp_vertexBuffer) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp vertex buffer");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp vertex buffer";
      free(vertices);
      free(indices);
      return;
  }

  VkMemoryRequirements vertexMemRequirements;
  vkGetBufferMemoryRequirements(v_ctx->device, assimp_ctx->assimp_vertexBuffer, &vertexMemRequirements);

  VkMemoryAllocateInfo vertexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  vertexAllocInfo.allocationSize = vertexMemRequirements.size;
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((vertexMemRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
          vertexAllocInfo.memoryTypeIndex = i;
          break;
      }
  }

  if (vkAllocateMemory(v_ctx->device, &vertexAllocInfo, NULL, &assimp_ctx->assimp_vertexBufferMemory) != VK_SUCCESS) {
      ecs_err("Failed to allocate Assimp vertex buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to allocate Assimp vertex buffer memory";
      free(vertices);
      free(indices);
      return;
  }

  if (vkBindBufferMemory(v_ctx->device, assimp_ctx->assimp_vertexBuffer, assimp_ctx->assimp_vertexBufferMemory, 0) != VK_SUCCESS) {
      ecs_err("Failed to bind Assimp vertex buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to bind Assimp vertex buffer memory";
      free(vertices);
      free(indices);
      return;
  }

  void *vertexData;
  if (vkMapMemory(v_ctx->device, assimp_ctx->assimp_vertexBufferMemory, 0, vertexBufferSize, 0, &vertexData) != VK_SUCCESS) {
      ecs_err("Failed to map Assimp vertex buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to map Assimp vertex buffer memory";
      free(vertices);
      free(indices);
      return;
  }
  memcpy(vertexData, vertices, (size_t)vertexBufferSize);
  vkUnmapMemory(v_ctx->device, assimp_ctx->assimp_vertexBufferMemory);
  free(vertices);

  // Index Buffer Setup (unchanged)
  VkDeviceSize indexBufferSize = sizeof(uint32_t) * assimp_ctx->assimp_indexCount;
  VkBufferCreateInfo indexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  indexBufferInfo.size = indexBufferSize;
  indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(v_ctx->device, &indexBufferInfo, NULL, &assimp_ctx->assimp_indexBuffer) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp index buffer");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp index buffer";
      free(indices);
      return;
  }

  VkMemoryRequirements indexMemRequirements;
  vkGetBufferMemoryRequirements(v_ctx->device, assimp_ctx->assimp_indexBuffer, &indexMemRequirements);

  VkMemoryAllocateInfo indexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  indexAllocInfo.allocationSize = indexMemRequirements.size;
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((indexMemRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
          indexAllocInfo.memoryTypeIndex = i;
          break;
      }
  }

  if (vkAllocateMemory(v_ctx->device, &indexAllocInfo, NULL, &assimp_ctx->assimp_indexBufferMemory) != VK_SUCCESS) {
      ecs_err("Failed to allocate Assimp index buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to allocate Assimp index buffer memory";
      free(indices);
      return;
  }

  if (vkBindBufferMemory(v_ctx->device, assimp_ctx->assimp_indexBuffer, assimp_ctx->assimp_indexBufferMemory, 0) != VK_SUCCESS) {
      ecs_err("Failed to bind Assimp index buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to bind Assimp index buffer memory";
      free(indices);
      return;
  }

  void *indexData;
  if (vkMapMemory(v_ctx->device, assimp_ctx->assimp_indexBufferMemory, 0, indexBufferSize, 0, &indexData) != VK_SUCCESS) {
      ecs_err("Failed to map Assimp index buffer memory");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to map Assimp index buffer memory";
      free(indices);
      return;
  }
  memcpy(indexData, indices, (size_t)indexBufferSize);
  vkUnmapMemory(v_ctx->device, assimp_ctx->assimp_indexBufferMemory);
  free(indices);

  // Uniform Buffer Setup
  if (!assimp_create_uniform_buffer(v_ctx, assimp_ctx, sdl_ctx)) {
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

  if (vkCreateDescriptorSetLayout(v_ctx->device, &layoutInfo, NULL, &assimp_ctx->assimp_descriptorSetLayout) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp descriptor set layout");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp descriptor set layout";
      return;
  }

  // Descriptor Pool
  VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;

  if (vkCreateDescriptorPool(v_ctx->device, &poolInfo, NULL, &assimp_ctx->assimp_descriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp descriptor pool");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp descriptor pool";
      return;
  }

  // Descriptor Set
  VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = assimp_ctx->assimp_descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &assimp_ctx->assimp_descriptorSetLayout;

  if (vkAllocateDescriptorSets(v_ctx->device, &allocInfo, &assimp_ctx->assimp_descriptorSet) != VK_SUCCESS) {
      ecs_err("Failed to allocate Assimp descriptor set");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to allocate Assimp descriptor set";
      return;
  }

  VkDescriptorBufferInfo bufferInfo = {0};
  bufferInfo.buffer = assimp_ctx->assimp_uniformBuffer;
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UniformBufferObject);

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = assimp_ctx->assimp_descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(v_ctx->device, 1, &descriptorWrite, 0, NULL);

  // Shader and Pipeline Setup
  VkShaderModule vertShaderModule = createShaderModuleH(v_ctx->device, assimp_shader3d_vert_spv, sizeof(assimp_shader3d_vert_spv));
  VkShaderModule fragShaderModule = createShaderModuleH(v_ctx->device, assimp_shader3d_frag_spv, sizeof(assimp_shader3d_frag_spv));

  if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
      ecs_err("Failed to create Assimp shader modules");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp shader modules";
      return;
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
  bindingDesc.stride = sizeof(Vertex3d);
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescs[3] = {0};
  attributeDescs[0].binding = 0;
  attributeDescs[0].location = 0;
  attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescs[0].offset = offsetof(Vertex3d, pos);
  attributeDescs[1].binding = 0;
  attributeDescs[1].location = 1;
  attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescs[1].offset = offsetof(Vertex3d, color);
  attributeDescs[2].binding = 0;
  attributeDescs[2].location = 2;
  attributeDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescs[2].offset = offsetof(Vertex3d, texCoord);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = 3;
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
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &assimp_ctx->assimp_descriptorSetLayout;

  if (vkCreatePipelineLayout(v_ctx->device, &pipelineLayoutInfo, NULL, &assimp_ctx->assimp_pipelineLayout) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp pipeline layout");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp pipeline layout";
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
  pipelineInfo.layout = assimp_ctx->assimp_pipelineLayout;
  pipelineInfo.renderPass = v_ctx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(v_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &assimp_ctx->assimp_graphicsPipeline) != VK_SUCCESS) {
      ecs_err("Failed to create Assimp graphics pipeline");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create Assimp graphics pipeline";
      return;
  }

  vkDestroyShaderModule(v_ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(v_ctx->device, vertShaderModule, NULL);

  ecs_log(1, "Assimp buffer and pipeline setup completed");
}

// Update uniform buffer
static void assimp_update_uniform_buffer(VulkanContext *v_ctx, AssimpModelContext *assimp_ctx, SDLContext *sdl_ctx, float time) {
  UniformBufferObject ubo;

  // Model matrix: rotate around Y-axis
  glm_mat4_identity(ubo.model);
  glm_rotate_y(ubo.model, time, ubo.model);

  // View matrix: camera at (0, 0, 5) looking at origin
  vec3 eye = {0.0f, 0.0f, 5.0f};
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up = {0.0f, 1.0f, 0.0f};
  glm_lookat(eye, center, up, ubo.view);

  // Projection matrix: perspective
  float aspect = (float)sdl_ctx->width / (float)sdl_ctx->height;
  glm_perspective(glm_rad(45.0f), aspect, 0.1f, 100.0f, ubo.proj);
  ubo.proj[1][1] *= -1; // Flip Y-axis for Vulkan's coordinate system

  void *data;
  vkMapMemory(v_ctx->device, assimp_ctx->assimp_uniformBufferMemory, 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(v_ctx->device, assimp_ctx->assimp_uniformBufferMemory);
}

// Update uniform buffer system
void AssimpModelUpdateSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;
  AssimpModelContext *assimp_ctx = ecs_singleton_ensure(it->world, AssimpModelContext);
  if (!assimp_ctx) return;

  static float time = 0.0f;
  time += it->delta_time;
  assimp_update_uniform_buffer(v_ctx, assimp_ctx, sdl_ctx, time);
}

// Render system
void AssimpModelRenderSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;
  AssimpModelContext *assimp_ctx = ecs_singleton_ensure(it->world, AssimpModelContext);
  if (!assimp_ctx) return;

  vkCmdBindPipeline(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, assimp_ctx->assimp_graphicsPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(v_ctx->commandBuffer, 0, 1, &assimp_ctx->assimp_vertexBuffer, offsets);
  vkCmdBindIndexBuffer(v_ctx->commandBuffer, assimp_ctx->assimp_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, assimp_ctx->assimp_pipelineLayout, 0, 1, &assimp_ctx->assimp_descriptorSet, 0, NULL);
  vkCmdDrawIndexed(v_ctx->commandBuffer, assimp_ctx->assimp_indexCount, 1, 0, 0, 0);
}

void flecs_assimp_model_cleanup(ecs_world_t *world) {
  VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
  if (!v_ctx || !v_ctx->device) return;
  AssimpModelContext *ctx = ecs_singleton_ensure(world, AssimpModelContext);
  if (!ctx) return;

  ecs_log(1, "Assimp model cleanup starting...");
  vkDeviceWaitIdle(v_ctx->device);

  if (ctx->assimp_vertexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(v_ctx->device, ctx->assimp_vertexBuffer, NULL);
      ctx->assimp_vertexBuffer = VK_NULL_HANDLE;
  }
  if (ctx->assimp_vertexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, ctx->assimp_vertexBufferMemory, NULL);
      ctx->assimp_vertexBufferMemory = VK_NULL_HANDLE;
  }
  if (ctx->assimp_indexBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(v_ctx->device, ctx->assimp_indexBuffer, NULL);
      ctx->assimp_indexBuffer = VK_NULL_HANDLE;
  }
  if (ctx->assimp_indexBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, ctx->assimp_indexBufferMemory, NULL);
      ctx->assimp_indexBufferMemory = VK_NULL_HANDLE;
  }
  if (ctx->assimp_uniformBuffer != VK_NULL_HANDLE) {
      vkDestroyBuffer(v_ctx->device, ctx->assimp_uniformBuffer, NULL);
      ctx->assimp_uniformBuffer = VK_NULL_HANDLE;
  }
  if (ctx->assimp_uniformBufferMemory != VK_NULL_HANDLE) {
      vkFreeMemory(v_ctx->device, ctx->assimp_uniformBufferMemory, NULL);
      ctx->assimp_uniformBufferMemory = VK_NULL_HANDLE;
  }
  if (ctx->assimp_descriptorPool != VK_NULL_HANDLE) {
      vkDestroyDescriptorPool(v_ctx->device, ctx->assimp_descriptorPool, NULL);
      ctx->assimp_descriptorPool = VK_NULL_HANDLE;
  }
  if (ctx->assimp_descriptorSetLayout != VK_NULL_HANDLE) {
      vkDestroyDescriptorSetLayout(v_ctx->device, ctx->assimp_descriptorSetLayout, NULL);
      ctx->assimp_descriptorSetLayout = VK_NULL_HANDLE;
  }
  if (ctx->assimp_graphicsPipeline != VK_NULL_HANDLE) {
      vkDestroyPipeline(v_ctx->device, ctx->assimp_graphicsPipeline, NULL);
      ctx->assimp_graphicsPipeline = VK_NULL_HANDLE;
  }
  if (ctx->assimp_pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(v_ctx->device, ctx->assimp_pipelineLayout, NULL);
      ctx->assimp_pipelineLayout = VK_NULL_HANDLE;
  }

  ecs_log(1, "Assimp model cleanup completed");
}

// Cleanup system
void assimp_model_cleanup_event_system(ecs_iter_t *it) {
  ecs_print(1, "[cleanup] assimp_model_cleanup_event_system");
  flecs_assimp_model_cleanup(it->world);
  module_break_name(it, "assimp_model_module");
}

// Register components
void assimp_register_components(ecs_world_t *world) {
  ECS_COMPONENT_DEFINE(world, AssimpModelContext);
}

// Register systems
void assimp_model_register_systems(ecs_world_t *world) {
    ecs_observer(world, {
        .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
        .events = { CleanUpEvent },
        .callback = assimp_model_cleanup_event_system
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "AssimpModelSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
        .callback = AssimpModelSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "AssimpModelUpdateSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.LogicUpdatePhase)) }),
        .callback = AssimpModelUpdateSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "AssimpModelRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
        .callback = AssimpModelRenderSystem
    });
}

// Initialize Assimp module
void flecs_assimp_module_init(ecs_world_t *world) {
  ecs_log(1, "Initializing assimp module...");

  assimp_register_components(world);

  ecs_singleton_set(world, AssimpModelContext, {0});

  ecs_entity_t e = ecs_new(world);
  ecs_set(world, e, PluginModule, { .name = "assimp_model_module", .isCleanUp = false });

  assimp_model_register_systems(world);

  ecs_log(1, "Assimp module initialized");
}