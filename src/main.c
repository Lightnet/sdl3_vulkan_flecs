// Main Entry

#include <SDL3/SDL.h>
#include <stdio.h>
#include "flecs.h"
#include "flecs_types.h"
#include "flecs_vulkan.h"
#include "flecs_imgui.h"
#include "flecs_text.h"
#include "flecs_sdl.h"
// #include "flecs_texture2d.h"
// #include "flecs_triangle2d.h"
// #include "flecs_cube3d.h"
// #include "flecs_cubetexture3d.h"
// #include "flecs_luajit.h"
#include "flecs_assimp.h"
#include "flecs_assets3d.h"

int main(int argc, char *argv[]) {

  ecs_world_t *world = ecs_init();
  //ecs_log_set_level(1);
  ecs_print(1, "init main, flecs ...");

  //this need to be load first for phase for setup and runtime render
  ecs_log(1, "Initializing main flecs_init_module...");
  flecs_init_module(world);
  // 
  ecs_log(1, "Calling flecs_sdl_module_init...");
  // setup SDL 3.x window and Input event 
  flecs_sdl_module_init(world);
  // setup Vulkan graphic
  ecs_log(1, "Calling flecs_vulkan_module_init...");
  flecs_vulkan_module_init(world);

  // example test module
  // ecs_log(1, "Calling flecs_cubetexture3d_module_init...");
  // flecs_cubetexture3d_module_init(world);

  // example test module
  // ecs_log(1, "Calling flecs_triangle2d_module_init...");
  // flecs_triangle2d_module_init(world);

  // example test module
  // ecs_log(1, "Calling flecs_texture2d_module_init...");
  // flecs_texture2d_module_init(world);

  // Add Assimp module
  // ecs_log(1, "Calling flecs_assimp_module_init...");
  // flecs_assimp_module_init(world);

  // example test module
  // ecs_log(1, "Calling flecs_text_module_init...");
  // flecs_text_module_init(world);

  // 
  // ecs_log(1, "Calling flecs_assets3d_module_init...");
  flecs_assets3d_module_init(world);
  
  
  // luajit module
  // ecs_log(1, "Calling flecs_luajit_module_init...");
  // flecs_luajit_module_init(world);

  // ecs_log(1, "Calling flecs_cube3d_module_init...");
  // flecs_cube3d_module_init(world);
  
  // example test module
  ecs_log(1, "Calling flecs_imgui_module_init...");
  flecs_imgui_module_init(world);
  
  ecs_print(1, "Entering main loop...");
  Uint64 previousTime = SDL_GetTicks(); // Time at the start
  //ecs_abort(ECS_INTERNAL_ERROR, "TEST"); // Test error

  bool shouldQuit = false;
  while (!shouldQuit) {
    SDLContext *sdl_ctx = ecs_singleton_ensure(world, SDLContext);
    if(!sdl_ctx)return;
    // Get current time and calculate delta time
    Uint64 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - previousTime) / 1000.0f; // Convert ms to seconds
    previousTime = currentTime; // Update previous time for next frame
    // Print delta time
    //printf("Delta Time: %f seconds\n", deltaTime);
    //flecs run time update
    ecs_progress(world, deltaTime);
    //check if SDL context is quit
    shouldQuit = sdl_ctx->shouldQuit;
  }
  //ecs_progress(world, 1);

  ecs_print(1, "Cleaning up...");
  // flecs_luajit_cleanup(world);
  // Vulkan graphic variable cleanup
  // flecs_imgui_cleanup(world);
  // flecs_cubetexture3d_cleanup(world);
  // flecs_text_cleanup(world);
  // flecs_triangle2d_cleanup(world);
  // flecs_texture2d_cleanup(world);
  // flecs_cube3d_cleanup(world);
  // flecs_vulkan_cleanup(world);
  // window cleanup
  // flecs_sdl_cleanup(world);
  // flecs cleanup world
  ecs_fini(world);

  ecs_print(1, "Program exiting");
  return 0;
}

