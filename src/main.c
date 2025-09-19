// main.c
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3 with ImGui",
                                          WIDTH, HEIGHT,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    init_vulkan(window, WIDTH, HEIGHT);
    create_triangle();
    create_quad();
    init_imgui(window);

    bool showTriangle = true;
    bool showQuad = true;
    bool running = true;
    uint32_t currentFrame = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_MINIMIZED) {
                recreate_swapchain(window);
            }
        }

        // Start ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        // ImGui interface
        igBegin("Controls", NULL, 0);
        igCheckbox("Show Triangle", &showTriangle);
        igCheckbox("Show Quad", &showQuad);
        igEnd();
        igRender();

        VulkanContext* vkCtx = get_vulkan_context();

        // Acquire next image
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx->device, vkCtx->swapchain, UINT64_MAX,
                                                vkCtx->imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreate_swapchain(window);
            continue;
        } else if (result != VK_SUCCESS) {
            printf("Failed to acquire swapchain image: %d\n", result);
            exit(1);
        }

        // Wait for the fence associated with this imageIndex
        vkWaitForFences(vkCtx->device, 1, &vkCtx->inFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx->device, 1, &vkCtx->inFlightFences[imageIndex]);

        // Reset command buffer
        if (vkResetCommandBuffer(vkCtx->commandBuffers[imageIndex], 0) != VK_SUCCESS) {
            printf("Failed to reset command buffer\n");
            exit(1);
        }

        // Record command buffer
        vulkan_begin_render(imageIndex);
        if (showTriangle) {
            render_triangle(vkCtx->commandBuffers[imageIndex]);
        }
        if (showQuad) {
            render_quad(vkCtx->commandBuffers[imageIndex]);
        }
        render_imgui(imageIndex);
        vulkan_end_render(imageIndex, currentFrame);

        // Increment frame index
        currentFrame = (currentFrame + 1) % vkCtx->imageCount;
    }

    // Cleanup
    VulkanContext* vkCtx = get_vulkan_context();
    vkDeviceWaitIdle(vkCtx->device);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    cleanup_vulkan();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}



