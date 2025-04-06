#define _CRT_SECURE_NO_WARNINGS

#include "flecs_vulkan.h"
#include "flecs.h"

#include "vert.spv.h"
#include "frag.spv.h"

#define WIDTH 800
#define HEIGHT 600


VkShaderModule createShaderModule(VkDevice device, const uint32_t* code, size_t codeSize) {
  VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = codeSize * sizeof(uint32_t);  // Size in bytes (uint32_t is 4 bytes)
  createInfo.pCode = code;
  VkShaderModule module;
  if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
      ecs_err("Failed to create shader module");
      return VK_NULL_HANDLE;
  }
  return module;
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT severity,
  VkDebugUtilsMessageTypeFlagsEXT type,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {
  // ecs_log(1, "Instance setup completed");
  ecs_err("Vulkan validation layer: %s", pCallbackData->pMessage);
  //printf("Vulkan validation layer: %s\n", pCallbackData->pMessage);
  return VK_FALSE;
}


void InstanceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"InstanceSetupSystem started");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) {
    ecs_err("Error: ctx is NULL or has error");
    return;
  }

  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan Triangle";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Get SDL3 Vulkan instance extensions
  uint32_t sdlExtensionCount = 0;
  const char *const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
  if (!sdlExtensions || sdlExtensionCount == 0) {
    ecs_err("Error: Failed to get Vulkan instance extensions from SDL");
    ctx->hasError = true;
    ctx->errorMessage = "Failed to get Vulkan instance extensions from SDL";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // Add VK_EXT_debug_utils
  uint32_t totalExtensionCount = sdlExtensionCount + 1;
  const char **extensions = malloc(sizeof(const char *) * totalExtensionCount);
  if (!extensions) {
    ecs_err("Error: Failed to allocate memory for extensions");
    ctx->hasError = true;
    ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  for (uint32_t i = 0; i < sdlExtensionCount; i++) {
    extensions[i] = sdlExtensions[i];
  }
  extensions[sdlExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  ecs_print(1,"Found %u Vulkan instance extensions:", totalExtensionCount);
  for (uint32_t i = 0; i < totalExtensionCount; i++) {
    ecs_print(1,"  %u: %s", i + 1, extensions[i]);
  }

  // Enable validation layer
  const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
  uint32_t layerCount = 1;

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = totalExtensionCount;
  createInfo.ppEnabledExtensionNames = extensions;
  createInfo.enabledLayerCount = layerCount;
  createInfo.ppEnabledLayerNames = validationLayers;

  VkResult result = vkCreateInstance(&createInfo, NULL, &ctx->instance);
  free(extensions);
  if (result != VK_SUCCESS) {
    ecs_err("Error: Failed to create Vulkan instance (VkResult: %d)", result);
    ctx->hasError = true;
    ctx->errorMessage = "Failed to create Vulkan instance";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // Setup debug messenger
  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkCreateDebugUtilsMessengerEXT");
  if (createDebugUtilsMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    result = createDebugUtilsMessenger(ctx->instance, &debugCreateInfo, NULL, &ctx->debugMessenger);
    if (result != VK_SUCCESS) {
      ecs_err("Warning: Failed to create debug messenger (VkResult: %d)", result);
      ctx->debugMessenger = VK_NULL_HANDLE;
    } else {
      ecs_err("Debug messenger created");
    }
  } else {
    ecs_err("Warning: vkCreateDebugUtilsMessengerEXT not found");
    ctx->debugMessenger = VK_NULL_HANDLE;
  }

  ecs_log(1, "Instance setup completed");
}


void SurfaceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"SurfaceSetupSystem started");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) {
    ecs_err("Error: ctx is NULL or has error");
    return;
  }

  if (!ctx->window) {
    ecs_err("Error: window is NULL");
    ctx->hasError = true;
    ctx->errorMessage = "SDL window not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  if (!ctx->instance) {
    ecs_err("Error: instance is NULL");
    ctx->hasError = true;
    ctx->errorMessage = "Vulkan instance not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  if (!SDL_Vulkan_CreateSurface(ctx->window, ctx->instance, NULL, &ctx->surface)) {
    ecs_err("Error: Failed to create Vulkan surface - %s", SDL_GetError());
    ctx->hasError = true;
    ctx->errorMessage = "Failed to create Vulkan surface";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  uint32_t deviceCount = 0;
  VkResult result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
  if (result != VK_SUCCESS) {
    ecs_err("Error: vkEnumeratePhysicalDevices failed (VkResult: %d)", result);
    ctx->hasError = true;
    ctx->errorMessage = "Failed to enumerate physical devices (first call)";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  
  if (deviceCount == 0) {
    ecs_err("Error: No physical devices found");
    ctx->hasError = true;
    ctx->errorMessage = "No Vulkan physical devices found";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
  if (!devices) {
    ecs_err("Error: Failed to allocate memory for devices");
    ctx->hasError = true;
    ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  result = vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
  if (result != VK_SUCCESS) {
    ecs_err("Error: vkEnumeratePhysicalDevices failed (VkResult: %d)", result);
    free(devices);
    ctx->hasError = true;
    ctx->errorMessage = "Failed to enumerate physical devices (second call)";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ctx->physicalDevice = devices[0];  // Pick first device for now
  free(devices);
  
  ecs_log(1, "Surface setup completed");
}


void DeviceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"DeviceSetupSystem started");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx) {
    ecs_err("Error: ctx is NULL");
    return;
  }
  if (ctx->hasError) {
    ecs_err("Error: ctx has error state");
    return;
  }

  if (ctx->physicalDevice == VK_NULL_HANDLE) {
    ecs_err("Error: physicalDevice is NULL");
    ctx->hasError = true;
    ctx->errorMessage = "Physical device not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  if (ctx->surface == VK_NULL_HANDLE) {
    ecs_err("Error: surface is NULL");
    ctx->hasError = true;
    ctx->errorMessage = "Surface not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
  ecs_print(1,"Queue family count: %u", queueFamilyCount);
  if (queueFamilyCount == 0) {
    ecs_err("Error: No queue families found");
    ctx->hasError = true;
    ctx->errorMessage = "No queue families available";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  if (!queueFamilies) {
    ecs_err("Error: Failed to allocate queueFamilies");
    ctx->hasError = true;
    ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

  ctx->graphicsFamily = UINT32_MAX;
  ctx->presentFamily = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ctx->graphicsFamily = i;
      VkBool32 presentSupport = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physicalDevice, i, ctx->surface, &presentSupport);
      if (presentSupport) ctx->presentFamily = i;
      if (ctx->graphicsFamily != UINT32_MAX && ctx->presentFamily != UINT32_MAX) break;
  }
  ecs_print(1,"Graphics family: %u, Present family: %u", ctx->graphicsFamily, ctx->presentFamily);
  free(queueFamilies);

  if (ctx->graphicsFamily == UINT32_MAX || ctx->presentFamily == UINT32_MAX) {
    ecs_err("Error: Failed to find required queue families");
    ctx->hasError = true;
    ctx->errorMessage = "No graphics or present queue family found";
    ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkDeviceQueueCreateInfo queueCreateInfos[2] = {{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}};
  float queuePriority = 1.0f;
  queueCreateInfos[0].queueFamilyIndex = ctx->graphicsFamily;
  queueCreateInfos[0].queueCount = 1;
  queueCreateInfos[0].pQueuePriorities = &queuePriority;
  uint32_t queueCreateInfoCount = 1;
  if (ctx->graphicsFamily != ctx->presentFamily) {
      queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfos[1].queueFamilyIndex = ctx->presentFamily;
      queueCreateInfos[1].queueCount = 1;
      queueCreateInfos[1].pQueuePriorities = &queuePriority;
      queueCreateInfoCount = 2;
  }

  VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};  // Fixed sType
  deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
  const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  deviceCreateInfo.enabledExtensionCount = 1;
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

  if (vkCreateDevice(ctx->physicalDevice, &deviceCreateInfo, NULL, &ctx->device) != VK_SUCCESS) {
      ecs_err("Error: vkCreateDevice failed");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create logical device";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  vkGetDeviceQueue(ctx->device, ctx->graphicsFamily, 0, &ctx->graphicsQueue);
  vkGetDeviceQueue(ctx->device, ctx->presentFamily, 0, &ctx->presentQueue);
  ecs_log(1, "Logical device and queues created successfully");
}


void SwapchainSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"SwapchainSetupSystem ");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // Query surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities) != VK_SUCCESS) {
      ctx->hasError = true;
      ctx->errorMessage = "Failed to query surface capabilities";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // Set swapchain extent
  ctx->swapchainExtent = capabilities.currentExtent;

  // Adjust image count
  ctx->imageCount = 2;  // Desired number
  if (ctx->imageCount < capabilities.minImageCount) {
      ctx->imageCount = capabilities.minImageCount;
  }
  if (capabilities.maxImageCount > 0 && ctx->imageCount > capabilities.maxImageCount) {
      ctx->imageCount = capabilities.maxImageCount;
  }

  // Query supported formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, NULL);
  VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physicalDevice, ctx->surface, &formatCount, formats);
  VkSurfaceFormatKHR selectedFormat = formats[0];
  for (uint32_t i = 0; i < formatCount; i++) {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
          formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          selectedFormat = formats[i];
          break;
      }
  }
  free(formats);

  // Query supported present modes
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, NULL);
  VkPresentModeKHR* presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(ctx->physicalDevice, ctx->surface, &presentModeCount, presentModes);
  VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32_t i = 0; i < presentModeCount; i++) {
      if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
          selectedPresentMode = presentModes[i];
          break;
      }
  }
  free(presentModes);

  // Create swapchain
  VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface = ctx->surface;
  swapchainCreateInfo.minImageCount = ctx->imageCount;
  swapchainCreateInfo.imageFormat = selectedFormat.format;
  swapchainCreateInfo.imageColorSpace = selectedFormat.colorSpace;
  swapchainCreateInfo.imageExtent = ctx->swapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.preTransform = capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCreateInfo.presentMode = selectedPresentMode;
  swapchainCreateInfo.clipped = VK_TRUE;

  VkResult result = vkCreateSwapchainKHR(ctx->device, &swapchainCreateInfo, NULL, &ctx->swapchain);
  if (result != VK_SUCCESS) {
      char errorMsg[64];
      ecs_err(errorMsg, "Failed to create swapchain (VkResult: %d)", result);
      ctx->hasError = true;
      ctx->errorMessage = errorMsg;
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // Get swapchain images
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, NULL);
  ctx->swapchainImages = malloc(sizeof(VkImage) * ctx->imageCount);
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, ctx->swapchainImages);

  // Create image views
  ctx->swapchainImageViews = malloc(sizeof(VkImageView) * ctx->imageCount);
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
      VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      viewInfo.image = ctx->swapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = selectedFormat.format;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.layerCount = 1;
      if (vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->swapchainImageViews[i]) != VK_SUCCESS) {
          ctx->hasError = true;
          ctx->errorMessage = "Failed to create swapchain image view";
          ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
      }
  }

  ecs_log(1, "Swapchain setup completed with %u images", ctx->imageCount);
}


void TriangleBufferSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"TriangleBufferSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  Vertex vertices[] = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
  };
  VkDeviceSize bufferSize = sizeof(vertices);

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(ctx->device, &bufferInfo, NULL, &ctx->vertexBuffer) != VK_SUCCESS) {
      ecs_err("Failed to create vertex buffer");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create vertex buffer";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(ctx->device, ctx->vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = 0;
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((memRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
          allocInfo.memoryTypeIndex = i;
          break;
      }
  }

  if (vkAllocateMemory(ctx->device, &allocInfo, NULL, &ctx->vertexBufferMemory) != VK_SUCCESS) {
      ecs_err("Failed to allocate vertex buffer memory");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to allocate vertex buffer memory";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  if (vkBindBufferMemory(ctx->device, ctx->vertexBuffer, ctx->vertexBufferMemory, 0) != VK_SUCCESS) {
      ecs_err("Failed to bind vertex buffer memory");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to bind vertex buffer memory";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  void* data;
  if (vkMapMemory(ctx->device, ctx->vertexBufferMemory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
      ecs_err("Failed to map vertex buffer memory");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to map vertex buffer memory";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  memcpy(data, vertices, (size_t)bufferSize);
  vkUnmapMemory(ctx->device, ctx->vertexBufferMemory);

  ecs_log(1, "Triangle vertex buffer setup completed");
}


void RenderPassSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"RenderPassSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  if (!ctx->device) {
      ecs_err("Device is null in RenderPassSetupSystem");
      ctx->hasError = true;
      ctx->errorMessage = "Device is null in RenderPassSetupSystem";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (vkCreateRenderPass(ctx->device, &renderPassInfo, NULL, &ctx->renderPass) != VK_SUCCESS) {
      ecs_err("Failed to create render pass");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create render pass";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ecs_log(1, "Render pass setup completed");
}


void FramebufferSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"FramebufferSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ctx->framebuffers = malloc(sizeof(VkFramebuffer) * ctx->imageCount);
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
      VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      framebufferInfo.renderPass = ctx->renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = &ctx->swapchainImageViews[i];
      framebufferInfo.width = ctx->swapchainExtent.width;
      framebufferInfo.height = ctx->swapchainExtent.height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(ctx->device, &framebufferInfo, NULL, &ctx->framebuffers[i]) != VK_SUCCESS) {
          ecs_err("Failed to create framebuffer");
          ctx->hasError = true;
          ctx->errorMessage = "Failed to create framebuffer";
          ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
      }
  }

  ecs_log(1, "Framebuffer setup completed");
}


void CommandPoolSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"CommandPoolSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = ctx->graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
      ecs_err("Failed to create command pool");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create command pool";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ecs_log(1, "Command pool setup completed");
}


void CommandBufferSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"CommandBufferSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  if (!ctx->device || !ctx->commandPool) {
      ecs_err("Required resources null in CommandBufferSetupSystem");
      ctx->hasError = true;
      ctx->errorMessage = "Required resources null in CommandBufferSetupSystem";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = ctx->commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &ctx->commandBuffer) != VK_SUCCESS) {
      ecs_err("Failed to allocate command buffer");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to allocate command buffer";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ecs_log(1, "Command buffer setup completed");
}


void PipelineSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"PipelineSetupSystem");
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  if (!ctx->device || !ctx->renderPass) {
      ecs_err("Required resources null in PipelineSetupSystem");
      ctx->hasError = true;
      ctx->errorMessage = "Required resources null in PipelineSetupSystem";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // Calculate the number of uint32_t elements in each array
  size_t vertSpvSize = sizeof(vert_spv) / sizeof(vert_spv[0]);
  size_t fragSpvSize = sizeof(frag_spv) / sizeof(frag_spv[0]);

  VkShaderModule vertShaderModule = createShaderModule(ctx->device, vert_spv, vertSpvSize);
  VkShaderModule fragShaderModule = createShaderModule(ctx->device, frag_spv, fragSpvSize);
  if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
      ecs_err("Failed to create shader modules");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create shader modules";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkPipelineShaderStageCreateInfo vertStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStageInfo.module = vertShaderModule;
  vertStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragStageInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
  fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStageInfo.module = fragShaderModule;
  fragStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

  VkVertexInputBindingDescription bindingDesc = {0};
  bindingDesc.binding = 0;
  bindingDesc.stride = sizeof(Vertex);
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributeDescs[2] = {0};
  attributeDescs[0].binding = 0;
  attributeDescs[0].location = 0;
  attributeDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
  attributeDescs[0].offset = offsetof(Vertex, pos);
  attributeDescs[1].binding = 0;
  attributeDescs[1].location = 1;
  attributeDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescs[1].offset = offsetof(Vertex, color);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount = 2;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f};
  VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
  
  VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->pipelineLayout) != VK_SUCCESS) {
      ecs_err("Failed to create pipeline layout");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create pipeline layout";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.layout = ctx->pipelineLayout;
  pipelineInfo.renderPass = ctx->renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->graphicsPipeline) != VK_SUCCESS) {
      ecs_err("Failed to create graphics pipeline");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to create graphics pipeline";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);
  ecs_log(1, "Pipeline setup completed");
}


void SyncSetupSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "SyncSetupSystem starting...");
  ecs_print(1, "Context pointer: %p", (void*)ctx);

  VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  if (vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphore) != VK_SUCCESS) {
      ecs_err("Failed to create image available semaphore");
      ctx->hasError = true;
      ctx->errorMessage = "Semaphore creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  ecs_print(1, "Image available semaphore created: %p", (void*)ctx->imageAvailableSemaphore);

  if (vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphore) != VK_SUCCESS) {
      ecs_err("Failed to create render finished semaphore");
      ctx->hasError = true;
      ctx->errorMessage = "Semaphore creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  ecs_print(1, "Render finished semaphore created: %p", (void*)ctx->renderFinishedSemaphore);

  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  if (vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->inFlightFence) != VK_SUCCESS) {
      ecs_err("Failed to create in-flight fence");
      ctx->hasError = true;
      ctx->errorMessage = "Fence creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }
  ecs_print(1, "In-flight fence created: %p", (void*)ctx->inFlightFence);

  ecs_print(1, "SyncSetupSystem completed");
}


void BeginRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // ecs_print(1, "BeginRenderSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "In-flight fence: %p", (void*)ctx->inFlightFence);

  if (ctx->inFlightFence == VK_NULL_HANDLE) {
      ecs_err("In-flight fence is null, skipping render");
      return;
  }

  // Wait for the previous frame to finish
  vkWaitForFences(ctx->device, 1, &ctx->inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->device, 1, &ctx->inFlightFence);

  // Acquire the next image
  VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
                                          ctx->imageAvailableSemaphore, VK_NULL_HANDLE, &ctx->imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      ecs_print(1, "Swapchain out of date, skipping frame");
      return;
  } else if (result != VK_SUCCESS) {
      ecs_err("Error: vkAcquireNextImageKHR failed (VkResult: %d)", result);
      ctx->hasError = true;
      ctx->errorMessage = "Failed to acquire next image";
      return; // Don’t abort, just skip frame
  }

  // ecs_print(1, "BeginRenderSystem completed, imageIndex: %u", ctx->imageIndex);
}


void RenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  // ecs_print(1,"RenderSystem");

  // ecs_print(1, "RenderSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "In-flight fence: %p", (void*)ctx->inFlightFence);

  if (vkResetCommandBuffer(ctx->commandBuffer, 0) != VK_SUCCESS) {
      ecs_err("Failed to reset command buffer");
      return;
  }

  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(ctx->commandBuffer, &beginInfo) != VK_SUCCESS) {
      ecs_err("Failed to begin command buffer");
      return;
  }

  VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  renderPassInfo.renderPass = ctx->renderPass;
  renderPassInfo.framebuffer = ctx->framebuffers[ctx->imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = ctx->swapchainExtent;
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(ctx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  // ecs_print(1, "RenderSystem completed");
}

void BeginCMDBufferSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  // ecs_print(1,"BeginCMDBufferSystem");
  //triangle
  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphicsPipeline);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->vertexBuffer, offsets);
  vkCmdDraw(ctx->commandBuffer, 3, 1, 0, 0);

}


void EndCMDBufferSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  // ecs_print(1,"EndCMDBufferSystem");

  vkCmdEndRenderPass(ctx->commandBuffer);
  if (vkEndCommandBuffer(ctx->commandBuffer) != VK_SUCCESS) {
      ecs_err("Failed to end command buffer");
      return;
  }
}


void EndRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // ecs_print(1, "EndRenderSystem starting...");

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSemaphores[] = {ctx->imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &ctx->commandBuffer;
  VkSemaphore signalSemaphores[] = {ctx->renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFence) != VK_SUCCESS) {
      ecs_err("Error: Failed to submit queue");
      ctx->hasError = true;
      ctx->errorMessage = "Failed to submit queue";
      return;
  }

  VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &ctx->swapchain;
  presentInfo.pImageIndices = &ctx->imageIndex;

  VkResult presentResult = vkQueuePresentKHR(ctx->presentQueue, &presentInfo);
  if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
      ecs_print(1, "Swapchain out of date or suboptimal, skipping present");
  } else if (presentResult != VK_SUCCESS) {
      ecs_err("Error: vkQueuePresentKHR failed (VkResult: %d)", presentResult);
      ctx->hasError = true;
      ctx->errorMessage = "Failed to present queue";
  }

  // ecs_print(1, "EndRenderSystem completed");
}


