
# sdl3_vulkan_flecs
  Project name is simple to understand prototype build test.

# Table of Contents:
 * [License](#license)
 * [Status](#status)
 * [Overview](#overview)
 * [Goals](#goals)
 * [Features](#features)
 * [Requirements](#requirements)
 * [Libraries](#libraries)
 * [Project Structure](#project-structure)
 * [Building](#building)
 * [Running](#running)
 * [Module Design](#module-design)
 * [Vulkan Set Up and Render Flow:](#vulkan-set-up-and-render-flow)
 * [Dev Shader](#dev-shader)
 * [Notes](#notes)
 * [Planned Improvements](#planned-improvements)
 * [Images](#images)
 * [Credits](#credits)
 * [Contributing](#contributing)

# License:

This project is licensed under the MIT License. See LICENSE for details.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

# Status:
 - Work in progress
 - Added feautures to get vulkan working for simple things to set up and render.
 - No entity added yet need to check set up and render.

# Overview:

This project is a testbed for building a modular 3D rendering system using Vulkan, SDL3, and the Flecs ECS framework. 

It currently features a basic triangle renderer with ImGui integration that build into module. 

The aim to expand into a flexible module-based design for 3D world-building experiments. Currently doing experiments using SDL3 and Flecs for modular 3D rendering. Mostly Vulkan to test how to set up world scene.

## Goals:
- Create a modular framework for 3D world-building tests.
- Leverage SDL3 for windowing/input, Vulkan for rendering, and Flecs for entity management.
- Minimize C++ usage, wrapping C libraries where beneficial (e.g., VulkanMemoryAllocator).

# Features:

- Modular Design (WIP):
    - Vulkan module for rendering and setup.
    - ImGui module for UI.
    - Easy addition/removal of modules for debugging and configuration.

- SDL 3.x (added)
    - resize window (event from update resize window)
    - clean up call for SDL
    - component context variable access
    - input system (work in progress)
      - close event for window
      - keyboard (work in progress)
        - 278 keys code range max.
      - mouse
        - button right (added)
        - button left (added)
        - motion state (added)
          - check if mouse is move state bool (added)
          - motion postion (added)
        - wheel state (added)
          - check if the wheel is move state bool (added)
          - x (added)
          - y (added)

- Vulkan Module (added)
    - resize window swap (added)
    - phase setup required for on start setup and run time order phase. (added)
    - clean up (added)
    - debugCallback (added)
    - VK_LAYER_KHRONOS_validation (added)
    - component context variable access
    - set up: (added)
      - InstanceSetupSystem
      - SurfaceSetupSystem
      - DeviceSetupSystem
      - SwapchainSetupSystem
      - RenderPassSetupSystem
      - FramebufferSetupSystem
      - CommandPoolSetupSystem
      - CommandBufferSetupSystem
      - SyncSetupSystem
    - run time render: (added)
      - BeginRenderSystem
      - BeginCMDBufferSystem
      - 'Place Holder Name System' > dependon > [CMDBufferPhase > CMDBuffer2Phase]
      - EndCMDBufferSystem
      - EndRenderSystem

- Simple Triangle: (added)
    - module design
    - component context variable access
    - Setup system
      - Vertex Buffer 
      - VkPipelineShader setup
      - VkPipeline setup
    - buffer system
      - commandBuffer render for triangle 2d
    - resize not added
        
- ImGui Integration: (added)
    - This wrapper from imgui to cimgui for c++ wrapper to c.
    - command buffer imgui system to handle render vulkan. (added)
    - input handler system. (added)
    - component context variable access (added)
    - resize added around ways (not added)

- Freetype Render Text font "Hello World" (added)
  - module for set up and render. (added)
  - component context variable access (added)
  - resize added ?

- Cube 3D Mesh (added)
  - module (added)
  - component context variable access (added)
  - resize (not added)
  
- Texture 2D (added)
  - module (added)
  - component context variable access (added)
  - resize (not added)

- Cube Texture 3D Mesh (added)
  - module (added)
  - component context variable access (added)
  - resize (not added)
        
- Flecs:
    - Custom logging system using Flecs, still under development.
    - add and remove entity not added for vulkan mesh or vertex buffer
    - module setup and render are added for SDL and Vulkan as those main build area.
    - world context variables
        
- Planned Features:
    - network libs researching
    - luajit for entity handle script for off load?
    - physics 3d
        
# Requirements:
- CMake: For building the project.
- Visual Studio 2022: C/C++ development environment.
- Vulkan SDK 1.4.304.1: Required for Vulkan functionality.

## Libraries:

- Included:
    - [SDL 3.2.10](https://github.com/libsdl-org/SDL): Windowing and input.
    - [VulkanHeaders 1.4.304.1](https://github.com/KhronosGroup/Vulkan-Headers): Vulkan API headers.
    - [Flecs 4.0.5](https://github.com/SanderMertens/flecs): Entity Component System.
    - [cimgui](https://github.com/cimgui/cimgui) render graphic user interface.
    - [FreeType 2.13.3](https://github.com/freetype/freetype): Font rendering.
    - [stb](https://github.com/nothings/stb): stb_image.h
    - [mimalloc](https://github.com/microsoft/mimalloc): Memory allocator.
    - [cglm](https://github.com/recp/cglm): Math library.
        
- Planned/Not Yet Added:
    - [VulkanMemoryAllocator 3.2.1](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Memory management.
    - [Assimp](https://github.com/assimp/assimp): Asset importing.
    
- Removed:
    - [Volk 1.4.304](https://github.com/zeux/volk): Replaced with direct Vulkan headers.

# Project Structure:

```text
sdl3_vulkan_flecs/
├── assets/                             # (Planned) Font assets
│   ├── fonts                           # test flecs
│   └── textures                        # test
├── docs/                               # docs
├── examples/                           # Example files
│   ├── flecs_test.c                    # test flecs
│   └── test.c                          # test
├── include/                            # Header files
├──── shaders/                          # Shader source files
│       ├── cube3d_frag.spv.h           # Fragment shader
│       ├── cube3d_vert.spv.h           # Vertex shader
│       ├── cubetexture3d_frag.spv.h    # Fragment shader
│       ├── cubetexture3d_vert.spv.h    # Vertex shader
│       ├── shader2d_frag.spv.h         # Fragment shader
│       ├── shader2d_vert.spvh          # Vertex shader
│       ├── text_frag.spv.h             # Fragment shader
│       ├── text_vert.spv.h             # Vertex shader
│       ├── texture2d_frag.spv.h        # Fragment shader
│       └── texture2d_vert.spv.h        # Vertex shader
│   ├── flecs_cube3d.h                  # cube 3d mesh
│   ├── flecs_cubetexture3d.h           # cube 3d mesh
│   ├── flecs_imgui.h                   # graphic user interface
│   ├── flecs_sdl.h                     # SDL Input module
│   ├── flecs_text.h                    # freetype text font module
│   ├── flecs_texture2d.h               # texture 2d module
│   ├── flecs_triangle2d.h              # triangle 2d module
│   ├── flecs_text.h                    # freetype text font module
│   ├── flecs_types.h                   # Global context for Flecs
│   ├── flecs_utils.h                   # helper for vulkan and sdl
│   └── flecs_vulkan.h                  # Vulkan setup and rendering
├── shaders/                            # Shader source files
│       ├── cube3d.frag                 # Fragment shader
│       ├── cube3d.vert                 # Vertex shader
│       ├── cubetexture3d.frag          # Fragment shader
│       ├── cubetexture3d.vert          # Vertex shader
│       ├── shader2d.frag               # Fragment shader
│       ├── shader2d.vert               # Vertex shader
│       ├── text.frag                   # Fragment shader
│       ├── text.vert                   # Vertex shader
│       ├── texture2d.frag              # Fragment shader
│       └── texture2d.vert              # Vertex shader
├── src/                                # Source files
│   ├── flecs_cube3d.c                  # cube 3d module
│   ├── flecs_cubetexture3d.c           # cube 3d texture module
│   ├── flces_imgui.c                   # graphic user interface module
│   ├── flces_sdl.c                     # SDL input module and other add later
│   ├── flces_text.c                    # Freetype font text module
│   ├── flecs_texture2d.c               # texture 2d module
│   ├── flecs_triangle2d.c              # triangle 2d module
│   ├── flecs_types.c                   # Flecs world context implementation
│   ├── flecs_vulkan.c                  # Vulkan module logic
│   └── main.c                          # Entry point
├── CMakeLists.txt                      # CMake build configuration
├── build.bat                           # Build script for VS2022
└── run.bat                             # Run script
```

## Building:

1. Ensure CMake and Visual Studio 2022 are installed.
2. Install the Vulkan SDK 1.4.304.1.
3. Clone the repository:
    
bash
```bash
git clone https://github.com/Lightnet/sdl3_vulkan_flecs.git
cd sdl3_vulkan_flecs
```
    
4. Run the build script (Windows, VS2022):
    
bash
```bash
build.bat
```
  This script handle config and compile application as well check if library for dll exist not to recompile dlls again. It reused dlls to save time compile the applicaiton.

## Running:

- Execute the run script:
    
bash
```bash
run.bat
```
- A window should open displaying a colored triangle and an ImGui "Test Window" with "Hello, Vulkan and ImGui!" text.
- Close the window to exit.

## Module Design:

The project uses a modular approach to simplify development:
- Vulkan Module: Handles initialization, rendering, and cleanup.
- ImGui Module: Manages UI setup and rendering.
- Freetype for render text font.
- Modules can be added or removed for debugging or feature testing.

### Vulkan Set Up and Render Flow:
- Setup (once):
    
    - InstanceSetupSystem -> SurfaceSetupSystem -> DeviceSetupSystem -> SwapchainSetupSystem -> RenderPassSetupSystem -> FramebufferSetupSystem -> CommandPoolSetupSystem -> CommandBufferSetupSystem -> SyncSetupSystem -> SetUpLogicSystem (modules init)

- Runtime (per frame):
    - LogicUpdatePhase: (empty for now)
    - BeginRenderPhase: BeginRenderSystem (acquire image)
    - BeginCMDBufferPhase: BeginCMDBufferSystem (start command buffer and render pass)
    - CMDBufferPhase: TriangleRenderBufferSystem -> TextRenderSystem -> ImGuiRenderSystem
    - EndCMDBufferPhase: EndCMDBufferSystem (end render pass and command buffer)
    - EndRenderPhase: EndRenderSystem (submit and present)

  This is minimal setup for vulkan to run correctly.

## Shaders:
- Shaders are written in GLSL and compiled to SPIR-V using the Vulkan SDK’s glslangValidator.
- Current approach embeds compiled shaders as headers (vert.spv.h, frag.spv.h) for simplicity.
- Source files (shader.vert, shader.frag) are provided for reference.
- note that header files might error due to AI model outdate format from vulkan compiler shaders.

### Dev Shader:
  Note that using the Vulkan SDK tool are easy to compile shader type for header or spv file. This for windows default path for Vulkan SDK. It depend on vulkan version may change.

  Note that space in batch script is sensitive. 

 - shader.bat
 - shaderh.bat
    - This is for shader header file load application instead load from current directory file.

# Felcs API modules:
  Currently world context variable are hard for those module setup. As to test out the vulkan varaible to make sure they are working.

  The plan phase using component gobal access as long there no same varaible name. Althought it required some setup for flecs to assign the ID system to handle struct c.

  I would say global variable. So in flecs logic it should be attach to world. Reason is simple world is entity so is module entity component system.
```c

typedef struct {
  float x, y;
} Gravity;
ECS_COMPONENT_DECLARE(Gravity);
//...
ecs_world_t *world = ecs_init();
ECS_COMPONENT_DEFINE(world, Gravity);
ecs_singleton_set(world, Gravity, {.x=0.0f,.y=0.0f});
//...
```
  This will attach to world as component.

  To edit or update the varaible there are two ways. One is query and other is ecs_singleton_ensure(world, name id)

```c
void TestSystem(ecs_iter_t *it){
Gravity *g = ecs_singleton_ensure(it->world, Gravity);
```
  This will update the variable

  Another way is query. Required some more setup.
```c
void TestSystem(ecs_iter_t *it){
Gravity *g = ecs_field(it, Gravity, 0); // zero index in .query.terms
//...
```
```c
ecs_system(world, {
//...
.query.terms = {
  { ecs_id(Gravity), .src = ecs_id(Gravity) } // Singleton source
},
//...
```
Full example.
```c
#include "flecs.h"
typedef struct {
  float x, y;
} Gravity;
ECS_COMPONENT_DECLARE(Gravity);

// singletons
void TestSystem(ecs_iter_t *it){
  Gravity *g = ecs_field(it, Gravity, 0);
  if(!g)return;
  ecs_print(1,"x: %f", g->x);
  g->x += 1.0f;
}

int main(){
  ecs_world_t *world = ecs_init();
  ECS_COMPONENT_DEFINE(world, Gravity);
  ecs_singleton_set(world, Gravity, {.x=0.0f,.y=0.0f});
  ecs_system_init(world, &(ecs_system_desc_t){
    .entity = ecs_entity(world, { 
        .name = "TestSystem", 
        .add = ecs_ids(ecs_dependson(EcsOnUpdate)) 
    }),
    .query.terms = {
	    // singletons
      { ecs_id(Gravity), .src = ecs_id(Gravity) } // match Gravity on itself
    },
    .callback = TestSystem
  });
  ecs_progress(world, 0);
  ecs_progress(world, 0);
  ecs_progress(world, 0);
  ecs_fini(world);
}
```

# Notes:
- Resize window will error on zero either height or width for vulkan layers.
- Hardcoding:
  - resize for imgui
  - ...
- ImGui elements work in progress for debug 
- C++ Usage: Minimal, primarily for wrapping libraries like VulkanMemoryAllocator to reduce Vulkan boilerplate.
- Logging: Flecs-based logging is a work in progress and may be incomplete.

# cimgui Notes:
- custom phase order render not possible. ex. Imgui1Phase > Imgui2Phase > Imgui3Phase. Reason vulkan layer error and render over lap phases.
- need to add more vulkan phases to handle imgui render setup. With CMDBufferPhase to CMDBuffer2Phase.
- not test full detail yet.

# Planned Improvements:
- Replace hardcoded elements with dynamic systems.
- Add VulkanMemoryAllocator for better memory management.

# Images:

![Triangle Render](screenshots/image01.png)
![Triangle Render](screenshots/image02.png)
![Triangle Render](screenshots/image03.png)

# Credits:
- [Vulkan Tutorial](https://vulkan-tutorial.com): Inspiration and guidance for Vulkan setup.
- [Kenney Fonts](https://kenney.nl/assets/kenney-fonts): Potential font assets (not yet integrated).
- https://x.com/i/grok (free tier, help setup build and readme docs)
- https://bevyengine.org everything into components. Base on how module for window setup idea. 

