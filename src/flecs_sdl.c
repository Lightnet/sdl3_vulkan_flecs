#include "flecs_sdl.h"

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
    }
  }
}

void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "Initializing SDL module...");

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