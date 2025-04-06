#ifndef FLECS_SDL_H
#define FLECS_SDL_H

#include "flecs.h"
#include "flecs_types.h"
//#include <SDL3/SDL.h>
//#include "cimgui.h"         // C ImGui wrapper
//#include "cimgui_impl.h"    // Implementation helpers

void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx);

// Systems for SDL
void SDLInputSystem(ecs_iter_t *it);

#endif