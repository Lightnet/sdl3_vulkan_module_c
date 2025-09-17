#include "imgui_module.h"
#include "cimgui.h"
#include "cimgui_impl.h"
#include <stdio.h>
#include <vulkan/vulkan.h>

#define igGetIO igGetIO_Nil

void init_imgui(SDL_Window* window) {
    VulkanContext* vkCtx = get_vulkan_context();

    // Initialize cimgui
    igCreateContext(NULL);
    ImGuiIO* io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard controls
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable docking

    // Create descriptor pool for ImGui
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
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(vkCtx->device, &pool_info, NULL, &vkCtx->imguiDescriptorPool) != VK_SUCCESS) {
        printf("Failed to create ImGui descriptor pool\n");
        exit(1);
    }

    // Initialize ImGui Vulkan backend
    ImGui_ImplVulkan_InitInfo init_info = {0};
    init_info.Instance = vkCtx->instance;
    init_info.PhysicalDevice = vkCtx->physicalDevice;
    init_info.Device = vkCtx->device;
    init_info.QueueFamily = 0; // Update this if your graphics queue family is different
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

    // Initialize ImGui SDL3 backend
    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        printf("Failed to initialize ImGui SDL3 backend\n");
        exit(1);
    }

    // Upload fonts
    VkCommandBuffer commandBuffer = vkCtx->commandBuffer;
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx->graphicsQueue);
}

void cleanup_imgui(void) {
    VulkanContext* vkCtx = get_vulkan_context();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);
    vkDestroyDescriptorPool(vkCtx->device, vkCtx->imguiDescriptorPool, NULL);
}