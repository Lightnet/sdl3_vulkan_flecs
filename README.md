
# sdl3_vulkan_flecs
  Project name is simple to understand prototype build test.

# Table of Contents:
 * [License](#license)
 * [Status](#status)
 * [Overview](#overview)
 * [Goals](#goals)
 * [Features](#features)
 * [Shutdown and Cleanup Procoess](#shutdown-and-cleanup-procoess)
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
 - No entity added yet need to check set up and render.
 - model testing.

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
    - [x] Vulkan module for rendering and setup.
    - [x] ImGui module for UI.
    - [x] Set Up init module
    - [x] Clean up module event
    - [ ] unload and load module
    - [ ] Easy addition/removal of modules for debugging and configuration.

- SDL 3.2.10 (added)
    - [x] resize window (event from update resize window)
    - [x] clean up call for SDL
    - [x] component context variable access
    - [x] clean up and shut down
    - [x] input system (work in progress)
      - [x] close event for window
      - [x] keyboard (work in progress)
        - 278 keys code range max.
      - [x] mouse
        - [x] button right 
        - [x] button left 
        - [x] motion state
          - [x] check if mouse is move state bool 
          - [x] motion postion
        - [x] wheel state 
          - [x] check if the wheel is move state bool 
          - [x] x 
          - [x] y 

- Vulkan Module
    - [ ] resize window swap
    - [x] phase setup required for on start setup and run time order phase.
    - [x] clean up
    - [x] debugCallback
    - [x] VK_LAYER_KHRONOS_validation
    - [x] component context variable access
    - [x] set up:
      - [x] InstanceSetupSystem
      - [x] SurfaceSetupSystem
      - [x] DeviceSetupSystem
      - [x] SwapchainSetupSystem
      - [x] RenderPassSetupSystem
      - [x] FramebufferSetupSystem
      - [x] CommandPoolSetupSystem
      - [x] CommandBufferSetupSystem
      - [x] SyncSetupSystem
    - [x] run time render:
      - [x] BeginRenderSystem
      - [x] BeginCMDBufferSystem
      - 'Place Holder Name System' > dependon > [CMDBufferPhase > CMDBuffer2Phase]
      - [x] EndCMDBufferSystem
      - [x] EndRenderSystem

- [x] Simple Triangle:
    - [x] module design
    - [x] component context variable access
    - [x] Setup system
      - [x] Vertex Buffer 
      - [x] VkPipelineShader setup
      - [x] VkPipeline setup
    - [x] buffer system
      - [x] commandBuffer render for triangle 2d
    - [x] clean up
    - [ ] resize not added
        
- [x] ImGui Integration
    - This wrapper from imgui to cimgui for c++ wrapper to c.
    - [x] command buffer imgui system to handle render vulkan.
    - [x] input handler system.
    - [x] component context variable access
    - [x] imgui will auto resize.
    - [x] clean up

- [x] Freetype Render Text font "Hello World".
  - [x] module for set up and render.
  - [x] component context variable access
  - [x] clean up
  - [ ] resize added


- [x] Cube 3D Mesh 
  - [x] module
  - [x] component context variable access
  - [x] clean up
  - [ ] resize
  
- [x] Texture 2D
  - [x] module
  - [x] component context variable access
  - [x] clean up
  - [ ] resize

- [x] Cube Texture 3D Mesh
  - [x] module
  - [x] component context variable access
  - [x] clean up
  - [ ] resize
        
- [ ] Flecs:
    - [ ] Custom logging system using Flecs, still under development.
    - [ ] add and remove entity not added for vulkan mesh or vertex buffer
    - [ ] module setup and render are added for SDL and Vulkan as those main build area.
    - [ ] world context variables
- [ ] luajit 
    - [x] module
    - [x] clean up
    - [ ] for entity handle script for off load?        
- Planned Features:
    - [ ] network libs researching
    - [ ] physics 3d
        
# Shutdown and Cleanup Process

This outlines the shutdown and cleanup procedure for an SDL3/Vulkan/Flecs application, ensuring orderly resource release during user-initiated close events (e.g., window close) and crash scenarios. The process uses Flecs’ Entity Component System (ECS) with observers, events, and systems to manage plugin modules, Vulkan resources, and SDL, avoiding errors like Vulkan validation layer crashes.

Objectives
- Orderly Shutdown: Clean up resources in the correct sequence to prevent Vulkan errors.
- Skip Rendering: Disable rendering during shutdown to avoid accessing freed resources.
- Module Synchronization: Ensure all plugin modules complete cleanup before proceeding.
- Crash Handling: (Future) Handle crashes by skipping invalid operations and forcing cleanup.
- User Interaction: Process input events (e.g., SDL_QUIT) to trigger shutdown.

Shutdown Phases
1. Input Event Handler:
    - Processes SDL events (e.g., SDL_QUIT from window close).
    - Triggers a ShutdownEvent to initiate cleanup.
    - (Future) Detects crash signals and skips unsafe operations.
2. Shutdown Event:
    - Broadcasts a Flecs event (ShutdownEvent) to flag all plugin modules for cleanup.
    - Disables rendering systems to prevent errors.
3. Clean Up Module Event:
    - Each plugin module (e.g., rendering, input) processes its cleanup (e.g., frees Vulkan resources).
    - Sets a flag (isCleanUp = true) when done.
4. Check Module Completion:
    - Monitors all modules to confirm cleanup completion.
    - Triggers a Clean Up Graphic Event when all modules are done.
5. Clean Up Graphics Event:
    - Destroys Vulkan device and resources (e.g., vkDestroyDevice) after module cleanup.
    - Ensures no Vulkan operations occur post-cleanup.
6. Close Event:
    - Shuts down SDL (e.g., SDL_DestroyWindow, SDL_Quit).
    - Terminates the ECS world (ecs_fini).

Implementation Notes

- Flecs Features:
    - Systems: Handle input, module cleanup, and checks in specific phases (e.g., EcsOnUpdate, EcsOnCleanup).
    - Observers: React to events like ShutdownEvent and ModulesCleanedEvent.
    - Events: Coordinate transitions (e.g., ShutdownEvent → ModulesCleanedEvent).
- Vulkan Safety:
    - Skip rendering during shutdown using a global flag or by disabling systems.
    - Clean up Vulkan resources in order (e.g., pipelines before device).
- Module Sync:
    - Use a counter or singleton to track module cleanup progress, avoiding random order issues.
- Crash Handling:
    - (TBD) Implement crash detection (e.g., signal handlers) to force cleanup while skipping invalid ECS loops.
- User Input:
    - SDL event loop triggers shutdown via user actions (e.g., SDL_QUIT).

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
    - [Assimp](https://github.com/assimp/assimp): Asset importing.
        
- Planned/Not Yet Added:
    - [VulkanMemoryAllocator 3.2.1](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator): Memory management.
    
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
│       ├── assimp_shader3d_frag.spv.h  # Fragment shader
│       ├── assimp_shader3d_vert.spv.h  # Vertex shader
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
│   ├── flecs_assimp.h                  # assimp 3d mesh
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
│       ├── assimp_shader3d_frag.frag   # Fragment shader
│       ├── assimp_shader3d_vert.vert   # Vertex shader
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
│   ├── flecs_assimp.c                  # assimp 3d module
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

