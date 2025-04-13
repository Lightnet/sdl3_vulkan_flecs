#include "flecs_types.h"
#include "flecs.h"
#include "flecs_triangle2d.h"
#include "shaders/shader2d_vert.spv.h"
#include "shaders/shader2d_frag.spv.h"
#include "flecs_utils.h" // createShaderModuleH
#include "flecs_sdl.h"
#include "flecs_vulkan.h"

// static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
//   VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
//   createInfo.codeSize = codeSize;
//   createInfo.pCode = code;
//   VkShaderModule module;
//   if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
//       ecs_err("Failed to create shader module");
//       return VK_NULL_HANDLE;
//   }
//   return module;
// }

void TriangleModuleSetupSystem(ecs_iter_t *it) {
    ecs_log(1, "TrianglePipelineSetupSystem");

    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError) return;
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    TriangleContext *tri_ctx = ecs_singleton_ensure(it->world, TriangleContext);
    if (!tri_ctx) return;

    // Vertex Buffer Setup
    Vertex vertices[] = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Bottom (red)
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // Top-right (green)
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // Top-left (blue)
    };
    VkDeviceSize vertexBufferSize = sizeof(vertices);

    VkBufferCreateInfo vertexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    vertexBufferInfo.size = vertexBufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(v_ctx->device, &vertexBufferInfo, NULL, &tri_ctx->triVertexBuffer) != VK_SUCCESS) {
        ecs_err("Failed to create triangle vertex buffer");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create triangle vertex buffer";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    VkMemoryRequirements vertexMemRequirements;
    vkGetBufferMemoryRequirements(v_ctx->device, tri_ctx->triVertexBuffer, &vertexMemRequirements);

    VkMemoryAllocateInfo vertexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    vertexAllocInfo.allocationSize = vertexMemRequirements.size;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((vertexMemRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            vertexAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(v_ctx->device, &vertexAllocInfo, NULL, &tri_ctx->triVertexBufferMemory) != VK_SUCCESS) {
        ecs_err("Failed to allocate triangle vertex buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to allocate triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    if (vkBindBufferMemory(v_ctx->device, tri_ctx->triVertexBuffer, tri_ctx->triVertexBufferMemory, 0) != VK_SUCCESS) {
        ecs_err("Failed to bind triangle vertex buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to bind triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    void* vertexData;
    if (vkMapMemory(v_ctx->device, tri_ctx->triVertexBufferMemory, 0, vertexBufferSize, 0, &vertexData) != VK_SUCCESS) {
        ecs_err("Failed to map triangle vertex buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to map triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }
    memcpy(vertexData, vertices, (size_t)vertexBufferSize);
    vkUnmapMemory(v_ctx->device, tri_ctx->triVertexBufferMemory);

    // Index Buffer Setup
    uint32_t indices[] = {0, 1, 2}; // One triangle
    VkDeviceSize indexBufferSize = sizeof(indices);

    VkBufferCreateInfo indexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    indexBufferInfo.size = indexBufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(v_ctx->device, &indexBufferInfo, NULL, &tri_ctx->triIndexBuffer) != VK_SUCCESS) {
        ecs_err("Failed to create triangle index buffer");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create triangle index buffer";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    VkMemoryRequirements indexMemRequirements;
    vkGetBufferMemoryRequirements(v_ctx->device, tri_ctx->triIndexBuffer, &indexMemRequirements);

    VkMemoryAllocateInfo indexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    indexAllocInfo.allocationSize = indexMemRequirements.size;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((indexMemRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            indexAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(v_ctx->device, &indexAllocInfo, NULL, &tri_ctx->triIndexBufferMemory) != VK_SUCCESS) {
        ecs_err("Failed to allocate triangle index buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to allocate triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    if (vkBindBufferMemory(v_ctx->device, tri_ctx->triIndexBuffer, tri_ctx->triIndexBufferMemory, 0) != VK_SUCCESS) {
        ecs_err("Failed to bind triangle index buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to bind triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    void* indexData;
    if (vkMapMemory(v_ctx->device, tri_ctx->triIndexBufferMemory, 0, indexBufferSize, 0, &indexData) != VK_SUCCESS) {
        ecs_err("Failed to map triangle index buffer memory");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to map triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }
    memcpy(indexData, indices, (size_t)indexBufferSize);
    vkUnmapMemory(v_ctx->device, tri_ctx->triIndexBufferMemory);

    ecs_log(1, "Triangle buffer setup completed");

    // VkShaderModule vertShaderModule = createShaderModule(v_ctx->device, shader2d_vert_spv, sizeof(shader2d_vert_spv));
    // VkShaderModule fragShaderModule = createShaderModule(v_ctx->device, shader2d_frag_spv, sizeof(shader2d_frag_spv));
    VkShaderModule vertShaderModule = createShaderModuleH(v_ctx->device, shader2d_vert_spv, sizeof(shader2d_vert_spv));
    VkShaderModule fragShaderModule = createShaderModuleH(v_ctx->device, shader2d_frag_spv, sizeof(shader2d_frag_spv));

    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        ecs_err("Failed to create shader modules");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create shader modules";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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

    VkViewport viewport = {0.0f, 0.0f, (float)sdl_ctx->width, (float)sdl_ctx->height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {sdl_ctx->width, sdl_ctx->height}};
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
    if (vkCreatePipelineLayout(v_ctx->device, &pipelineLayoutInfo, NULL, &tri_ctx->triPipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create triangle pipeline layout");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create triangle pipeline layout";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
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
    pipelineInfo.layout = tri_ctx->triPipelineLayout;
    pipelineInfo.renderPass = v_ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(v_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &tri_ctx->triGraphicsPipeline) != VK_SUCCESS) {
        ecs_err("Failed to create triangle graphics pipeline");
        sdl_ctx->hasError = true;
        sdl_ctx->errorMessage = "Failed to create triangle graphics pipeline";
        ecs_abort(ECS_INTERNAL_ERROR, sdl_ctx->errorMessage);
    }

    vkDestroyShaderModule(v_ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(v_ctx->device, vertShaderModule, NULL);
}

void TriangleRenderBufferSystem(ecs_iter_t *it) {
    // WorldContext *ctx = ecs_get_ctx(it->world);
    // if (!ctx || ctx->hasError) return;
    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError) return;
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    TriangleContext *tri_ctx = ecs_singleton_ensure(it->world, TriangleContext);
    if (!tri_ctx) return;

    vkCmdBindPipeline(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tri_ctx->triGraphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(v_ctx->commandBuffer, 0, 1, &tri_ctx->triVertexBuffer, offsets);
    vkCmdBindIndexBuffer(v_ctx->commandBuffer, tri_ctx->triIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(v_ctx->commandBuffer, 3, 1, 0, 0, 0); // 3 indices, 1 instance
}

void triangle2d_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] triangle2d_cleanup_event_system");
}

void flecs_triangle2d_cleanup(ecs_world_t *world) {
    // if (!ctx || !ctx->device) return;
    VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
    if (!v_ctx || !v_ctx->device) return;
    TriangleContext *ctx = ecs_singleton_ensure(world, TriangleContext);
    if (!ctx) return;
    
    ecs_log(1, "Triangle2D cleanup starting...");
    vkDeviceWaitIdle(v_ctx->device);

    if (ctx->triVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(v_ctx->device, ctx->triVertexBuffer, NULL);
        ctx->triVertexBuffer = VK_NULL_HANDLE;
    }
    if (ctx->triVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(v_ctx->device, ctx->triVertexBufferMemory, NULL);
        ctx->triVertexBufferMemory = VK_NULL_HANDLE;
    }
    if (ctx->triIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(v_ctx->device, ctx->triIndexBuffer, NULL);
        ctx->triIndexBuffer = VK_NULL_HANDLE;
    }
    if (ctx->triIndexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(v_ctx->device, ctx->triIndexBufferMemory, NULL);
        ctx->triIndexBufferMemory = VK_NULL_HANDLE;
    }
    if (ctx->triGraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(v_ctx->device, ctx->triGraphicsPipeline, NULL);
        ctx->triGraphicsPipeline = VK_NULL_HANDLE;
    }
    if (ctx->triPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(v_ctx->device, ctx->triPipelineLayout, NULL);
        ctx->triPipelineLayout = VK_NULL_HANDLE;
    }

    ecs_log(1, "Triangle2D cleanup completed");
}

void triangle2d_register_components(ecs_world_t *world){
  ECS_COMPONENT_DEFINE(world, TriangleContext);
}

void triangle2d_register_systems(ecs_world_t *world){

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = triangle2d_cleanup_event_system
  });

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "TrianglePipelineSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
    .callback = TriangleModuleSetupSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "TriangleRenderBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
      .callback = TriangleRenderBufferSystem
  });
}

void flecs_triangle2d_module_init(ecs_world_t *world) {
    ecs_log(1, "Initializing triangle2d module...");

    triangle2d_register_components(world);

    ecs_singleton_set(world, TriangleContext, {0});

    triangle2d_register_systems(world);

    ecs_log(1, "Triangle2d module initialized");
}