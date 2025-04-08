# module
  It would required context to set up the world.

# Flecs imgui module:
 * flecs_imgui_module_init
 * flecs_imgui_cleanup
 * ImGuiSetupSystem
 * ImGuiCMDBufferSystem

# Flecs SDL:
 * flecs_sdl_module_init
 * SDLInputSystem
 * flecs_sdl_cleanup

# Flecs Types:
  Most important area for Flecs setup context variables.
 * WIDTH (never zero else layer error)
 * HEIGHT (never zero else layer error)
 * WorldContext (SDL and Vulkan Handle)
 * FlecsPhases (ecs phase)
 * GlobalPhases (Flecs handle setup and render phase for global variables.)
 * flecs_phases_init (setup phase varaible)

## ECS Phase
  Note that phase need to be in order for setup once and loop.

### Render:
```c
//..
// Runtime phases (main loop, single flow)
// phases->LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);

LogicUpdatePhase > EcsDependsOn > EcsPreUpdate
BeginRenderPhase > EcsDependsOn > LogicUpdatePhase
BeginCMDBufferPhase > EcsDependsOn > BeginRenderPhase
CMDBufferPhase > EcsDependsOn > BeginCMDBufferPhase
EndCMDBufferPhase > EcsDependsOn > CMDBufferPhase
EndRenderPhase > EcsDependsOn > EndCMDBufferPhase
```
### On Start:
  Note this need to start up once in order build for vulkan to work. After that done. Example set up triangle mesh by using depend on SetupModulePhase.
```
// Setup phases (single flow, run once under EcsOnStart)
//phases->InstanceSetupPhase = ecs_new_w_id(world, EcsPhase);

InstanceSetupPhase > EcsDependsOn > EcsOnStart
SurfaceSetupPhase > EcsDependsOn > InstanceSetupPhase
DeviceSetupPhase > EcsDependsOn > SurfaceSetupPhase
SwapchainSetupPhase > EcsDependsOn > DeviceSetupPhase
RenderPassSetupPhase > EcsDependsOn > SwapchainSetupPhase
FramebufferSetupPhase > EcsDependsOn > RenderPassSetupPhase
CommandPoolSetupPhase > EcsDependsOn > FramebufferSetupPhase
CommandBufferSetupPhase > EcsDependsOn > CommandPoolSetupPhase
PipelineSetupPhase > EcsDependsOn > CommandBufferSetupPhase
SyncSetupPhase > EcsDependsOn > PipelineSetupPhase
SetupModulePhase > EcsDependsOn > SyncSetupPhase
```
SetupModulePhase for user to setup like imgui

# flecs vulkan:
 * flecs_vulkan_module_init
 * InstanceSetupSystem
 * SurfaceSetupSystem
 * DeviceSetupSystem
 * SwapchainSetupSystem
 * TriangleBufferSetupSystem
 * RenderPassSetupSystem
 * FramebufferSetupSystem
 * CommandPoolSetupSystem
 * CommandBufferSetupSystem
 * PipelineSetupSystem
 * SyncSetupSystem
 * BeginRenderSystem
 * RenderSystem
 * EndRenderSystem
 * flecs_vulkan_cleanup
 * createShaderModule
 * debugCallback

# flecs text:
 * 

# flecs mesh:
 * 