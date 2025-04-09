#ifndef FLECS_VULKAN_H
#define FLECS_VULKAN_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "flecs.h"
#include "flecs_types.h"

void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx);

void InstanceSetupSystem(ecs_iter_t *it);
void SurfaceSetupSystem(ecs_iter_t *it);
void DeviceSetupSystem(ecs_iter_t *it);
void SwapchainSetupSystem(ecs_iter_t *it);
void TriangleBufferSetupSystem(ecs_iter_t *it);
void RenderPassSetupSystem(ecs_iter_t *it);    
void FramebufferSetupSystem(ecs_iter_t *it);   
void CommandPoolSetupSystem(ecs_iter_t *it);   
void CommandBufferSetupSystem(ecs_iter_t *it); 
void PipelineSetupSystem(ecs_iter_t *it);      
void SyncSetupSystem(ecs_iter_t *it);
void BeginRenderSystem(ecs_iter_t *it);
void RenderSystem(ecs_iter_t *it);
void EndRenderSystem(ecs_iter_t *it);

void flecs_vulkan_cleanup(ecs_world_t *world, WorldContext *ctx);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData);

#endif