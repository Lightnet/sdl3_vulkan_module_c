#pragma once

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkBuffer vertexBuffer;     // For triangle
    VkDeviceMemory vertexMemory;
    VkBuffer quadBuffer;       // For quad vertices
    VkDeviceMemory quadMemory;
    VkBuffer quadIndexBuffer;  // For quad indices
    VkDeviceMemory quadIndexMemory;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    uint32_t imageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* swapchainFramebuffers;
    VkDescriptorPool imguiDescriptorPool;
    uint32_t width;
    uint32_t height;
} VulkanContext;

VulkanContext* get_vulkan_context(void);
void init_vulkan(SDL_Window* window, uint32_t width, uint32_t height);
void create_pipeline(void);
void record_command_buffer(uint32_t imageIndex);
void cleanup_vulkan(void);
uint32_t find_memory_type(VulkanContext* ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// render handle
void recreate_swapchain(SDL_Window* window);
void vulkan_begin_render(uint32_t imageIndex);
void vulkan_end_render(uint32_t imageIndex);