#ifndef FLECS_TYPES_H
#define FLECS_TYPES_H

#include <SDL3/SDL.h>
#include <vulkan/vulkan.h>
#include "flecs.h"

typedef struct {
    SDL_Window* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    uint32_t imageCount;
    uint32_t currentImageIndex;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* swapchainFramebuffers;
} WorldContext;

typedef struct {
    float pos[2];
    float color[3];
} Vertex;

// Define a structure for phases
typedef struct {
  ecs_entity_t LogicUpdatePhase;
  ecs_entity_t BeginRenderPhase;
  ecs_entity_t BeginGUIPhase;
  ecs_entity_t UpdateGUIPhase;
  ecs_entity_t EndGUIPhase;
  ecs_entity_t RenderPhase;
  ecs_entity_t EndRenderPhase;
} FlecsPhases;

// Function to initialize phases
void flecs_phases_init(ecs_world_t *world, FlecsPhases *phases);

// Global access to phases (declare as extern)
extern FlecsPhases GlobalPhases;

#endif