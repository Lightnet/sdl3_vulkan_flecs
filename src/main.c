#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"
#include "flecs_imgui.h"


int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("Vulkan Triangle with ImGui", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
  printf("init main!\n");
  fflush(stdout);

  ecs_world_t *world = ecs_init();
  WorldContext *ctx = calloc(1, sizeof(WorldContext));
  if (!ctx) {
      printf("Failed to allocate WorldContext\n");
      SDL_Quit();
      return -1;
  }
  ctx->window = window;

  ecs_print(1, "Setting world context...");
  ecs_set_ctx(world, ctx, NULL);

  ecs_print(1, "Initializing GlobalPhases...");
  flecs_phases_init(world, &GlobalPhases);

  ecs_print(1, "Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world, ctx);

  ecs_print(1, "Calling flecs_imgui_module_init...");
  flecs_imgui_module_init(world, ctx);

  ecs_print(1, "Running setup phases...");
  ecs_progress(world, 0);
  ecs_print(1, "Post-setup context: %p", (void*)ctx);
  ecs_print(1, "Post-setup fence: %p", (void*)ctx->inFlightFence);
  fflush(stdout);

  ecs_print(1, "Entering main loop...");
  while (!ctx->shouldQuit) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
          if (ctx->isImGuiInitialized) {
              ImGui_ImplSDL3_ProcessEvent(&event);
          }
          if (event.type == SDL_EVENT_QUIT) {
              ecs_print(1, "Quit event received");
              ctx->shouldQuit = true;
          }
      }
      ecs_progress(world, 0);
  }

  ecs_print(1, "Cleaning up...");
  // Explicit cleanup in order: ImGui first, then Vulkan, then Flecs
  if (ctx->isImGuiInitialized) {
      flecs_imgui_cleanup(ctx);
  }
  flecs_vulkan_cleanup(world, ctx);
  ecs_fini(world);
  SDL_DestroyWindow(ctx->window);
  SDL_Quit();
  free(ctx);
  ecs_print(1, "Program exiting");
  return 0;
}


