#ifndef FLECS_TYPES_H
#define FLECS_TYPES_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "flecs.h"
#include "cimgui.h"         // C ImGui wrapper
#include "cimgui_impl.h"    // Implementation helpers

#define WIDTH 800
#define HEIGHT 600

typedef struct {
  SDL_Window *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue graphicsQueue;
  VkQueue presentQueue;
  uint32_t graphicsFamily;            // Added: Graphics queue family index
  uint32_t presentFamily;             // Added: Present queue family index
  VkSwapchainKHR swapchain;
  VkImage *swapchainImages;
  VkImageView *swapchainImageViews;
  uint32_t imageCount;
  VkExtent2D swapchainExtent;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  VkRenderPass renderPass;
  VkDescriptorPool descriptorPool;  // New: For ImGui
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkFramebuffer *framebuffers;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence renderFinishedFence;
  uint32_t imageIndex;
  VkFence inFlightFence;              
  bool shouldQuit;
  bool hasError;
  const char *errorMessage;
  // ImGui additions
  ImGuiContext* imguiContext;
  bool isImGuiInitialized;
} WorldContext;


typedef struct {
    float pos[2];
    float color[3];
} Vertex;

typedef struct {
  ecs_entity_t LogicUpdatePhase;
  ecs_entity_t BeginRenderPhase;
  ecs_entity_t BeginGUIPhase;
  ecs_entity_t UpdateGUIPhase;
  ecs_entity_t EndGUIPhase;
  ecs_entity_t RenderPhase;
  ecs_entity_t EndRenderPhase;
  ecs_entity_t InstanceSetupPhase;
  ecs_entity_t SurfaceSetupPhase;
  ecs_entity_t DeviceSetupPhase;
  ecs_entity_t SwapchainSetupPhase;
  ecs_entity_t TriangleBufferSetupPhase;
  ecs_entity_t RenderPassSetupPhase;    
  ecs_entity_t FramebufferSetupPhase;   
  ecs_entity_t CommandPoolSetupPhase;   
  ecs_entity_t CommandBufferSetupPhase; 
  ecs_entity_t PipelineSetupPhase;      
  ecs_entity_t SyncSetupPhase;
  ecs_entity_t SetupLogicPhase;
} FlecsPhases;

extern FlecsPhases GlobalPhases;

void flecs_phases_init(ecs_world_t *world, FlecsPhases *phases);

#endif