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
shaders
src
- main.c
CMakeLists.txt
```

# Features:
 * module design

  Need to add some features.

# Module design:
 To keep thing simple for add or remove module to debug or config correctly to develop application.

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