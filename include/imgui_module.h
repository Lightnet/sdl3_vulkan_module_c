#pragma once

#include <SDL3/SDL.h>
#include "vulkan_module.h"

void init_imgui(SDL_Window* window);
void cleanup_imgui(void);
void render_imgui(uint32_t imageIndex); // for rendering