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
  //ECS_SDL_INPUT_T *input = ecs_field(it, ECS_SDL_INPUT_T, 0);

  ECS_SDL_INPUT_T *input = ecs_singleton_get(it->world, ECS_SDL_INPUT_T);
  if (!input) return; // Safety check

  bool isMotion = false;
  bool isWheel = false;

  SDL_Event event;
  //for (int i = 0; i < it->count; i ++) {
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
      }else if (event.type == SDL_EVENT_KEY_DOWN){
        if (event.key.key < SDL_KEYS_MAX) { // Ensure index is within bounds
          input->keys[event.key.key].pressed = true;
          input->keys[event.key.key].state = true;
          input->keys[event.key.key].current = true;
        }
        ecs_print(1, "Key Up: %d", event.key.key);
        ecs_print(1,"KEY DOWN");
        if(event.key.key == SDLK_A){
          ecs_print(1,"KEY A");
        }
      }else if (event.type == SDL_EVENT_KEY_UP){
        if (event.key.key < SDL_KEYS_MAX) { // Ensure index is within bounds
          input->keys[event.key.key].pressed = false;
          input->keys[event.key.key].state = false;
          input->keys[event.key.key].current = false;
        }
        ecs_print(1, "Key Up: %d", event.key.key);
        ecs_print(1,"KEY UP");
        if(event.key.key == SDLK_A){
          ecs_print(1,"KEY A");
        }
      }else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
        input->mouse.left.pressed = true;
        input->mouse.left.state = true;
        input->mouse.left.current = true;
      }else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP){
        input->mouse.left.pressed = false;
        input->mouse.left.state = false;
        input->mouse.left.current = false;
      } else if (event.type == SDL_EVENT_MOUSE_MOTION){
        //https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_events.h
        ecs_print(1,"event x:%f, y:%f ", event.motion.x, event.motion.y);
        isMotion = true;
        input->mouse.rel.pressed = true;
        input->mouse.rel.state = true;
        input->mouse.rel.current = true;
      }else if (event.type == SDL_EVENT_MOUSE_WHEEL){
        ecs_print(1,"event x:%f, y:%f ", event.wheel.x, event.wheel.y);
        isWheel = true;
        input->mouse.scroll.pressed = true;
        input->mouse.scroll.state = true;
        input->mouse.scroll.current = true;
      }
    }
    // check for mouse motion if not move
    if(isMotion == false){
      input->mouse.rel.pressed = false;
      input->mouse.rel.state = false;
      input->mouse.rel.current = false;
    }
    // wheel scroll if not scroll set to false
    if(isWheel == false){
      input->mouse.scroll.pressed = false;
      input->mouse.scroll.state = false;
      input->mouse.scroll.current = false;
    }
    ecs_singleton_modified(it->world, ECS_SDL_INPUT_T);

  //}


  // for (int i = 0; i < 128; i++) {
  //   input->keys[i].current = false;
  // }
}

void DebugInputSystem(ecs_iter_t *it) {
  //ECS_SDL_INPUT_T *input = ecs_field(it, ECS_SDL_INPUT_T, 0);

  const ECS_SDL_INPUT_T *input = ecs_singleton_get(it->world, ECS_SDL_INPUT_T);
  if (!input) return; // Safety check

  if (input->keys[SDLK_W].state) {
    ecs_print(1, "W key is held down!");
  }

  if (input->keys[SDLK_A].state) {
    ecs_print(1, "A key is held down!");
  }

  if (input->keys[SDLK_S].state) {
    ecs_print(1, "S key is held down!");
  }

  if (input->keys[SDLK_D].state) {
    ecs_print(1, "D key is held down!");
  }

  //move state when in window
  if (input->mouse.rel.state) {
    ecs_print(1, "Move State!");
  }

  if (input->mouse.scroll.state) {
    ecs_print(1, "Wheel State!");
  }
  // for (int i = 0; i < it->count; i ++) {
  //   if (input[i].keys[SDLK_A].state) {
  //     ecs_print(1, "A key is held down!");
  //   }
  // }
}

void flecs_sdl_cleanup(WorldContext *ctx){
  SDL_DestroyWindow(ctx->window);
  SDL_Quit();
}

void register_componets(ecs_world_t *world){

  ECS_COMPONENT_DEFINE(world, ECS_SDL_KEY_STATE_T);
  ECS_COMPONENT_DEFINE(world, ECS_SDL_MOUSE_T);
  ECS_COMPONENT_DEFINE(world, ECS_SDL_INPUT_T);


}

void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "Initializing SDL module...");

  register_componets(world);

  // ecs_entity_t sdl_input_entity = ecs_entity(world, { .name = "SDL_INPUT" });

  // printf("Entity name: %s\n", ecs_get_name(world, sdl_input_entity));
  // ecs_add(world, sdl_input_entity, ECS_SDL_INPUT_T);
  // ecs_set(world, sdl_input_entity, ECS_SDL_INPUT_T, {0}); // Initialize with zeros


  ecs_singleton_set(world, ECS_SDL_INPUT_T, {0});


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
      // .query.terms = {
        // A singleton is a component matched on itself
        // { ecs_id(ECS_SDL_INPUT_T), .src.id = ecs_id(ECS_SDL_INPUT_T) }
      // },
      .callback = SDLInputSystem
  });


  ecs_print(1, "DebugInputSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { 
          .name = "DebugInputSystem", 
          .add = ecs_ids(ecs_dependson(GlobalPhases.LogicUpdatePhase)) 
      }),
      // .query.terms = {
      //   { ecs_id(ECS_SDL_INPUT_T) },
      // },
      .callback = DebugInputSystem
  });

  ecs_print(1, "SDL module initialized");
}