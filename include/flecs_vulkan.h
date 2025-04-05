#ifndef FLECS_VULKAN_H
#define FLECS_VULKAN_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "flecs.h"
#include "flecs_types.h"

void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx);

void init_vulkan_system(ecs_iter_t *it);
void BeginRenderSystem(ecs_iter_t *it);
void RenderSystem(ecs_iter_t *it);
void EndRenderSystem(ecs_iter_t *it);
void cleanup_vulkan(WorldContext* ctx);

VkShaderModule createShaderModule(VkDevice device, const char* code);

#endif