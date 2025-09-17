@echo off
setlocal

set VULKAN_VERSION=1.4.313.0
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"

@REM %VULKAN_Path% -V --vn shader2d_vert_spv imgui.vert -o build/shader2d_vert_spv.h
@REM %VULKAN_Path% -V --vn shader2d_frag_spv imgui.frag -o build/shader2d_frag_spv.h

@REM %VULKAN_Path% -V imgui.vert -o imgui.vert.spv
@REM %VULKAN_Path% -V imgui.frag -o imgui.frag.spv

%VULKAN_Path% -V assets/shader.vert -o build/vert.spv
%VULKAN_Path% -V assets/shader.frag -o build/frag.spv

endlocal