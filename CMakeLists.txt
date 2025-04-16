cmake_minimum_required(VERSION 3.14)
project(VulkanTriangle C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Set all outputs to the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Download CPM.cmake v0.40.7
file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.7/CPM.cmake"
    "${CMAKE_BINARY_DIR}/CPM_0.40.7.cmake"
)
include(${CMAKE_BINARY_DIR}/CPM_0.40.7.cmake)

# SDL3 DLL handling for Windows
if(WIN32)
  set(SDL3_DLL_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/SDL3.dll")
  set(SDL3_LIB_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/SDL3.lib")
  set(SDL3_INCLUDE_DIR "${SDL3_SOURCE_DIR}/include")

  message(status "SDL 3.x DIR: >> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/SDL3.dll")
  message(status "SDL 3.x INCLUDE: >> ${SDL3_SOURCE_DIR}/include")

  # Check if SDL3.dll exists and include path is valid
  if(EXISTS "${SDL3_DLL_DEST}")
    message(status " FOUND >>> SDL 3.x ")
    add_library(SDL3::SDL3 SHARED IMPORTED)
    set_target_properties(SDL3::SDL3 PROPERTIES
        IMPORTED_LOCATION "${SDL3_DLL_DEST}"
        IMPORTED_IMPLIB "${SDL3_LIB_DEST}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL3_INCLUDE_DIR}"
    )
  endif()
  if(NOT EXISTS "${SDL3_INCLUDE_DIR}")
    message(status " NOT FOUND >>> SDL 3.x ")
    unset(SDL3_ADDED CACHE)  # Force CPM to re-evaluate SDL3
    # Add dependencies with CPM
    CPMAddPackage(
      NAME SDL3
      GITHUB_REPOSITORY libsdl-org/SDL
      GIT_TAG release-3.2.10
      FORCE TRUE
    )
  endif()
endif()

# CPMAddPackage(
#     NAME VMA
#     GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
#     GIT_TAG v3.2.1
# )

# CPMAddPackage(
#     NAME volk
#     GITHUB_REPOSITORY zeux/volk
#     GIT_TAG 1.4.304
# )

CPMAddPackage(
    NAME VulkanHeaders
    GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
    GIT_TAG vulkan-sdk-1.4.304.1
)

if(WIN32)
  set(flecs_DLL_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/flecs.dll")
  set(flecs_LIB_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/flecs.lib")
  set(flecs_INCLUDE_DIR "${flecs_SOURCE_DIR}/include")

  # flecs
  if(EXISTS "${flecs_DLL_DEST}")
    message(status " FOUND >>> flecs 4.x ")
    message(status " flecs include dir >> ${flecs_SOURCE_DIR}/include ")
    add_library(flecs SHARED IMPORTED)
    set_target_properties(flecs PROPERTIES
        IMPORTED_LOCATION "${flecs_DLL_DEST}"
        IMPORTED_IMPLIB "${flecs_LIB_DEST}"
        INTERFACE_INCLUDE_DIRECTORIES "${flecs_INCLUDE_DIR}"
    )
  endif()
  if(NOT EXISTS "${flecs_DLL_DEST}")
    message(status " NOT FOUND >>> flecs 4.x ")
    CPMAddPackage(
        NAME flecs
        GITHUB_REPOSITORY SanderMertens/flecs
        GIT_TAG v4.0.5
        OPTIONS
            "FLECS_STATIC OFF"
            "FLECS_SHARED ON"
    )
  endif()
endif()

CPMAddPackage(
    NAME freetype
    GITHUB_REPOSITORY freetype/freetype
    GIT_TAG VER-2-13-3
    OPTIONS
      "FT_DISABLE_ZLIB TRUE"
      "FT_DISABLE_BZIP2 TRUE"
      "FT_DISABLE_PNG TRUE"
      "FT_DISABLE_HARFBUZZ TRUE"
      "FT_DISABLE_BROTLI TRUE"
)
if(WIN32)
  set(mimalloc_DLL_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/mimalloc-debug.dll")
  set(mimalloc_LIB_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/mimalloc-debug.lib")
  
  if(EXISTS "${mimalloc_DLL_DEST}")
    if(NOT EXISTS "${mimalloc_SOURCE_DIR}")
      set(mimalloc_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/mimalloc-src")
    endif()
    set(mimalloc_INCLUDE_DIR "${mimalloc_SOURCE_DIR}/include")
    message(status " FOUND >>> mimalloc 2.x ")
    message(status " mimalloc include dir >> ${mimalloc_SOURCE_DIR}/include ")
    add_library(mimalloc SHARED IMPORTED)
    set_target_properties(mimalloc PROPERTIES
        IMPORTED_LOCATION "${mimalloc_DLL_DEST}"
        IMPORTED_IMPLIB "${mimalloc_LIB_DEST}"
        INTERFACE_INCLUDE_DIRECTORIES "${mimalloc_SOURCE_DIR}/include"
    )
  endif()
  if(NOT EXISTS "${mimalloc_DLL_DEST}")
    CPMAddPackage(
        NAME mimalloc
        GITHUB_REPOSITORY microsoft/mimalloc
        GIT_TAG v2.2.3
        OPTIONS
          "MI_BUILD_TESTS OFF"
    )
  endif()
endif()

if(WIN32)
  set(cglm_DLL_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/cglm-0.dll")
  set(cglm_LIB_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/cglm.lib")
  
  if(EXISTS "${cglm_DLL_DEST}")
    if(NOT EXISTS "${cglm_SOURCE_DIR}")
      set(cglm_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/cglm-src")
    endif()
    set(cglm_INCLUDE_DIR "${cglm_SOURCE_DIR}/include")
    message(status " FOUND >>> cglm 0.9.6")
    message(status " cglm include dir >> ${cglm_SOURCE_DIR}/include ")
    add_library(cglm SHARED IMPORTED)
    set_target_properties(cglm PROPERTIES
        IMPORTED_LOCATION "${cglm_DLL_DEST}"
        IMPORTED_IMPLIB "${cglm_LIB_DEST}"
        INTERFACE_INCLUDE_DIRECTORIES "${cglm_INCLUDE_DIR}"
    )
  endif()
  if(NOT EXISTS "${mimalloc_DLL_DEST}")
    CPMAddPackage(
      NAME cglm
      GITHUB_REPOSITORY recp/cglm
      GIT_TAG v0.9.6
    )
  endif()
endif()

if(WIN32)
  CPMAddPackage(
    NAME luajit
    GITHUB_REPOSITORY openresty/luajit2
    GIT_TAG v2.1-agentzh
    DOWNLOAD_ONLY TRUE
  )
  set(LUAJIT_DLL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/lua51.dll")
  set(LUAJIT_LIB "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/lua51.lib")
  set(LUAJIT_SRC_DIR "${CMAKE_BINARY_DIR}/_deps/luajit-src")
  set(LUAJIT_BUILD_DIR "${LUAJIT_SRC_DIR}/src")

  if(EXISTS "${LUAJIT_DLL}")
    add_library(luajit_lib SHARED IMPORTED)
    set_target_properties(luajit_lib PROPERTIES
        IMPORTED_LOCATION "${LUAJIT_DLL}"
        IMPORTED_IMPLIB "${LUAJIT_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_SRC_DIR}/src"
    )
  endif()

  if(NOT EXISTS "${LUAJIT_DLL}")
    set(VS_VCVARSALL "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvarsall.bat")
    add_custom_command(
        OUTPUT "${LUAJIT_DLL}" "${LUAJIT_LIB}"
        # COMMAND ${CMAKE_COMMAND} -E chdir "${LUAJIT_BUILD_DIR}" msvcbuild.bat
        
        COMMAND cmd /c "call \"${VS_VCVARSALL}\" x64 && cd /d ${LUAJIT_SRC_DIR}/src && call ${LUAJIT_SRC_DIR}/src/msvcbuild.bat dll"
        DEPENDS ${LUAJIT_SRC_DIR}/src/luajit.c
        COMMAND ${CMAKE_COMMAND} -E copy "${LUAJIT_BUILD_DIR}/lua51.dll" "${LUAJIT_DLL}"
        COMMAND ${CMAKE_COMMAND} -E copy "${LUAJIT_BUILD_DIR}/lua51.lib" "${LUAJIT_LIB}"
        # COMMENT "Building LuaJIT"
        WORKING_DIRECTORY "${LUAJIT_BUILD_DIR}"
        COMMENT "Building LuaJIT shared library with MSVC"
    )
    add_custom_target(BuildLuaJIT DEPENDS "${LUAJIT_DLL}" "${LUAJIT_LIB}")

    add_library(luajit_lib SHARED IMPORTED)

    set_target_properties(luajit_lib 
      PROPERTIES 
      IMPORTED_LOCATION "${LUAJIT_DLL}"
      IMPORTED_IMPLIB "${LUAJIT_LIB}"
      INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_BUILD_DIR}"
    )

    add_dependencies(luajit_lib BuildLuaJIT)

  endif()

endif()

CPMAddPackage(
    NAME cimgui
    GITHUB_REPOSITORY cimgui/cimgui
    GIT_TAG 1.91.9
    DOWNLOAD_ONLY TRUE
)

# Add stb_image using CPM
CPMAddPackage(
    NAME stb_image
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG master
    DOWNLOAD_ONLY YES
)

# Find Vulkan from the system
find_package(Vulkan REQUIRED)

# cimgui library
set(CIMGUI_SRC
    ${cimgui_SOURCE_DIR}/cimgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp
    ${cimgui_SOURCE_DIR}/imgui/misc/freetype/imgui_freetype.cpp
)

add_library(cimgui_lib STATIC ${CIMGUI_SRC})
target_compile_definitions(cimgui_lib PUBLIC
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    -DCIMGUI_USE_VULKAN
    -DCIMGUI_USE_SDL3
    -DCIMGUI_FREETYPE=1
    "-DIMGUI_USER_CONFIG=\"${cimgui_SOURCE_DIR}/cimconfig.h\""
)
target_include_directories(cimgui_lib PUBLIC
    ${cimgui_SOURCE_DIR}
    ${cimgui_SOURCE_DIR}/imgui
    ${cimgui_SOURCE_DIR}/imgui/backends
    ${SDL3_SOURCE_DIR}/include
    ${VulkanHeaders_SOURCE_DIR}/include
)
target_link_libraries(cimgui_lib PUBLIC
    SDL3::SDL3
    Vulkan::Vulkan
    # volk
    freetype
)

if (WIN32)
    target_compile_definitions(cimgui_lib PUBLIC "-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
    target_compile_definitions(cimgui_lib PUBLIC "-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

# Add stb_image using CPM
CPMAddPackage(
    NAME cjson
    GITHUB_REPOSITORY DaveGamble/cJSON
    GIT_TAG v1.7.18
    OPTIONS
      "ENABLE_CJSON_TEST OFF"
)

CPMAddPackage(
    NAME physfs
    GITHUB_REPOSITORY icculus/physfs
    GIT_TAG stable-3.2
)

CPMAddPackage(
    NAME assimp
    GITHUB_REPOSITORY assimp/assimp
    GIT_TAG v5.4.3
)

# Define source and include directories
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

if(NOT EXISTS "${SOURCE_DIR}")
    message(FATAL_ERROR "Source directory '${SOURCE_DIR}' does not exist. Please create it and add your source files.")
endif()

set(SRC_FILES
  ${SOURCE_DIR}/flecs_types.c
  ${SOURCE_DIR}/flecs_sdl.c
  ${SOURCE_DIR}/flecs_vulkan.c
  ${SOURCE_DIR}/flecs_imgui.c
  ${SOURCE_DIR}/flecs_text.c
  ${SOURCE_DIR}/flecs_texture2d.c
  ${SOURCE_DIR}/flecs_cube3d.c
  ${SOURCE_DIR}/flecs_triangle2d.c
  ${SOURCE_DIR}/flecs_cubetexture3d.c
  ${SOURCE_DIR}/flecs_luajit.c
  ${SOURCE_DIR}/flecs_assimp.c
  ${SOURCE_DIR}/flecs_assets3d.c
)

# Define the executable with all source files
add_executable(${PROJECT_NAME}
  ${SOURCE_DIR}/main.c
  ${SRC_FILES}
  # ${CMAKE_SOURCE_DIR}/examples/test.c
  # ${CMAKE_SOURCE_DIR}/examples/flecs_test.c
  # ${CMAKE_SOURCE_DIR}/examples/lua_test.c
  # ${CMAKE_SOURCE_DIR}/examples/json_data.c
  # ${CMAKE_SOURCE_DIR}/examples/physfs_test.c
)
# raylib config
target_compile_definitions(${PROJECT_NAME} PUBLIC
    -DCIMGUI_DEFINE_ENUMS_AND_STRUCTS
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    Vulkan::Vulkan
    flecs
    freetype
    # cimgui
    cimgui_lib
    cglm
    mimalloc
    luajit_lib
    cjson
    physfs
    assimp::assimp
)
message(STATUS "cjson_SOURCE_DIR >>> ${cjson_SOURCE_DIR}")
# Include directories with proper path validation
target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    ${SDL3_SOURCE_DIR}/include
    # ${VMA_SOURCE_DIR}/include
    ${cimgui_SOURCE_DIR}
    ${cimgui_SOURCE_DIR}/imgui
    ${cimgui_SOURCE_DIR}/imgui/backends
    ${VulkanHeaders_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${stb_image_SOURCE_DIR}
    ${cjson_SOURCE_DIR}
    ${physfs_SOURCE_DIR}/src

    ${cglm_SOURCE_DIR}/include
    ${mimalloc_SOURCE_DIR}/include
)

# # Shader handling
# set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
# set(SHADER_DEST_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/shaders)

# if(NOT EXISTS ${SHADER_SRC_DIR})
#     message(FATAL_ERROR "Shaders directory '${SHADER_SRC_DIR}' does not exist. Please create it and add 'tri.vert' and 'tri.frag' files.")
# endif()

# # Find glslc shader compiler
# find_program(GLSLC_EXECUTABLE glslc HINTS "C:/VulkanSDK/1.4.304.1/Bin" "$ENV{VULKAN_SDK}/Bin")
# if(NOT GLSLC_EXECUTABLE)
#     message(FATAL_ERROR "glslc not found. Ensure Vulkan SDK is installed at C:/VulkanSDK/1.4.304.1/")
# endif()

# # Define shader files
# set(SHADER_FILES
#     ${SHADER_SRC_DIR}/shader2d.vert
#     ${SHADER_SRC_DIR}/shader2d.frag
#     ${SHADER_SRC_DIR}/text.vert
#     ${SHADER_SRC_DIR}/text.frag
# )

# # Compile shaders
# foreach(SHADER ${SHADER_FILES})
#     get_filename_component(SHADER_NAME ${SHADER} NAME)
#     set(SHADER_OUTPUT ${SHADER_DEST_DIR}/${SHADER_NAME}.spv)
#     add_custom_command(
#         OUTPUT ${SHADER_OUTPUT}
#         COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DEST_DIR}
#         COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_OUTPUT}
#         DEPENDS ${SHADER}
#         COMMENT "Compiling ${SHADER_NAME} to SPIR-V in ${SHADER_DEST_DIR}"
#     )
#     list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
# endforeach()

# add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
# add_dependencies(${PROJECT_NAME} Shaders)

# # Copy SDL3 DLL only if it doesn't exist
# if(WIN32 AND TARGET SDL3::SDL3-shared)
#     set(SDL3_DLL_DEST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/SDL3.dll")
#     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         $<TARGET_FILE:SDL3::SDL3-shared>
#         ${SDL3_DLL_DEST}
#         COMMENT "Copying SDL3.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG> if needed"
#     )
# endif()

# Copy entire assets folder (including subdirectories) to build output
set(ASSETS_SRC_DIR "${CMAKE_SOURCE_DIR}/assets")  # Use quotes and forward slashes
set(ASSETS_DEST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/assets")

# Check if assets directory exists
if(NOT EXISTS "${ASSETS_SRC_DIR}")
    message(WARNING "Assets directory '${ASSETS_SRC_DIR}' does not exist. Creating empty directory in build output.")
endif()

# Add custom command to copy entire assets directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SRC_DIR}" "${ASSETS_DEST_DIR}"
    COMMENT "Copying assets directory to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/assets"
)