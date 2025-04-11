#ifndef FLECS_CUBETEXTURE3D_H
#define FLECS_CUBETEXTURE3D_H

#include "flecs_types.h"

typedef struct {
  // CubeTexture3D
  VkBuffer cubetexture3dVertexBuffer;
  VkDeviceMemory cubetexture3dVertexBufferMemory;
  VkBuffer cubetexture3dIndexBuffer;
  VkDeviceMemory cubetexture3dIndexBufferMemory;
  VkBuffer cubetexture3dUniformBuffer;
  VkDeviceMemory cubetexture3dUniformBufferMemory;
  VkDescriptorPool cubetexture3dDescriptorPool;
  VkDescriptorSet cubetexture3dDescriptorSet;
  VkDescriptorSetLayout cubetexture3dDescriptorSetLayout;
  VkPipelineLayout cubetexture3dPipelineLayout;
  VkPipeline cubetexture3dPipeline;
  VkImage cubetexture3dImage;
  VkDeviceMemory cubetexture3dImageMemory;
  VkImageView cubetexture3dImageView;
  VkSampler cubetexture3dSampler;
} CubeText3DContext;

ECS_COMPONENT_DECLARE(CubeText3DContext);

void flecs_cubetexture3d_module_init(ecs_world_t *world);
void flecs_cubetexture3d_cleanup(ecs_world_t *world);

#endif