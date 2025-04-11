#ifndef FLECS_CUBE3D_H
#define FLECS_CUBE3D_H

#include <flecs.h>
#include <vulkan/vulkan.h>
#include "flecs_types.h"

typedef struct {
  // Cube3D
  VkBuffer cubeVertexBuffer;
  VkDeviceMemory cubeVertexBufferMemory;
  VkBuffer cubeIndexBuffer;
  VkDeviceMemory cubeIndexBufferMemory;
  VkBuffer cubeUniformBuffer;
  VkDeviceMemory cubeUniformBufferMemory;
  VkDescriptorPool cubeDescriptorPool;
  VkDescriptorSet cubeDescriptorSet;
  VkDescriptorSetLayout cubeDescriptorSetLayout;
  VkPipelineLayout cubePipelineLayout;
  VkPipeline cubePipeline;
} Cube3DContext;
ECS_COMPONENT_DECLARE(Cube3DContext);

void flecs_cube3d_module_init(ecs_world_t *world);
void flecs_cube3d_cleanup(ecs_world_t *world);

#endif