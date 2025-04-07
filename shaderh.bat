@echo off 
SetLocal
set VULKAN_VERSION=1.4.304.1
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"
%VULKAN_Path% -V --vn vert_spv shaders/shader.vert -o shaders/vert.spv.h
%VULKAN_Path% -V --vn frag_spv shaders/shader.frag -o shaders/frag.spv.h
%VULKAN_Path% -V --vn text_vert_spv shaders/text.vert -o shaders/text_vert.spv.h
%VULKAN_Path% -V --vn text_frag_spv shaders/text.frag -o shaders/text_frag.spv.h
endlocal
