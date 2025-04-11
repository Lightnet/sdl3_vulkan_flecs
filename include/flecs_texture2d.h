#ifndef FLECS_TEXTURE2D_H
#define FLECS_TEXTURE2D_H

#include "flecs_types.h"

typedef struct {
  // Texture2D
  VkBuffer texture2dVertexBuffer;
  VkDeviceMemory texture2dVertexBufferMemory;
  VkBuffer texture2dIndexBuffer;
  VkDeviceMemory texture2dIndexBufferMemory;
  VkDescriptorPool texture2dDescriptorPool;
  VkDescriptorSet texture2dDescriptorSet;
  VkDescriptorSetLayout texture2dDescriptorSetLayout;
  VkPipelineLayout texture2dPipelineLayout;
  VkPipeline texture2dPipeline;
  VkImage texture2dImage;
  VkDeviceMemory texture2dImageMemory;
  VkImageView texture2dImageView;
  VkSampler texture2dSampler;
} Texture2DContext;
ECS_COMPONENT_DECLARE(Texture2DContext);

void flecs_texture2d_module_init(ecs_world_t *world);
void flecs_texture2d_cleanup(ecs_world_t *world);

#endif