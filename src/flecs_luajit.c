#include "flecs_luajit.h"

void error(lua_State *L, const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    if (L) {
        lua_pop(L, 1); // Remove error message from stack
    }
}

void flecs_luajit_cleanup(ecs_world_t *world) {
    LuaContext *lua_ctx = ecs_singleton_ensure(world, LuaContext);
    if (lua_ctx && lua_ctx->L) {
        lua_close(lua_ctx->L);
        lua_ctx->L = NULL;
    }
}

void luajit_register_components(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, LuaContext);
}

void flecs_luajit_module_init(ecs_world_t *world) {
    luajit_register_components(world);

    LuaContext lua_ctx = {0};
    lua_ctx.L = luaL_newstate();
    if (!lua_ctx.L) {
        fprintf(stderr, "Failed to create Lua state\n");
        return;
    }

    luaL_openlibs(lua_ctx.L);
    ecs_singleton_set(world, LuaContext, {.L=lua_ctx.L});

    if (luaL_loadfile(lua_ctx.L, "script.lua") || lua_pcall(lua_ctx.L, 0, 0, 0)) {
        error(lua_ctx.L, "Error loading or running script.lua: %s\n", lua_tostring(lua_ctx.L, -1));
    } else {
        lua_settop(lua_ctx.L, 0); // Clear stack
    }
}