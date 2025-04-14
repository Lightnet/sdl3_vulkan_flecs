#ifndef FLECS_TYPES_H
#define FLECS_TYPES_H

#include <SDL3/SDL.h>           //SDL 3.x
#include <SDL3/SDL_vulkan.h>    //SDL 3.x
#include <vulkan/vulkan.h>
#include "flecs.h"              // flecs v4.x
#include "cimgui.h"             // C ImGui wrapper
#include "cimgui_impl.h"        // Implementation helpers
#include <ft2build.h>           // freetype
#include FT_FREETYPE_H          // freetype

// #define WIDTH 800
// #define HEIGHT 600

// typedef struct {
//   float width, height;  // Glyph size in pixels
//   float advance;        // Horizontal advance to next glyph
//   float bearingX, bearingY; // Offset from baseline
//   float uvX, uvY;       // Bottom-left UV coords in atlas
//   float uvWidth, uvHeight; // UV dimensions
// } GlyphMetrics;

// typedef struct {
//   char *text;           // Text to render
//   float x, y;           // Position in screen space
// } TextComponent;

// ECS_COMPONENT_DECLARE(TextComponent);

// typedef struct {
  // // Vulkan Core
  // SDL_Window *window;                          // SDL Window
  // VkInstance instance;                         // Vulkan instance
  // VkDebugUtilsMessengerEXT debugMessenger;     // Vulkan debug messenger
  // VkSurfaceKHR surface;                        // Vulkan surface from SDL
  // VkPhysicalDevice physicalDevice;             // Vulkan physical device
  // VkDevice device;                             // Vulkan logical device
  // VkQueue graphicsQueue;                       // Vulkan graphics queue
  // VkQueue presentQueue;                        // Vulkan present queue
  // uint32_t graphicsFamily;                     // Graphics queue family index
  // uint32_t presentFamily;                      // Present queue family index
  // VkSwapchainKHR swapchain;                    // Vulkan swapchain
  // VkImage *swapchainImages;                    // Vulkan swapchain images
  // VkImageView *swapchainImageViews;            // Vulkan swapchain image views
  // uint32_t imageCount;                         // Number of swapchain images
  // VkExtent2D swapchainExtent;                  // Swapchain dimensions
  // VkRenderPass renderPass;                     // Vulkan render pass (shared)
  // VkFramebuffer *framebuffers;                 // Vulkan framebuffers
  // VkCommandPool commandPool;                   // Vulkan command pool
  // VkCommandBuffer commandBuffer;               // Vulkan command buffer
  // VkSemaphore imageAvailableSemaphore;         // Sync: acquire image
  // VkSemaphore renderFinishedSemaphore;         // Sync: rendering complete
  // VkFence renderFinishedFence;                 // Sync: fence for render completion
  // uint32_t imageIndex;                         // Current swapchain image index
  // VkFence inFlightFence;                       // Sync: fence for frame in flight
  // uint32_t width;                              // Window width (from swapchain)
  // uint32_t height;                             // Window height (from swapchain)
  // bool shouldQuit;                             // SDL quit flag
  // bool hasError;                               // Error flag
  // const char *errorMessage;                    // Error message
  // bool needsSwapchainRecreation;               // Flag to indicate swapchain needs recreation

  // // Triangle Mesh
  // VkBuffer triVertexBuffer;                    // Triangle vertex buffer
  // VkDeviceMemory triVertexBufferMemory;        // Triangle vertex buffer memory
  // VkBuffer triIndexBuffer;                     // Triangle index buffer
  // VkDeviceMemory triIndexBufferMemory;         // Triangle index buffer memory
  // VkPipelineLayout triPipelineLayout;          // Triangle pipeline layout
  // VkPipeline triGraphicsPipeline;              // Triangle graphics pipeline

  // // Text Rendering
  // VkBuffer textVertexBuffer;                   // Text vertex buffer
  // VkDeviceMemory textVertexBufferMemory;       // Text vertex buffer memory
  // VkBuffer textIndexBuffer;                    // Text index buffer
  // VkDeviceMemory textIndexBufferMemory;        // Text index buffer memory
  // VkDescriptorPool textDescriptorPool;         // Text descriptor pool
  // VkDescriptorSet textDescriptorSet;           // Text descriptor set
  // VkDescriptorSetLayout textDescriptorSetLayout; // Text descriptor set layout
  // VkPipelineLayout textPipelineLayout;         // Text pipeline layout
  // VkPipeline textPipeline;                     // Text pipeline
  // VkImage textFontImage;                       // Font atlas texture
  // VkDeviceMemory textFontImageMemory;          // Font atlas memory
  // VkImageView textFontImageView;               // Font atlas image view
  // VkSampler textFontSampler;                   // Font texture sampler
  // void *textGlyphs;                            // Metrics for ASCII 32-126
  // int textAtlasWidth;                          // Font atlas width
  // int textAtlasHeight;                         // Font atlas height

  // // ImGui
  // VkDescriptorPool imguiDescriptorPool;        // ImGui descriptor pool
  // ImGuiContext* imguiContext;                  // ImGui context
  // bool isImGuiInitialized;                     // ImGui initialization flag

  // // Texture2D
  // VkBuffer texture2dVertexBuffer;
  // VkDeviceMemory texture2dVertexBufferMemory;
  // VkBuffer texture2dIndexBuffer;
  // VkDeviceMemory texture2dIndexBufferMemory;
  // VkDescriptorPool texture2dDescriptorPool;
  // VkDescriptorSet texture2dDescriptorSet;
  // VkDescriptorSetLayout texture2dDescriptorSetLayout;
  // VkPipelineLayout texture2dPipelineLayout;
  // VkPipeline texture2dPipeline;
  // VkImage texture2dImage;
  // VkDeviceMemory texture2dImageMemory;
  // VkImageView texture2dImageView;
  // VkSampler texture2dSampler;

  // // Cube3D
  // VkBuffer cubeVertexBuffer;
  // VkDeviceMemory cubeVertexBufferMemory;
  // VkBuffer cubeIndexBuffer;
  // VkDeviceMemory cubeIndexBufferMemory;
  // VkBuffer cubeUniformBuffer;
  // VkDeviceMemory cubeUniformBufferMemory;
  // VkDescriptorPool cubeDescriptorPool;
  // VkDescriptorSet cubeDescriptorSet;
  // VkDescriptorSetLayout cubeDescriptorSetLayout;
  // VkPipelineLayout cubePipelineLayout;
  // VkPipeline cubePipeline;

  // // CubeTexture3D
  // VkBuffer cubetexture3dVertexBuffer;
  // VkDeviceMemory cubetexture3dVertexBufferMemory;
  // VkBuffer cubetexture3dIndexBuffer;
  // VkDeviceMemory cubetexture3dIndexBufferMemory;
  // VkBuffer cubetexture3dUniformBuffer;
  // VkDeviceMemory cubetexture3dUniformBufferMemory;
  // VkDescriptorPool cubetexture3dDescriptorPool;
  // VkDescriptorSet cubetexture3dDescriptorSet;
  // VkDescriptorSetLayout cubetexture3dDescriptorSetLayout;
  // VkPipelineLayout cubetexture3dPipelineLayout;
  // VkPipeline cubetexture3dPipeline;
  // VkImage cubetexture3dImage;
  // VkDeviceMemory cubetexture3dImageMemory;
  // VkImageView cubetexture3dImageView;
  // VkSampler cubetexture3dSampler;
   
