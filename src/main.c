// Main Entry

#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"
#include "flecs_imgui.h"
#include "flecs_text.h"
#include "flecs_sdl.h"


int main(int argc, char *argv[]) {
  // printf("init main!\n");
  // fflush(stdout);

  // if(!SDL_Init(SDL_INIT_VIDEO)){
  //   SDL_Log( "SDL could not initialize! SDL error: %s\n", SDL_GetError() );
  //   return -1;
  // }

  // SDL_Window *window = SDL_CreateWindow("Vulkan Triangle with ImGui",
  //   WIDTH, 
  //   HEIGHT, 
  //   SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
  // );

  // if (!window){
  //   printf("SDL_CreateWindow fail!\n");
  //   SDL_Log( "Window could not be created! SDL error: %s\n", SDL_GetError() );
  //   SDL_Quit();
  //   return -1;
  // }
  
  ecs_world_t *world = ecs_init();
  ecs_print(1, "init flecs ...");
  WorldContext *ctx = calloc(1, sizeof(WorldContext));
  if (!ctx) {
      printf("Failed to allocate WorldContext\n");
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
  // SDL window and Input event 
  flecs_sdl_module_init(world, ctx);
  // Vulkan graphic
  ecs_print(1, "Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world, ctx);

  ecs_print(1, "Calling flecs_imgui_module_init...");
  flecs_imgui_module_init(world, ctx);

  ecs_print(1, "Calling flecs_text_module_init...");
  flecs_text_module_init(world, ctx);

  ecs_print(1, "Running setup phases...");
  ecs_progress(world, 0);
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
  flecs_text_cleanup(ctx); 
  flecs_vulkan_cleanup(world, ctx);
  ecs_fini(world);
  SDL_DestroyWindow(ctx->window);
  SDL_Quit();
  free(ctx);
  ecs_print(1, "Program exiting");
  return 0;
}


