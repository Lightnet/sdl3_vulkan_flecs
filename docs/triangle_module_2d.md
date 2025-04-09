# Triangle2D Module

## Overview

Renders a 2D triangle with colored vertices using Vulkan.

## Understanding the 2D Triangle Mesh Workflow

The 2D triangle mesh in your Vulkan-based ECS system involves three key stages:

1. Setup: Creating the resources (buffers, pipeline) needed to define and render the triangle.
2. Rendering: Using those resources to draw the triangle each frame.
3. Cleanup: Destroying the resources when the application exits.
    

Below, I’ll break this down into a simple explanation, followed by a visual flow and a modular design guide for documentation.

---

## Simplified Explanation

1. Setup Stage
- What Happens: You define a triangle (3 vertices with positions and colors) and set up Vulkan objects to render it.
- Key Components:
    - Vertex Buffer: Stores the triangle’s vertex data (position and color).
    - Index Buffer: Defines how vertices connect to form the triangle (e.g., indices 0, 1, 2).
    - Graphics Pipeline: Configures how Vulkan renders the triangle (shaders, vertex layout, etc.).
- Where: TriangleModuleSetupSystem (runs once during SetupModulePhase).
    
1. Rendering Stage
 - What Happens: Each frame, you tell Vulkan to draw the triangle using the setup resources.
 - Key Actions:
    - Bind the pipeline (how to render).       
    - Bind the vertex and index buffers (what to render).
    - Issue a draw command (execute the render).
- Where: TriangleRenderBufferSystem (runs every frame in CMDBufferPhase).
1. Cleanup Stage
- What Happens: Free all Vulkan resources to avoid leaks when the app closes.
- Key Components:
    - Destroy buffers, memory, pipeline, and layout.
- Where: flecs_triangle2d_cleanup (called at shutdown).

---

# Visual Design Flow

Here’s a visual representation of the workflow, suitable for documentation (imagine this as a flowchart):

```text
[Start]
   |
   v
[SetupModulePhase]
   |--> [TriangleModuleSetupSystem]
   |      |
   |      v
   |    [Create Vertex Buffer]
   |    [Allocate Vertex Memory]
   |    [Fill Vertex Data: 3 vertices]
   |      |
   |      v
   |    [Create Index Buffer]
   |    [Allocate Index Memory]
   |    [Fill Index Data: 0, 1, 2]
   |      |
   |      v
   |    [Create Shaders: vert, frag]
   |    [Setup Pipeline Layout]
   |    [Create Graphics Pipeline]
   |
   v
[Main Loop]
   |
   v
[CMDBufferPhase]
   |--> [TriangleRenderBufferSystem]
   |      |
   |      v
   |    [Bind Graphics Pipeline]
   |    [Bind Vertex Buffer]
   |    [Bind Index Buffer]
   |    [Draw Indexed: 3 vertices]
   |
   v
[Shutdown]
   |
   v
[flecs_triangle2d_cleanup]
   |--> [Destroy Pipeline]
   |--> [Destroy Pipeline Layout]
   |--> [Destroy Vertex Buffer]
   |--> [Free Vertex Memory]
   |--> [Destroy Index Buffer]
   |--> [Free Index Memory]
   |
   v
[End]
```

## Notes for Visual Design:

- Use boxes for systems/functions (e.g., TriangleModuleSetupSystem).
- Use arrows to show the flow from setup to render to cleanup.
- Color-code phases (e.g., green for setup, blue for render, red for cleanup) to make it visually distinct.

---

## How It Works: Step-by-Step Breakdown

Setup (TriangleModuleSetupSystem)
1. Vertex Buffer:
    - Define 3 vertices: bottom (red), top-right (green), top-left (blue).
    - Create a VkBuffer to hold this data.
    - Allocate VkDeviceMemory and copy the vertex data into it.
2. Index Buffer:
    - Define indices (0, 1, 2) to connect the vertices into a triangle.
    - Create a VkBuffer and allocate memory, then copy the indices.
