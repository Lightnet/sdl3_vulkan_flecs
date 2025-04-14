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

void flecs_shutdown_event_system(ecs_iter_t *it){
  ecs_print(1,"[module] flecs_shutdown_event_system");

  ecs_emit(it->world, &(ecs_event_desc_t) {
    .event = CleanUpEvent,
    .entity = CleanUpModule
  });

  ecs_query_t *q = ecs_query(it->world, {
    .terms = {
      { .id = ecs_id(PluginModule) },
    }
  });

  ecs_iter_t s_it = ecs_query_iter(it->world, q);

  while (ecs_query_next(&s_it)) {
    PluginModule *p = ecs_field(&s_it, PluginModule, 0);
    for (int i = 0; i < s_it.count; i ++) {
      ecs_print(1,"Module Name : %s", 
        p[i].name
        //ecs_get_name(s_it.world, s_it.entities[i])
      
      );
    }
  }
}

void flecs_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[module] flecs_cleanup_event_system");
  // ecs_emit(it->world, &(ecs_event_desc_t) {
  //   .event = CleanUpGraphicEvent,
  //   .entity = CleanUpGraphic
  // });
}

void flecs_cleanup_graphic_event_system(ecs_iter_t *it){
  ecs_print(1,"[module] flecs_cleanup_graphic_event_system");
  ecs_emit(it->world, &(ecs_event_desc_t) {
    .event = CloseEvent,
    .entity = CloseModule
  });
}

void flecs_close_event_system(ecs_iter_t *it){
  ecs_print(1,"[module] flecs_close_event_system");
}

void flecs_cleanup_checks_system(ecs_iter_t *it){
  ModuleContext *g_module = ecs_singleton_ensure(it->world, ModuleContext);
  if(!g_module)return;
  PluginModule *plugin = ecs_field(it, PluginModule, 0);
  //ecs_print(1,"checking...");

  ecs_query_t *q = ecs_query(it->world, {
    .terms = {
      { .id = ecs_id(PluginModule) },
    }
  });

  ecs_iter_t s_it = ecs_query_iter(it->world, q);

  bool isFinishCleanup = false;
  int count = 0;
  while (ecs_query_next(&s_it)) {
    PluginModule *p = ecs_field(&s_it, PluginModule, 0);
    for (int i = 0; i < s_it.count; i ++) {
      // ecs_print(1,"Module Name : %s", 
      //   p[i].name
      //   //ecs_get_name(s_it.world, s_it.entities[i])
      // );
      //ecs_print(1,"isCleanUp %d", p[i].isCleanUp);
      if(p[i].isCleanUp){
        isFinishCleanup = true;
        count+=1;
      }
    }
  }

  //
  //ecs_print(1," COUNT: %d Len:%d ", count, s_it.count);
  if((count == s_it.count && g_module->isCleanUpModule == false) ){
    g_module->isCleanUpModule = true;
    ecs_emit(it->world, &(ecs_event_desc_t) {
      .event = CleanUpGraphicEvent,
      .entity = CleanUpGraphic
    });
  }
  //=============================================
  // 
  //=============================================

}

void flecs_register_components(ecs_world_t *world){

  ECS_COMPONENT_DEFINE(world, PluginModule);
  ECS_COMPONENT_DEFINE(world, ModuleContext);
  // ECS_COMPONENT_DEFINE(world, PluginModuleContext);
}

void flecs_register_systems(ecs_world_t *world){
  // Create an entity observer
  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = ShutDownModule }},
    .events = { ShutDownEvent },
    .callback = flecs_shutdown_event_system
  });

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = flecs_cleanup_event_system
  });

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpGraphic }},
    .events = { CleanUpGraphicEvent },
    .callback = flecs_cleanup_graphic_event_system
  });

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CloseModule }},
    .events = { CloseEvent },
    .callback = flecs_close_event_system
  });


  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
      .name = "flecs_cleanup_checks_system",
      .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    }),
    .query.terms = {{ ecs_id(PluginModule), .inout = EcsIn }},
    .callback = flecs_cleanup_checks_system,
  });

}

void flecs_init_module(ecs_world_t *world){

  flecs_phases_init(world, &GlobalPhases);
  flecs_register_components(world);

  ShutDownEvent = ecs_new(world);
  ShutDownModule = ecs_entity(world, { .name = "ShutDownModule" });

  CleanUpEvent = ecs_new(world);
  CleanUpModule = ecs_entity(world, { .name = "CleanUpModule" });

  CleanUpGraphicEvent = ecs_new(world);
  CleanUpGraphic = ecs_entity(world, { .name = "CleanUpGraphic" });

  CloseEvent = ecs_new(world);
  CloseModule = ecs_entity(world, { .name = "CloseModule" });

  ecs_singleton_set(world, ModuleContext, {
    .isCleanUpModule=false,
    .moduleCount=0
  });

  flecs_register_systems(world);

}

ecs_entity_t add_module_name(ecs_world_t *world, const char *name) {
  ecs_entity_t e = ecs_new(world);
  PluginModule module = { .isCleanUp = false };
  
  // Copy name safely
  #ifdef _MSC_VER
      strncpy_s(module.name, sizeof(module.name), name, _TRUNCATE); // MSVC-safe
  #else
      strncpy(module.name, name, sizeof(module.name) - 1);
      module.name[sizeof(module.name) - 1] = '\0'; // Ensure null-termination
  #endif
  
  // Set component
  //ecs_set(world, e, PluginModule, module);
  ecs_set_id(world, e, ecs_id(PluginModule), sizeof(PluginModule), &module);
  return e;
}

void module_break_name(ecs_iter_t *it, const char *module_name){
  // Create query for PluginModule
  ecs_query_t *q = ecs_query(it->world, {
    .terms = {
      { .id = ecs_id(PluginModule) }
    }
  });

  ecs_iter_t s_it = ecs_query_iter(it->world, q);
  bool found = false;

  while (ecs_query_next(&s_it) && !found) {
    PluginModule *p = ecs_field(&s_it, PluginModule, 0); // 1-based index
    for (int i = 0; i < s_it.count; i++) {
        printf("Checking module: %s\n", p[i].name);
        if (strcmp(p[i].name, module_name) == 0) {
            p[i].isCleanUp = true;
            printf("Marked %s for cleanup\n", module_name);
            found = true;
            break; // Exit inner loop
        }
    }
  }
  
}

