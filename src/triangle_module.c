#include "triangle_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

static uint32_t find_memory_type(VulkanContext* ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(ctx->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    printf("Failed to find suitable memory type!\n");
    exit(1);
}

void create_triangle(void) {
    VulkanContext* vkCtx = get_vulkan_context();

    float vertices[] = {
        0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Top, red
       -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Bottom-left, green
        0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // Bottom-right, blue
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx->device, &bufferInfo, NULL, &vkCtx->vertexBuffer) != VK_SUCCESS) {
        printf("Failed to create vertex buffer\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkCtx->device, vkCtx->vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(vkCtx, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx->device, &allocInfo, NULL, &vkCtx->vertexMemory) != VK_SUCCESS) {
        printf("Failed to allocate vertex buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx->device, vkCtx->vertexBuffer, vkCtx->vertexMemory, 0);

    void* data;
    vkMapMemory(vkCtx->device, vkCtx->vertexMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vkCtx->device, vkCtx->vertexMemory);
}