# sdl3_vulkan_module_c

# License: MIT

# Status:
- Work in progress build.
- Adding some example to build for sample test.

# Required:
- cmake to build project.
- vulkan sdk for compile shader.
- internet need to download package.

# Infromation:
  Sample project for SDL 3.2, vulkan and cimgui tests in module to keep it simple.

# Sample:
  Create window and button to close application. The button triangle and quad to toggle visible render. 
  
  There are quad and triangle colors render screen.

# Examples:
  Simple triangle and imgui files in examples folder.

  Work on shader with two type of files. One is using the spv and other is header files.

  Added reize window.

# shader:
  version 450

```
%VULKAN_Path% -V --vn triangle_vert_spv assets/triangle.vert -o include/triangle_vert.h
%VULKAN_Path% -V --vn triangle_frag_spv assets/triangle.frag -o include/triangle_frag.h
```
  Convert to header file.

# imgui:
  There are couple of stage to handle imgui.
```
- First setup

```
```
- loop
    //...
    - imgui...
    - igNewFrame (Start draw imgui)
    - create widgets
    - igRender (End draw imgui)
    //...
```
  This handle setup and design or write widgets.

```
   //...
   - vulkan_begin_render
   - render_imgui
   - vulkan_end_render
```
  This handle vulkan render as well render imgui.


# Credits:
- Grok https://x.com/i/grok
- https://github.com/cimgui/cimgui
- https://github.com/ocornut/imgui

## Notes:
- There are examples files in github which help rework the builds.