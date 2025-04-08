```c

typedef struct {
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

  // Triangle Mesh
  VkBuffer triVertexBuffer;                    // Triangle vertex buffer
  VkDeviceMemory triVertexBufferMemory;        // Triangle vertex buffer memory
  VkBuffer triIndexBuffer;                     // Triangle index buffer
  VkDeviceMemory triIndexBufferMemory;         // Triangle index buffer memory
  VkPipelineLayout triPipelineLayout;          // Triangle pipeline layout
  VkPipeline triGraphicsPipeline;              // Triangle graphics pipeline

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

  // ImGui
  VkDescriptorPool imguiDescriptorPool;        // ImGui descriptor pool
  ImGuiContext* imguiContext;                  // ImGui context
  bool isImGuiInitialized;                     // ImGui initialization flag
} WorldContext;
```
