#include "flecs_imgui.h"
#include <stdio.h>

// Helper function to create ImGui descriptor pool
static VkDescriptorPool createImGuiDescriptorPool(VkDevice device) {
  VkDescriptorPoolSize poolSizes[] = {
      { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
      { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
      { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
      { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
  };

  VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.maxSets = 1000;
  poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
  poolInfo.pPoolSizes = poolSizes;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Add this flag

  VkDescriptorPool descriptorPool;
  if (vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create ImGui descriptor pool");
      return VK_NULL_HANDLE;
  }
  return descriptorPool;
}



// Optional: Vulkan result checker
static void check_vk_result(VkResult err) {
  if (err != VK_SUCCESS) {
      ecs_err("Vulkan error: %d", err);
  }
}


void ImGuiSetupSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  ecs_print(1, "Initialize ImGui context");
  ctx->imguiContext = igCreateContext(NULL);
  ImGuiIO* io = igGetIO();
  io->DisplaySize.x = (float)WIDTH;
  io->DisplaySize.y = (float)HEIGHT;

  ecs_print(1, "ImGui_ImplSDL3_InitForVulkan");
  if (!ImGui_ImplSDL3_InitForVulkan(ctx->window)) {
      ecs_err("Failed to initialize ImGui SDL3 backend");
      ctx->hasError = true;
      ctx->errorMessage = "ImGui SDL3 init failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ctx->descriptorPool = createImGuiDescriptorPool(ctx->device);
  ecs_print(1, "Initialize Vulkan backend");
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = ctx->instance;
  init_info.PhysicalDevice = ctx->physicalDevice;
  init_info.Device = ctx->device;
  init_info.QueueFamily = ctx->graphicsFamily;
  init_info.Queue = ctx->graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = ctx->descriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = ctx->imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.RenderPass = ctx->renderPass;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = NULL; // Add if desired
  if (!ImGui_ImplVulkan_Init(&init_info)) {
      ecs_err("Failed to initialize ImGui Vulkan backend");
      ctx->hasError = true;
      ctx->errorMessage = "ImGui Vulkan init failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ecs_print(1, "ImGui_ImplVulkan_CreateFontsTexture");
  if (!ImGui_ImplVulkan_CreateFontsTexture()) {
      ecs_err("Failed to create ImGui fonts texture");
      ctx->hasError = true;
      ctx->errorMessage = "ImGui font texture creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ctx->isImGuiInitialized = true; // Set flag after successful setup
  ecs_log(1, "ImGui setup completed");
}


void ImGuiBeginSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();
}

void ImGuiUpdateSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    //Example ImGui window
    igBegin("Hello, Vulkan!", NULL, 0);
    igText("This is a test window.");
    igEnd();
}

void ImGuiEndSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    igRender();
}


void flecs_imgui_cleanup(WorldContext *ctx) {
  if (!ctx) return;

  if (ctx->device) {
      vkDeviceWaitIdle(ctx->device);
      if (ctx->isImGuiInitialized) { // Only shutdown if initialized
          ImGui_ImplVulkan_Shutdown();
          ImGui_ImplSDL3_Shutdown();
          ctx->isImGuiInitialized = false; // Reset flag
      }
      if (ctx->imguiContext) {
          igDestroyContext(ctx->imguiContext);
          ctx->imguiContext = NULL;
      }
      if (ctx->descriptorPool != VK_NULL_HANDLE) {
          vkDestroyDescriptorPool(ctx->device, ctx->descriptorPool, NULL);
          ctx->descriptorPool = VK_NULL_HANDLE;
      }
  }
}




void flecs_imgui_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing ImGui module...");
    //ecs_set_ctx(world, ctx, NULL);//no need to readded it from vulkan order.

    ecs_print(1, "ImGuiSetupSystem=============");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "ImGui_SetupSystem", 
            .add = ecs_ids(ecs_dependson(GlobalPhases.SetupLogicPhase)) 
        }),
        .callback = ImGuiSetupSystem
    });

    ecs_print(1, "ImGuiBeginSystem");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "ImGuiBeginSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginGUIPhase)) }),
        .callback = ImGuiBeginSystem
    });
    ecs_print(1, "ImGuiUpdateSystem");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "ImGuiUpdateSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.UpdateGUIPhase)) }),
        .callback = ImGuiUpdateSystem
    });
    ecs_print(1, "ImGuiEndSystem");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "ImGuiEndSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndGUIPhase)) }),
        .callback = ImGuiEndSystem
    });

    ecs_print(1, "ImGui module initialized");
}