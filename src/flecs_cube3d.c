#include "flecs_cube3d.h"
#include <flecs.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include "shaders/cube3d_vert.spv.h"
#include "shaders/cube3d_frag.spv.h"
#include "flecs_utils.h" // createShaderModuleH
#include "flecs_vulkan.h"
#include "flecs_sdl.h"

typedef struct {
    float pos[3];    // 3D position
    float color[3];  // RGB color
} CubeVertex;

typedef struct {
    float model[16];
    float view[16];
    float proj[16];
} UniformBufferObject;

// static VkShaderModule createShaderModule(VkDevice device, const uint32_t *code, size_t codeSize) {
//     VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
//     createInfo.codeSize = codeSize;
//     createInfo.pCode = code;
//     VkShaderModule module;
//     if (vkCreateShaderModule(device, &createInfo, NULL, &module) != VK_SUCCESS) {
//         ecs_err("Failed to create shader module");
//         return VK_NULL_HANDLE;
//     }
//     return module;
// }

static void createBuffer(VulkanContext *v_ctx, SDLContext *sdl_ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory) {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(v_ctx->device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        ecs_err("Failed to create buffer");
        sdl_ctx->hasError = true;
        return;
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(v_ctx->device, *buffer, &memReqs);

    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(v_ctx->physicalDevice, &memProps);
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memReqs.size;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memReqs.memoryTypeBits & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(v_ctx->device, &allocInfo, NULL, memory) != VK_SUCCESS) {
        ecs_err("Failed to allocate buffer memory");
        sdl_ctx->hasError = true;
        return;
    }

    vkBindBufferMemory(v_ctx->device, *buffer, *memory, 0);
}

static void updateBuffer(VulkanContext *v_ctx, VkDeviceMemory memory, VkDeviceSize size, void *data) {
    void *mapped;
    vkMapMemory(v_ctx->device, memory, 0, size, 0, &mapped);
    memcpy(mapped, data, (size_t)size);
    vkUnmapMemory(v_ctx->device, memory);
}

static void createUniformBuffer(VulkanContext *v_ctx, SDLContext *sdl_ctx, Cube3DContext *cube_ctx) {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(v_ctx, sdl_ctx, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &cube_ctx->cubeUniformBuffer, &cube_ctx->cubeUniformBufferMemory);
}

