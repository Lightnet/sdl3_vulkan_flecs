#include "flecs_luajit.h"
#include "flecs_sdl.h"

typedef struct { float x, y; } Position;

//sample
// static int lua_get_position(lua_State *L) {
//   ecs_world_t *world = lua_touserdata(L, lua_upvalueindex(1));
//   ecs_entity_t e = lua_tointeger(L, 1);
//   Position *p = ecs_get(world, e, Position);
//   lua_pushnumber(L, p->x);
//   lua_pushnumber(L, p->y);
//   return 2;
// }

void error(lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  if (L) {
      lua_pop(L, 1); // Remove error message from stack
  }
}

// Lua system to call update function
static void LuaUpdateSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  LuaContext *lua_ctx = ecs_singleton_ensure(it->world, LuaContext);
  if (!lua_ctx || !lua_ctx->L) return;

  lua_State *L = lua_ctx->L;
  // Get the global 'update' function
  lua_getglobal(L, "update");
  if (!lua_isfunction(L, -1)) {
      lua_pop(L, 1); // Clean up stack
      return; // Silently skip if update is not a function
  }

  // Push delta time as argument
  lua_pushnumber(L, it->delta_time);

  // Call update(dt)
  if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
      error(L, "Error in Lua update: %s\n", lua_tostring(L, -1));
  }
}

void luajit_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] luajit_cleanup_event_system");
  flecs_luajit_cleanup(it->world);

  module_break_name(it, "luajit_module");
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
    //ECS_COMPONENT_DEFINE(world, Position);
}

ecs_entity_t ecs_lua_update_sys;

void flecs_luajit_register_systems(ecs_world_t *world){
  //ECS_SYSTEM(world, LuaUpdateSystem, EcsOnUpdate, [in] LuaContext);
  //ecs_enable(world, ecs_id(LuaUpdateSystem), false);

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = luajit_cleanup_event_system
  });

  // Register system, disabled by default
  ecs_lua_update_sys = ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
      .name = "LuaUpdateSystem",
      .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    }),
    .query.terms = {{ 
      ecs_id(LuaContext), 
      .inout = EcsIn 
    }},
    .callback = LuaUpdateSystem,
    });
    ecs_enable(world, ecs_lua_update_sys, false); // Store and disable

}

void flecs_luajit_module_init(ecs_world_t *world) {
  ecs_log(1, "luajit_module_init");
  
  luajit_register_components(world);

  // ecs_entity_t e = ecs_new(world);
  // ecs_set(world, e, PluginModule, { .name = "luajit_module", .isCleanUp = false });

  add_module_name(world, "luajit_module");


  

  LuaContext lua_ctx = {0};
  lua_ctx.L = luaL_newstate();
  if (!lua_ctx.L) {
      fprintf(stderr, "Failed to create Lua state\n");
      return;
  }

  luaL_openlibs(lua_ctx.L);
  lua_ctx.script_loaded = false; // Default to false
  //ecs_singleton_set(world, LuaContext, lua_ctx);
  //ecs_set(world, ecs_id(LuaContext), LuaContext, lua_ctx);
  ecs_set_id(world, ecs_id(LuaContext), ecs_id(LuaContext), sizeof(LuaContext), &lua_ctx);

  // Register systems (disabled by default)
  flecs_luajit_register_systems(world);

  // Try loading script.lua
  if (luaL_loadfile(lua_ctx.L, "script.lua")) {
    error(lua_ctx.L, "Error loading script.lua: %s\n", lua_tostring(lua_ctx.L, -1));
    return;
  }

  // Execute the script
  if (lua_pcall(lua_ctx.L, 0, 0, 0)) {
      error(lua_ctx.L, "Error running script.lua: %s\n", lua_tostring(lua_ctx.L, -1));
      return;
  }

  // Script loaded successfully, enable system
  lua_ctx.script_loaded = true;
  // ecs_singleton_set(world, LuaContext, {lua_ctx});
  //ecs_set(world, ecs_id(LuaContext), LuaContext, lua_ctx);
  ecs_set_id(world, ecs_id(LuaContext), ecs_id(LuaContext), sizeof(LuaContext), &lua_ctx);
  // ecs_enable(world, LuaUpdateSystem, true);
  //ecs_enable(world, ecs_lua_update_sys, true);
  ecs_enable(world, ecs_lookup(world, "LuaUpdateSystem"), true);

}

