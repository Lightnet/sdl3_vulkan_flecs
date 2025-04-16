# flecs world context:
  Current no world context variables.

# Flecs API modules:
  There will be default phases for on setup once and run time update phases. As the need to be order flow build and render loop order.

  There will be struct component for handle module single access. So in flecs attach to world variable. 
```c
ecs_world_t *world = ecs_init();
```
  Reason is simple world is entity so is module entity component system.

```c
typedef struct {
  float x, y;
} Gravity;
ECS_COMPONENT_DECLARE(Gravity);
//...
ecs_world_t *world = ecs_init();
ECS_COMPONENT_DEFINE(world, Gravity);
ecs_singleton_set(world, Gravity, {.x=0.0f,.y=0.0f});
//...
```
  This will attach to world as component.

  To edit or update the varaible there are two ways. One is query and other is ecs_singleton_ensure(world, name id)

```c
void TestSystem(ecs_iter_t *it){
Gravity *g = ecs_singleton_ensure(it->world, Gravity);
```
  This will update the variable

  Another way is query. Required some more setup.
```c
void TestSystem(ecs_iter_t *it){
Gravity *g = ecs_field(it, Gravity, 0); // zero index in .query.terms
//...
```
```c
ecs_system(world, {
//...
.query.terms = {
  { ecs_id(Gravity), .src = ecs_id(Gravity) } // Singleton source
},
//...
```
Full example.
```c
#include "flecs.h"
typedef struct {
  float x, y;
} Gravity;
ECS_COMPONENT_DECLARE(Gravity);

// singletons
void TestSystem(ecs_iter_t *it){
  Gravity *g = ecs_field(it, Gravity, 0);
  if(!g)return;
  ecs_print(1,"x: %f", g->x);
  g->x += 1.0f;
}

int main(){
  ecs_world_t *world = ecs_init();
  ECS_COMPONENT_DEFINE(world, Gravity);
  ecs_singleton_set(world, Gravity, {.x=0.0f,.y=0.0f});
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
        .name = "TestSystem", 
        .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    }),
    .query.terms = {
	    // singletons
      { ecs_id(Gravity), .src = ecs_id(Gravity) } // match Gravity on itself
    },
    .callback = TestSystem
  });
  ecs_progress(world, 0);
  ecs_progress(world, 0);
  ecs_progress(world, 0);
  ecs_fini(world);
}
```
# Module Setup Example:

This should in header.
```c
typedef struct {
  VkBuffer meshBuffer;
}
NameContext;
ECS_COMPONENT_DECLARE(NameContext);
```
Flecs need to assign component to handle ID.

```c
void NameSetupSystem(ecs_iter_t *it) {
}
```
```c
void flecs_name_cleanup(ecs_world_t *world) {
}
```
```c
void flecs_name_cleanup_event_system(ecs_iter_t *it){
  flecs_name_cleanup(it->world);
  module_break_name(it, "name_module");
}
```
```c
name_register_components(ecs_world_t *world){
  ECS_COMPONENT_DEFINE(world, NameContext);
}
```
```c
name_register_systems(ecs_world_t *world){

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = flecs_name_cleanup_event_system
  });

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "NameSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
    .callback = NameSetupSystem
  });
}
```
```c
void flecs_name_module_init(ecs_world_t *world){
  ecs_log(1, "Initializing name module...");

  name_register_components(world);

  ecs_singleton_set(world, NameContext, {0});

  // ecs_entity_t e = ecs_new(world);
  // ecs_set(world, e, PluginModule, { .name = "name_module", .isCleanUp = false });
  add_module_name(world, "luajit_module");

  name_register_systems(world);
}
```
  Work in progress test.
