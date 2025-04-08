@echo off 
SetLocal
set VULKAN_VERSION=1.4.304.1
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"
%VULKAN_Path% -V --vn vert_spv shaders/shader.vert -o include/vert.spv.h
%VULKAN_Path% -V --vn frag_spv shaders/shader.frag -o include/frag.spv.h
%VULKAN_Path% -V --vn text_vert_spv shaders/text.vert -o include/text_vert.spv.h
%VULKAN_Path% -V --vn text_frag_spv shaders/text.frag -o include/text_frag.spv.h
%VULKAN_Path% -V --vn texture2d_vert_spv shaders/texture2d.vert -o include/texture2d_vert.spv.h
%VULKAN_Path% -V --vn texture2d_frag_spv shaders/texture2d.frag -o include/texture2d_frag.spv.h
endlocal
