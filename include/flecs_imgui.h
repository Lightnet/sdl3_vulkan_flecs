#ifndef FLECS_IMGUI_H
#define FLECS_IMGUI_H

#include "flecs.h"
#include "flecs_types.h"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
// #include "cimgui.h"         // C ImGui wrapper
// #include "cimgui_impl.h"    // Implementation helpers

void flecs_imgui_module_init(ecs_world_t *world, WorldContext *ctx);
void flecs_imgui_cleanup(WorldContext *ctx);

// Systems for ImGui
void ImGuiSetupSystem(ecs_iter_t *it);
void ImGuiBeginSystem(ecs_iter_t *it);
void ImGuiUpdateSystem(ecs_iter_t *it);
void ImGuiEndSystem(ecs_iter_t *it);

#endif