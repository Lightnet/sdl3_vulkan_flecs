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

  phases->CMDBuffer1Phase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->CMDBuffer1Phase, EcsDependsOn, phases->CMDBufferPhase);

  phases->CMDBuffer2Phase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->CMDBuffer2Phase, EcsDependsOn, phases->CMDBuffer1Phase);

  phases->EndCMDBufferPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->EndCMDBufferPhase, EcsDependsOn, phases->CMDBuffer2Phase);

  phases->EndRenderPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->EndRenderPhase, EcsDependsOn, phases->EndCMDBufferPhase);

  // Setup phases (single flow, run once under EcsOnStart)
  phases->SetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SetupPhase, EcsDependsOn, EcsOnStart);  // Start of setup chain

  phases->InstanceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->InstanceSetupPhase, EcsDependsOn, phases->SetupPhase);

  phases->SurfaceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SurfaceSetupPhase, EcsDependsOn, phases->InstanceSetupPhase);

  phases->DeviceSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->DeviceSetupPhase, EcsDependsOn, phases->SurfaceSetupPhase);

  phases->SwapchainSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SwapchainSetupPhase, EcsDependsOn, phases->DeviceSetupPhase);

  phases->RenderPassSetupPhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->RenderPassSetupPhase, EcsDependsOn, phases->SwapchainSetupPhase);

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

  //set up module init atfter.
  phases->SetupModulePhase = ecs_new_w_id(world, EcsPhase);
  ecs_add_pair(world, phases->SetupModulePhase, EcsDependsOn, phases->SyncSetupPhase);
  
}

void CleanUpSystem(ecs_iter_t *it){
  ecs_print(1,"[module] clean up event test...");
}

void flecs_register_systems(ecs_world_t *world){
  // Create an entity observer
  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = CleanUpSystem
  });
}

void flecs_init_module(ecs_world_t *world){

  flecs_phases_init(world, &GlobalPhases);

  CleanUpEvent = ecs_new(world);
  CleanUpModule = ecs_entity(world, { .name = "CleanUpModule" });

  flecs_register_systems(world);

}