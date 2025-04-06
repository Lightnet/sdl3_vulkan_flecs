@echo off 
SET VULKAN_VERSION = "1.4.304.1"
"C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe" -V --vn vert_spv shaders/shader.vert -o shaders/vert.spv.h
"C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe" -V --vn frag_spv shaders/shader.frag -o shaders/frag.spv.h
