@echo off 

"C:\VulkanSDK\1.4.304.1\Bin\glslangValidator.exe" -V shaders/shader.vert -o shaders/vert.spv
"C:\VulkanSDK\1.4.304.1\Bin\glslangValidator.exe" -V shaders/shader.frag -o shaders/frag.spv