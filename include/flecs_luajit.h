#ifndef FLECS_LUAJIT_H
#define FLECS_LUAJIT_H

#include "flecs_types.h"
#include "flecs.h"
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    lua_State *L;
    bool script_loaded; // Tracks if script loaded successfully
} LuaContext;

ECS_COMPONENT_DECLARE(LuaContext);

void flecs_luajit_module_init(ecs_world_t *world);
void flecs_luajit_cleanup(ecs_world_t *world);
void flecs_luajit_register_systems(ecs_world_t *world); // Register Lua system

#endif