void Cube3DSetupSystem(ecs_iter_t *it) {
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError) return;
    Cube3DContext *cube_ctx = ecs_singleton_ensure(it->world, Cube3DContext);
    if (!cube_ctx) return;

    ecs_log(1, "Cube3DSetupSystem starting...");

    // Descriptor Pool
    VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1};
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(v_ctx->device, &poolInfo, NULL, &cube_ctx->cubeDescriptorPool) != VK_SUCCESS) {
        ecs_err("Failed to create cube descriptor pool");
        sdl_ctx->hasError = true;
        return;
    }

    // Descriptor Set Layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    if (vkCreateDescriptorSetLayout(v_ctx->device, &layoutInfo, NULL, &cube_ctx->cubeDescriptorSetLayout) != VK_SUCCESS) {
        ecs_err("Failed to create cube descriptor set layout");
        sdl_ctx->hasError = true;
        return;
    }

    // Allocate Descriptor Set
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = cube_ctx->cubeDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &cube_ctx->cubeDescriptorSetLayout;
    if (vkAllocateDescriptorSets(v_ctx->device, &allocInfo, &cube_ctx->cubeDescriptorSet) != VK_SUCCESS) {
        ecs_err("Failed to allocate cube descriptor set");
        sdl_ctx->hasError = true;
        return;
    }

    // Create Buffers
    CubeVertex vertices[] = {
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 0.0f}}
    };
    uint32_t indices[] = {
        0, 3, 2, 2, 1, 0,
        4, 5, 6, 6, 7, 4,
        1, 2, 6, 6, 5, 1,
        0, 4, 7, 7, 3, 0,
        3, 7, 6, 6, 2, 3,
        0, 1, 5, 5, 4, 0
    };

    createBuffer(v_ctx, sdl_ctx, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &cube_ctx->cubeVertexBuffer, &cube_ctx->cubeVertexBufferMemory);
    createBuffer(v_ctx, sdl_ctx, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                 &cube_ctx->cubeIndexBuffer, &cube_ctx->cubeIndexBufferMemory);
    createUniformBuffer(v_ctx, sdl_ctx, cube_ctx);

    updateBuffer(v_ctx, cube_ctx->cubeVertexBufferMemory, sizeof(vertices), vertices);
    updateBuffer(v_ctx, cube_ctx->cubeIndexBufferMemory, sizeof(indices), indices);

    // Update Descriptor Set
    VkDescriptorBufferInfo bufferInfo = {0};
    bufferInfo.buffer = cube_ctx->cubeUniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = cube_ctx->cubeDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(v_ctx->device, 1, &descriptorWrite, 0, NULL);

    // VkShaderModule vertShaderModule = createShaderModule(v_ctx->device, cube3d_vert_spv, sizeof(cube3d_vert_spv));
    // VkShaderModule fragShaderModule = createShaderModule(v_ctx->device, cube3d_frag_spv, sizeof(cube3d_frag_spv));
    VkShaderModule vertShaderModule = createShaderModuleH(v_ctx->device, cube3d_vert_spv, sizeof(cube3d_vert_spv));
    VkShaderModule fragShaderModule = createShaderModuleH(v_ctx->device, cube3d_frag_spv, sizeof(cube3d_frag_spv));

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", NULL},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", NULL}
    };

    VkVertexInputBindingDescription bindingDesc = {0, sizeof(CubeVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attributeDescs[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, color)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &cube_ctx->cubeDescriptorSetLayout;
    if (vkCreatePipelineLayout(v_ctx->device, &pipelineLayoutInfo, NULL, &cube_ctx->cubePipelineLayout) != VK_SUCCESS) {
        ecs_err("Failed to create cube pipeline layout");
        sdl_ctx->hasError = true;
        return;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = cube_ctx->cubePipelineLayout;
    pipelineInfo.renderPass = v_ctx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(v_ctx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &cube_ctx->cubePipeline) != VK_SUCCESS) {
        ecs_err("Failed to create cube graphics pipeline");
        sdl_ctx->hasError = true;
        return;
    }

    vkDestroyShaderModule(v_ctx->device, fragShaderModule, NULL);
    vkDestroyShaderModule(v_ctx->device, vertShaderModule, NULL);

    ecs_log(1, "Cube3DSetupSystem completed");
}

void Cube3DRenderSystem(ecs_iter_t *it) {
    VulkanContext *v_ctx = ecs_singleton_ensure(it->world, VulkanContext);
    if (!v_ctx) return;
    SDLContext *sdl_ctx = ecs_singleton_ensure(it->world, SDLContext);
    if (!sdl_ctx || sdl_ctx->hasError) return;
    Cube3DContext *cube_ctx = ecs_singleton_ensure(it->world, Cube3DContext);
    if (!cube_ctx) return;

    // Update uniform buffer with rightward spin
    static float angleY = 0.0f;
    angleY += 0.02f;

    UniformBufferObject ubo = {0};

    float cosY = cosf(angleY), sinY = sinf(angleY);
    ubo.model[0] = cosY;
    ubo.model[2] = sinY;
    ubo.model[5] = 1.0f;
    ubo.model[8] = -sinY;
    ubo.model[10] = cosY;
    ubo.model[15] = 1.0f;

    ubo.view[0] = 1.0f;
    ubo.view[5] = 1.0f;
    ubo.view[10] = 1.0f;
    ubo.view[14] = -5.0f;
    ubo.view[15] = 1.0f;

    float aspect = (float)sdl_ctx->width / (float)sdl_ctx->height;
    float fov = 45.0f * 3.14159f / 180.0f;
    float near = 0.1f, far = 100.0f;
    float tanHalfFov = tanf(fov / 2.0f);
    ubo.proj[0] = 1.0f / (aspect * tanHalfFov);
    ubo.proj[5] = -1.0f / tanHalfFov;
    ubo.proj[10] = -(far + near) / (far - near);
    ubo.proj[11] = -1.0f;
    ubo.proj[14] = -(2.0f * far * near) / (far - near);
    ubo.proj[15] = 0.0f;

    updateBuffer(v_ctx, cube_ctx->cubeUniformBufferMemory, sizeof(ubo), &ubo);

    // Render commands
    vkCmdBindPipeline(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cube_ctx->cubePipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(v_ctx->commandBuffer, 0, 1, &cube_ctx->cubeVertexBuffer, offsets);
    vkCmdBindIndexBuffer(v_ctx->commandBuffer, cube_ctx->cubeIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(v_ctx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cube_ctx->cubePipelineLayout, 0, 1, &cube_ctx->cubeDescriptorSet, 0, NULL);

    vkCmdDrawIndexed(v_ctx->commandBuffer, 36, 1, 0, 0, 0);
}

void flecs_cube3d_cleanup(ecs_world_t *world) {
    VulkanContext *v_ctx = ecs_singleton_ensure(world, VulkanContext);
    if (!v_ctx) return;
    Cube3DContext *cube_ctx = ecs_singleton_ensure(world, Cube3DContext);
    if (!cube_ctx) return;

    ecs_log(1, "Cube3D cleanup starting...");
    vkDeviceWaitIdle(v_ctx->device);

    if (cube_ctx->cubePipeline != VK_NULL_HANDLE) vkDestroyPipeline(v_ctx->device, cube_ctx->cubePipeline, NULL);
    if (cube_ctx->cubePipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(v_ctx->device, cube_ctx->cubePipelineLayout, NULL);
    if (cube_ctx->cubeDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(v_ctx->device, cube_ctx->cubeDescriptorSetLayout, NULL);
    if (cube_ctx->cubeDescriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(v_ctx->device, cube_ctx->cubeDescriptorPool, NULL);
    if (cube_ctx->cubeVertexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cube_ctx->cubeVertexBufferMemory, NULL);
    if (cube_ctx->cubeVertexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cube_ctx->cubeVertexBuffer, NULL);
    if (cube_ctx->cubeIndexBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cube_ctx->cubeIndexBufferMemory, NULL);
    if (cube_ctx->cubeIndexBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cube_ctx->cubeIndexBuffer, NULL);
    if (cube_ctx->cubeUniformBufferMemory != VK_NULL_HANDLE) vkFreeMemory(v_ctx->device, cube_ctx->cubeUniformBufferMemory, NULL);
    if (cube_ctx->cubeUniformBuffer != VK_NULL_HANDLE) vkDestroyBuffer(v_ctx->device, cube_ctx->cubeUniformBuffer, NULL);

    ecs_log(1, "Cube3D cleanup completed");
}

void flecs_cube3d_cleanup_event_system(ecs_iter_t *it){
  ecs_print(1,"[cleanup] flecs_cube3d_cleanup_event_system");
}

void cube3d_register_components(ecs_world_t *world) {
    ECS_COMPONENT_DEFINE(world, Cube3DContext);
}

void cube3d_register_systems(ecs_world_t *world){

  ecs_observer(world, {
    // Not interested in any specific component
    .query.terms = {{ EcsAny, .src.id = CleanUpModule }},
    .events = { CleanUpEvent },
    .callback = flecs_cube3d_cleanup_event_system
  });

  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { .name = "Cube3DSetupSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.SetupModulePhase)) }),
    .callback = Cube3DSetupSystem
  });

  ecs_system_init(world, &(ecs_system_desc_t){
      .entity = ecs_entity(world, { .name = "Cube3DRenderSystem", .add = ecs_ids(ecs_dependson(GlobalPhases.CMDBufferPhase)) }),
      .callback = Cube3DRenderSystem
  });
}

void flecs_cube3d_module_init(ecs_world_t *world) {
  ecs_log(1, "Initializing cube3d module...");

  cube3d_register_components(world);

  ecs_singleton_set(world, Cube3DContext, {0});

  cube3d_register_systems(world);

  ecs_log(1, "Cube3d module initialized");
}