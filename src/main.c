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

  ecs_world_t *world = ecs_init();
  WorldContext ctx = {0};
  ctx.window = window;

  // Initialize GlobalPhases once here
  ecs_print(1, "Initializing GlobalPhases...");
  flecs_phases_init(world, &GlobalPhases);

  ecs_print(1, "Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world, &ctx);

  ecs_print(1, "Calling flecs_imgui_module_init...");
  flecs_imgui_module_init(world, &ctx);

  ecs_print(1, "Entering main loop...");
  while (!ctx.shouldQuit) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
          if (ctx.isImGuiInitialized) { // Only process events if ImGui is ready
            ImGui_ImplSDL3_ProcessEvent(&event);
          }
          if (event.type == SDL_EVENT_QUIT) {
              ecs_print(1, "Quit event received");
              ctx.shouldQuit = true;
          }
      }
      ecs_progress(world, 0);
  }

  ecs_print(1, "Cleaning up...");
  flecs_imgui_cleanup(&ctx);
  flecs_vulkan_cleanup(world, &ctx);
  ecs_fini(world);
  SDL_DestroyWindow(window);
  SDL_Quit();
  ecs_print(1, "Program exiting");
  return 0;
}

