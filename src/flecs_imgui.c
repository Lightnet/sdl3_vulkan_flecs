#include "flecs_imgui.h"
#include <stdio.h>
#include "flecs_sdl.h"
#include "flecs_vulkan.h"
#include "flecs_sdl.h"

// note bug input? reason pass to sdl input component
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
  // WorldContext *ctx = ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world,SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError) return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;

  ecs_log(1, "Initialize ImGui context");
  imgui_ctx->imguiContext = igCreateContext(NULL);
  ImGuiIO* io = igGetIO();
  io->DisplaySize.x = (float)sdl_ctx->width;
  io->DisplaySize.y = (float)sdl_ctx->height;

  // ecs_print(1, "ImGui_ImplSDL3_InitForVulkan");
  if (!ImGui_ImplSDL3_InitForVulkan(sdl_ctx->window)) {
      ecs_err("Failed to initialize ImGui SDL3 backend");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "ImGui SDL3 init failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  imgui_ctx->imguiDescriptorPool = createImGuiDescriptorPool(v_ctx->device);
  // ecs_print(1, "Initialize Vulkan backend");
  ImGui_ImplVulkan_InitInfo init_info = {0};
  init_info.Instance = v_ctx->instance;
  init_info.PhysicalDevice = v_ctx->physicalDevice;
  init_info.Device = v_ctx->device;
  init_info.QueueFamily = v_ctx->graphicsFamily;
  init_info.Queue = v_ctx->graphicsQueue;
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = imgui_ctx->imguiDescriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = 2;
  init_info.ImageCount = v_ctx->imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.RenderPass = v_ctx->renderPass;
  init_info.Allocator = NULL;
  init_info.CheckVkResultFn = NULL;

  bool vulkanInitSuccess = ImGui_ImplVulkan_Init(&init_info);
  // ecs_print(1, "ImGui Vulkan init result: %d", vulkanInitSuccess);
  if (!vulkanInitSuccess) {
      ecs_err("Failed to initialize ImGui Vulkan backend");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "ImGui Vulkan init failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // ecs_print(1, "Pre-font fence: %p", (void*)ctx->inFlightFence);
  // ecs_print(1, "Context pointer pre-font: %p", (void*)ctx);

  // Use ImGui’s default font creation (like the example)
  // ecs_print(1, "ImGui_ImplVulkan_CreateFontsTexture");
  if (!ImGui_ImplVulkan_CreateFontsTexture()) {
      ecs_err("Failed to create ImGui font texture");
      sdl_ctx->hasError = true;
      sdl_ctx->errorMessage = "Font texture creation failed";
      ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
  }

  // ecs_print(1, "Post-font fence: %p", (void*)ctx->inFlightFence);
  // ecs_print(1, "Context pointer post-font: %p", (void*)ctx);

  imgui_ctx->isImGuiInitialized = true;
  // ecs_print(1, "ImGui setup completed");
}

void ImguiInputSystem(ecs_iter_t *it){
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world,SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  const ECS_SDL_INPUT_T *input = ecs_singleton_get(it->world, ECS_SDL_INPUT_T);
  if(!input)return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;
  if (imgui_ctx->isImGuiInitialized) {
    ImGui_ImplSDL3_ProcessEvent(&input->event);
    // SDL_Event event;
    // while (SDL_PollEvent(&event)) {// does not work. not correct way.
    //   ImGui_ImplSDL3_ProcessEvent(&event);
    // }
    // ImGuiIO* io = igGetIO();
    // ecs_print(1,"imgui resize x:%0.0f, y:%0.0f", io->DisplaySize.x, io->DisplaySize.y);
  }
  // Update ImGui display size ???
  // if (imgui_ctx->isImGuiInitialized) {
  //   ImGuiIO* io = igGetIO();
  //   ecs_print(1,"imgui resize x:%0.0f, y:%0.0f", io->DisplaySize.x, io->DisplaySize.y);
  //   if(sdl_ctx->needsSwapchainRecreation){
  //     ecs_print(1,"imgui resize");
  //     //ImGuiIO* io = igGetIO();
  //     io->DisplaySize.x = (float)sdl_ctx->width;
  //     io->DisplaySize.y = (float)sdl_ctx->height;
  //   }
  // }
}

//base ref build.
void ImGuiCMDBufferSystem(ecs_iter_t *it){
  // WorldContext *ctx = ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;

  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world,SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;

  if (imgui_ctx->isImGuiInitialized) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();

    igBegin("Test Window", NULL, 0);
    if (igButton("Click Me", (ImVec2){0, 0})) {
      ecs_print(1, "Button clicked!");
      // Emit entity event. Note how no component ids are provided.
      ecs_emit(it->world, &(ecs_event_desc_t) {
        .event = Clicked,
        .entity = widget
      });
    }
    igText("TEST Vulkan and ImGui! ");
    
    igEnd();

    igRender();
    ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), v_ctx->commandBuffer, VK_NULL_HANDLE);
  }
}

void ImGuiBeginSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;
  if (!imgui_ctx->isImGuiInitialized)return;

  // ecs_print(1, "ImGuiBeginSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiBeginSystem: %p", (void*)ctx->inFlightFence);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  igNewFrame();

  // ecs_print(1, "ImGuiBeginSystem completed");
}

