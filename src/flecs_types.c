#include "flecs_types.h"

FlecsPhases GlobalPhases = {0};

void flecs_phases_init(ecs_world_t *world, FlecsPhases *phases) {
    phases->LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
    phases->BeginRenderPhase = ecs_new_w_id(world, EcsPhase);
    phases->BeginGUIPhase = ecs_new_w_id(world, EcsPhase);
    phases->UpdateGUIPhase = ecs_new_w_id(world, EcsPhase);
    phases->EndGUIPhase = ecs_new_w_id(world, EcsPhase);
    phases->RenderPhase = ecs_new_w_id(world, EcsPhase);
    phases->EndRenderPhase = ecs_new_w_id(world, EcsPhase);
    phases->InstanceSetupPhase = ecs_new_w_id(world, EcsPhase);
    phases->SurfaceSetupPhase = ecs_new_w_id(world, EcsPhase);
    phases->DeviceSetupPhase = ecs_new_w_id(world, EcsPhase);
    phases->SwapchainSetupPhase = ecs_new_w_id(world, EcsPhase);
    phases->RenderSetupPhase = ecs_new_w_id(world, EcsPhase);
    phases->SyncSetupPhase = ecs_new_w_id(world, EcsPhase);

    ecs_add_pair(world, phases->LogicUpdatePhase, EcsDependsOn, EcsPreUpdate);
    ecs_add_pair(world, phases->BeginRenderPhase, EcsDependsOn, phases->LogicUpdatePhase);
    ecs_add_pair(world, phases->BeginGUIPhase, EcsDependsOn, phases->BeginRenderPhase);
    ecs_add_pair(world, phases->UpdateGUIPhase, EcsDependsOn, phases->BeginGUIPhase);
    ecs_add_pair(world, phases->EndGUIPhase, EcsDependsOn, phases->UpdateGUIPhase);
    ecs_add_pair(world, phases->RenderPhase, EcsDependsOn, phases->EndGUIPhase);
    ecs_add_pair(world, phases->EndRenderPhase, EcsDependsOn, phases->RenderPhase);
    ecs_add_pair(world, phases->InstanceSetupPhase, EcsDependsOn, EcsOnStart);
    ecs_add_pair(world, phases->SurfaceSetupPhase, EcsDependsOn, phases->InstanceSetupPhase);
    ecs_add_pair(world, phases->DeviceSetupPhase, EcsDependsOn, phases->SurfaceSetupPhase);
    ecs_add_pair(world, phases->SwapchainSetupPhase, EcsDependsOn, phases->DeviceSetupPhase);
    ecs_add_pair(world, phases->RenderSetupPhase, EcsDependsOn, phases->SwapchainSetupPhase);
    ecs_add_pair(world, phases->SyncSetupPhase, EcsDependsOn, phases->RenderSetupPhase);
}