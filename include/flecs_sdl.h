#ifndef FLECS_SDL_H
#define FLECS_SDL_H

#include "flecs.h"
#include "flecs_types.h"

// ECS_STRUCT_EXTERN(ecs_sdl_key_state_t, {
//   bool pressed;
//   bool state;
//   bool current;
// });

// https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_keycode.h  277
#define SDL_KEYS_MAX 278

typedef struct {
  SDL_Window *window;
  VkSurfaceKHR surface;
  uint32_t width;                              // Window width (from swapchain)
  uint32_t height;                             // Window height (from swapchain)
  bool shouldQuit;                             // SDL quit flag
  bool hasError;                               // Error flag
  const char *errorMessage;
  bool needsSwapchainRecreation;               // Flag to indicate swapchain needs recreation
} SDLContext;
ECS_COMPONENT_DECLARE(SDLContext);

typedef struct {
  bool pressed;
  bool state;
  bool current;
} ECS_SDL_KEY_STATE_T;

//extern ECS_COMPONENT_DECLARE(ECS_SDL_KEY_STATE_T);
ECS_COMPONENT_DECLARE(ECS_SDL_KEY_STATE_T);

//mouse motion from SDL
typedef struct {
  float x;
  float y;
  float xrel;
  float yrel;
} ECS_SDL_MOTION_T;

//wheel scroll from SDL
typedef struct {
  float x;
  float y;
} ECS_SDL_WHEEL_T;


typedef struct {
  ECS_SDL_KEY_STATE_T left;
  ECS_SDL_KEY_STATE_T right;
  ECS_SDL_KEY_STATE_T wnd;
  ECS_SDL_KEY_STATE_T rel;
  ECS_SDL_KEY_STATE_T view;
  ECS_SDL_KEY_STATE_T scroll;
  ECS_SDL_WHEEL_T wheel;
  ECS_SDL_MOTION_T motion;
} ECS_SDL_MOUSE_T;

ECS_COMPONENT_DECLARE(ECS_SDL_MOUSE_T);

typedef struct {
  ECS_SDL_KEY_STATE_T keys[128];
  ECS_SDL_MOUSE_T mouse;
} ECS_SDL_INPUT_T;

ECS_COMPONENT_DECLARE(ECS_SDL_INPUT_T);

// https://wiki.libsdl.org/SDL3/SDL_Keycode
// https://wiki.libsdl.org/SDL3/SDL_KeyboardEvent
// https://wiki.libsdl.org/SDL3/BestKeyboardPractices
// https://github.com/flecs-hub/flecs-components-input/blob/master/include/flecs_components_input.h
// https://github.com/libsdl-org/SDL/blob/main/include/SDL3/SDL_scancode.h
#define ECS_SDLK_W SDLK_W
#define ECS_SDLK_A SDLK_A
#define ECS_SDLK_S SDLK_S
#define ECS_SDLK_D SDLK_D


// module
void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx);

// Systems for SDL
// void SDLInputSystem(ecs_iter_t *it);
void flecs_sdl_cleanup(WorldContext *ctx);

#endif