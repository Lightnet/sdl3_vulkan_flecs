#ifndef FLECS_ASSETS3D_H
#define FLECS_ASSETS3D_H

#include "flecs.h"
#include <vulkan/vulkan.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Vertex structure for 3D models (used with Assimp and other 3D rendering)
// typedef struct {
//   float pos[3];      // 3D position (x, y, z)
//   float color[3];    // RGB color (r, g, b)
//   float texCoord[2]; // UV texture coordinates (u, v)
// } Vertex3d;

// // Vertex structure for 2D rendering (used with triangle2d module)
// typedef struct {
//   float pos[2];      // 2D position (x, y)
//   float color[3];    // RGB color (r, g, b)
// } Vertex2d;

typedef struct {
  VkBuffer assets3d_vertexBuffer;
  VkDeviceMemory assets3d_vertexBufferMemory;
  VkBuffer assets3d_indexBuffer;
  VkDeviceMemory assets3d_indexBufferMemory;
  uint32_t assets3d_vertexCount;
  uint32_t assets3d_indexCount;
  VkBuffer assets3d_uniformBuffer;
  VkDeviceMemory assets3d_uniformBufferMemory;
  VkDescriptorPool assets3d_descriptorPool;
  VkDescriptorSet assets3d_descriptorSet;
  VkDescriptorSetLayout assets3d_descriptorSetLayout;
  VkPipelineLayout assets3d_pipelineLayout;
  VkPipeline assets3d_graphicsPipeline;
} Assets3DModelContext;

ECS_COMPONENT_DECLARE(Assets3DModelContext);

void flecs_assets3d_module_init(ecs_world_t *world);
void flecs_assets3d_cleanup(ecs_world_t *world);

#endif