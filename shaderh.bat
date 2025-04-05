@echo off 

"C:\VulkanSDK\1.4.304.1\Bin\glslangValidator.exe" -V --vn vert_spv shaders/shader.vert -o shaders/vert.spv.h
"C:\VulkanSDK\1.4.304.1\Bin\glslangValidator.exe" -V --vn frag_spv shaders/shader.frag -o shaders/frag.spv.h