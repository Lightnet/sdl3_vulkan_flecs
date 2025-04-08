# Status:
 - Work in progress.

# Design Information:
  Keep it simple and module design to handle setup, input, drawing, add, remove, system and other things need for flecs work well.

# Flecs Phase:
  There are couple types for phase. We talk about two for now. On Start Phase which load once. The other is Update phase which loop in order depend on the phases.

# Flecs SDL:
  Note that SDL use by Vulkan module as well cimgui.

  One is vulkan SDL_Vulkan_GetInstanceExtensions and SDL_Vulkan_CreateSurface.

  cimgui use to get input api call ImGui_ImplSDL3_ProcessEvent.

  Add SDLInputSystem so partly module which need work.

# Flecs Vulkan Phase:
  Idea to break up into module to handle user creation setup. To find where to set up phase need to added to vulkan setup. For example triangle mesh setup.

```c
// flecs_vulkan .h/.c
void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx);
// set up once for OnStart
//void Name_SetupSystem(ecs_iter_t *it)         // SetupPhase
void InstanceSetupSystem(ecs_iter_t *it);       // InstanceSetupPhase
void SurfaceSetupSystem(ecs_iter_t *it);        // SurfaceSetupPhase
void DeviceSetupSystem(ecs_iter_t *it);         // DeviceSetupPhase
void SwapchainSetupSystem(ecs_iter_t *it);      // SwapchainSetupPhase
void RenderPassSetupSystem(ecs_iter_t *it);     // RenderPassSetupPhase
void FramebufferSetupSystem(ecs_iter_t *it);    // FramebufferSetupPhase
void CommandPoolSetupSystem(ecs_iter_t *it);    // CommandPoolSetupPhase
void CommandBufferSetupSystem(ecs_iter_t *it);  // CommandBufferSetupPhase
void PipelineSetupSystem(ecs_iter_t *it);       // PipelineSetupPhase
void SyncSetupSystem(ecs_iter_t *it);           // SyncSetupPhase
// void ModuleSetupSystem(ecs_iter_t *it);      // SetupModulePhase

void flecs_vulkan_cleanup(ecs_world_t *world, WorldContext *ctx);
```

```c
// flecs_vulkan.h
// loop render or run time render                   // Phase order run time
// void UpdateLogicSystem(ecs_iter_t *it)           // LogicUpdatePhase
void BeginRenderSystem(ecs_iter_t *it);             // BeginRenderPhase
void BeginCMDBufferSystem(ecs_iter_t *it);          // BeginCMDBufferPhase
void TriangleRenderBufferSystem(ecs_iter_t *it);    // CMDBufferPhase
void EndCMDBufferSystem(ecs_iter_t *it);            // EndCMDBufferPhase
void EndRenderSystem(ecs_iter_t *it);               // EndRenderPhase
// clean up

```

  Those function system have goes by order by the phase tag that depend system query.

```c
ecs_ids(ecs_dependson(GlobalPhases.InstanceSetupPhase))
```
  Which trigger depend phase id ( name ). But they can't be trigger more pairs than once for set up else crashed. For this code. It need to finish after for next phase to work.

  Same with the render. They need to be phase render order as added next phase else it would crashed or error on vulkan layers.

  The reason is simple if render. I would say vulkan command buffer begin render, render and end render.

# Flecs cimgui:
  This is c for cimgui build. This is c++ wrapper for c base on imgui. Some varaible can be access due how it setup to handle cimgui render.

  It need to config to set up imgui.

  By using the commandbuffer pass to draw imgui.

# Flecs Mesh 2D:
  Very simple triangle with shader header. Need to add some detail later.

# Flecs Render Text:
  Freetype font

  Freetype render transparnt text font but required to align text to preset position text. As well set up plane, load texture file and font. Create text "hello world" to draw and render.

# Flecs Mesh 3D:
 None