void flecs_vulkan_cleanup(ecs_world_t *world, WorldContext *ctx) {
  if (!ctx) return;

  ecs_print(1, "Vulkan cleanup starting...");
  ecs_print(1, "Cleanup called, fence: %p", (void*)ctx->inFlightFence);

  if (ctx->device) {
      vkDeviceWaitIdle(ctx->device);

      // Destroy device-specific objects
      if (ctx->inFlightFence != VK_NULL_HANDLE) {
          vkDestroyFence(ctx->device, ctx->inFlightFence, NULL);
          ctx->inFlightFence = VK_NULL_HANDLE;
      }
      if (ctx->renderFinishedSemaphore != VK_NULL_HANDLE) {
          vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphore, NULL);
          ctx->renderFinishedSemaphore = VK_NULL_HANDLE;
      }
      if (ctx->imageAvailableSemaphore != VK_NULL_HANDLE) {
          vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphore, NULL);
          ctx->imageAvailableSemaphore = VK_NULL_HANDLE;
      }
      if (ctx->commandPool != VK_NULL_HANDLE) {
          vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
          ctx->commandPool = VK_NULL_HANDLE;
      }
      if (ctx->vertexBufferMemory != VK_NULL_HANDLE) {
          vkFreeMemory(ctx->device, ctx->vertexBufferMemory, NULL);
          ctx->vertexBufferMemory = VK_NULL_HANDLE;
      }
      if (ctx->vertexBuffer != VK_NULL_HANDLE) {
          vkDestroyBuffer(ctx->device, ctx->vertexBuffer, NULL);
          ctx->vertexBuffer = VK_NULL_HANDLE;
      }
      if (ctx->graphicsPipeline != VK_NULL_HANDLE) {
          vkDestroyPipeline(ctx->device, ctx->graphicsPipeline, NULL);
          ctx->graphicsPipeline = VK_NULL_HANDLE;
      }
      if (ctx->pipelineLayout != VK_NULL_HANDLE) { // Added
          vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, NULL);
          ctx->pipelineLayout = VK_NULL_HANDLE;
      }
      if (ctx->renderPass != VK_NULL_HANDLE) {
          vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
          ctx->renderPass = VK_NULL_HANDLE;
      }
      for (uint32_t i = 0; i < ctx->imageCount; i++) {
          if (ctx->framebuffers && ctx->framebuffers[i] != VK_NULL_HANDLE) {
              vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
              ctx->framebuffers[i] = VK_NULL_HANDLE;
          }
          if (ctx->swapchainImageViews && ctx->swapchainImageViews[i] != VK_NULL_HANDLE) {
              vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
              ctx->swapchainImageViews[i] = VK_NULL_HANDLE;
          }
      }
      if (ctx->framebuffers) {
          free(ctx->framebuffers);
          ctx->framebuffers = NULL;
      }
      if (ctx->swapchainImageViews) {
          free(ctx->swapchainImageViews);
          ctx->swapchainImageViews = NULL;
      }
      if (ctx->swapchainImages) {
          free(ctx->swapchainImages);
          ctx->swapchainImages = NULL;
      }
      if (ctx->swapchain != VK_NULL_HANDLE) {
          vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
          ctx->swapchain = VK_NULL_HANDLE;
      }
      vkDestroyDevice(ctx->device, NULL);
      ctx->device = VK_NULL_HANDLE;
  }

  // Destroy instance-specific objects after device is gone
  if (ctx->instance) {
      if (ctx->debugMessenger != VK_NULL_HANDLE) { // Added
          PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger =
              (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(ctx->instance, "vkDestroyDebugUtilsMessengerEXT");
          if (destroyDebugUtilsMessenger) {
              destroyDebugUtilsMessenger(ctx->instance, ctx->debugMessenger, NULL);
              ctx->debugMessenger = VK_NULL_HANDLE;
          }
      }
      if (ctx->surface != VK_NULL_HANDLE) {
          vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
          ctx->surface = VK_NULL_HANDLE;
      }
      vkDestroyInstance(ctx->instance, NULL);
      ctx->instance = VK_NULL_HANDLE;
  }

  ecs_print(1, "Vulkan cleanup completed");
}


void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "init vulkan module");

  // Setup systems (unchanged)
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "InstanceSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.InstanceSetupPhase)) }),
      .callback = InstanceSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "SurfaceSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SurfaceSetupPhase)) }),
      .callback = SurfaceSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "DeviceSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.DeviceSetupPhase)) }),
      .callback = DeviceSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "SwapchainSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SwapchainSetupPhase)) }),
      .callback = SwapchainSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "TriangleBufferSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.TriangleBufferSetupPhase)) }),
      .callback = TriangleBufferSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "RenderPassSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.RenderPassSetupPhase)) }),
      .callback = RenderPassSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "FramebufferSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.FramebufferSetupPhase)) }),
      .callback = FramebufferSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "CommandPoolSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CommandPoolSetupPhase)) }),
      .callback = CommandPoolSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "CommandBufferSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CommandBufferSetupPhase)) }),
      .callback = CommandBufferSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "PipelineSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.PipelineSetupPhase)) }),
      .callback = PipelineSetupSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "SyncSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SyncSetupPhase)) }),
      .callback = SyncSetupSystem
  });

  // Runtime systems
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "BeginRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginRenderPhase)) }),
      .callback = BeginRenderSystem
  });
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "RenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.RenderPhase)) }),
      .callback = RenderSystem
  });

  //cmd buffer triangle buffer
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "BeginCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginCMDBufferPhase)) }),
    .callback = BeginCMDBufferSystem
  });

  //end cmd buffer
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "EndCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndCMDBufferPhase)) }),
    .callback = EndCMDBufferSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "EndRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndRenderPhase)) }),
      .callback = EndRenderSystem
  });
}





