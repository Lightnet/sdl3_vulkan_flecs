#ifndef FLECS_IMGUI_H
#define FLECS_IMGUI_H

#include "flecs.h"
#include "flecs_types.h"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>

typedef struct {
  VkDescriptorPool imguiDescriptorPool;        // ImGui descriptor pool
  ImGuiContext* imguiContext;                  // ImGui context
  bool isImGuiInitialized;                     // ImGui initialization flag
} IMGUIContext;
ECS_COMPONENT_DECLARE(IMGUIContext);

void flecs_imgui_module_init(ecs_world_t *world);
void flecs_imgui_cleanup(ecs_world_t *world);

// Systems for ImGui
// void ImGuiSetupSystem(ecs_iter_t *it);
// void ImGuiInputSystem(ecs_iter_t *it);
// void ImGuiBeginSystem(ecs_iter_t *it);
// void ImGuiUpdateSystem(ecs_iter_t *it);
// void ImGuiEndSystem(ecs_iter_t *it);

#endif