@echo off 
SET VULKAN_VERSION = "1.4.304.1"
"C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe" -V shaders/shader.vert -o shaders/vert.spv
"C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe" -V shaders/shader.frag -o shaders/frag.spv