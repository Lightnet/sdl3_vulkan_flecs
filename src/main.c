// Main Entry

#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"
#include "flecs_imgui.h"
#include "flecs_text.h"
#include "flecs_sdl.h"
#include "flecs_texture2d.h"
#include "flecs_triangle2d.h"
#include "flecs_cube3d.h"
#include "flecs_cubetexture3d.h"

int main(int argc, char *argv[]) {

  ecs_world_t *world = ecs_init();
  ecs_print(1, "init main, flecs ...");
  WorldContext *ctx = calloc(1, sizeof(WorldContext));
  if (!ctx) {
      ecs_err("Failed to allocate WorldContext");
      return -1;
  }

  //this must be set for the window size
  ctx->width = 800;
  ctx->height = 600;
  //this need to be load first
  ecs_print(1, "Setting world context...");
  ecs_set_ctx(world, ctx, NULL);
  //this need to be load second for phase for setup and runtime render
  ecs_print(1, "Initializing GlobalPhases...");
  flecs_phases_init(world, &GlobalPhases);
  ecs_print(1, "Calling flecs_sdl_module_init...");
  // setup SDL 3.x window and Input event 
  flecs_sdl_module_init(world, ctx);
  // setup Vulkan graphic
  ecs_print(1, "Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world, ctx);

  ecs_print(1, "Calling flecs_cubetexture3d_module_init...");
  flecs_cubetexture3d_module_init(world, ctx);

  // ecs_print(1, "Calling flecs_triangle2d_module_init...");
  // flecs_triangle2d_module_init(world, ctx);

  // ecs_print(1, "Calling flecs_texture2d_module_init...");
  // flecs_texture2d_module_init(world, ctx);

  ecs_print(1, "Calling flecs_imgui_module_init...");
  flecs_imgui_module_init(world, ctx);

  // ecs_print(1, "Calling flecs_text_module_init...");
  // flecs_text_module_init(world, ctx);

  // ecs_print(1, "Calling flecs_cube3d_module_init...");
  // flecs_cube3d_module_init(world, ctx);

  // ecs_print(1, "Running setup phases...");
  // ecs_progress(world, 0);
  // ecs_print(1, "Post-setup context: %p", (void*)ctx);
  // ecs_print(1, "Post-setup fence: %p", (void*)ctx->inFlightFence);

  ecs_print(1, "Entering main loop...");

  Uint64 previousTime = SDL_GetTicks(); // Time at the start
  //ecs_abort(ECS_INTERNAL_ERROR, "TEST"); // Test error

  while (!ctx->shouldQuit) {
    // Get current time and calculate delta time
    Uint64 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - previousTime) / 1000.0f; // Convert ms to seconds
    previousTime = currentTime; // Update previous time for next frame
    // Print delta time
    //printf("Delta Time: %f seconds\n", deltaTime);

    ecs_progress(world, deltaTime);  // All event handling is now in ImGuiInputSystem
  }

  ecs_print(1, "Cleaning up...");
  // Explicit cleanup in order: ImGui first, then Vulkan, then Flecs
  if (ctx->isImGuiInitialized) {
      flecs_imgui_cleanup(ctx);
  }
  flecs_cubetexture3d_cleanup(ctx);
  // flecs_text_cleanup(ctx);
  // flecs_triangle2d_cleanup(ctx);
  // flecs_texture2d_cleanup(ctx);
  // flecs_cube3d_cleanup(ctx);
  flecs_vulkan_cleanup(world, ctx);
  ecs_fini(world);
  flecs_sdl_cleanup(ctx);
  free(ctx);
  ecs_print(1, "Program exiting");
  return 0;
}