3. Pipeline:
    - Load vertex and fragment shaders (shader2d_vert.spv, shader2d_frag.spv).
    - Define how vertices are interpreted (2D position + RGB color).
    - Set up the rendering pipeline (fill mode, no blending, etc.).
    - Create the VkPipeline and VkPipelineLayout.

Rendering (TriangleRenderBufferSystem)
1. Bind Resources:
    - Bind the triGraphicsPipeline to tell Vulkan how to render.
    - Bind triVertexBuffer and triIndexBuffer to provide the triangle data.
2. Draw:
    - Issue vkCmdDrawIndexed to draw 1 triangle with 3 vertices.

Cleanup (flecs_triangle2d_cleanup)
1. Destroy Everything:
    - Free the pipeline, layout, buffers, and memory in reverse order of creation.

---

# Modular Design for Documentation

For your docs, structure the module design like this to make it easy to build and extend:
1. Module Overview
- Purpose: Renders a simple 2D triangle with colored vertices.
- Dependencies:
    - Vulkan core (flecs_vulkan): Provides VkDevice, VkRenderPass, etc.
    - WorldContext: Stores shared Vulkan handles (e.g., triVertexBuffer).
    - GlobalPhases: Defines execution order (SetupModulePhase, CMDBufferPhase).

1. Components
- Systems:
    - TriangleModuleSetupSystem: One-time setup of buffers and pipeline.
    - TriangleRenderBufferSystem: Per-frame rendering.
- Resources (stored in WorldContext):
    - triVertexBuffer: Vertex data.
    - triVertexBufferMemory: Memory for vertex buffer.
    - triIndexBuffer: Index data.
    - triIndexBufferMemory: Memory for index buffer.
    - triGraphicsPipeline: Rendering pipeline.
    - triPipelineLayout: Pipeline layout.

1. Setup Process
- Phase: SetupModulePhase (runs after Vulkan core setup).
- Steps:
    1. Create and fill vertex buffer.
    2. Create and fill index buffer.
    3. Create shaders and pipeline.
- Error Handling: Aborts on failure with descriptive messages.

1. Rendering Process
- Phase: CMDBufferPhase (runs between BeginCMDBufferSystem and EndCMDBufferSystem).
- Steps:
    1. Bind pipeline and buffers.
    2. Draw the triangle.
- Note: Assumes vkCmdBeginRenderPass has already started (handled by flecs_vulkan).
    
1. Cleanup Process
- Function: flecs_triangle2d_cleanup.
- Steps:
    1. Wait for device idle.
    2. Destroy all resources in reverse order.
1. Extending the Module
- Adding More Triangles:
    - Expand vertices and indices arrays in TriangleModuleSetupSystem.
    - Update vkCmdDrawIndexed call with new index count.
- Dynamic Data:
    - Map/unmap buffers each frame to update vertex positions/colors.
- Custom Shaders:
    - Replace shader2d_vert.spv/frag.spv and adjust pipeline config.

---

# Tips for Building Correctly

1. Phase Order:
    - Ensure SetupModulePhase runs after CommandBufferSetupPhase (for VkCommandBuffer) and before CMDBufferPhase.
    - Check flecs_phases_init in flecs_types.c to confirm this.
2. Resource Lifetime:
    - All Vulkan objects must persist from setup to cleanup, stored in WorldContext.
3. Error Checking:
    - Keep the ecs_abort calls for critical failures, but consider logging warnings for recoverable issues.
4. Testing:
    - Verify the triangle renders by checking the window output (should see a red-green-blue triangle).
    - Use Vulkan validation layers to catch any missed resource cleanup.
    
## Resources
- `triVertexBuffer`: 3 vertices (pos: 2D, color: RGB).
- `triIndexBuffer`: Indices [0, 1, 2].
- `triGraphicsPipeline`: Vulkan pipeline for rendering.

## Usage
Call `flecs_triangle2d_module_init(world, ctx)` after `flecs_vulkan_module_init`.