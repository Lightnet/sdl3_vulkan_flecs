@echo off 
set VULKAN_VERSION=1.4.304.1
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"
%VULKAN_Path% -V shaders/shader.vert -o shaders/vert.spv
%VULKAN_Path% -V shaders/shader.frag -o shaders/frag.spv