// 

#include "vulkan_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600

// Global Vulkan context
static VulkanContext vkCtx = {0};

// Accessor function
VulkanContext* get_vulkan_context(void) {
    return &vkCtx;
}

void init_vulkan(SDL_Window* window) {
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
    swapchainInfo.imageExtent.width = WIDTH;
    swapchainInfo.imageExtent.height = HEIGHT;
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
        framebufferInfo.width = WIDTH;
        framebufferInfo.height = HEIGHT;
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

void create_pipeline(void) {
    FILE* vertFile = fopen("vert.spv", "rb");
    if (!vertFile) {
        printf("Failed to open vert.spv\n");
        exit(1);
    }
    fseek(vertFile, 0, SEEK_END);
    long vertSize = ftell(vertFile);
    fseek(vertFile, 0, SEEK_SET);
    char* vertShaderCode = malloc(vertSize);
    fread(vertShaderCode, 1, vertSize, vertFile);
    fclose(vertFile);

    FILE* fragFile = fopen("frag.spv", "rb");
    if (!fragFile) {
        printf("Failed to open frag.spv\n");
        exit(1);
    }
    fseek(fragFile, 0, SEEK_END);
    long fragSize = ftell(fragFile);
    fseek(fragFile, 0, SEEK_SET);
    char* fragShaderCode = malloc(fragSize);
    fread(fragShaderCode, 1, fragSize, fragFile);
    fclose(fragFile);

    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = vertSize;
    vertShaderInfo.pCode = (uint32_t*)vertShaderCode;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx.device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        printf("Failed to create vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = fragSize;
    fragShaderInfo.pCode = (uint32_t*)fragShaderCode;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx.device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        printf("Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    VkVertexInputBindingDescription bindingDesc = {0, 6 * sizeof(float), VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attrDesc[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},                // Position
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float)} // Color
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {WIDTH, HEIGHT}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

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
    pipelineInfo.layout = vkCtx.pipelineLayout;
    pipelineInfo.renderPass = vkCtx.renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx.graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx.device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx.device, vertModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
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
    renderPassInfo.renderArea.extent = (VkExtent2D){WIDTH, HEIGHT};
    VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}}; // Gray background
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkCtx.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Draw triangle
    vkCmdBindPipeline(vkCtx.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkCtx.graphicsPipeline);
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCtx.commandBuffer, 0, 1, &vkCtx.vertexBuffer, offsets);
    vkCmdDraw(vkCtx.commandBuffer, 3, 1, 0, 0);

    // Note: ImGui rendering is handled in main.c to allow flexibility
    vkCmdEndRenderPass(vkCtx.commandBuffer);
    if (vkEndCommandBuffer(vkCtx.commandBuffer) != VK_SUCCESS) {
        printf("Failed to end command buffer\n");
        exit(1);
    }
}

void cleanup_vulkan(void) {
    vkDeviceWaitIdle(vkCtx.device);
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
}