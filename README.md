# sdl3_vulkan_module_c

# License: MIT

# Required:
- cmake to build project.
- vulkan sdk for compile shader.
- internet need to download package.

# Infromation:
  Sample project for SDL 3.2, vulkan and cimgui tests in module to keep it simple.

# Sample:
  Create window and button to close application. There are quad and triangle colors render screen.

# Examples:
  Simple triangle and imgui files in examples folder.

# shader:
  version 450

```
%VULKAN_Path% -V --vn triangle_vert_spv assets/triangle.vert -o include/triangle_vert.h
%VULKAN_Path% -V --vn triangle_frag_spv assets/triangle.frag -o include/triangle_frag.h
```
  Convert to header file.

# Credits:
- Grok https://x.com/i/grok
- https://github.com/cimgui/cimgui
- https://github.com/ocornut/imgui

## Notes:
- There are examples files in github which help rework the builds.