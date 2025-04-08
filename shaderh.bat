@echo off 
SetLocal
set VULKAN_VERSION=1.4.304.1
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"
%VULKAN_Path% -V --vn shader2d_vert_spv shaders/shader2d.vert -o include/shaders/shader2d_vert.spv.h
%VULKAN_Path% -V --vn shader2d_frag_spv shaders/shader2d.frag -o include/shaders/shader2d_frag.spv.h
%VULKAN_Path% -V --vn text_vert_spv shaders/text.vert -o include/shaders/text_vert.spv.h
%VULKAN_Path% -V --vn text_frag_spv shaders/text.frag -o include/shaders/text_frag.spv.h
%VULKAN_Path% -V --vn texture2d_vert_spv shaders/texture2d.vert -o include/shaders/texture2d_vert.spv.h
%VULKAN_Path% -V --vn texture2d_frag_spv shaders/texture2d.frag -o include/shaders/texture2d_frag.spv.h
endlocal
