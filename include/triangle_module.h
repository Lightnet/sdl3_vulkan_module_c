#pragma once

#include "vulkan_module.h"

// Create vertex buffer for a triangle
void create_triangle(void);
// Render the triangle
void render_triangle(VkCommandBuffer commandBuffer);