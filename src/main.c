#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flecs.h"

#define WIDTH 800
#define HEIGHT 600

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

// Add vertex structure
typedef struct {
  float pos[2];
  float color[3];
} Vertex;

static const Vertex vertices[] = {
  {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},  // Top - Red
  {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},   // Bottom Right - Green
  {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}   // Bottom Left - Blue
};

// Simple vertex shader (SPIR-V binary would be used in practice)
const char* vertexShaderCode = 
  "#version 450\n"
  "layout(location = 0) in vec2 inPosition;\n"
  "layout(location = 1) in vec3 inColor;\n"
  "layout(location = 0) out vec3 fragColor;\n"
  "void main() {\n"
  "    gl_Position = vec4(inPosition, 0.0, 1.0);\n"
  "    fragColor = inColor;\n"
  "}\0";

// Simple fragment shader
const char* fragmentShaderCode = 
  "#version 450\n"
  "layout(location = 0) in vec3 fragColor;\n"
  "layout(location = 0) out vec4 outColor;\n"
  "void main() {\n"
  "    outColor = vec4(fragColor, 1.0);\n"
  "}\0";

// Helper function to create shader module (in real app, you'd compile to SPIR-V)
VkShaderModule createShaderModule(VkDevice device, const char* code) {
  // This is a simplification - normally you'd use precompiled SPIR-V
  // For this example, we'll assume a function to convert GLSL to SPIR-V exists
  VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = strlen(code) + 1; // Include null terminator
  createInfo.pCode = (const uint32_t*)code; // This is wrong in practice!
  
  VkShaderModule module;
  vkCreateShaderModule(device, &createInfo, NULL, &module);
  return module;
}


