@echo off
setlocal

set VULKAN_VERSION=1.4.313.0
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"

%VULKAN_Path% -V --vn triangle_vert_spv assets/triangle.vert -o include/triangle_vert.h
%VULKAN_Path% -V --vn triangle_frag_spv assets/triangle.frag -o include/triangle_frag.h

@REM %VULKAN_Path% -V imgui.vert -o imgui.vert.spv
@REM %VULKAN_Path% -V imgui.frag -o imgui.frag.spv

@REM %VULKAN_Path% -V assets/shader.vert -o build/vert.spv
@REM %VULKAN_Path% -V assets/shader.frag -o build/frag.spv

endlocal