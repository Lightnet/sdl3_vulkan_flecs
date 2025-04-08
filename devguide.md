# status:
 * Work in progress. 

# Information:
  This guide and trouble shoot for AI model and code review. As the module is broken up for vulkan to setup custom module.

# Design:
  To point out the area needed to setup.

# CMakeLists build:
  This script will handle config and build.

  This will check if libaray dll exist to not rebuild dll. If not exist rebuild or compile new dll.


# Flecs refs:
* https://www.flecs.dev/flecs/log_8h_source.html#l00376
 * ECS_INTERNAL_ERROR 
* https://www.flecs.dev/flecs/group__c__addons__log.html#gadb59e689daa4a653f4723cf6f098932b
 * ECS_INVALID_OPERATION 

# Design plans:
 * add and remove entity mesh.
 * struct components for transform for 2D, 3D and mesh data.

## Flecs features:
  * log
  * event
  * add
  * remove
  * query
  * observers
  * perfabs
  * relationships
  * entity
  * component 
  * system function

# Module setup:

  Work in progress.

  There are still some hard code on world context variable. Reason is the flecs struct component has not been added to as entity yet. As there some need to correct way to handle entity id system.

  Vulkan and SDL setup variable. Flecs component is not added yet as to develop and test those shaders for mesh and texture needed to be tested.

```c
// Vulkan Core
  SDL_Window *window;                          // SDL Window
  VkInstance instance;                         // Vulkan instance
  VkDebugUtilsMessengerEXT debugMessenger;     // Vulkan debug messenger
  VkSurfaceKHR surface;                        // Vulkan surface from SDL
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
```
  
```c
  // Triangle Mesh
  VkBuffer triVertexBuffer;                    // Triangle vertex buffer
  VkDeviceMemory triVertexBufferMemory;        // Triangle vertex buffer memory
  VkBuffer triIndexBuffer;                     // Triangle index buffer
  VkDeviceMemory triIndexBufferMemory;         // Triangle index buffer memory
  VkPipelineLayout triPipelineLayout;          // Triangle pipeline layout
  VkPipeline triGraphicsPipeline;              // Triangle graphics pipeline
```

```c
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
```

```c
  // ImGui
  VkDescriptorPool imguiDescriptorPool;        // ImGui descriptor pool
  ImGuiContext* imguiContext;                  // ImGui context
  bool isImGuiInitialized;                     // ImGui initialization flag
```

Note I missing some variable but it those ideas base on AI model build and refs. As it still need to use vulkan ref variable to access different area.

# Set Up Phase:
  Note that it need to setup in order to on start and loop render.

## On Start Phase (Vulkan)
  It use EcsDependsOn on the call once start. Only use single EcsDependsOn.

```
  - InstanceSetupSystem -> SurfaceSetupSystem -> DeviceSetupSystem -> SwapchainSetupSystem -> TriangleBufferSetupSystem -> RenderPassSetupSystem -> FramebufferSetupSystem -> CommandPoolSetupSystem -> CommandBufferSetupSystem -> PipelineSetupSystem -> SyncSetupSystem -> SetUpLogicSystem (modules init)
```

## Runtime (per frame)

```
- LogicUpdatePhase: (empty for now)
- BeginRenderPhase: BeginRenderSystem (acquire image)
- BeginCMDBufferPhase: BeginCMDBufferSystem (start command buffer and render pass)
- CMDBufferPhase: TriangleRenderBufferSystem -> TextRenderSystem -> ImGuiRenderSystem
- EndCMDBufferPhase: EndCMDBufferSystem (end render pass and command buffer)
- EndRenderPhase: EndRenderSystem (submit and present)
```

# Font Text:
  Work in progres. It required some setup in font text for position and render to screen in 2D world.

 * https://github.com/freetype/freetype
 * https://freetype.org/freetype2/docs/tutorial/step1.html#section-7

 There are couple steps need to render text. 
## steps:
  * create plane or quad triangle mesh.
  * load font
  * text data
  * create text into image with freetype format.
  * apply texture to mesh and shader.

  Just a guess need to look into.

# triangle 2D mesh:

## On Start
```
TriangleBufferSetupSystem
```
## Render:
```
TriangleRenderBufferSystem > ecs_dependson > BeginCMDBufferPhase
```
Note it still need to setup shader as well. frag.spv.h and vert.spv.h

Need to review code area.

# imgui:
  Note it use vulkan set up to warp into the imgui to create vulkan api.

## On Start:
```
ImGuiSetupSystem > ecs_dependson > SetupLogicPhase
```
Vulkan setup every things when finish call the next SetupLogicPhase.

## On Render:
```
ImGuiCMDBufferSystem > ecs_dependson > BeginCMDBufferPhase
```


# Credits:
 * Grok 3 AI Model
 * SDL 3.x Github
 * Flecs 4.x Github

