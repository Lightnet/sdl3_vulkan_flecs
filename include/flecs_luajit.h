#ifndef FLECS_LUAJIT_H
#define FLECS_LUAJIT_H

#include "flecs.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    lua_State *L;
} LuaContext;

ECS_COMPONENT_DECLARE(LuaContext);

void flecs_luajit_module_init(ecs_world_t *world);
void flecs_luajit_cleanup(ecs_world_t *world);

#endif