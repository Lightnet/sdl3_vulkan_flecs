#ifndef FLECS_UTILS_H
#define FLECS_UTILS_H

#include <SDL3/SDL.h>       //SDL 3.x
#include <SDL3/SDL_vulkan.h>//SDL 3.x
#include <vulkan/vulkan.h>
#include <flecs.h>
#include <stdbool.h>
#include "flecs_sdl.h"

// Helper function to report SDL errors and abort
static inline void report_sdl_error(SDLContext *sdl_ctx, const char *error_msg) {
  sdl_ctx->hasError = true;
  sdl_ctx->errorMessage = error_msg;
  ecs_err("Error: %s", error_msg);
  ecs_abort(ECS_INTERNAL_ERROR, error_msg);
}

// Helper function to check SDL context and handle errors
static inline bool ensure_sdl_context(ecs_world_t *world, SDLContext **out_sdl_ctx, const char *error_msg) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) {
      if (sdl_ctx) {
          sdl_ctx->hasError = true;
          sdl_ctx->errorMessage = error_msg;
          ecs_err("Error: %s", error_msg);
          ecs_abort(ECS_INTERNAL_ERROR, error_msg);
      }
      *out_sdl_ctx = NULL;
      return false;
  }
  *out_sdl_ctx = sdl_ctx;
  return true;
}

// H for header files
static VkShaderModule createShaderModuleH(VkDevice device, const uint32_t *code, size_t codeSize) {
  if (!device || !code || codeSize == 0) {
      ecs_err("Invalid shader module parameters");
      return VK_NULL_HANDLE;
  }
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


#endif // FLECS_UTILS_H