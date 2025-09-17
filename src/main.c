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
    init_vulkan(window);
    create_triangle();
    create_pipeline();
    init_imgui(window);

    bool running = true;
    SDL_Event event;

    while (running) {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        igNewFrame();

        igBegin("Hello, ImGui!", NULL, 0);
        igText("This is a test window.");
        if (igButton("Close", (ImVec2){0, 0})) {
            running = false;
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
        record_command_buffer(imageIndex);

        // Add ImGui rendering to the command buffer
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        vkBeginCommandBuffer(vkCtx->commandBuffer, &beginInfo);
        VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassInfo.renderPass = vkCtx->renderPass;
        renderPassInfo.framebuffer = vkCtx->swapchainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
        renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT};
        VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        vkCmdBeginRenderPass(vkCtx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(vkCtx->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx->graphicsPipeline);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCtx->commandBuffer, 0, 1, &vkCtx->vertexBuffer, offsets);
        vkCmdDraw(vkCtx->commandBuffer, 3, 1, 0, 0);
        ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), vkCtx->commandBuffer, VK_NULL_HANDLE);
        vkCmdEndRenderPass(vkCtx->commandBuffer);
        vkEndCommandBuffer(vkCtx->commandBuffer);

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkSemaphore waitSemaphores[] = {vkCtx->imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCtx->commandBuffer;
        VkSemaphore signalSemaphores[] = {vkCtx->renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, vkCtx->inFlightFence) != VK_SUCCESS) {
            printf("Failed to submit draw command buffer\n");
            exit(1);
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx->swapchain;
        presentInfo.pImageIndices = &imageIndex;

        if (vkQueuePresentKHR(vkCtx->graphicsQueue, &presentInfo) != VK_SUCCESS) {
            printf("Failed to present image\n");
            exit(1);
        }
    }

    vkDeviceWaitIdle(vkCtx->device);
    cleanup_imgui();
    cleanup_vulkan();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}