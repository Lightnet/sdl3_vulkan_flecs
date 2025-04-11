#include "flecs_imgui.h"
#include <stdio.h>
#include "flecs_sdl.h"

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

  VkDescriptorPool imguiDescriptorPool;
  if (vkCreateDescriptorPool(device, &poolInfo, NULL, &imguiDescriptorPool) != VK_SUCCESS) {
      ecs_err("Failed to create ImGui descriptor pool");
      return VK_NULL_HANDLE;
  }
  return imguiDescriptorPool;
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
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world,SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;

  ecs_print(1, "Initialize ImGui context");
  ctx->imguiContext = igCreateContext(NULL);
  ImGuiIO* io = igGetIO();
  io->DisplaySize.x = (float)sdl_ctx->width;
  io->DisplaySize.y = (float)sdl_ctx->height;

  // ecs_print(1, "ImGui_ImplSDL3_InitForVulkan");
  if (!ImGui_ImplSDL3_InitForVulkan(ctx->window)) {
      ecs_err("Failed to initialize ImGui SDL3 backend");
      ctx->hasError = true;
      ctx->errorMessage = "ImGui SDL3 init failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  ctx->imguiDescriptorPool = createImGuiDescriptorPool(ctx->device);
  // ecs_print(1, "Initialize Vulkan backend");
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = ctx->instance;
  init_info.PhysicalDevice = ctx->physicalDevice;
  init_info.Device = ctx->device;
  init_info.QueueFamily = ctx->graphicsFamily;
  init_info.Queue = ctx->graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = ctx->imguiDescriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = ctx->imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.RenderPass = ctx->renderPass;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = NULL;

  bool vulkanInitSuccess = ImGui_ImplVulkan_Init(&init_info);
  // ecs_print(1, "ImGui Vulkan init result: %d", vulkanInitSuccess);
  if (!vulkanInitSuccess) {
      ecs_err("Failed to initialize ImGui Vulkan backend");
      ctx->hasError = true;
      ctx->errorMessage = "ImGui Vulkan init failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // ecs_print(1, "Pre-font fence: %p", (void*)ctx->inFlightFence);
  // ecs_print(1, "Context pointer pre-font: %p", (void*)ctx);

  // Use ImGui’s default font creation (like the example)
  // ecs_print(1, "ImGui_ImplVulkan_CreateFontsTexture");
  if (!ImGui_ImplVulkan_CreateFontsTexture()) {
      ecs_err("Failed to create ImGui font texture");
      ctx->hasError = true;
      ctx->errorMessage = "Font texture creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
  }

  // ecs_print(1, "Post-font fence: %p", (void*)ctx->inFlightFence);
  // ecs_print(1, "Context pointer post-font: %p", (void*)ctx);

  ctx->isImGuiInitialized = true;
  // ecs_print(1, "ImGui setup completed");
}


void ImGuiCMDBufferSystem(ecs_iter_t *it){
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  if (ctx->isImGuiInitialized) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();

    igBegin("Test Window", NULL, 0);
    if (igButton("Click Me", (ImVec2){0, 0})) {
      ecs_print(1, "Button clicked!");
    }
    igText("TEST Vulkan and ImGui! ");
    
    igEnd();

    igRender();
    ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), ctx->commandBuffer, VK_NULL_HANDLE);
  }
}


void ImGuiBeginSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // ecs_print(1, "ImGuiBeginSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiBeginSystem: %p", (void*)ctx->inFlightFence);

  // ImGui_ImplVulkan_NewFrame();
  // ImGui_ImplSDL3_NewFrame();
  // igNewFrame();

  // ecs_print(1, "ImGuiBeginSystem completed");
}


void ImGuiUpdateSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // ecs_print(1, "ImGuiUpdateSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiUpdateSystem: %p", (void*)ctx->inFlightFence);

  // igBegin("Hello, Vulkan!", NULL, 0);
  // igText("This is a Vulkan and ImGui demo!");
  // if (igButton("Click Me", (ImVec2){0, 0})) {
  //   ecs_print(1, "Button clicked!");
  // }
  // igEnd();

  // ecs_print(1, "ImGuiUpdateSystem completed");
}


void ImGuiEndSystem(ecs_iter_t *it) {
  WorldContext *ctx = ecs_get_ctx(it->world);
  if (!ctx || ctx->hasError) return;

  // ecs_print(1, "ImGuiEndSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiEndSystem: %p", (void*)ctx->inFlightFence);

  // igRender();

  // ecs_print(1, "ImGuiEndSystem completed");
}


void flecs_imgui_cleanup(WorldContext *ctx) {
  if (!ctx) return;

  if (ctx->device) {
      ecs_print(1, "Waiting for device idle...");
      vkDeviceWaitIdle(ctx->device);

      if (ctx->isImGuiInitialized) {
          ecs_print(1, "Destroying ImGui fonts texture...");
          ImGui_ImplVulkan_DestroyFontsTexture(); // Explicitly destroy fonts
          ecs_print(1, "Shutting down ImGui Vulkan backend...");
          ImGui_ImplVulkan_Shutdown();
          ecs_print(1, "Shutting down ImGui SDL3 backend...");
          ImGui_ImplSDL3_Shutdown();
          ctx->isImGuiInitialized = false;
      }

      if (ctx->imguiContext) {
          ecs_print(1, "Destroying ImGui context...");
          igDestroyContext(ctx->imguiContext);
          ctx->imguiContext = NULL;
      }

      if (ctx->imguiDescriptorPool != VK_NULL_HANDLE) {
          ecs_print(1, "Destroying ImGui descriptor pool...");
          vkDestroyDescriptorPool(ctx->device, ctx->imguiDescriptorPool, NULL);
          ctx->imguiDescriptorPool = VK_NULL_HANDLE;
      }

      ecs_print(1, "ImGui cleanup completed");
  }
}


void flecs_imgui_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing ImGui module...");
    //ecs_set_ctx(world, ctx, NULL);//no need to readded it from vulkan order.

    // ecs_print(1, "ImGuiSetupSystem");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { 
            .name = "ImGui_SetupSystem", 
            .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) 
        }),
        .callback = ImGuiSetupSystem
    });

    // ecs_print(1, "ImGuiCMDBufferSystem");
    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "ImGuiCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
        .callback = ImGuiCMDBufferSystem
    });

    // // ecs_print(1, "ImGuiBeginSystem");
    // ecs_system_init(world, &(ecs_system_desc_t){
    //     .entity = ecs_entity(world, { .name = "ImGuiBeginSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.BeginGUIPhase)) }),
    //     .callback = ImGuiBeginSystem
    // });
    // // ecs_print(1, "ImGuiUpdateSystem");
    // ecs_system_init(world, &(ecs_system_desc_t){
    //     .entity = ecs_entity(world, { .name = "ImGuiUpdateSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.UpdateGUIPhase)) }),
    //     .callback = ImGuiUpdateSystem
    // });
    // // ecs_print(1, "ImGuiEndSystem");
    // ecs_system_init(world, &(ecs_system_desc_t){
    //     .entity = ecs_entity(world, { .name = "ImGuiEndSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.EndGUIPhase)) }),
    //     .callback = ImGuiEndSystem
    // });

    // ecs_print(1, "ImGui module initialized");
}