void init_vulkan_system(ecs_iter_t *it) {
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);

  // 1. Vulkan Instance
  Uint32 extensionCount = 0;
  const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
  VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan SDL3";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  instanceCreateInfo.pApplicationInfo = &appInfo;
  instanceCreateInfo.enabledExtensionCount = extensionCount;
  instanceCreateInfo.ppEnabledExtensionNames = extensionNames;

  if (vkCreateInstance(&instanceCreateInfo, NULL, &ctx->instance) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan instance");
      return;
  }

  // 2. Surface
  if (!SDL_Vulkan_CreateSurface(ctx->window, ctx->instance, NULL, &ctx->surface)) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Vulkan surface");
      return;
  }

  // 3. Physical Device and Queue Family
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
  vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);
  ctx->physicalDevice = devices[0];
  free(devices);

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, NULL);
  VkQueueFamilyProperties* queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(ctx->physicalDevice, &queueFamilyCount, queueFamilies);

  uint32_t graphicsFamily = UINT32_MAX;
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
          VkBool32 presentSupport = false;
          vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physicalDevice, i, ctx->surface, &presentSupport);
          if (presentSupport) {
              graphicsFamily = i;
              break;
          }
      }
  }
  free(queueFamilies);

  // 4. Logical Device
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueCreateInfo.queueFamilyIndex = graphicsFamily;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
  deviceCreateInfo.enabledExtensionCount = 1;
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

  if (vkCreateDevice(ctx->physicalDevice, &deviceCreateInfo, NULL, &ctx->device) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create logical device");
      return;
  }
  vkGetDeviceQueue(ctx->device, graphicsFamily, 0, &ctx->graphicsQueue);

  // 5. Swapchain
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physicalDevice, ctx->surface, &capabilities);

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainCreateInfo.surface = ctx->surface;
  swapchainCreateInfo.minImageCount = 2;
  swapchainCreateInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
  swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchainCreateInfo.imageExtent = capabilities.currentExtent;
  swapchainCreateInfo.imageArrayLayers = 1;
  swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCreateInfo.preTransform = capabilities.currentTransform;
  swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchainCreateInfo.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(ctx->device, &swapchainCreateInfo, NULL, &ctx->swapchain) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create swapchain");
      return;
  }

  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, NULL);
  ctx->swapchainImages = malloc(sizeof(VkImage) * ctx->imageCount);
  vkGetSwapchainImagesKHR(ctx->device, ctx->swapchain, &ctx->imageCount, ctx->swapchainImages);

  ctx->swapchainImageViews = malloc(sizeof(VkImageView) * ctx->imageCount);
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
      VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      viewInfo.image = ctx->swapchainImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = VK_FORMAT_B8G8R8A8_SRGB;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.layerCount = 1;
      vkCreateImageView(ctx->device, &viewInfo, NULL, &ctx->swapchainImageViews[i]);
  }

  // 6. Render Pass
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
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create render pass");
      return;
  }

  // 7. Framebuffers
  ctx->swapchainFramebuffers = malloc(sizeof(VkFramebuffer) * ctx->imageCount);
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
      VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
      framebufferInfo.renderPass = ctx->renderPass;
      framebufferInfo.attachmentCount = 1;
      framebufferInfo.pAttachments = &ctx->swapchainImageViews[i];
      framebufferInfo.width = capabilities.currentExtent.width;
      framebufferInfo.height = capabilities.currentExtent.height;
      framebufferInfo.layers = 1;
      vkCreateFramebuffer(ctx->device, &framebufferInfo, NULL, &ctx->swapchainFramebuffers[i]);
  }

  // 8. Command Pool
  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = graphicsFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (vkCreateCommandPool(ctx->device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create command pool");
      return;
  }

  // 9. Command Buffer
  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = ctx->commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(ctx->device, &cmdAllocInfo, &ctx->commandBuffer) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate command buffer");
      return;
  }

  // Create vertex buffer
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = sizeof(vertices);
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateBuffer(ctx->device, &bufferInfo, NULL, &ctx->vertexBuffer) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create vertex buffer");
      return;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(ctx->device, ctx->vertexBuffer, &memRequirements);

  VkMemoryAllocateInfo memAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memAllocInfo.allocationSize = memRequirements.size;
  
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);
  uint32_t memoryTypeIndex = 0;
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((memRequirements.memoryTypeBits & (1 << i)) &&
          (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
          memoryTypeIndex = i;
          break;
      }
  }
  memAllocInfo.memoryTypeIndex = memoryTypeIndex;

  if (vkAllocateMemory(ctx->device, &memAllocInfo, NULL, &ctx->vertexMemory) != VK_SUCCESS) {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate vertex memory");
      return;
  }
  vkBindBufferMemory(ctx->device, ctx->vertexBuffer, ctx->vertexMemory, 0);

  // Copy vertex data
  void* data;
  vkMapMemory(ctx->device, ctx->vertexMemory, 0, sizeof(vertices), 0, &data);
  memcpy(data, vertices, sizeof(vertices));
  vkUnmapMemory(ctx->device, ctx->vertexMemory);

  // Create shader modules
  VkShaderModule vertShaderModule = createShaderModule(ctx->device, vertexShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(ctx->device, fragmentShaderCode);

  // Pipeline
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
  vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->pipelineLayout);

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

  vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->graphicsPipeline);

  // Cleanup shader modules
  vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
  vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);



  // Add synchronization objects
  VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphore);
  vkCreateSemaphore(ctx->device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphore);

  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(ctx->device, &fenceInfo, NULL, &ctx->inFlightFence);


  printf("Vulkan setup completed\n");
}


void RenderSystem(ecs_iter_t *it) {
  WorldContext *ctx = (WorldContext *)ecs_get_ctx(it->world);

  vkWaitForFences(ctx->device, 1, &ctx->inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(ctx->device, 1, &ctx->inFlightFence);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, 
                       ctx->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

  vkResetCommandBuffer(ctx->commandBuffer, 0);
  
  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(ctx->commandBuffer, &beginInfo);

  VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  renderPassInfo.renderPass = ctx->renderPass;
  renderPassInfo.framebuffer = ctx->swapchainFramebuffers[imageIndex];
  renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
  renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT};
  VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColor;

  vkCmdBeginRenderPass(ctx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  
  // Draw triangle
  vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphicsPipeline);
  VkBuffer vertexBuffers[] = {ctx->vertexBuffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, vertexBuffers, offsets);
  vkCmdDraw(ctx->commandBuffer, 3, 1, 0, 0);

  vkCmdEndRenderPass(ctx->commandBuffer);
  vkEndCommandBuffer(ctx->commandBuffer);

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

  vkQueueSubmit(ctx->graphicsQueue, 1, &submitInfo, ctx->inFlightFence);

  VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &ctx->swapchain;
  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR(ctx->graphicsQueue, &presentInfo);
}



