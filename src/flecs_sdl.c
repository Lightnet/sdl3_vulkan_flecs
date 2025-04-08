#include "flecs_sdl.h"
#include "flecs_vulkan.h"
#include "flecs_types.h"
#include <SDL3/SDL.h>       //SDL 3.x
#include <SDL3/SDL_vulkan.h>//SDL 3.x
#include <vulkan/vulkan.h>

// set up SDL window.
void SDLSetUpSystem(ecs_iter_t *it){
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  ecs_print(1, "SDL_Init");

  ecs_print(1, "WINDOW SIZE - WIDTH: %d, HEIGHT: %d", ctx->width, ctx->height);

  if(!SDL_Init(SDL_INIT_VIDEO)){
    ecs_err( "SDL could not initialize! SDL error: %s\n", SDL_GetError() );
  }

  ecs_print(1, "SDL_CreateWindow");
  ctx->window = SDL_CreateWindow("Vulkan Triangle with ImGui",
    ctx->width, 
    ctx->height, 
    SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
  );

  if (!ctx->window){
    ecs_err( "Window could not be created! SDL error: %s\n", SDL_GetError());
    ecs_abort(ECS_INTERNAL_ERROR, "Window could not be created! SDL error!");
  }
  ctx->needsSwapchainRecreation = false;

}

void SDLInputSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (ctx->isImGuiInitialized) {
      ImGui_ImplSDL3_ProcessEvent(&event);
    }
    if (event.type == SDL_EVENT_QUIT) {
      ecs_print(1, "Quit event received");
      ctx->shouldQuit = true;
    } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
      int newWidth = event.window.data1;
      int newHeight = event.window.data2;
      ecs_print(1, "Window resized to %dx%d", newWidth, newHeight);
      ctx->width = newWidth;
      ctx->height = newHeight;
      ctx->needsSwapchainRecreation = true; // Add this flag to WorldContext
    }
  }
}

void flecs_sdl_cleanup(WorldContext *ctx){
  SDL_DestroyWindow(ctx->window);
  SDL_Quit();
}

void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "Initializing SDL module...");

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
        .name = "SDLSetUpSystem", 
        .add = ecs_ids(ecs_dependson(GlobalPhases.SetupPhase)) 
    }),
    .callback = SDLSetUpSystem
  });

  ecs_print(1, "SDLInputSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { 
          .name = "SDLInputSystem", 
          .add = ecs_ids(ecs_dependson(GlobalPhases.LogicUpdatePhase)) 
      }),
      .callback = SDLInputSystem
  });

  ecs_print(1, "SDL module initialized");
}