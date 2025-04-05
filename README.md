# sdl3_vulkan_module_c_cpp

# Licenses: MIT

# Required:
 * CMake
 * VS2022 c/c++

# Information:
  
  Simple triangle test with flecs.

  Using the c as base and c++ wrapper for c to handle and help c build api with minimal.

# Goal:
  To create module design for 3D world build test. To build the module for SDL3 and Vulkan module as well some libraries.

# Image:


# Project:
```
fonts
include
- flecs_types.h (global variable in context for flecs)
- flecs_vulkan.h (vulkan set up and render)
- frag.spv.h (shader in header)
- vert.spv.h (shader in header)
shaders
- shader.frag
- shader.vert
src
- main.c
- flecs_types.c
- flecs_vulkan.c
CMakeLists.txt
```

# Features:
 * module design

  Need to add some features.

# Module design:
 To keep thing simple for add or remove module to debug or config correctly to develop application.

# Shader:
  Note that Vulkan SDK has compiler for shader to help build file format. There are couple ways but we working on shader header and shader.spv. It depend on the job.


# Libraries using c++:
 * VulkanMemoryAllocator

  Using the VulkanMemoryAllocator. https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator

  Reason a lot of boilerplate code.


# Libraries:
 * SDL (added)
 * volk (remove)
 * VulkanHeaders (added)
 * VulkanMemoryAllocator (added)
 * mimalloc (not added)
 * freetype (added)
 * cglm (not added)
 * assimp (not added)
 * 

## github:
 * https://github.com/libsdl-org/SDL
 * https://github.com/zeux/volk
 * https://github.com/KhronosGroup/Vulkan-Headers
 * https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
 * https://github.com/microsoft/mimalloc
 * https://github.com/freetype/freetype
 * https://github.com/recp/cglm
 * https://github.com/assimp/assimp
 * 

# Credits:
 * https://vulkan-tutorial.com
 * https://kenney.nl/assets/kenney-fonts 
 * 