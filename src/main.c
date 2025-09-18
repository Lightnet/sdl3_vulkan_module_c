#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "vulkan_module.h"
#include "imgui_module.h"
#include "triangle_module.h"
#include "cimgui.h"
#include "cimgui_impl.h"

#define igGetIO igGetIO_Nil
#define WIDTH 800
#define HEIGHT 600

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 with ImGui", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    VulkanContext* vkCtx = get_vulkan_context();
    init_vulkan(window, WIDTH, HEIGHT); // Pass width and height
    create_triangle();
    create_quad(); // Create quad
    create_pipeline();
    init_imgui(window);

    bool running = true;
    SDL_Event event;
    bool is_triangle = false;
    bool is_quad = false;

    while (running) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        igBegin("Hello, ImGui!", NULL, 0);
        igText("This is a test window.");
        if (igButton("Close", (ImVec2){0, 0})) {
            running = false;
        }
        if (igButton("triangle", (ImVec2){0, 0})) {
            is_triangle = !is_triangle;
        }
        if (igButton("quad", (ImVec2){0, 0})) {
            is_quad = !is_quad;
        }
        igEnd();

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        igRender();

        vkWaitForFences(vkCtx->device, 1, &vkCtx->inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx->device, 1, &vkCtx->inFlightFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx->device, vkCtx->swapchain, UINT64_MAX, vkCtx->imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result != VK_SUCCESS) {
            printf("Failed to acquire next image: %d\n", result);
            exit(1);
        }

        vkResetCommandBuffer(vkCtx->commandBuffer, 0);

        // begin render
        vulkan_begin_render(imageIndex);
        if(is_triangle){
            // Render triangle 
            render_triangle(vkCtx->commandBuffer);    
        }

        if(is_quad){
            // Render quad
            render_quad(vkCtx->commandBuffer);
        }
        
        // imgui
        render_imgui(imageIndex);
        // end render
        vulkan_end_render(imageIndex);

    }

    vkDeviceWaitIdle(vkCtx->device);
    cleanup_imgui();
    cleanup_vulkan();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}