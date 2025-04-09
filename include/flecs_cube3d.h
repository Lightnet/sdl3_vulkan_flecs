#ifndef FLECS_CUBE3D_H
#define FLECS_CUBE3D_H

#include <flecs.h>
#include <vulkan/vulkan.h>
#include "flecs_types.h"

void flecs_cube3d_module_init(ecs_world_t *world, WorldContext *ctx);
void flecs_cube3d_cleanup(WorldContext *ctx);

#endif