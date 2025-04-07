#include "flecs_types.h"

FlecsPhases GlobalPhases = {0};

//init once setup for vulkan but not others else error same entity setup.
void flecs_phases_init(ecs_world_t *world, FlecsPhases *phases) {
  // Runtime phases (main loop, single flow)
  phases->LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->LogicUpdatePhase, EcsDependsOn, EcsPreUpdate);

  phases->BeginRenderPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->BeginRenderPhase, EcsDependsOn, phases->LogicUpdatePhase);

  phases->BeginCMDBufferPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->BeginCMDBufferPhase, EcsDependsOn, phases->BeginRenderPhase);

  phases->CMDBufferPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->CMDBufferPhase, EcsDependsOn, phases->BeginCMDBufferPhase);

  phases->EndCMDBufferPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->EndCMDBufferPhase, EcsDependsOn, phases->CMDBufferPhase);

  phases->EndRenderPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->EndRenderPhase, EcsDependsOn, phases->EndCMDBufferPhase);

  // Setup phases (single flow, run once under EcsOnStart)
  phases->InstanceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->InstanceSetupPhase, EcsDependsOn, EcsOnStart);  // Start of setup chain

  phases->SurfaceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SurfaceSetupPhase, EcsDependsOn, phases->InstanceSetupPhase);

  phases->DeviceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->DeviceSetupPhase, EcsDependsOn, phases->SurfaceSetupPhase);

  phases->SwapchainSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SwapchainSetupPhase, EcsDependsOn, phases->DeviceSetupPhase);

  phases->TriangleBufferSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->TriangleBufferSetupPhase, EcsDependsOn, phases->SwapchainSetupPhase);

  phases->RenderPassSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->RenderPassSetupPhase, EcsDependsOn, phases->TriangleBufferSetupPhase);

  phases->FramebufferSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->FramebufferSetupPhase, EcsDependsOn, phases->RenderPassSetupPhase);  // Single dependency

  phases->CommandPoolSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->CommandPoolSetupPhase, EcsDependsOn, phases->FramebufferSetupPhase);

  phases->CommandBufferSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->CommandBufferSetupPhase, EcsDependsOn, phases->CommandPoolSetupPhase);

  phases->PipelineSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->PipelineSetupPhase, EcsDependsOn, phases->CommandBufferSetupPhase);

  phases->SyncSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SyncSetupPhase, EcsDependsOn, phases->PipelineSetupPhase);

  phases->SetupLogicPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SetupLogicPhase, EcsDependsOn, phases->SyncSetupPhase);

}


