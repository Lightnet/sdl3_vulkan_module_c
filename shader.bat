@echo off
setlocal

set VULKAN_VERSION=1.4.313.0
set VULKAN_Path=C:\VulkanSDK\%VULKAN_VERSION%\Bin\glslangValidator.exe
echo path "%VULKAN_Path%"

%VULKAN_Path% -V --vn triangle_vert_spv assets/triangle.vert -o include/triangle_vert.h
%VULKAN_Path% -V --vn triangle_frag_spv assets/triangle.frag -o include/triangle_frag.h

%VULKAN_Path% -V assets/minimal.vert -o build/minimal_vert.spv
%VULKAN_Path% -V assets/minimal.frag -o build/minimal_frag.spv

%VULKAN_Path% -V --vn font_vert_spv assets/font.vert -o include/font_vert_spv.h
%VULKAN_Path% -V --vn font_frag_spv assets/font.frag -o include/font_frag_spv.h

@REM %VULKAN_Path% -V assets/font.vert -o build/font_vert.spv
@REM %VULKAN_Path% -V assets/font.frag -o build/font_frag.spv

@REM %VULKAN_Path% -V imgui.vert -o imgui.vert.spv
@REM %VULKAN_Path% -V imgui.frag -o imgui.frag.spv

@REM %VULKAN_Path% -V assets/shader.vert -o build/vert.spv
@REM %VULKAN_Path% -V assets/shader.frag -o build/frag.spv

endlocal