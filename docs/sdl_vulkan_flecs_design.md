

# Design Information:
 Work in progress. Keept it simple and module design to handle setup, input, drawing, add, remove, system and other things need for flecs work well.

# flecs phase:
  There are couple types for phase. We talk about two for now. On Start Phase which load once. The other is Update phase which loop in order depend on the phases.

# flecs sdl:
  Note that sdl use by vulkan module as well cimgui.

  One is vulkan SDL_Vulkan_GetInstanceExtensions and SDL_Vulkan_CreateSurface.

  cimgui use to get input api call ImGui_ImplSDL3_ProcessEvent.

  Add SDLInputSystem so partly module which need work.

# flecs vulkan:
  Idea to break up into module to handle user creation setup. To find where to set up phase need to added to vulkan setup. For example triangle mesh setup.

```c
void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx);
// set up once for OnStart
void InstanceSetupSystem(ecs_iter_t *it);
void SurfaceSetupSystem(ecs_iter_t *it);
void DeviceSetupSystem(ecs_iter_t *it);
void SwapchainSetupSystem(ecs_iter_t *it);
void TriangleBufferSetupSystem(ecs_iter_t *it); // triangle
void RenderPassSetupSystem(ecs_iter_t *it);    
void FramebufferSetupSystem(ecs_iter_t *it);   
void CommandPoolSetupSystem(ecs_iter_t *it);   
void CommandBufferSetupSystem(ecs_iter_t *it); 
void PipelineSetupSystem(ecs_iter_t *it);      
void SyncSetupSystem(ecs_iter_t *it);
// SetupLogicPhase
// this where user setup their own custom system

// loop render
void BeginRenderSystem(ecs_iter_t *it);
void RenderSystem(ecs_iter_t *it);
void EndRenderSystem(ecs_iter_t *it);
// clean up
void flecs_vulkan_cleanup(ecs_world_t *world, WorldContext *ctx);
```

  Those function system have 
```
ecs_ids(ecs_dependson(GlobalPhases.InstanceSetupPhase))
```
  which trigger depend phase id ( name ). But they can't be trigger more than once for set up else crashed. For this code. It need to finish after for next phase to work.

  Same with the render. They need to be phase render order as added next phase else it would crashed or error on vulkan layers.

  The reason is simple if render. I would say vulkan command buffer begin render, render and end render.

# flecs cimgui:
  This is c for cimgui build. This is c++ wrapper for c base on imgui. Some varaible can be access due how it setup to handle cimgui render.

  It need to config to set up imgui.

  By using the commandbuffer pass to draw imgui.

# flecs mesh 2d:
  Very simple triangle with shader header. Need to add some detail later.

# flecs text:
  freetype font

# flecs mesh 3d:
 None
