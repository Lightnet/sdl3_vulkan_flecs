#ifndef FLECS_TYPES_H
#define FLECS_TYPES_H

#include <SDL3/SDL.h>       //SDL 3.x
#include <SDL3/SDL_vulkan.h>//SDL 3.x
#include <vulkan/vulkan.h>
#include "flecs.h"          //flecs v4.x
#include "cimgui.h"         // C ImGui wrapper
#include "cimgui_impl.h"    // Implementation helpers
#include <ft2build.h>
#include FT_FREETYPE_H

#define WIDTH 800
#define HEIGHT 600

// typedef struct {
//   float width, height;  // Glyph size in pixels
//   float advance;        // Horizontal advance to next glyph
//   float bearingX, bearingY; // Offset from baseline
//   float uvX, uvY;       // Bottom-left UV coords in atlas
//   float uvWidth, uvHeight; // UV dimensions
// } GlyphMetrics;

typedef struct {
  char *text;           // Text to render
  float x, y;           // Position in screen space
} TextComponent;

ECS_COMPONENT_DECLARE(TextComponent);

typedef struct {
  SDL_Window *window;                                       // SDL Window
  VkInstance instance;                                      // Vulkan instance
  VkDebugUtilsMessengerEXT debugMessenger;                  // Vulkan 
  VkSurfaceKHR surface;                                     // SDL surface
  VkPhysicalDevice physicalDevice;                          // Vulkan 
  VkDevice device;                                          // Vulkan 
  VkQueue graphicsQueue;                                    // Vulkan 
  VkQueue presentQueue;                                     // Vulkan 
  uint32_t graphicsFamily;                                  // Graphics queue family index
  uint32_t presentFamily;                                   // Present queue family index
  VkSwapchainKHR swapchain;                                 // Vulkan 
  VkImage *swapchainImages;                                 // Vulkan 
  VkImageView *swapchainImageViews;                         // Vulkan 
  uint32_t imageCount;                                      // Vulkan 
  VkExtent2D swapchainExtent;                               // Vulkan 

  VkBuffer vertexBuffer;                                    // triangle
  VkDeviceMemory vertexBufferMemory;                        // triangle
  VkDeviceMemory indexBufferMemory;                         // triangle
  VkBuffer indexBuffer;                                     // triangle

  VkBuffer textVertexBuffer;                                // text vertices
  VkDeviceMemory textVertexBufferMemory;                    // text vertices
  VkBuffer textIndexBuffer;                                 // text indices
  VkDeviceMemory textIndexBufferMemory;                     // text indices
  VkDescriptorPool textDescriptorPool;                      // text system
  VkDescriptorSet textDescriptorSet;                        // store the text descriptor set
  VkDescriptorSetLayout textDescriptorSetLayout;            // font text texture binding
  VkPipelineLayout textPipelineLayout;                      // Text pipeline layout
  VkPipeline textPipeline;                                  // Text pipeline
  
  VkImage fontImage;                                        // Font atlas texture
  VkDeviceMemory fontImageMemory;                           // Font atlas memory
  VkImageView fontImageView;                                // Font atlas image view
  VkSampler fontSampler;                                    // Sampler for font texture
  
  void *glyphs;                                             // Metrics for ASCII 32-126 (95 characters)
  int atlasWidth, atlasHeight;                              // Font atlas dimensions
  
  VkDescriptorPool descriptorPool;                          // ImGui
  
  VkRenderPass renderPass;                                  // Vulkan 
  VkPipelineLayout pipelineLayout;                          //  
  VkPipeline graphicsPipeline;                              // 
  VkFramebuffer *framebuffers;                              // Vulkan 
  VkCommandPool commandPool;                                // Vulkan 
  VkCommandBuffer commandBuffer;                            // Vulkan 
  VkSemaphore imageAvailableSemaphore;                      // Vulkan 
  VkSemaphore renderFinishedSemaphore;                      // Vulkan 
  VkFence renderFinishedFence;                              // Vulkan 
  uint32_t imageIndex;                                      // Vulkan 
  VkFence inFlightFence;                                    // Vulkan       
  bool shouldQuit;                                          // SDL 
  bool hasError;                                            // Error
  const char *errorMessage;                                 // Error
  uint32_t width;                                           // Add this for window width
  uint32_t height;                                          // Add this for window height
  // ImGui additions
  ImGuiContext* imguiContext;                               // imgui 
  bool isImGuiInitialized;                                  // imgui
} WorldContext;


typedef struct {
    float pos[2];
    float color[3];
} Vertex;

typedef struct {
  ecs_entity_t LogicUpdatePhase;
  ecs_entity_t BeginRenderPhase;
  //vertex and imgui
  ecs_entity_t BeginCMDBufferPhase;
  ecs_entity_t EndCMDBufferPhase;

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