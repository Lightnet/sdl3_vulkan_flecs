#ifndef FLECS_TRIANGLE2D_H
#define FLECS_TRIANGLE2D_H

#include "flecs_types.h"

typedef struct {
// Triangle Mesh
VkBuffer triVertexBuffer;                    // Triangle vertex buffer
VkDeviceMemory triVertexBufferMemory;        // Triangle vertex buffer memory
VkBuffer triIndexBuffer;                     // Triangle index buffer
VkDeviceMemory triIndexBufferMemory;         // Triangle index buffer memory
VkPipelineLayout triPipelineLayout;          // Triangle pipeline layout
VkPipeline triGraphicsPipeline;              // Triangle graphics pipeline
} TriangleContext;
ECS_COMPONENT_DECLARE(TriangleContext);


void flecs_triangle2d_module_init(ecs_world_t *world, WorldContext *ctx);
void TrianglePipelineSetupSystem(ecs_iter_t *it);
void flecs_triangle2d_cleanup(ecs_world_t *world);

#endif