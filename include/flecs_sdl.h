#ifndef FLECS_SDL_H
#define FLECS_SDL_H

#include "flecs.h"
#include "flecs_types.h"

// module
void flecs_sdl_module_init(ecs_world_t *world, WorldContext *ctx);

// Systems for SDL
// void SDLInputSystem(ecs_iter_t *it);
void flecs_sdl_cleanup(WorldContext *ctx);

#endif