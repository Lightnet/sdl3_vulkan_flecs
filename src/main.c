#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"

#define WIDTH 800
#define HEIGHT 600



int main(int argc, char *argv[]) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window *window = SDL_CreateWindow("Vulkan Triangle", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
  printf("init main!\n");  // Confirm main starts

  ecs_world_t *world = ecs_init();
  WorldContext ctx = {0};
  ctx.window = window;
  printf("Calling flecs_vulkan_module_init...\n");
  flecs_vulkan_module_init(world, &ctx);

  printf("Entering main loop...\n");
  while (!ctx.shouldQuit) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_EVENT_QUIT) {
              printf("Quit event received\n");
              ctx.shouldQuit = true;
          }
      }
      ecs_progress(world, 0);
  }

  printf("Cleaning up...\n");
  flecs_vulkan_cleanup(world, &ctx);
  ecs_fini(world);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf("Program exiting\n");
  return 0;
}



