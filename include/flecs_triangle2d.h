#ifndef FLECS_TRIANGLE2D_H
#define FLECS_TRIANGLE2D_H

#include "flecs_types.h"

void flecs_triangle2d_module_init(ecs_world_t *world, WorldContext *ctx);
void TrianglePipelineSetupSystem(ecs_iter_t *it);
void flecs_triangle2d_cleanup(WorldContext *ctx);

#endif