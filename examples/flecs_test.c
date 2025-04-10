#include "flecs.h"

typedef struct {
  float x, y;
} Gravity;
ECS_COMPONENT_DECLARE(Gravity);

// singletons
void TestSystem(ecs_iter_t *it){
  //Gravity *g = ecs_field(it, Gravity, 0);// work single
  //Gravity *g = ecs_singleton_get(it->world, Gravity); // working single but error
  //const Gravity *g = ecs_singleton_get(it->world, Gravity); // working single const read only
  Gravity *g = ecs_singleton_ensure(it->world, Gravity); // working single editable
  //Gravity *g = ecs_singleton_modified(it->world, Gravity); //nope return void
  if(!g)return;
  ecs_print(1,"x: %f", g->x);
  g->x += 1.0f;
}
int main(){
  ecs_world_t *world = ecs_init();
  ECS_COMPONENT_DEFINE(world, Gravity);
  //ECS_COMPONENT(world, Gravity);
  ecs_singleton_set(world, Gravity, {.x=0.0f,.y=0.0f});
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
        .name = "TestSystem", 
        .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    }),
    .query.terms = {
      //A singleton is a component matched on itself
      { ecs_id(Gravity), .src = ecs_id(Gravity) } // match Input on itself
    },
    .callback = TestSystem
  });
  ecs_progress(world, 0); // update
  ecs_progress(world, 0); // update
  ecs_progress(world, 0); // update
  ecs_fini(world);
}