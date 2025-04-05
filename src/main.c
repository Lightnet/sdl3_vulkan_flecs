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
  //ecs_log_set_level(1);  // Enable logging at level 1 (info)

  WorldContext ctx = {0};
  ctx.window = window;
  ecs_print(1,"Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world, &ctx);

  ecs_print(1,"Entering main loop...");
  while (!ctx.shouldQuit) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_EVENT_QUIT) {
              ecs_print(1,"Quit event received");
              ctx.shouldQuit = true;
          }
      }
      ecs_progress(world, 0);
  }

  ecs_print(1,"Cleaning up...");
  flecs_vulkan_cleanup(world, &ctx);
  ecs_fini(world);
  SDL_DestroyWindow(window);
  SDL_Quit();
  ecs_print(1,"Program exiting");
  return 0;
}



