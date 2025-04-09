#include "flecs_types.h"
#include "flecs.h"
#include "flecs_triangle2d.h"
#include "shaders/shader2d_vert.spv.h"
#include "shaders/shader2d_frag.spv.h"

static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
    VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.codeSize = codeSize * sizeof(uint32_t); // Size in bytes
    createInfo.pCode = code;
    VkShaderModule module;
    if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
        ecs_err("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    return module;
}

void TriangleModuleSetupSystem(ecs_iter_t *it) {
    ecs_print(1, "TrianglePipelineSetupSystem");
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;
    if (!ctx->device || !ctx->renderPass) {
        ecs_err("Required resources null in TrianglePipelineSetupSystem");
        ctx->hasError = true;
        ctx->errorMessage = "Required resources null in TrianglePipelineSetupSystem";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

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

    if (vkCreateBuffer(ctx->device, &vertexBufferInfo, NULL, &ctx->triVertexBuffer) != VK_SUCCESS) {
        ecs_err("Failed to create triangle vertex buffer");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create triangle vertex buffer";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    VkMemoryRequirements vertexMemRequirements;
    vkGetBufferMemoryRequirements(ctx->device, ctx->triVertexBuffer, &vertexMemRequirements);

    VkMemoryAllocateInfo vertexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    vertexAllocInfo.allocationSize = vertexMemRequirements.size;
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((vertexMemRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            vertexAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(ctx->device, &vertexAllocInfo, NULL, &ctx->triVertexBufferMemory) != VK_SUCCESS) {
        ecs_err("Failed to allocate triangle vertex buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to allocate triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    if (vkBindBufferMemory(ctx->device, ctx->triVertexBuffer, ctx->triVertexBufferMemory, 0) != VK_SUCCESS) {
        ecs_err("Failed to bind triangle vertex buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to bind triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    void* vertexData;
    if (vkMapMemory(ctx->device, ctx->triVertexBufferMemory, 0, vertexBufferSize, 0, &vertexData) != VK_SUCCESS) {
        ecs_err("Failed to map triangle vertex buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to map triangle vertex buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }
    memcpy(vertexData, vertices, (size_t)vertexBufferSize);
    vkUnmapMemory(ctx->device, ctx->triVertexBufferMemory);

    // Index Buffer Setup
    uint32_t indices[] = {0, 1, 2}; // One triangle
    VkDeviceSize indexBufferSize = sizeof(indices);

    VkBufferCreateInfo indexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    indexBufferInfo.size = indexBufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(ctx->device, &indexBufferInfo, NULL, &ctx->triIndexBuffer) != VK_SUCCESS) {
        ecs_err("Failed to create triangle index buffer");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create triangle index buffer";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    VkMemoryRequirements indexMemRequirements;
    vkGetBufferMemoryRequirements(ctx->device, ctx->triIndexBuffer, &indexMemRequirements);

    VkMemoryAllocateInfo indexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    indexAllocInfo.allocationSize = indexMemRequirements.size;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((indexMemRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            indexAllocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(ctx->device, &indexAllocInfo, NULL, &ctx->triIndexBufferMemory) != VK_SUCCESS) {
        ecs_err("Failed to allocate triangle index buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to allocate triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    if (vkBindBufferMemory(ctx->device, ctx->triIndexBuffer, ctx->triIndexBufferMemory, 0) != VK_SUCCESS) {
        ecs_err("Failed to bind triangle index buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to bind triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    void* indexData;
    if (vkMapMemory(ctx->device, ctx->triIndexBufferMemory, 0, indexBufferSize, 0, &indexData) != VK_SUCCESS) {
        ecs_err("Failed to map triangle index buffer memory");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to map triangle index buffer memory";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }
    memcpy(indexData, indices, (size_t)indexBufferSize);
    vkUnmapMemory(ctx->device, ctx->triIndexBufferMemory);

    ecs_log(1, "Triangle buffer setup completed");


    size_t vertSpvSize = sizeof(shader2d_vert_spv) / sizeof(shader2d_vert_spv[0]);
    size_t fragSpvSize = sizeof(shader2d_frag_spv) / sizeof(shader2d_frag_spv[0]);

    VkShaderModule vertShaderModule = createShaderModule(ctx->device, shader2d_vert_spv, vertSpvSize);
    VkShaderModule fragShaderModule = createShaderModule(ctx->device, shader2d_frag_spv, fragSpvSize);
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

    VkViewport viewport = {0.0f, 0.0f, (float)ctx->width, (float)ctx->height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {ctx->width, ctx->height}};
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
    if (vkCreatePipelineLayout(ctx->device, &pipelineLayoutInfo, NULL, &ctx->triPipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create triangle pipeline layout");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create triangle pipeline layout";
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
    pipelineInfo.layout = ctx->triPipelineLayout;
    pipelineInfo.renderPass = ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->triGraphicsPipeline) != VK_SUCCESS) {
        ecs_err("Failed to create triangle graphics pipeline");
        ctx->hasError = true;
        ctx->errorMessage = "Failed to create triangle graphics pipeline";
        ecs_abort(ECS_INTERNAL_ERROR, ctx->errorMessage);
    }

    vkDestroyShaderModule(ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(ctx->device, vertShaderModule, NULL);
}

void TriangleRenderBufferSystem(ecs_iter_t *it) {
    WorldContext *ctx = ecs_get_ctx(it->world);
    if (!ctx || ctx->hasError) return;

    vkCmdBindPipeline(ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->triGraphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(ctx->commandBuffer, 0, 1, &ctx->triVertexBuffer, offsets);
    vkCmdBindIndexBuffer(ctx->commandBuffer, ctx->triIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(ctx->commandBuffer, 3, 1, 0, 0, 0); // 3 indices, 1 instance
}

void flecs_triangle2d_cleanup(WorldContext *ctx) {
    if (!ctx || !ctx->device) return;

    ecs_print(1, "Triangle2D cleanup starting...");
    vkDeviceWaitIdle(ctx->device);

    if (ctx->triVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(ctx->device, ctx->triVertexBuffer, NULL);
        ctx->triVertexBuffer = VK_NULL_HANDLE;
    }
    if (ctx->triVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(ctx->device, ctx->triVertexBufferMemory, NULL);
        ctx->triVertexBufferMemory = VK_NULL_HANDLE;
    }
    if (ctx->triIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(ctx->device, ctx->triIndexBuffer, NULL);
        ctx->triIndexBuffer = VK_NULL_HANDLE;
    }
    if (ctx->triIndexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(ctx->device, ctx->triIndexBufferMemory, NULL);
        ctx->triIndexBufferMemory = VK_NULL_HANDLE;
    }
    if (ctx->triGraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx->device, ctx->triGraphicsPipeline, NULL);
        ctx->triGraphicsPipeline = VK_NULL_HANDLE;
    }
    if (ctx->triPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx->device, ctx->triPipelineLayout, NULL);
        ctx->triPipelineLayout = VK_NULL_HANDLE;
    }

    ecs_print(1, "Triangle2D cleanup completed");
}

void flecs_triangle2d_module_init(ecs_world_t *world, WorldContext *ctx) {
    ecs_print(1, "Initializing triangle2d module...");

    ecs_system_init(world, &(ecs_system_desc_t){
        //.entity = ecs_entity(world, { .name = "TrianglePipelineSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.PipelineSetupPhase)) }),
        .entity = ecs_entity(world, { .name = "TrianglePipelineSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
        .callback = TriangleModuleSetupSystem
    });

    ecs_system_init(world, &(ecs_system_desc_t){
        .entity = ecs_entity(world, { .name = "TriangleRenderBufferSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
        .callback = TriangleRenderBufferSystem
    });

    ecs_print(1, "Triangle2d module initialized");
}