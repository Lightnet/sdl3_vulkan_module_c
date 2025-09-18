// vulkan
// working

/*

Font Positioning: The text is positioned near the top-left corner (based on the ascent and normalized coordinates). Adjust the x and y starting positions in init_font to reposition the text.
Font Size: The font_size parameter (e.g., 32.0f) controls the text size. Adjust as needed.
Texture Size: The font bitmap is fixed at 512x512 pixels. For longer text or larger fonts, increase bitmap_width and bitmap_height.
Dependencies: Ensure stb_truetype.h and "Kenney Pixel.ttf" are in the correct paths.
Performance: The current implementation creates a static text buffer. For dynamic text, you’d need to update init_font to regenerate vertex/index buffers when the text changes.


*/


/*
shader.frag
```
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```
shader.vert
```
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0); // No transformations, just raw positions
    fragColor = inColor;
}
```
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> // for triangle_vert_spv.h triangle_frag_spv.h
#include "triangle_vert_spv.h"
#include "triangle_frag_spv.h"

#include "font_vert_spv.h"
#include "font_frag_spv.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define WIDTH 800
#define HEIGHT 600

struct VulkanContext {
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
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    uint32_t imageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* swapchainFramebuffers;
    uint32_t width;  // Add window width
    uint32_t height; // Add window height

    VkBuffer fontVertexBuffer;
    VkDeviceMemory fontVertexMemory;
    VkBuffer fontIndexBuffer;
    VkDeviceMemory fontIndexMemory;
    VkImage fontImage;
    VkDeviceMemory fontImageMemory;
    VkImageView fontImageView;
    VkSampler fontSampler;
    VkDescriptorSetLayout fontDescriptorSetLayout;
    VkDescriptorPool fontDescriptorPool;
    VkDescriptorSet fontDescriptorSet;
    VkPipelineLayout fontPipelineLayout;
    VkPipeline fontGraphicsPipeline;
    uint32_t fontVertexCount;
    uint32_t fontIndexCount;

} vkCtx = {0};

uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkCtx.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    printf("Failed to find suitable memory type!\n");
    exit(1);
}

void init_vulkan(SDL_Window* window) {
    // Initialize width and height
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    vkCtx.width = (uint32_t)width;
    vkCtx.height = (uint32_t)height;

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Vulkan SDL3";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, "VK_KHR_win32_surface"};
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, NULL, &vkCtx.instance) != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        exit(1);
    }

    if (!SDL_Vulkan_CreateSurface(window, vkCtx.instance, NULL, &vkCtx.surface)) {
        printf("Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(1);
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkCtx.instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vkCtx.instance, &deviceCount, devices);
    vkCtx.physicalDevice = devices[0];
    free(devices);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx.physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx.physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsFamily = -1;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            break;
        }
    }
    free(queueFamilies);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = graphicsFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(vkCtx.physicalDevice, &deviceCreateInfo, NULL, &vkCtx.device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        exit(1);
    }

    vkGetDeviceQueue(vkCtx.device, graphicsFamily, 0, &vkCtx.graphicsQueue);

    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = vkCtx.surface;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width = vkCtx.width; // Use vkCtx.width
    swapchainInfo.imageExtent.height = vkCtx.height; // Use vkCtx.height
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(vkCtx.device, &swapchainInfo, NULL, &vkCtx.swapchain) != VK_SUCCESS) {
        printf("Failed to create swapchain\n");
        exit(1);
    }

    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, NULL);
    vkCtx.swapchainImages = malloc(vkCtx.imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, vkCtx.swapchainImages);

    vkCtx.swapchainImageViews = malloc(vkCtx.imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkCtx.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views\n");
            exit(1);
        }
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(vkCtx.device, &renderPassInfo, NULL, &vkCtx.renderPass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        exit(1);
    }

    vkCtx.swapchainFramebuffers = malloc(vkCtx.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx.swapchainImageViews[i];
        framebufferInfo.width = vkCtx.width; // Use vkCtx.width
        framebufferInfo.height = vkCtx.height; // Use vkCtx.height
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx.device, &framebufferInfo, NULL, &vkCtx.swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }

    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vkCtx.device, &poolInfo, NULL, &vkCtx.commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(1);
    }

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(vkCtx.device, &allocInfo, &vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Failed to allocate command buffer\n");
        exit(1);
    }

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(vkCtx.device, &semaphoreInfo, NULL, &vkCtx.renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(vkCtx.device, &fenceInfo, NULL, &vkCtx.inFlightFence) != VK_SUCCESS) {
        printf("Failed to create synchronization objects\n");
        exit(1);
    }
}


void init_font(const char* text, float font_size) {
    // Load font file
    FILE* font_file = fopen("Kenney Pixel.ttf", "rb");
    if (!font_file) {
        printf("Failed to open font file\n");
        exit(1);
    }
    fseek(font_file, 0, SEEK_END);
    size_t font_size_bytes = ftell(font_file);
    fseek(font_file, 0, SEEK_SET);
    unsigned char* font_buffer = malloc(font_size_bytes);
    fread(font_buffer, 1, font_size_bytes, font_file);
    fclose(font_file);

    // Initialize stb_truetype
    stbtt_fontinfo font;
    stbtt_InitFont(&font, font_buffer, 0);

    // Create font bitmap
    float scale = stbtt_ScaleForPixelHeight(&font, font_size);
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
    ascent = (int)(ascent * scale);
    descent = (int)(descent * scale);

    int bitmap_width = 512, bitmap_height = 512;
    unsigned char* bitmap = malloc(bitmap_width * bitmap_height);
    memset(bitmap, 0, bitmap_width * bitmap_height);

    float x = 0, y = (float)ascent;
    int vertices_count = 0, indices_count = 0;
    float* vertices = malloc(strlen(text) * 4 * 4 * sizeof(float)); // 4 vertices per char, 4 floats (x, y, u, v)
    uint32_t* indices = malloc(strlen(text) * 6 * sizeof(uint32_t)); // 6 indices per char

    for (size_t i = 0; i < strlen(text); i++) {
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font, text[i], &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font, text[i], scale, scale, &x0, &y0, &x1, &y1);

        int w = x1 - x0, h = y1 - y0;
        int xpos = (int)(x + x0);
        int ypos = (int)(y + y0);

        // Render character to bitmap
        stbtt_MakeCodepointBitmap(&font, bitmap + ypos * bitmap_width + xpos, w, h, bitmap_width, scale, scale, text[i]);

        // Normalize coordinates to [-1, 1] for Vulkan
        float x0_norm = (x + x0) / (float)vkCtx.width * 2.0f - 1.0f;
        float x1_norm = (x + x1) / (float)vkCtx.width * 2.0f - 1.0f;
        float y0_norm = 1.0f - (y + y0) / (float)vkCtx.height * 2.0f;
        float y1_norm = 1.0f - (y + y1) / (float)vkCtx.height * 2.0f;

        // Texture coordinates
        float u0 = (float)xpos / bitmap_width;
        float u1 = (float)(xpos + w) / bitmap_width;
        float v0 = (float)ypos / bitmap_height;
        float v1 = (float)(ypos + h) / bitmap_height;

        // Vertex data: x, y, u, v
        vertices[vertices_count++] = x0_norm; vertices[vertices_count++] = y0_norm; vertices[vertices_count++] = u0; vertices[vertices_count++] = v0;
        vertices[vertices_count++] = x1_norm; vertices[vertices_count++] = y0_norm; vertices[vertices_count++] = u1; vertices[vertices_count++] = v0;
        vertices[vertices_count++] = x1_norm; vertices[vertices_count++] = y1_norm; vertices[vertices_count++] = u1; vertices[vertices_count++] = v1;
        vertices[vertices_count++] = x0_norm; vertices[vertices_count++] = y1_norm; vertices[vertices_count++] = u0; vertices[vertices_count++] = v1;

        // Index data
        uint32_t base = i * 4;
        indices[indices_count++] = base + 0; indices[indices_count++] = base + 1; indices[indices_count++] = base + 2;
        indices[indices_count++] = base + 0; indices[indices_count++] = base + 2; indices[indices_count++] = base + 3;

        x += advance * scale;
    }

    vkCtx.fontVertexCount = vertices_count / 4;
    vkCtx.fontIndexCount = indices_count;

    // Create font texture
    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8_UNORM;
    imageInfo.extent.width = bitmap_width;
    imageInfo.extent.height = bitmap_height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(vkCtx.device, &imageInfo, NULL, &vkCtx.fontImage) != VK_SUCCESS) {
        printf("Failed to create font image\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkCtx.device, vkCtx.fontImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.fontImageMemory) != VK_SUCCESS) {
        printf("Failed to allocate font image memory\n");
        exit(1);
    }
    vkBindImageMemory(vkCtx.device, vkCtx.fontImage, vkCtx.fontImageMemory, 0);

    // Create staging buffer to upload bitmap
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bitmap_width * bitmap_height;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &stagingBuffer) != VK_SUCCESS) {
        printf("Failed to create staging buffer\n");
        exit(1);
    }
    vkGetBufferMemoryRequirements(vkCtx.device, stagingBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &stagingBufferMemory) != VK_SUCCESS) {
        printf("Failed to allocate staging buffer memory\n");
        exit(1);
    }
    vkBindBufferMemory(vkCtx.device, stagingBuffer, stagingBufferMemory, 0);

    void* data;
    vkMapMemory(vkCtx.device, stagingBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, bitmap, bufferInfo.size);
    vkUnmapMemory(vkCtx.device, stagingBufferMemory);

    // Transition image layout and copy buffer to image
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfoCmd = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfoCmd.commandPool = vkCtx.commandPool;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = 1;
    vkAllocateCommandBuffers(vkCtx.device, &allocInfoCmd, &commandBuffer);
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image = vkCtx.fontImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = bitmap_width;
    region.imageExtent.height = bitmap_height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkCtx.fontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkCtx.graphicsQueue);
    vkFreeCommandBuffers(vkCtx.device, vkCtx.commandPool, 1, &commandBuffer);
    vkDestroyBuffer(vkCtx.device, stagingBuffer, NULL);
    vkFreeMemory(vkCtx.device, stagingBufferMemory, NULL);

    // Create image view
    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = vkCtx.fontImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.fontImageView) != VK_SUCCESS) {
        printf("Failed to create font image view\n");
        exit(1);
    }

    // Create sampler
    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (vkCreateSampler(vkCtx.device, &samplerInfo, NULL, &vkCtx.fontSampler) != VK_SUCCESS) {
        printf("Failed to create font sampler\n");
        exit(1);
    }

    // Create vertex buffer
    bufferInfo.size = vertices_count * sizeof(float);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.fontVertexBuffer) != VK_SUCCESS) {
        printf("Failed to create font vertex buffer\n");
        exit(1);
    }
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.fontVertexBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.fontVertexMemory) != VK_SUCCESS) {
        printf("Failed to allocate font vertex buffer memory\n");
        exit(1);
    }
    vkBindBufferMemory(vkCtx.device, vkCtx.fontVertexBuffer, vkCtx.fontVertexMemory, 0);
    vkMapMemory(vkCtx.device, vkCtx.fontVertexMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, bufferInfo.size);
    vkUnmapMemory(vkCtx.device, vkCtx.fontVertexMemory);

    // Create index buffer
    bufferInfo.size = indices_count * sizeof(uint32_t);
    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.fontIndexBuffer) != VK_SUCCESS) {
        printf("Failed to create font index buffer\n");
        exit(1);
    }
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.fontIndexBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.fontIndexMemory) != VK_SUCCESS) {
        printf("Failed to allocate font index buffer memory\n");
        exit(1);
    }
    vkBindBufferMemory(vkCtx.device, vkCtx.fontIndexBuffer, vkCtx.fontIndexMemory, 0);
    vkMapMemory(vkCtx.device, vkCtx.fontIndexMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, indices, bufferInfo.size);
    vkUnmapMemory(vkCtx.device, vkCtx.fontIndexMemory);

    // Create descriptor set layout
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    if (vkCreateDescriptorSetLayout(vkCtx.device, &layoutInfo, NULL, &vkCtx.fontDescriptorSetLayout) != VK_SUCCESS) {
        printf("Failed to create font descriptor set layout\n");
        exit(1);
    }

    // Create descriptor pool
    VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1};
    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(vkCtx.device, &poolInfo, NULL, &vkCtx.fontDescriptorPool) != VK_SUCCESS) {
        printf("Failed to create font descriptor pool\n");
        exit(1);
    }

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfoDesc = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfoDesc.descriptorPool = vkCtx.fontDescriptorPool;
    allocInfoDesc.descriptorSetCount = 1;
    allocInfoDesc.pSetLayouts = &vkCtx.fontDescriptorSetLayout;
    if (vkAllocateDescriptorSets(vkCtx.device, &allocInfoDesc, &vkCtx.fontDescriptorSet) != VK_SUCCESS) {
        printf("Failed to allocate font descriptor set\n");
        exit(1);
    }

    // Update descriptor set
    VkDescriptorImageInfo descImageInfo = {}; // Renamed to avoid conflict
    descImageInfo.imageView = vkCtx.fontImageView;
    descImageInfo.sampler = vkCtx.fontSampler;
    descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptorWrite.dstSet = vkCtx.fontDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.pImageInfo = &descImageInfo;
    vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrite, 0, NULL);

    free(bitmap);
    free(vertices);
    free(indices);
    free(font_buffer);
}


void create_font_pipeline() {
    // Load font shaders (you need to compile font.vert and font.frag to SPIR-V, e.g., font_vert_spv and font_frag_spv)
    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = sizeof(font_vert_spv); // Replace with actual SPIR-V data
    vertShaderInfo.pCode = font_vert_spv;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx.device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        printf("Failed to create font vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = sizeof(font_frag_spv); // Replace with actual SPIR-V data
    fragShaderInfo.pCode = font_frag_spv;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx.device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        printf("Failed to create font fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    // Vertex input configuration
    VkVertexInputBindingDescription bindingDesc = {0, 4 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrDesc[] = {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, 0}, // Position
        {1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float)} // TexCoord
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Enable alpha blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vkCtx.fontDescriptorSetLayout;
    if (vkCreatePipelineLayout(vkCtx.device, &pipelineLayoutInfo, NULL, &vkCtx.fontPipelineLayout) != VK_SUCCESS) {
        printf("Failed to create font pipeline layout\n");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vkCtx.fontPipelineLayout;
    pipelineInfo.renderPass = vkCtx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx.fontGraphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create font graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx.device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx.device, vertModule, NULL);
}

void create_triangle() {
    float vertices[] = {
        0.0f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Top, red
       -0.5f,  0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Bottom-left, green
        0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // Bottom-right, blue
    };

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(vertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(vkCtx.device, &bufferInfo, NULL, &vkCtx.vertexBuffer) != VK_SUCCESS) {
        printf("Failed to create vertex buffer\n");
        exit(1);
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkCtx.device, vkCtx.vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(vkCtx.device, &allocInfo, NULL, &vkCtx.vertexMemory) != VK_SUCCESS) {
        printf("Failed to allocate vertex buffer memory\n");
        exit(1);
    }

    vkBindBufferMemory(vkCtx.device, vkCtx.vertexBuffer, vkCtx.vertexMemory, 0);

    void* data;
    vkMapMemory(vkCtx.device, vkCtx.vertexMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, vertices, sizeof(vertices));
    vkUnmapMemory(vkCtx.device, vkCtx.vertexMemory);
}

void create_pipeline() {
    // Create shader modules (unchanged)
    size_t vertSize = sizeof(triangle_vert_spv);
    size_t fragSize = sizeof(triangle_frag_spv);

    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = vertSize;
    vertShaderInfo.pCode = triangle_vert_spv;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx.device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        printf("Failed to create vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = fragSize;
    fragShaderInfo.pCode = triangle_frag_spv;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx.device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        printf("Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    // Vertex input configuration (unchanged)
    VkVertexInputBindingDescription bindingDesc = {0, 6 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrDesc[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)}
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Set dynamic states for viewport and scissor
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Viewport state with dynamic viewport and scissor
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(vkCtx.device, &pipelineLayoutInfo, NULL, &vkCtx.pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(1);
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState; // Add dynamic state
    pipelineInfo.layout = vkCtx.pipelineLayout;
    pipelineInfo.renderPass = vkCtx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx.graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx.device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx.device, vertModule, NULL);
}

void record_command_buffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(vkCtx.commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin command buffer\n");
        exit(1);
    }

    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = vkCtx.renderPass;
    renderPassInfo.framebuffer = vkCtx.swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = (VkExtent2D){vkCtx.width, vkCtx.height};
    VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {0.0f, 0.0f, (float)vkCtx.width, (float)vkCtx.height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {vkCtx.width, vkCtx.height}};
    vkCmdSetViewport(vkCtx.commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(vkCtx.commandBuffer, 0, 1, &scissor);

    // Draw triangle
    vkCmdBindPipeline(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.graphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.vertexBuffer, offsets);
    vkCmdDraw(vkCtx.commandBuffer, 3, 1, 0, 0);

    // Draw text
    vkCmdBindPipeline(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.fontGraphicsPipeline);
    vkCmdBindDescriptorSets(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.fontPipelineLayout, 0, 1, &vkCtx.fontDescriptorSet, 0, NULL);
    vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.fontVertexBuffer, offsets);
    vkCmdBindIndexBuffer(vkCtx.commandBuffer, vkCtx.fontIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(vkCtx.commandBuffer, vkCtx.fontIndexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(vkCtx.commandBuffer);
    if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Failed to end command buffer\n");
        exit(1);
    }
}

void recreate_swapchain(SDL_Window* window) {
    // Wait for the device to be idle to safely destroy resources
    vkDeviceWaitIdle(vkCtx.device);

    // Clean up old swapchain resources
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        vkDestroyFramebuffer(vkCtx.device, vkCtx.swapchainFramebuffers[i], NULL);
        vkDestroyImageView(vkCtx.device, vkCtx.swapchainImageViews[i], NULL);
    }
    free(vkCtx.swapchainFramebuffers);
    free(vkCtx.swapchainImageViews);
    free(vkCtx.swapchainImages);

    // Get new window size
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    vkCtx.width = (uint32_t)width;
    vkCtx.height = (uint32_t)height;

    // Recreate swapchain
    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = vkCtx.surface;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width = vkCtx.width;
    swapchainInfo.imageExtent.height = vkCtx.height;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = vkCtx.swapchain; // Pass old swapchain for smooth recreation

    if (vkCreateSwapchainKHR(vkCtx.device, &swapchainInfo, NULL, &vkCtx.swapchain) != VK_SUCCESS) {
        printf("Failed to recreate swapchain\n");
        exit(1);
    }

    // Destroy old swapchain
    vkDestroySwapchainKHR(vkCtx.device, swapchainInfo.oldSwapchain, NULL);

    // Get new swapchain images
    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, NULL);
    vkCtx.swapchainImages = malloc(vkCtx.imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkCtx.device, vkCtx.swapchain, &vkCtx.imageCount, vkCtx.swapchainImages);

    // Create new image views
    vkCtx.swapchainImageViews = malloc(vkCtx.imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkCtx.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image views\n");
            exit(1);
        }
    }

    // Create new framebuffers
    vkCtx.swapchainFramebuffers = malloc(vkCtx.imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx.swapchainImageViews[i];
        framebufferInfo.width = vkCtx.width;
        framebufferInfo.height = vkCtx.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx.device, &framebufferInfo, NULL, &vkCtx.swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }
}


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Vulkan SDL3", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    init_vulkan(window);
    create_triangle();
    create_pipeline();
    init_font("Hello Vulkan!", 32.0f); // Initialize font with text and size
    create_font_pipeline();

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                recreate_swapchain(window);
            }
        }

        vkWaitForFences(vkCtx.device, 1, &vkCtx.inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(vkCtx.device, 1, &vkCtx.inFlightFence);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(vkCtx.device, vkCtx.swapchain, UINT64_MAX, vkCtx.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(window);
            continue;
        } else if (result != VK_SUCCESS) {
            printf("Failed to acquire next image: %d\n", result);
            exit(1);
        }

        vkResetCommandBuffer(vkCtx.commandBuffer, 0);
        record_command_buffer(imageIndex);

        VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        VkSemaphore waitSemaphores[] = {vkCtx.imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &vkCtx.commandBuffer;
        VkSemaphore signalSemaphores[] = {vkCtx.renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, vkCtx.inFlightFence) != VK_SUCCESS) {
            printf("Failed to submit draw command buffer\n");
            exit(1);
        }

        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vkCtx.swapchain;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(vkCtx.graphicsQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(window);
            continue;
        } else if (result != VK_SUCCESS) {
            printf("Failed to present image: %d\n", result);
            exit(1);
        }
    }

    vkDeviceWaitIdle(vkCtx.device);

    // Cleanup font resources
    vkDestroyPipeline(vkCtx.device, vkCtx.fontGraphicsPipeline, NULL);
    vkDestroyPipelineLayout(vkCtx.device, vkCtx.fontPipelineLayout, NULL);
    vkDestroyDescriptorPool(vkCtx.device, vkCtx.fontDescriptorPool, NULL);
    vkDestroyDescriptorSetLayout(vkCtx.device, vkCtx.fontDescriptorSetLayout, NULL);
    vkDestroySampler(vkCtx.device, vkCtx.fontSampler, NULL);
    vkDestroyImageView(vkCtx.device, vkCtx.fontImageView, NULL);
    vkDestroyImage(vkCtx.device, vkCtx.fontImage, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.fontImageMemory, NULL);
    vkDestroyBuffer(vkCtx.device, vkCtx.fontVertexBuffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.fontVertexMemory, NULL);
    vkDestroyBuffer(vkCtx.device, vkCtx.fontIndexBuffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.fontIndexMemory, NULL);

    // Existing cleanup
    vkDestroySemaphore(vkCtx.device, vkCtx.renderFinishedSemaphore, NULL);
    vkDestroySemaphore(vkCtx.device, vkCtx.imageAvailableSemaphore, NULL);
    vkDestroyFence(vkCtx.device, vkCtx.inFlightFence, NULL);
    vkDestroyCommandPool(vkCtx.device, vkCtx.commandPool, NULL);
    vkDestroyPipeline(vkCtx.device, vkCtx.graphicsPipeline, NULL);
    vkDestroyPipelineLayout(vkCtx.device, vkCtx.pipelineLayout, NULL);
    for (uint32_t i = 0; i < vkCtx.imageCount; i++) {
        vkDestroyFramebuffer(vkCtx.device, vkCtx.swapchainFramebuffers[i], NULL);
        vkDestroyImageView(vkCtx.device, vkCtx.swapchainImageViews[i], NULL);
    }
    free(vkCtx.swapchainFramebuffers);
    free(vkCtx.swapchainImages);
    free(vkCtx.swapchainImageViews);
    vkDestroyRenderPass(vkCtx.device, vkCtx.renderPass, NULL);
    vkDestroySwapchainKHR(vkCtx.device, vkCtx.swapchain, NULL);
    vkDestroyBuffer(vkCtx.device, vkCtx.vertexBuffer, NULL);
    vkFreeMemory(vkCtx.device, vkCtx.vertexMemory, NULL);
    vkDestroyDevice(vkCtx.device, NULL);
    vkDestroySurfaceKHR(vkCtx.instance, vkCtx.surface, NULL);
    vkDestroyInstance(vkCtx.instance, NULL);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
