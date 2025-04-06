# module
  It would required context to set up the world.

# flecs imgui module:
 * flecs_imgui_module_init
 * flecs_imgui_cleanup
 * ImGuiSetupSystem
 * ImGuiInputSystem
 * ImGuiBeginSystem
 * ImGuiUpdateSystem
 * ImGuiEndSystem

# flecs sdl:
 * flecs_sdl_module_init
 * SDLInputSystem

# flecs types:
  Most important area for flecs setup context variables
 * WIDTH
 * HEIGHT
 * WorldContext
 * Vertex ( 2D )
 * FlecsPhases (ecs phase)
 * GlobalPhases
 * flecs_phases_init

## ECS Phase
  Note that phase need to be in order for setup once and loop.

### Render:
```
//..
// Runtime phases (main loop, single flow)
//phases->LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);

LogicUpdatePhase > EcsDependsOn > EcsPreUpdate
BeginRenderPhase > EcsDependsOn > LogicUpdatePhase
BeginGUIPhase > EcsDependsOn > BeginRenderPhase
UpdateGUIPhase > EcsDependsOn > BeginGUIPhase
EndGUIPhase > EcsDependsOn > UpdateGUIPhase
RenderPhase > EcsDependsOn > EndGUIPhase
BeginCMDBufferPhase > EcsDependsOn > RenderPhase
EndCMDBufferPhase > EcsDependsOn > BeginCMDBufferPhase
EndRenderPhase > EcsDependsOn > EndCMDBufferPhase
```
  Note need to fixed GUI and CMDBuffer.
### On Start:
  Note this need to start up once in order build for vulkan to work.
  But there will be custom or triangle setup for need to diplay.
```
// Setup phases (single flow, run once under EcsOnStart)
//phases->InstanceSetupPhase = ecs_new_w_id(world, EcsPhase);

InstanceSetupPhase > EcsDependsOn > EcsOnStart
SurfaceSetupPhase > EcsDependsOn > InstanceSetupPhase
DeviceSetupPhase > EcsDependsOn > SurfaceSetupPhase
SwapchainSetupPhase > EcsDependsOn > DeviceSetupPhase
TriangleBufferSetupPhase > EcsDependsOn > SwapchainSetupPhase
RenderPassSetupPhase > EcsDependsOn > TriangleBufferSetupPhase
FramebufferSetupPhase > EcsDependsOn > RenderPassSetupPhase
CommandPoolSetupPhase > EcsDependsOn > FramebufferSetupPhase
CommandBufferSetupPhase > EcsDependsOn > CommandPoolSetupPhase
PipelineSetupPhase > EcsDependsOn > CommandBufferSetupPhase
SyncSetupPhase > EcsDependsOn > PipelineSetupPhase
SetupLogicPhase > EcsDependsOn > SyncSetupPhase
```
SetupLogicPhase for user to setup like imgui

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