void ImGuiUpdateSystem(ecs_iter_t *it) {
  // WorldContext *ctx = ecs_get_ctx(it->world);
  // if (!ctx || ctx->hasError) return;
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;
  if (!imgui_ctx->isImGuiInitialized)return;

  // ecs_print(1, "ImGuiUpdateSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiUpdateSystem: %p", (void*)ctx->inFlightFence);

  igBegin("Hello, Vulkan!", NULL, 0);
  igText("This is a Vulkan and ImGui demo!");
  if (igButton("Click Me", (ImVec2){0, 0})) {
    ecs_print(1, "Button clicked! 1");

    ecs_emit(it->world, &(ecs_event_desc_t) {
      .event = Clicked,
      .entity = widget
    });
  }

  if (igButton("Clean Up", (ImVec2){0, 0})) {
    ecs_emit(it->world, &(ecs_event_desc_t) {
      .event = CleanUpEvent,
      .entity = CleanUpModule
    });
  }
  igEnd();

  // ecs_print(1, "ImGuiUpdateSystem completed");
}

void ImGuiEndSystem(ecs_iter_t *it) {
  SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
  if (!sdl_ctx || sdl_ctx->hasError || sdl_ctx->isShutDown) return;
  IMGUIContext *imgui_ctx = ecs_singleton_ensure(it->world,IMGUIContext);
  if (!imgui_ctx) return;
  if (!imgui_ctx->isImGuiInitialized)return;
  VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
  if (!v_ctx) return;

  // ecs_print(1, "ImGuiEndSystem starting...");
  // ecs_print(1, "Context pointer: %p", (void*)ctx);
  // ecs_print(1, "Fence in ImGuiEndSystem: %p", (void*)ctx->inFlightFence);

  igRender();
  ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), v_ctx->commandBuffer, VK_NULL_HANDLE);

  // ecs_print(1, "ImGuiEndSystem completed");
}

void OnClick(ecs_iter_t *it){
  ecs_print(1,"Click Event System...");
}

void imgui_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] imgui_cleanup_event_system");
  flecs_imgui_cleanup(it->world);
}

void flecs_imgui_cleanup(ecs_world_t *world) {

  VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
  if (!v_ctx || !v_ctx->device) return;
  IMGUIContext *ctx = ecs_singleton_ensure(world, IMGUIContext);
  if (!ctx) return;

  if (v_ctx->device) {
      ecs_log(1, "Waiting for device idle...");
      vkDeviceWaitIdle(v_ctx->device);

      if (ctx->isImGuiInitialized) {
          ecs_log(1, "Destroying ImGui fonts texture...");
          ImGui_ImplVulkan_DestroyFontsTexture(); // Explicitly destroy fonts
          ecs_log(1, "Shutting down ImGui Vulkan backend...");
          ImGui_ImplVulkan_Shutdown();
          ecs_log(1, "Shutting down ImGui SDL3 backend...");
          ImGui_ImplSDL3_Shutdown();
          ctx->isImGuiInitialized = false;
      }

      if (ctx->imguiContext) {
        ecs_log(1, "Destroying ImGui context...");
          igDestroyContext(ctx->imguiContext);
          ctx->imguiContext = NULL;
      }

      if (ctx->imguiDescriptorPool != VK_NULL_HANDLE) {
        ecs_log(1, "Destroying ImGui descriptor pool...");
          vkDestroyDescriptorPool(v_ctx->device, ctx->imguiDescriptorPool, NULL);
          ctx->imguiDescriptorPool = VK_NULL_HANDLE;
      }

      ecs_log(1, "ImGui cleanup completed");
  }
}

void imgui_register_components(ecs_world_t *world){
  ECS_COMPONENT_DEFINE(world, IMGUIContext);
}

void imgui_register_systems(ecs_world_t *world){
  // Create an entity observer
  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = widget }},
    .events = { Clicked },
    .callback = imgui_cleanup_event_system
  });

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpGraphic }},
    .events = { CleanUpGraphicEvent },
    .callback = imgui_cleanup_event_system
  });

  // ecs_print(1, "ImGuiSetupSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { 
          .name = "ImGui_SetupSystem", 
          .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) 
      }),
      .callback = ImGuiSetupSystem
  });

  // ecs_print(1, "ImGuiCMDBufferSystem");
  // ecs_system_init(world, &(ecs_system_desc_t){
  //     .entity = ecs_entity(world, { .name = "ImGuiCMDBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
  //     .callback = ImGuiCMDBufferSystem
  // });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "ImguiInputSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.LogicUpdatePhase)) }),
      .callback = ImguiInputSystem
  });
  // ecs_print(1, "ImGuiBeginSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "ImGuiBeginSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
      .callback = ImGuiBeginSystem
  });
  // ecs_print(1, "ImGuiUpdateSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "ImGuiUpdateSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBuffer1Phase)) }),
      .callback = ImGuiUpdateSystem
  });
  // ecs_print(1, "ImGuiEndSystem");
  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "ImGuiEndSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBuffer2Phase)) }),
      .callback = ImGuiEndSystem
  });
}

void flecs_imgui_module_init(ecs_world_t *world) {
  ecs_log(1, "Initializing ImGui module...");

  imgui_register_components(world);

  ecs_singleton_set(world, IMGUIContext, {0});

  // Create entity
  Clicked = ecs_new(world);
  widget = ecs_entity(world, { .name = "widget" });

  // add_module_name(world,"imgui_module");
  ecs_entity_t e = ecs_new(world);
  ecs_set(world, e, PluginModule, { .name = "imgui_module", .isCleanUp = false });

  imgui_register_systems(world);

  ecs_log(1, "ImGui module initialized");
}