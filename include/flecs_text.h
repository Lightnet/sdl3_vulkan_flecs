#ifndef FLECS_TEXT_H
#define FLECS_TEXT_H

#include "flecs.h"
#include "flecs_types.h"
#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
  // Text Rendering
  VkBuffer textVertexBuffer;                   // Text vertex buffer
  VkDeviceMemory textVertexBufferMemory;       // Text vertex buffer memory
  VkBuffer textIndexBuffer;                    // Text index buffer
  VkDeviceMemory textIndexBufferMemory;        // Text index buffer memory
  VkDescriptorPool textDescriptorPool;         // Text descriptor pool
  VkDescriptorSet textDescriptorSet;           // Text descriptor set
  VkDescriptorSetLayout textDescriptorSetLayout; // Text descriptor set layout
  VkPipelineLayout textPipelineLayout;         // Text pipeline layout
  VkPipeline textPipeline;                     // Text pipeline
  VkImage textFontImage;                       // Font atlas texture
  VkDeviceMemory textFontImageMemory;          // Font atlas memory
  VkImageView textFontImageView;               // Font atlas image view
  VkSampler textFontSampler;                   // Font texture sampler
  void *textGlyphs;                            // Metrics for ASCII 32-126
  int textAtlasWidth;                          // Font atlas width
  int textAtlasHeight;                         // Font atlas height
} Text2DContext;
ECS_COMPONENT_DECLARE(Text2DContext);

void flecs_text_module_init(ecs_world_t *world);
void flecs_text_cleanup(ecs_world_t *world);

// void TextSetupSystem(ecs_iter_t *it);
// void TextPipelineSetupSystem(ecs_iter_t *it);
// void TextRenderSystem(ecs_iter_t *it);

#endif