void cleanup_vulkan(WorldContext* ctx) {


  vkDestroyPipeline(ctx->device, ctx->graphicsPipeline, NULL);
  vkDestroyPipelineLayout(ctx->device, ctx->pipelineLayout, NULL);
  vkDestroyBuffer(ctx->device, ctx->vertexBuffer, NULL);
  vkFreeMemory(ctx->device, ctx->vertexMemory, NULL);

  vkDestroySemaphore(ctx->device, ctx->renderFinishedSemaphore, NULL);
  vkDestroySemaphore(ctx->device, ctx->imageAvailableSemaphore, NULL);
  vkDestroyFence(ctx->device, ctx->inFlightFence, NULL);

  vkFreeCommandBuffers(ctx->device, ctx->commandPool, 1, &ctx->commandBuffer);
  vkDestroyCommandPool(ctx->device, ctx->commandPool, NULL);
  for (uint32_t i = 0; i < ctx->imageCount; i++) {
      vkDestroyFramebuffer(ctx->device, ctx->swapchainFramebuffers[i], NULL);
      vkDestroyImageView(ctx->device, ctx->swapchainImageViews[i], NULL);
  }
  free(ctx->swapchainFramebuffers);
  free(ctx->swapchainImages);
  free(ctx->swapchainImageViews);
  vkDestroyRenderPass(ctx->device, ctx->renderPass, NULL);
  vkDestroySwapchainKHR(ctx->device, ctx->swapchain, NULL);
  vkDestroyDevice(ctx->device, NULL);
  vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
  vkDestroyInstance(ctx->instance, NULL);
}



// Update main function to include cleanup
int main(int argc, char* argv[]) {
  printf("init main!\n");
  ecs_world_t *world = ecs_init();

  SDL_Init(SDL_INIT_VIDEO);

  SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 Flecs", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
  if (!window) {
      printf("Window creation failed: %s\n", SDL_GetError());
      return 1;
  }

  // Flecs set up
  // Define custom phases
  ecs_entity_t LogicUpdatePhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t BeginRenderPhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t BeginGUIPhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t UpdateGUIPhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t EndGUIPhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t RenderPhase = ecs_new_w_id(world, EcsPhase);
  ecs_entity_t EndRenderPhase = ecs_new_w_id(world, EcsPhase);


  ecs_add_pair(world, LogicUpdatePhase, EcsDependsOn, EcsPreUpdate);
  ecs_add_pair(world, BeginRenderPhase, EcsDependsOn, LogicUpdatePhase);
  ecs_add_pair(world, BeginGUIPhase, EcsDependsOn, BeginRenderPhase);
  ecs_add_pair(world, UpdateGUIPhase, EcsDependsOn, BeginGUIPhase);
  ecs_add_pair(world, EndGUIPhase, EcsDependsOn, UpdateGUIPhase);
  ecs_add_pair(world, RenderPhase, EcsDependsOn, EndGUIPhase);
  ecs_add_pair(world, EndRenderPhase, EcsDependsOn, RenderPhase);


  WorldContext world_ctx = { 
    .window = window,
    .imageCount = 0,
    .physicalDevice = VK_NULL_HANDLE,
    .device = VK_NULL_HANDLE,
    .instance = VK_NULL_HANDLE,
    .surface = VK_NULL_HANDLE,
    .swapchain = VK_NULL_HANDLE,
    .renderPass = VK_NULL_HANDLE,
    .commandPool = VK_NULL_HANDLE,
    .commandBuffer = VK_NULL_HANDLE
  };

  ecs_set_ctx(world, &world_ctx, NULL);

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, {
          .name = "initVulkanSystem",
          .add = ecs_ids(ecs_dependson(EcsOnStart))
      }),
      .callback = init_vulkan_system
  });

  // Render system
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, {
        .name = "RenderSystem",
        .add = ecs_ids(ecs_dependson(EcsPreUpdate))
    }),
    .callback = RenderSystem
  });

  bool running = true;
  SDL_Event event;

  while (running) {
      while (SDL_PollEvent(&event)) {
          if (event.type == SDL_EVENT_QUIT) running = false;
      }
      ecs_progress(world, 1.0f);
  }

  printf("clean up\n");
  cleanup_vulkan(&world_ctx);
  ecs_fini(world);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

