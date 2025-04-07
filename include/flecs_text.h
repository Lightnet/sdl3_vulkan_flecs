#ifndef FLECS_TEXT_H
#define FLECS_TEXT_H

#include "flecs.h"
#include "flecs_types.h"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <ft2build.h>
#include FT_FREETYPE_H

void flecs_text_module_init(ecs_world_t *world, WorldContext *ctx);
void flecs_text_cleanup(WorldContext *ctx);

void TextSetupSystem(ecs_iter_t *it);
void TextPipelineSetupSystem(ecs_iter_t *it);
void TextRenderSystem(ecs_iter_t *it);

#endif