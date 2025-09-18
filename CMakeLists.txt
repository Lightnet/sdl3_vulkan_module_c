cmake_minimum_required(VERSION 3.22)
project(sdl3_cimgui)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set C standard (since you're using .c files)
# set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_STANDARD 99)
# set(CMAKE_C_STANDARD_REQUIRED ON)

# Find Vulkan
# find_package(Vulkan REQUIRED)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Find Vulkan
# find_package(Vulkan REQUIRED)

# Fetch raylib
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.22
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(sdl3)

# Provides the Vulkan::Headers target, which sets up include directories for <vulkan/vulkan.h> and other headers.
FetchContent_Declare(
    vulkanheaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG vulkan-sdk-1.4.313.0
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(vulkanheaders)

# Fetch Vulkan-Loader
# Builds the Vulkan loader library, providing the vulkan target (equivalent to vulkan-1.lib on Windows).
# The vulkan target includes the runtime implementations of Vulkan API functions, resolving the linker errors (e.g., vkCreateInstance, vkCmdBeginRenderPass).
FetchContent_Declare(
    VulkanLoader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG vulkan-sdk-1.4.313.0 # Match the Vulkan-Headers version
)
FetchContent_MakeAvailable(VulkanLoader)

# Fetch cimgui
FetchContent_Declare(
    cimgui
    GIT_REPOSITORY https://github.com/cimgui/cimgui.git
    # GIT_TAG master 
    GIT_TAG docking_inter 
    # GIT_TAG 1.92.1
    EXCLUDE_FROM_ALL #this disable build? since customize for sdl3 and opengl.
    GIT_SHALLOW TRUE
    USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(cimgui)

#================================================
# cimgui custom
#================================================

## Define cimgui sources
set(IMGUI_SOURCES
    ${cimgui_SOURCE_DIR}/cimgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_widgets.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${cimgui_SOURCE_DIR}/imgui/imgui_tables.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp
    ${cimgui_SOURCE_DIR}/imgui/backends/imgui_impl_vulkan.cpp
)

## Build cimgui as a static library with a custom name
add_library(custom_cimgui STATIC ${IMGUI_SOURCES})

## Set include directories for custom_cimgui
target_include_directories(custom_cimgui PUBLIC
    ${cimgui_SOURCE_DIR}                    # cimgui
    ${cimgui_SOURCE_DIR}/imgui              # cimgui/imgui
    ${cimgui_SOURCE_DIR}/imgui/backends     # imgui/backends
    ${SDL3_SOURCE_DIR}/include              # SDL 3.2
)

## Set compile definitions for custom_cimgui (no CIMGUI_DEFINE_ENUMS_AND_STRUCTS)
# target_compile_definitions(custom_cimgui PUBLIC
#     IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
#     IMGUI_IMPL_API=extern\ \"C\"
# )
target_compile_definitions(custom_cimgui PUBLIC
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    CIMGUI_USE_SDL3=1
    CIMGUI_USE_VULKAN=1
    IMGUI_IMPL_API=extern\ \"C\"
    # CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
)

## Link custom_cimgui with GLFW and OpenGL
target_link_libraries(custom_cimgui PUBLIC 
    Vulkan::Headers # Vulkan headers
    vulkan # Vulkan loader library (from Vulkan-Loader)
    SDL3::SDL3
)

#================================================
# APP
#================================================

# Application NAME
set(APP_NAME sdl3_imgui)

# Source files (add more if needed)
set(SRC_FILES
    # src/main.c
    # Add other .c files here if necessary

)

message(STATUS "cimgui_SOURCE_DIR: >> ${cimgui_SOURCE_DIR}")

# Create executable
add_executable(${APP_NAME}
${SRC_FILES}
    # src/vulkan_module.c
    # src/imgui_module.c
    # src/triangle_module.c
    # src/main.c
    # examples/sdl3_vulkan_triangle_imgui_spv.c
    # examples/sdl3_vulkan_triangle_imgui_h.c
    examples/sdl3_vulkan_triangle_imgui_h_resize.c
    # examples/sdl3_vulkan_triangle_h_resize.c
    # examples/sdl3_vulkan_triangle_h_sample.c
    # examples/sdl3_vulkan_triangle_spv_sample.c
    # examples/sdl3_vulkan_triangle_minimal.c
)

# SDL3 
target_link_libraries(${APP_NAME} PUBLIC SDL3::SDL3) # SDL 3.2.22
target_link_libraries(${APP_NAME} PRIVATE custom_cimgui) # custom cimgui

# vulkan libraries
target_link_libraries(${APP_NAME} PRIVATE 
    Vulkan::Headers # Vulkan headers
    vulkan # Vulkan loader library (from Vulkan-Loader)
)

# Include directories
target_include_directories(${APP_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}/include             # root project
    ${cimgui_SOURCE_DIR}                    # cimgui
    ${cimgui_SOURCE_DIR}/imgui              # imgui
    ${cimgui_SOURCE_DIR}/imgui/backends     # imgui/backends
    ${SDL3_SOURCE_DIR}/include              # SDL 3.2.22
)

# for c #define
target_compile_definitions(
	${APP_NAME}
	PUBLIC
    # IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
	CIMGUI_DEFINE_ENUMS_AND_STRUCTS=1
	CIMGUI_USE_SDL3=1
    CIMGUI_USE_VULKAN=1
	# CIMGUI_USE_OPENGL3=1
)

# Platform-specific settings for Windows (MinGW/MSYS2)
if (WIN32)
    # Link necessary Windows libraries for raylib
    target_link_libraries(${APP_NAME} PRIVATE
        # opengl32
        gdi32
        winmm
    )

    # Static linking for libgcc and libstdc++
    target_link_options(${APP_NAME} PRIVATE
        -static-libgcc
        -static-libstdc++  # Uncomment if C++ code is used
        -static           # Avoid full static linking to prevent issues with system libraries
    )
endif()

# Shader compilation
# find_program(GLSLC glslc REQUIRED HINTS ENV VULKAN_SDK PATH_SUFFIXES bin)
# set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/assets)
# set(SHADER_OUT_DIR ${CMAKE_BINARY_DIR}/shaders)
# file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

# add_custom_command(
#     OUTPUT ${SHADER_OUT_DIR}/frag.spv
#     COMMAND ${GLSLC} -fshader-stage=frag ${SHADER_SRC_DIR}/frag.glsl -o ${SHADER_OUT_DIR}/frag.spv
#     DEPENDS ${SHADER_SRC_DIR}/frag.glsl
#     COMMENT "Compiling frag.glsl to SPIR-V (fragment shader)"
# )
# add_custom_command(
#     OUTPUT ${SHADER_OUT_DIR}/vert.spv
#     COMMAND ${GLSLC} -fshader-stage=vert ${SHADER_SRC_DIR}/vert.glsl -o ${SHADER_OUT_DIR}/vert.spv
#     DEPENDS ${SHADER_SRC_DIR}/vert.glsl
#     COMMENT "Compiling vert.glsl to SPIR-V (vertex shader)"
# )

# add_custom_target(
#     Shaders ALL
#     DEPENDS ${SHADER_OUT_DIR}/frag.spv ${SHADER_OUT_DIR}/vert.spv
# )
# add_dependencies(${PROJECT_NAME} Shaders)

# # Copy shaders to output directory
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     ${SHADER_OUT_DIR}/vert.spv
#     $<TARGET_FILE_DIR:${PROJECT_NAME}>/vert.spv
#     COMMENT "Copying vert.spv to output directory"
# )
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     ${SHADER_OUT_DIR}/frag.spv
#     $<TARGET_FILE_DIR:${PROJECT_NAME}>/frag.spv
#     COMMENT "Copying frag.spv to output directory"
# )