// } WorldContext;

typedef struct {
    float pos[2];
    float color[3];
} Vertex;

typedef struct {
  // loop order
  ecs_entity_t LogicUpdatePhase;
  ecs_entity_t BeginRenderPhase;
  ecs_entity_t BeginCMDBufferPhase;
  ecs_entity_t CMDBufferPhase;
  ecs_entity_t CMDBuffer1Phase;
  ecs_entity_t CMDBuffer2Phase;
  ecs_entity_t EndCMDBufferPhase;
  ecs_entity_t EndRenderPhase;
  // setup once
  ecs_entity_t SetupPhase;
  ecs_entity_t InstanceSetupPhase;
  ecs_entity_t SurfaceSetupPhase;
  ecs_entity_t DeviceSetupPhase;
  ecs_entity_t SwapchainSetupPhase;
  ecs_entity_t RenderPassSetupPhase;    
  ecs_entity_t FramebufferSetupPhase;   
  ecs_entity_t CommandPoolSetupPhase;   
  ecs_entity_t CommandBufferSetupPhase; 
  ecs_entity_t PipelineSetupPhase;      
  ecs_entity_t SyncSetupPhase;
  ecs_entity_t SetupModulePhase;
} FlecsPhases;

// TEST
typedef struct {
  bool isCleanUpModule;
  int moduleCount;
} ModuleContext;
ECS_COMPONENT_DECLARE(ModuleContext);

typedef struct {
  char name[32]; // Fixed-size string for simplicity
  bool isCleanUp;
} PluginModule;
ECS_COMPONENT_DECLARE(PluginModule);

ecs_entity_t ShutDownEvent;
ecs_entity_t ShutDownModule;

ecs_entity_t CleanUpEvent;
ecs_entity_t CleanUpModule;

ecs_entity_t CleanUpGraphicEvent;
ecs_entity_t CleanUpGraphic;

ecs_entity_t CloseEvent;
ecs_entity_t CloseModule;

extern FlecsPhases GlobalPhases;

void flecs_phases_init(ecs_world_t *world, FlecsPhases *phases);

void flecs_init_module(ecs_world_t *world);

// ecs_entity_t add_module_name(ecs_world_t *world, const char *name);

void module_break_name(ecs_iter_t *it, const char *module_name);

ecs_entity_t add_module_name(ecs_world_t *world, const char *name);

#endif