#ifndef FLECS_VULKAN_H
#define FLECS_VULKAN_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include "flecs.h"
#include "flecs_types.h"

typedef struct {
  // Vulkan Core
  // SDL_Window *window;                          // SDL Window
  VkInstance instance;                         // Vulkan instance
  VkDebugUtilsMessengerEXT debugMessenger;     // Vulkan debug messenger
  // VkSurfaceKHR surface;                        // Vulkan surface from SDL
  VkPhysicalDevice physicalDevice;             // Vulkan physical device
  VkDevice device;                             // Vulkan logical device
  VkQueue graphicsQueue;                       // Vulkan graphics queue
  VkQueue presentQueue;                        // Vulkan present queue
  uint32_t graphicsFamily;                     // Graphics queue family index
  uint32_t presentFamily;                      // Present queue family index
  VkSwapchainKHR swapchain;                    // Vulkan swapchain
  VkImage *swapchainImages;                    // Vulkan swapchain images
  VkImageView *swapchainImageViews;            // Vulkan swapchain image views
  uint32_t imageCount;                         // Number of swapchain images
  VkExtent2D swapchainExtent;                  // Swapchain dimensions
  VkRenderPass renderPass;                     // Vulkan render pass (shared)
  VkFramebuffer *framebuffers;                 // Vulkan framebuffers
  VkCommandPool commandPool;                   // Vulkan command pool
  VkCommandBuffer commandBuffer;               // Vulkan command buffer
  VkSemaphore imageAvailableSemaphore;         // Sync: acquire image
  VkSemaphore renderFinishedSemaphore;         // Sync: rendering complete
  VkFence renderFinishedFence;                 // Sync: fence for render completion
  uint32_t imageIndex;                         // Current swapchain image index
  VkFence inFlightFence;                       // Sync: fence for frame in flight
  uint32_t width;                              // Window width (from swapchain)
  uint32_t height;                             // Window height (from swapchain)
  bool shouldQuit;                             // SDL quit flag
  bool hasError;                               // Error flag
  const char *errorMessage;                    // Error message
  bool needsSwapchainRecreation;               // Flag to indicate swapchain needs recreation
} VulkanContext;

ECS_COMPONENT_DECLARE(VulkanContext);


void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx);

// void InstanceSetupSystem(ecs_iter_t *it);
// void SurfaceSetupSystem(ecs_iter_t *it);
// void DeviceSetupSystem(ecs_iter_t *it);
// void SwapchainSetupSystem(ecs_iter_t *it);
// void TriangleBufferSetupSystem(ecs_iter_t *it);
// void RenderPassSetupSystem(ecs_iter_t *it);    
// void FramebufferSetupSystem(ecs_iter_t *it);   
// void CommandPoolSetupSystem(ecs_iter_t *it);   
// void CommandBufferSetupSystem(ecs_iter_t *it); 
// void PipelineSetupSystem(ecs_iter_t *it);      
// void SyncSetupSystem(ecs_iter_t *it);
// void BeginRenderSystem(ecs_iter_t *it);
// void RenderSystem(ecs_iter_t *it);
// void EndRenderSystem(ecs_iter_t *it);

void flecs_vulkan_cleanup(ecs_world_t *world, WorldContext *ctx);

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData);

#endif