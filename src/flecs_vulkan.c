#define _CRT_SECURE_NO_WARNINGS

#include "flecs_vulkan.h"
#include "flecs.h"
#include "flecs_sdl.h"

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
//=====================================
// VULKAN SETUP
//=====================================

void InstanceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"InstanceSetupSystem started");
  //WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  // if (!ctx || ctx->hasError) {
  //   ecs_err("Error: ctx is NULL or has error");
  //   return;
  // }

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
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to get Vulkan instance extensions from SDL";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // Add VK_EXT_debug_utils
  uint32_t totalExtensionCount = sdlExtensionCount + 1;
  const char **extensions = malloc(sizeof(const char *) * totalExtensionCount);
  if (!extensions) {
    ecs_err("Error: Failed to allocate memory for extensions");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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

  VkResult result = vkCreateInstance(&createInfo, NULL, &v_ctx->instance);
  free(extensions);
  if (result != VK_SUCCESS) {
    ecs_err("Error: Failed to create Vulkan instance (VkResult: %d)", result);
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to create Vulkan instance";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // Setup debug messenger
  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(v_ctx->instance, "vkCreateDebugUtilsMessengerEXT");
  if (createDebugUtilsMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    result = createDebugUtilsMessenger(v_ctx->instance, &debugCreateInfo, NULL, &v_ctx->debugMessenger);
    if (result != VK_SUCCESS) {
      ecs_err("Warning: Failed to create debug messenger (VkResult: %d)", result);
      v_ctx->debugMessenger = VK_NULL_HANDLE;
    } else {
      ecs_print(1,"Debug messenger created");
    }
  } else {
    ecs_err("Warning: vkCreateDebugUtilsMessengerEXT not found");
    v_ctx->debugMessenger = VK_NULL_HANDLE;
  }

  ecs_log(1, "Instance setup completed");
}


void SurfaceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"SurfaceSetupSystem started");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) {
  //   ecs_err("Error: ctx is NULL or has error");
  //   return;
  // }
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  if (!sdl_ctx->window) {
    ecs_err("Error: window is NULL");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "SDL window not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  if (!v_ctx->instance) {
    ecs_err("Error: instance is NULL");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Vulkan instance not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  if (!SDL_Vulkan_CreateSurface(sdl_ctx->window, v_ctx->instance, NULL, &sdl_ctx->surface)) {
    ecs_err("Error: Failed to create Vulkan surface - %s", SDL_GetError());
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to create Vulkan surface";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  uint32_t deviceCount = 0;
  VkResult result = vkEnumeratePhysicalDevices(v_ctx->instance, &deviceCount, NULL);
  if (result != VK_SUCCESS) {
    ecs_err("Error: vkEnumeratePhysicalDevices failed (VkResult: %d)", result);
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to enumerate physical devices (first call)";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  ecs_print(1,"vkEnumeratePhysicalDevices");
  
  if (deviceCount == 0) {
    ecs_err("Error: No physical devices found");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "No Vulkan physical devices found";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
  if (!devices) {
    ecs_err("Error: Failed to allocate memory for devices");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  
  result = vkEnumeratePhysicalDevices(v_ctx->instance, &deviceCount, devices);
  if (result != VK_SUCCESS) {
    ecs_err("Error: vkEnumeratePhysicalDevices failed (VkResult: %d)", result);
    free(devices);
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to enumerate physical devices (second call)";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  v_ctx->physicalDevice = devices[0];  // Pick first device for now
  free(devices);
  
  ecs_print(1, "Surface setup completed");
  ecs_log(1, "Surface setup completed");
}


void DeviceSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"DeviceSetupSystem started");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx) {
  //   ecs_err("Error: ctx is NULL");
  //   return;
  // }
  // if (ctx->hasError) {
  //   ecs_err("Error: ctx has error state");
  //   return;
  // }

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  if (v_ctx->physicalDevice == VK_NULL_HANDLE) {
    ecs_err("Error: physicalDevice is NULL");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Physical device not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  if (sdl_ctx->surface == VK_NULL_HANDLE) {
    ecs_err("Error: surface is NULL");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Surface not initialized";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(v_ctx->physicalDevice, &queueFamilyCount, NULL);
  ecs_print(1,"Queue family count: %u", queueFamilyCount);
  if (queueFamilyCount == 0) {
    ecs_err("Error: No queue families found");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "No queue families available";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  if (!queueFamilies) {
    ecs_err("Error: Failed to allocate queueFamilies");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Memory allocation failed";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  vkGetPhysicalDeviceQueueFamilyProperties(v_ctx->physicalDevice, &queueFamilyCount, queueFamilies);

  v_ctx->graphicsFamily = UINT32_MAX;
  v_ctx->presentFamily = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) v_ctx->graphicsFamily = i;
      VkBool32 presentSupport = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(v_ctx->physicalDevice, i, sdl_ctx->surface, &presentSupport);
      if (presentSupport) v_ctx->presentFamily = i;
      if (v_ctx->graphicsFamily != UINT32_MAX && v_ctx->presentFamily != UINT32_MAX) break;
  }
  ecs_print(1,"Graphics family: %u, Present family: %u", v_ctx->graphicsFamily, v_ctx->presentFamily);
  free(queueFamilies);

  if (v_ctx->graphicsFamily == UINT32_MAX || v_ctx->presentFamily == UINT32_MAX) {
    ecs_err("Error: Failed to find required queue families");
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "No graphics or present queue family found";
    ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  VkDeviceQueueCreateInfo queueCreateInfos[2] = {{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}};
  float queuePriority = 1.0f;
  queueCreateInfos[0].queueFamilyIndex = v_ctx->graphicsFamily;
  queueCreateInfos[0].queueCount = 1;
  queueCreateInfos[0].pQueuePriorities = &queuePriority;
  uint32_t queueCreateInfoCount = 1;
  if (v_ctx->graphicsFamily != v_ctx->presentFamily) {
      queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfos[1].queueFamilyIndex = v_ctx->presentFamily;
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
  //ecs_err("vkCreateDevice");
  if (vkCreateDevice(v_ctx->physicalDevice, &deviceCreateInfo, NULL, &v_ctx->device) != VK_SUCCESS) {
      ecs_err("Error: vkCreateDevice failed");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create logical device";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  vkGetDeviceQueue(v_ctx->device, v_ctx->graphicsFamily, 0, &v_ctx->graphicsQueue);
  vkGetDeviceQueue(v_ctx->device, v_ctx->presentFamily, 0, &v_ctx->presentQueue);
  ecs_log(1, "Logical device and queues created successfully");
  ecs_print(1, "Logical device and queues created successfully");
}


void SwapchainSetupSystem(ecs_iter_t *it) {
  ecs_print(1, "SwapchainSetupSystem ");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  // Query surface capabilities
  VkSurfaceCapabilitiesKHR capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(v_ctx->physicalDevice, sdl_ctx->surface, &capabilities) != VK_SUCCESS) {
    sdl_ctx->hasError = true;
    sdl_ctx->errorMessage = "Failed to query surface capabilities";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  ecs_print(1, "Set swapchain extent ");
  // Set swapchain extent
  v_ctx->swapchainExtent = capabilities.currentExtent;
  sdl_ctx->width = v_ctx->swapchainExtent.width;  // Store width
  sdl_ctx->height = v_ctx->swapchainExtent.height; // Store height
  ecs_print(1, "Swapchain extent set - WIDTH: %d, HEIGHT: %d", sdl_ctx->width, sdl_ctx->height);

  ecs_print(1, "Ensure valid dimensions");
  // Ensure valid dimensions (from my suggestion)
  if (sdl_ctx->width == 0 || sdl_ctx->height == 0) {
      ecs_err("Invalid swapchain dimensions: WIDTH=%d, HEIGHT=%d", sdl_ctx->width, sdl_ctx->height);
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Invalid swapchain dimensions";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  ecs_print(1, "imageCount");
  // Adjust image count
  v_ctx->imageCount = 2;  // Desired number
  if (v_ctx->imageCount < capabilities.minImageCount) {
    v_ctx->imageCount = capabilities.minImageCount;
  }
  if (capabilities.maxImageCount > 0 && v_ctx->imageCount > capabilities.maxImageCount) {
    v_ctx->imageCount = capabilities.maxImageCount;
  }
  ecs_print(1, "Query supported formats");
  // Query supported formats
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(v_ctx->physicalDevice, sdl_ctx->surface, &formatCount, NULL);
  VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(v_ctx->physicalDevice, sdl_ctx->surface, &formatCount, formats);
  VkSurfaceFormatKHR selectedFormat = formats[0];
  for (uint32_t i = 0; i < formatCount; i++) {
      if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && 
          formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
          selectedFormat = formats[i];
          break;
      }
  }
  free(formats);
  ecs_print(1, "Query supported present modes");
  // Query supported present modes
  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(v_ctx->physicalDevice, sdl_ctx->surface, &presentModeCount, NULL);
  VkPresentModeKHR* presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(v_ctx->physicalDevice, sdl_ctx->surface, &presentModeCount, presentModes);
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
  swapchainCreateInfo.surface = sdl_ctx->surface;
  swapchainCreateInfo.minImageCount = v_ctx->imageCount;
  swapchainCreateInfo.imageFormat = selectedFormat.format;
  swapchainCreateInfo.imageColorSpace = selectedFormat.colorSpace;
  swapchainCreateInfo.imageExtent = v_ctx->swapchainExtent;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.preTransform = capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCreateInfo.presentMode = selectedPresentMode;
  swapchainCreateInfo.clipped = VK_TRUE;

  ecs_print(1, "vkCreateSwapchainKHR");
  VkResult result = vkCreateSwapchainKHR(v_ctx->device, &swapchainCreateInfo, NULL, &v_ctx->swapchain);
  if (result != VK_SUCCESS) {
      char errorMsg[64];
      ecs_err(errorMsg, "Failed to create swapchain (VkResult: %d)", result);
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = errorMsg;
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  ecs_print(1, "vkGetSwapchainImagesKHR");
  // Get swapchain images
  vkGetSwapchainImagesKHR(v_ctx->device, v_ctx->swapchain, &v_ctx->imageCount, NULL);
  v_ctx->swapchainImages = malloc(sizeof(VkImage) * v_ctx->imageCount);
  vkGetSwapchainImagesKHR(v_ctx->device, v_ctx->swapchain, &v_ctx->imageCount, v_ctx->swapchainImages);

  // Create image views
  v_ctx->swapchainImageViews = malloc(sizeof(VkImageView) * v_ctx->imageCount);
  for (uint32_t i = 0; i < v_ctx->imageCount; i++) {
      VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      viewInfo.image = v_ctx->swapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = selectedFormat.format;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.layerCount = 1;
      if (vkCreateImageView(v_ctx->device, &viewInfo, NULL, &v_ctx->swapchainImageViews[i]) != VK_SUCCESS) {
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create swapchain image view";
          ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
      }
  }

  ecs_log(1, "Swapchain setup completed with %u images", v_ctx->imageCount);
  ecs_print(1, "Swapchain setup completed with %u images", v_ctx->imageCount);
}


void RenderPassSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"RenderPassSetupSystem");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  
  if (!v_ctx->device) {
      ecs_err("Device is null in RenderPassSetupSystem");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Device is null in RenderPassSetupSystem";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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

  if (vkCreateRenderPass(v_ctx->device, &renderPassInfo, NULL, &v_ctx->renderPass) != VK_SUCCESS) {
      ecs_err("Failed to create render pass");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create render pass";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  //ecs_log(1, "Render pass setup completed");
  ecs_print(1, "Render pass setup completed");
}

void FramebufferSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"FramebufferSetupSystem");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;


  v_ctx->framebuffers = malloc(sizeof(VkFramebuffer) * v_ctx->imageCount);
  for (uint32_t i = 0; i < v_ctx->imageCount; i++) {
      VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      framebufferInfo.renderPass = v_ctx->renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = &v_ctx->swapchainImageViews[i];
      framebufferInfo.width = v_ctx->swapchainExtent.width;
      framebufferInfo.height = v_ctx->swapchainExtent.height;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(v_ctx->device, &framebufferInfo, NULL, &v_ctx->framebuffers[i]) != VK_SUCCESS) {
          ecs_err("Failed to create framebuffer");
          sdl_ctx->hasError = true;
          sdl_ctx->errorMessage = "Failed to create framebuffer";
          ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
      }
  }

  // ecs_log(1, "Framebuffer setup completed");
  ecs_print(1, "Framebuffer setup completed");
}


void CommandPoolSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"CommandPoolSetupSystem");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = v_ctx->graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(v_ctx->device, &poolInfo, NULL, &v_ctx->commandPool) != VK_SUCCESS) {
      ecs_err("Failed to create command pool");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to create command pool";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // ecs_log(1, "Command pool setup completed");
  ecs_print(1, "Command pool setup completed");
}


void CommandBufferSetupSystem(ecs_iter_t *it) {
  ecs_print(1,"CommandBufferSetupSystem");
  // WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;
  // if (!ctx->device || !ctx->commandPool) {
  //     ecs_err("Required resources null in CommandBufferSetupSystem");
  //     ctx->hasError = true;
  //     ctx->errorMessage = "Required resources null in CommandBufferSetupSystem";
  //     ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  // }

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;


  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = v_ctx->commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(v_ctx->device, &cmdAllocInfo, &v_ctx->commandBuffer) != VK_SUCCESS) {
      ecs_err("Failed to allocate command buffer");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Failed to allocate command buffer";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // ecs_log(1, "Command buffer setup completed");
  ecs_print(1, "Command buffer setup completed");
}

// PipelineSetupSystem //from triangle 2d setup

void SyncSetupSystem(ecs_iter_t *it) {
  // WorldContext *ctx = ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;

  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  // ecs_print(1, "SyncSetupSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);

  VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  if (vkCreateSemaphore(v_ctx->device, &semaphoreInfo, NULL, &v_ctx->imageAvailableSemaphore) != VK_SUCCESS) {
      ecs_err("Failed to create image available semaphore");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Semaphore creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  // ecs_print(1, "Image available semaphore created: %p", (void*)ctx->imageAvailableSemaphore);

  if (vkCreateSemaphore(v_ctx->device, &semaphoreInfo, NULL, &v_ctx->renderFinishedSemaphore) != VK_SUCCESS) {
      ecs_err("Failed to create render finished semaphore");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Semaphore creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  // ecs_print(1, "Render finished semaphore created: %p", (void*)ctx->renderFinishedSemaphore);

  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  if (vkCreateFence(v_ctx->device, &fenceInfo, NULL, &v_ctx->inFlightFence) != VK_SUCCESS) {
      ecs_err("Failed to create in-flight fence");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Fence creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }
  // ecs_print(1, "In-flight fence created: %p", (void*)ctx->inFlightFence);

  ecs_print(1, "SyncSetupSystem completed");
}

//=====================================
// RENDER LOOP
//=====================================


void BeginRenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  if (ctx->inFlightFence == VK_NULL_HANDLE) {
    ecs_err("In-flight fence is null, skipping render");
    return;
  }

  vkWaitForFences(ctx->device, 1, &ctx->inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->device, 1, &ctx->inFlightFence);

  VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX,
                                          ctx->imageAvailableSemaphore, VK_NULL_HANDLE, &ctx->imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    ecs_print(1, "Swapchain out of date or suboptimal, triggering recreation");
    ctx->needsSwapchainRecreation = true;
    return;
  } else if (result != VK_SUCCESS) {
    ecs_err("Error: vkAcquireNextImageKHR failed (VkResult: %d)", result);
    ctx->hasError = true;
    ctx->errorMessage = "Failed to acquire next image";
    return;
  }
}


void BeginCMDBufferSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;
  // ecs_print(1,"BeginCMDBufferSystem");

  // ecs_print(1, "BeginCMDBufferSystem starting...");
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

  // ecs_print(1, "BeginCMDBufferSystem completed");
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

  if (ctx->device) {
      vkDeviceWaitIdle(ctx->device);

      // Destroy device-specific objects (excluding triangle-specific resources)
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

  // Destroy instance-specific objects
  if (ctx->instance) {
      if (ctx->debugMessenger != VK_NULL_HANDLE) {
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


// resize window and vulkan destory image
static void cleanupSwapchain(WorldContext *ctx) {
  vkDeviceWaitIdle(ctx->device);

  // Destroy framebuffers
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
    if (ctx->framebuffers[i] != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
      ctx->framebuffers[i] = VK_NULL_HANDLE;
    }
    if (ctx->swapchainImageViews[i] != VK_NULL_HANDLE) {
      vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
      ctx->swapchainImageViews[i] = VK_NULL_HANDLE;
    }
  }
  free(ctx->framebuffers);
  free(ctx->swapchainImageViews);
  free(ctx->swapchainImages);
  vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);

  ctx->framebuffers = NULL;
  ctx->swapchainImageViews = NULL;
  ctx->swapchainImages = NULL;
  ctx->swapchain = VK_NULL_HANDLE;
}


// resize window and vulkan create image
// imgui resize
static void recreateSwapchain(WorldContext *ctx) {
  if (!ctx->device || !ctx->surface) return;

  cleanupSwapchain(ctx);

  // Query surface capabilities with updated window size
  VkSurfaceCapabilitiesKHR capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities) != VK_SUCCESS) {
    ecs_err("Failed to query surface capabilities during resize");
    ctx->hasError = true;
    ctx->errorMessage = "Surface capabilities query failed";
    return;
  }

  // Update swapchain extent based on new window size
  ctx->swapchainExtent.width = ctx->width;
  ctx->swapchainExtent.height = ctx->height;

  // Clamp to min/max extents
  if (ctx->swapchainExtent.width < capabilities.minImageExtent.width)
    ctx->swapchainExtent.width = capabilities.minImageExtent.width;
  if (ctx->swapchainExtent.width > capabilities.maxImageExtent.width)
    ctx->swapchainExtent.width = capabilities.maxImageExtent.width;
  if (ctx->swapchainExtent.height < capabilities.minImageExtent.height)
    ctx->swapchainExtent.height = capabilities.minImageExtent.height;
  if (ctx->swapchainExtent.height > capabilities.maxImageExtent.height)
    ctx->swapchainExtent.height = capabilities.maxImageExtent.height;

  // Recreate swapchain (similar to SwapchainSetupSystem)
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

  if (vkCreateSwapchainKHR(ctx->device, &swapchainCreateInfo, NULL, &ctx->swapchain) != VK_SUCCESS) {
    ecs_err("Failed to recreate swapchain");
    ctx->hasError = true;
    ctx->errorMessage = "Swapchain recreation failed";
    return;
  }

  // Get new swapchain images
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, NULL);
  ctx->swapchainImages = malloc(sizeof(VkImage) * ctx->imageCount);
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, ctx->swapchainImages);

  // Create new image views
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
      ecs_err("Failed to recreate swapchain image view");
      ctx->hasError = true;
      ctx->errorMessage = "Swapchain image view recreation failed";
      return;
    }
  }

  // Recreate framebuffers
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
      ecs_err("Failed to recreate framebuffer");
      ctx->hasError = true;
      ctx->errorMessage = "Framebuffer recreation failed";
      return;
    }
  }

  // Update ImGui display size
  if (ctx->isImGuiInitialized) {
    ImGuiIO* io = igGetIO();
    io->DisplaySize.x = (float)ctx->width;
    io->DisplaySize.y = (float)ctx->height;
  }

  ctx->needsSwapchainRecreation = false;
  ecs_print(1, "Swapchain recreated successfully: %dx%d", ctx->width, ctx->height);
}

