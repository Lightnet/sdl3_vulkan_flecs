#ifndef FLECS_ASSIMP_H
#define FLECS_ASSIMP_H

#include "flecs.h"
#include <vulkan/vulkan.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Vertex structure for 3D models (used with Assimp and other 3D rendering)
typedef struct {
  float pos[3];      // 3D position (x, y, z)
  float color[3];    // RGB color (r, g, b)
  float texCoord[2]; // UV texture coordinates (u, v)
} Vertex3d;

// Vertex structure for 2D rendering (used with triangle2d module)
typedef struct {
  float pos[2];      // 2D position (x, y)
  float color[3];    // RGB color (r, g, b)
} Vertex2d;

typedef struct {
  VkBuffer assimp_vertexBuffer;
  VkDeviceMemory assimp_vertexBufferMemory;
  VkBuffer assimp_indexBuffer;
  VkDeviceMemory assimp_indexBufferMemory;
  uint32_t assimp_vertexCount;
  uint32_t assimp_indexCount;
  VkBuffer assimp_uniformBuffer;
  VkDeviceMemory assimp_uniformBufferMemory;
  VkDescriptorPool assimp_descriptorPool;
  VkDescriptorSet assimp_descriptorSet;
  VkDescriptorSetLayout assimp_descriptorSetLayout;
  VkPipelineLayout assimp_pipelineLayout;
  VkPipeline assimp_graphicsPipeline;
} AssimpModelContext;

ECS_COMPONENT_DECLARE(AssimpModelContext);

void flecs_assimp_module_init(ecs_world_t *world);
void flecs_assimp_cleanup(ecs_world_t *world);

#endif