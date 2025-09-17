#include "triangle_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

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

void render_triangle(VkCommandBuffer commandBuffer) {
    VulkanContext* vkCtx = get_vulkan_context();
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx->graphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkCtx->vertexBuffer, offsets);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}


void create_quad(void) {
    VulkanContext* vkCtx = get_vulkan_context();

    // Quad vertices in top-left quadrant
    float vertices[] = {
        -0.75f, -0.75f, 0.0f,  1.0f, 1.0f, 0.0f, // Bottom-left, yellow
        -0.25f, -0.75f, 0.0f,  1.0f, 0.0f, 1.0f, // Bottom-right, magenta
        -0.75f, -0.25f, 0.0f,  0.0f, 1.0f, 1.0f, // Top-left, cyan
        -0.25f, -0.25f, 0.0f,  1.0f, 1.0f, 1.0f  // Top-right, white
    };

    // Indices for two triangles (0,1,2) and (1,2,3)
    uint16_t indices[] = {0, 1, 2, 1, 2, 3};

    // Create vertex buffer
    VkBufferCreateInfo vertexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    vertexBufferInfo.size = sizeof(vertices);
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx->device, &vertexBufferInfo, NULL, &vkCtx->quadBuffer) != VK_SUCCESS) {
        printf("Failed to create quad vertex buffer\n");
        exit(1);
    }

    VkMemoryRequirements vertexMemRequirements;
    vkGetBufferMemoryRequirements(vkCtx->device, vkCtx->quadBuffer, &vertexMemRequirements);

    VkMemoryAllocateInfo vertexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    vertexAllocInfo.allocationSize = vertexMemRequirements.size;
    vertexAllocInfo.memoryTypeIndex = find_memory_type(vkCtx, vertexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx->device, &vertexAllocInfo, NULL, &vkCtx->quadMemory) != VK_SUCCESS) {
        printf("Failed to allocate quad vertex buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx->device, vkCtx->quadBuffer, vkCtx->quadMemory, 0);

    void* vertexData;
    vkMapMemory(vkCtx->device, vkCtx->quadMemory, 0, vertexBufferInfo.size, 0, &vertexData);
    memcpy(vertexData, vertices, sizeof(vertices));
    vkUnmapMemory(vkCtx->device, vkCtx->quadMemory);

    // Create index buffer
    VkBufferCreateInfo indexBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    indexBufferInfo.size = sizeof(indices);
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx->device, &indexBufferInfo, NULL, &vkCtx->quadIndexBuffer) != VK_SUCCESS) {
        printf("Failed to create quad index buffer\n");
        exit(1);
    }

    VkMemoryRequirements indexMemRequirements;
    vkGetBufferMemoryRequirements(vkCtx->device, vkCtx->quadIndexBuffer, &indexMemRequirements);

    VkMemoryAllocateInfo indexAllocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    indexAllocInfo.allocationSize = indexMemRequirements.size;
    indexAllocInfo.memoryTypeIndex = find_memory_type(vkCtx, indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx->device, &indexAllocInfo, NULL, &vkCtx->quadIndexMemory) != VK_SUCCESS) {
        printf("Failed to allocate quad index buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx->device, vkCtx->quadIndexBuffer, vkCtx->quadIndexMemory, 0);

    void* indexData;
    vkMapMemory(vkCtx->device, vkCtx->quadIndexMemory, 0, indexBufferInfo.size, 0, &indexData);
    memcpy(indexData, indices, sizeof(indices));
    vkUnmapMemory(vkCtx->device, vkCtx->quadIndexMemory);

    printf("Quad vertex and index buffers created successfully\n");
}

void render_quad(VkCommandBuffer commandBuffer) {
    VulkanContext* vkCtx = get_vulkan_context();

    if (vkCtx->quadBuffer == VK_NULL_HANDLE || vkCtx->quadIndexBuffer == VK_NULL_HANDLE) {
        printf("Quad vertex or index buffer is null!\n");
        exit(1);
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx->graphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vkCtx->quadBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vkCtx->quadIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0); // 6 indices for two triangles

    // printf("Quad draw command issued\n");
}




