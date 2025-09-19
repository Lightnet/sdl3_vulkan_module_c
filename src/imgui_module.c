#include "imgui_module.h"
#include "cimgui.h"
#include "cimgui_impl.h"
#include "triangle_module.h"
#include <stdio.h>
#include <vulkan/vulkan.h>

#define igGetIO igGetIO_Nil

void init_imgui(SDL_Window* window) {
    VulkanContext* vkCtx = get_vulkan_context();

    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Add this flag
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(vkCtx->device, &pool_info, NULL, &vkCtx->imguiDescriptorPool) != VK_SUCCESS) {
        printf("Failed to create ImGui descriptor pool\n");
        exit(1);
    }

    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = vkCtx->instance;
    init_info.PhysicalDevice = vkCtx->physicalDevice;
    init_info.Device = vkCtx->device;
    init_info.QueueFamily = 0;
    init_info.Queue = vkCtx->graphicsQueue;
    init_info.DescriptorPool = vkCtx->imguiDescriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = vkCtx->imageCount;
    init_info.RenderPass = vkCtx->renderPass;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        printf("Failed to initialize ImGui Vulkan backend\n");
        exit(1);
    }

    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        printf("Failed to initialize ImGui SDL3 backend\n");
        exit(1);
    }
    // ImGui fonts does not need to create imgui 1.92.
    
    // Create a temporary command buffer for font uploading
    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(vkCtx->device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffer for ImGui initialization\n");
        exit(1);
    }

    // VkCommandBuffer commandBuffer = vkCtx->commandBuffer;
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx->graphicsQueue);

    // ImGui_ImplVulkan_DestroyFontsTexture imgui 1.92 is remove
}

void cleanup_imgui(void) {
    VulkanContext* vkCtx = get_vulkan_context();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    vkDestroyDescriptorPool(vkCtx->device, vkCtx->imguiDescriptorPool, NULL);
}

void render_imgui(uint32_t imageIndex) {
    VulkanContext* vkCtx = get_vulkan_context();

    // Render triangle and quad
    // render_triangle(vkCtx->commandBuffer);
    // render_quad(vkCtx->commandBuffer);

    ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), vkCtx->commandBuffers[imageIndex], VK_NULL_HANDLE);



    // Render ImGui
    // ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), vkCtx->commandBuffer, VK_NULL_HANDLE);

    // vkCmdEndRenderPass(vkCtx->commandBuffer);
    // vkEndCommandBuffer(vkCtx->commandBuffer);

}

// this ref which break up the code for render.
// void render_imgui(uint32_t imageIndex) {
//     VulkanContext* vkCtx = get_vulkan_context();
//     VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//     vkBeginCommandBuffer(vkCtx->commandBuffer, &beginInfo);
//     VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
//     renderPassInfo.renderPass = vkCtx->renderPass;
//     renderPassInfo.framebuffer = vkCtx->swapchainFramebuffers[imageIndex];
//     renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
//     renderPassInfo.renderArea.extent = (VkExtent2D){vkCtx->width, vkCtx->height};
//     VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}};
//     renderPassInfo.clearValueCount = 1;
//     renderPassInfo.pClearValues = &clearColor;
//     vkCmdBeginRenderPass(vkCtx->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
//     // Render triangle and quad
//     render_triangle(vkCtx->commandBuffer);
//     render_quad(vkCtx->commandBuffer);
//     // Render ImGui
//     ImGui_ImplVulkan_RenderDrawData(igGetDrawData(), vkCtx->commandBuffer, VK_NULL_HANDLE);
//     vkCmdEndRenderPass(vkCtx->commandBuffer);
//     vkEndCommandBuffer(vkCtx->commandBuffer);
//     VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
//     VkSemaphore waitSemaphores[] = {vkCtx->imageAvailableSemaphore};
//     VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//     submitInfo.waitSemaphoreCount = 1;
//     submitInfo.pWaitSemaphores = waitSemaphores;
//     submitInfo.pWaitDstStageMask = waitStages;
//     submitInfo.commandBufferCount = 1;
//     submitInfo.pCommandBuffers = &vkCtx->commandBuffer;
//     VkSemaphore signalSemaphores[] = {vkCtx->renderFinishedSemaphore};
//     submitInfo.signalSemaphoreCount = 1;
//     submitInfo.pSignalSemaphores = signalSemaphores;
//     if (vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, vkCtx->inFlightFence) != VK_SUCCESS) {
//         printf("Failed to submit draw command buffer\n");
//         exit(1);
//     }
//     VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
//     presentInfo.waitSemaphoreCount = 1;
//     presentInfo.pWaitSemaphores = signalSemaphores;
//     presentInfo.swapchainCount = 1;
//     presentInfo.pSwapchains = &vkCtx->swapchain;
//     presentInfo.pImageIndices = &imageIndex;
//     if (vkQueuePresentKHR(vkCtx->graphicsQueue, &presentInfo) != VK_SUCCESS) {
//         printf("Failed to present image\n");
//         exit(1);
//     }
// }