// resize window when SDL input handle
void SwapchainRecreationSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  if (ctx->needsSwapchainRecreation) {
    recreateSwapchain(ctx);
  }
}


void vulkan_register_components(ecs_world_t *world){
  //VulkanContext

  ECS_COMPONENT_DEFINE(world, VulkanContext);
}


void flecs_vulkan_module_init(ecs_world_t *world, WorldContext *ctx) {
  ecs_print(1, "init vulkan module");

  vulkan_register_components(world);

  ecs_singleton_set(world, VulkanContext, {0});

  //=====================================
  // VULKAN SETUP
  //=====================================

  // Setup systems (once start up)
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
      .entity = ecs_entity(world, { .name = "SyncSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SyncSetupPhase)) }),
      .callback = SyncSetupSystem
  });

  //=====================================
  // VULKAN LOOP
  //=====================================

  // Runtime systems

  // ecs_system_init(world, &(ecs_system_desc_t){
  //   .entity = ecs_entity(world, { .name = "SwapchainRecreationSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginRenderPhase)) }),
  //   .callback = SwapchainRecreationSystem
  // });

  // ecs_system_init(world, &(ecs_system_desc_t){
  //     .entity = ecs_entity(world, { .name = "BeginRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginRenderPhase)) }),
  //     .callback = BeginRenderSystem
  // });

  // ecs_system_init(world, &(ecs_system_desc_t){
  //     .entity = ecs_entity(world, { .name = "BeginCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginCMDBufferPhase)) }),
  //     .callback = BeginCMDBufferSystem
  // });
  
  // //end cmd buffer
  // ecs_system_init(world, &(ecs_system_desc_t){
  //   .entity = ecs_entity(world, { .name = "EndCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndCMDBufferPhase)) }),
  //   .callback = EndCMDBufferSystem
  // });

  // ecs_system_init(world, &(ecs_system_desc_t){
  //     .entity = ecs_entity(world, { .name = "EndRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndRenderPhase)) }),
  //     .callback = EndRenderSystem
  // });
}

