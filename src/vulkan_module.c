// 

#include "vulkan_module.h"
#include "triangle_vert.h" // Include vertex shader array
#include "triangle_frag.h" // Include fragment shader array
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 800
#define HEIGHT 600

// Global Vulkan context
static VulkanContext vkCtx = {0};

uint32_t find_memory_type(VulkanContext* ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

// Accessor function
VulkanContext* get_vulkan_context(void) {
    return &vkCtx;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    SDL_Log("Validation layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}



void recreate_swapchain(SDL_Window* window) {
    VulkanContext* vkCtx = get_vulkan_context();
    vkDeviceWaitIdle(vkCtx->device);

    // Destroy old swapchain resources
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        vkDestroyFramebuffer(vkCtx->device, vkCtx->swapchainFramebuffers[i], NULL);
        vkDestroyImageView(vkCtx->device, vkCtx->swapchainImageViews[i], NULL);
    }
    free(vkCtx->swapchainFramebuffers);
    free(vkCtx->swapchainImages);
    free(vkCtx->swapchainImageViews);

    // Free old command buffers
    vkFreeCommandBuffers(vkCtx->device, vkCtx->commandPool, vkCtx->imageCount, vkCtx->commandBuffers);
    free(vkCtx->commandBuffers);

    // Destroy old semaphores and fences
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        vkDestroySemaphore(vkCtx->device, vkCtx->imageAvailableSemaphores[i], NULL);
        vkDestroySemaphore(vkCtx->device, vkCtx->renderFinishedSemaphores[i], NULL);
        vkDestroyFence(vkCtx->device, vkCtx->inFlightFences[i], NULL);
    }
    free(vkCtx->imageAvailableSemaphores);
    free(vkCtx->renderFinishedSemaphores);
    free(vkCtx->inFlightFences);

    // Get new window size
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    vkCtx->width = (uint32_t)width;
    vkCtx->height = (uint32_t)height;

    // Recreate swapchain
    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = vkCtx->surface;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width = vkCtx->width;
    swapchainInfo.imageExtent.height = vkCtx->height;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = vkCtx->swapchain;

    if (vkCreateSwapchainKHR(vkCtx->device, &swapchainInfo, NULL, &vkCtx->swapchain) != VK_SUCCESS) {
        printf("Failed to recreate swapchain\n");
        exit(1);
    }

    vkDestroySwapchainKHR(vkCtx->device, swapchainInfo.oldSwapchain, NULL);

    // Get new swapchain images
    vkGetSwapchainImagesKHR(vkCtx->device, vkCtx->swapchain, &vkCtx->imageCount, NULL);
    vkCtx->swapchainImages = malloc(vkCtx->imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkCtx->device, vkCtx->swapchain, &vkCtx->imageCount, vkCtx->swapchainImages);

    // Create new image views
    vkCtx->swapchainImageViews = malloc(vkCtx->imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkCtx->swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkCtx->device, &viewInfo, NULL, &vkCtx->swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image view\n");
            exit(1);
        }
    }

    // Create new framebuffers
    vkCtx->swapchainFramebuffers = malloc(vkCtx->imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx->swapchainImageViews[i];
        framebufferInfo.width = vkCtx->width;
        framebufferInfo.height = vkCtx->height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx->device, &framebufferInfo, NULL, &vkCtx->swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }

    // Allocate new command buffers
    vkCtx->commandBuffers = malloc(vkCtx->imageCount * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = vkCtx->imageCount;
    if (vkAllocateCommandBuffers(vkCtx->device, &allocInfo, vkCtx->commandBuffers) != VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        exit(1);
    }

    // Recreate semaphores and fences
    vkCtx->imageAvailableSemaphores = malloc(vkCtx->imageCount * sizeof(VkSemaphore));
    vkCtx->renderFinishedSemaphores = malloc(vkCtx->imageCount * sizeof(VkSemaphore));
    vkCtx->inFlightFences = malloc(vkCtx->imageCount * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        if (vkCreateSemaphore(vkCtx->device, &semaphoreInfo, NULL, &vkCtx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkCtx->device, &semaphoreInfo, NULL, &vkCtx->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkCtx->device, &fenceInfo, NULL, &vkCtx->inFlightFences[i]) != VK_SUCCESS) {
            printf("Failed to create semaphores or fences\n");
            exit(1);
        }
    }
}



void init_vulkan(SDL_Window* window, uint32_t width, uint32_t height) {
    VulkanContext* vkCtx = get_vulkan_context();
    vkCtx->width = width;
    vkCtx->height = height;

    // Create Vulkan instance
    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "Vulkan SDL3 with ImGui";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Get SDL3 Vulkan extensions
    Uint32 sdlExtensionCount = 0;
    const char *const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    // Add debug utils extension
    const char* additionalExtensions[] = {"VK_EXT_debug_utils"};
    uint32_t extensionCount = sdlExtensionCount + 1;
    const char** extensions = malloc(extensionCount * sizeof(const char*));
    for (uint32_t i = 0; i < sdlExtensionCount; i++) {
        extensions[i] = sdlExtensions[i];
    }
    extensions[sdlExtensionCount] = "VK_EXT_debug_utils";

    const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    uint32_t layerCount = 1;

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensionCount;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledLayerCount = layerCount;
    createInfo.ppEnabledLayerNames = validationLayers;

    if (vkCreateInstance(&createInfo, NULL, &vkCtx->instance) != VK_SUCCESS) {
        printf("Failed to create Vulkan instance\n");
        exit(1);
    }
    free(extensions);

    // Setup debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = vulkan_debug_callback_function;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkCtx->instance, "vkCreateDebugUtilsMessengerEXT");
    if (createDebugMessenger && createDebugMessenger(vkCtx->instance, &debugInfo, NULL, &vkCtx->debugMessenger) != VK_SUCCESS) {
        printf("Failed to set up debug messenger\n");
    }

    // Create surface
    if (!SDL_Vulkan_CreateSurface(window, vkCtx->instance, NULL, &vkCtx->surface)) {
        printf("Failed to create Vulkan surface: %s\n", SDL_GetError());
        exit(1);
    }

    // Pick physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vkCtx->instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(vkCtx->instance, &deviceCount, devices);
    vkCtx->physicalDevice = devices[0]; // Simplistic selection
    free(devices);

    // Find queue family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(vkCtx->physicalDevice, &queueFamilyCount, queueFamilies);

    uint32_t graphicsFamily = UINT32_MAX;
    VkBool32 surfaceSupport = VK_FALSE;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsFamily = i;
            vkGetPhysicalDeviceSurfaceSupportKHR(vkCtx->physicalDevice, i, vkCtx->surface, &surfaceSupport);
            if (surfaceSupport) break;
        }
    }
    free(queueFamilies);

    if (graphicsFamily == UINT32_MAX || !surfaceSupport) {
        printf("Failed to find suitable queue family\n");
        exit(1);
    }

    // Create logical device
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
    deviceCreateInfo.enabledLayerCount = layerCount;
    deviceCreateInfo.ppEnabledLayerNames = validationLayers;

    if (vkCreateDevice(vkCtx->physicalDevice, &deviceCreateInfo, NULL, &vkCtx->device) != VK_SUCCESS) {
        printf("Failed to create logical device\n");
        exit(1);
    }

    vkGetDeviceQueue(vkCtx->device, graphicsFamily, 0, &vkCtx->graphicsQueue);

    // Create swapchain
    VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchainInfo.surface = vkCtx->surface;
    swapchainInfo.minImageCount = 2;
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width = width;
    swapchainInfo.imageExtent.height = height;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(vkCtx->device, &swapchainInfo, NULL, &vkCtx->swapchain) != VK_SUCCESS) {
        printf("Failed to create swapchain\n");
        exit(1);
    }

    vkGetSwapchainImagesKHR(vkCtx->device, vkCtx->swapchain, &vkCtx->imageCount, NULL);
    vkCtx->swapchainImages = malloc(vkCtx->imageCount * sizeof(VkImage));
    vkGetSwapchainImagesKHR(vkCtx->device, vkCtx->swapchain, &vkCtx->imageCount, vkCtx->swapchainImages);

    // Create image views
    vkCtx->swapchainImageViews = malloc(vkCtx->imageCount * sizeof(VkImageView));
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = vkCtx->swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vkCtx->device, &viewInfo, NULL, &vkCtx->swapchainImageViews[i]) != VK_SUCCESS) {
            printf("Failed to create image view\n");
            exit(1);
        }
    }

    // Create render pass
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

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(vkCtx->device, &renderPassInfo, NULL, &vkCtx->renderPass) != VK_SUCCESS) {
        printf("Failed to create render pass\n");
        exit(1);
    }

    // Create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(vkCtx->device, &pipelineLayoutInfo, NULL, &vkCtx->pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(1);
    }

    // Create graphics pipeline
    VkShaderModule vertShaderModule, fragShaderModule;
    VkShaderModuleCreateInfo shaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    shaderInfo.codeSize = sizeof(triangle_vert_spv);
    shaderInfo.pCode = triangle_vert_spv;
    if (vkCreateShaderModule(vkCtx->device, &shaderInfo, NULL, &vertShaderModule) != VK_SUCCESS) {
        printf("Failed to create vertex shader module\n");
        exit(1);
    }
    shaderInfo.codeSize = sizeof(triangle_frag_spv);
    shaderInfo.pCode = triangle_frag_spv;
    if (vkCreateShaderModule(vkCtx->device, &shaderInfo, NULL, &fragShaderModule) != VK_SUCCESS) {
        printf("Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main", NULL},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main", NULL}
    };

    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = 6 * sizeof(float);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDesc[2] = {
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
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
    VkRect2D scissor = {{0, 0}, {width, height}};
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

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
    pipelineInfo.layout = vkCtx->pipelineLayout;
    pipelineInfo.renderPass = vkCtx->renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(vkCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx->graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }

    vkDestroyShaderModule(vkCtx->device, vertShaderModule, NULL);
    vkDestroyShaderModule(vkCtx->device, fragShaderModule, NULL);

    // Create framebuffers
    vkCtx->swapchainFramebuffers = malloc(vkCtx->imageCount * sizeof(VkFramebuffer));
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebufferInfo.renderPass = vkCtx->renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkCtx->swapchainImageViews[i];
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkCtx->device, &framebufferInfo, NULL, &vkCtx->swapchainFramebuffers[i]) != VK_SUCCESS) {
            printf("Failed to create framebuffer\n");
            exit(1);
        }
    }

    // Create command pool
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(vkCtx->device, &poolInfo, NULL, &vkCtx->commandPool) != VK_SUCCESS) {
        printf("Failed to create command pool\n");
        exit(1);
    }

    // Allocate command buffers (one per swapchain image)
    vkCtx->commandBuffers = malloc(vkCtx->imageCount * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = vkCtx->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = vkCtx->imageCount;
    if (vkAllocateCommandBuffers(vkCtx->device, &allocInfo, vkCtx->commandBuffers) != VK_SUCCESS) {
        printf("Failed to allocate command buffers\n");
        exit(1);
    }

    // Create semaphores and fences
    vkCtx->imageAvailableSemaphores = malloc(vkCtx->imageCount * sizeof(VkSemaphore));
    vkCtx->renderFinishedSemaphores = malloc(vkCtx->imageCount * sizeof(VkSemaphore));
    vkCtx->inFlightFences = malloc(vkCtx->imageCount * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        if (vkCreateSemaphore(vkCtx->device, &semaphoreInfo, NULL, &vkCtx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkCtx->device, &semaphoreInfo, NULL, &vkCtx->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkCtx->device, &fenceInfo, NULL, &vkCtx->inFlightFences[i]) != VK_SUCCESS) {
            printf("Failed to create semaphores or fences\n");
            exit(1);
        }
    }
}



void create_pipeline(void) {
    VulkanContext* vkCtx = get_vulkan_context();

    // Shader modules (unchanged)
    VkShaderModuleCreateInfo vertShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertShaderInfo.codeSize = sizeof(triangle_vert_spv);
    vertShaderInfo.pCode = triangle_vert_spv;
    VkShaderModule vertModule;
    if (vkCreateShaderModule(vkCtx->device, &vertShaderInfo, NULL, &vertModule) != VK_SUCCESS) {
        printf("Failed to create vertex shader module\n");
        exit(1);
    }

    VkShaderModuleCreateInfo fragShaderInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragShaderInfo.codeSize = sizeof(triangle_frag_spv);
    fragShaderInfo.pCode = triangle_frag_spv;
    VkShaderModule fragModule;
    if (vkCreateShaderModule(vkCtx->device, &fragShaderInfo, NULL, &fragModule) != VK_SUCCESS) {
        printf("Failed to create fragment shader module\n");
        exit(1);
    }

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_VERTEX_BIT, vertModule, "main", 0},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, NULL, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragModule, "main", 0}
    };

    // Vertex input (unchanged)
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

    // Input assembly (unchanged)
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Dynamic state
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Viewport state (no static viewport/scissor)
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1; // Must be 1 even if dynamic
    viewportState.scissorCount = 1;  // Must be 1 even if dynamic

    // Rasterization (unchanged)
    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.lineWidth = 1.0f;

    // Multisampling (unchanged)
    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending (unchanged)
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Pipeline layout (unchanged)
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if (vkCreatePipelineLayout(vkCtx->device, &pipelineLayoutInfo, NULL, &vkCtx->pipelineLayout) != VK_SUCCESS) {
        printf("Failed to create pipeline layout\n");
        exit(1);
    }

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = vkCtx->pipelineLayout;
    pipelineInfo.renderPass = vkCtx->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.pDynamicState = &dynamicState; // Add dynamic state

    if (vkCreateGraphicsPipelines(vkCtx->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &vkCtx->graphicsPipeline) != VK_SUCCESS) {
        printf("Failed to create graphics pipeline\n");
        exit(1);
    }

    // Clean up shader modules
    vkDestroyShaderModule(vkCtx->device, fragModule, NULL);
    vkDestroyShaderModule(vkCtx->device, vertModule, NULL);
}


// void record_command_buffer(uint32_t imageIndex) {
//     VulkanContext* vkCtx = get_vulkan_context();
//     VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
//     beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//     if (vkBeginCommandBuffer(vkCtx->commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
//         printf("Failed to begin command buffer\n");
//         exit(1);
//     }
//     VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
//     renderPassInfo.renderPass = vkCtx->renderPass;
//     renderPassInfo.framebuffer = vkCtx->swapchainFramebuffers[imageIndex];
//     renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
//     renderPassInfo.renderArea.extent = (VkExtent2D){vkCtx->width, vkCtx->height};
//     VkClearValue clearColor = {{{0.5f, 0.5f, 0.5f, 1.0f}}};
//     renderPassInfo.clearValueCount = 1;
//     renderPassInfo.pClearValues = &clearColor;
//     vkCmdBeginRenderPass(vkCtx->commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
// }



//vulkan render begin
void vulkan_begin_render(uint32_t imageIndex) {
    VulkanContext* vkCtx = get_vulkan_context();

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = 0; // Remove VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    if (vkBeginCommandBuffer(vkCtx->commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        printf("Failed to begin command buffer\n");
        exit(1);
    }

    // Begin render pass
    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = vkCtx->renderPass;
    renderPassInfo.framebuffer = vkCtx->swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderArea.extent = (VkExtent2D){vkCtx->width, vkCtx->height};
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(vkCtx->commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor
    VkViewport viewport = {0.0f, 0.0f, (float)vkCtx->width, (float)vkCtx->height, 0.0f, 1.0f};
    vkCmdSetViewport(vkCtx->commandBuffers[imageIndex], 0, 1, &viewport);

    VkRect2D scissor = {{0, 0}, {vkCtx->width, vkCtx->height}};
    vkCmdSetScissor(vkCtx->commandBuffers[imageIndex], 0, 1, &scissor);
}


//vulkan render end
void vulkan_end_render(uint32_t imageIndex, uint32_t semaphoreIndex) {
    VulkanContext* vkCtx = get_vulkan_context();

    // End render pass
    vkCmdEndRenderPass(vkCtx->commandBuffers[imageIndex]);

    // End command buffer
    if (vkEndCommandBuffer(vkCtx->commandBuffers[imageIndex]) != VK_SUCCESS) {
        printf("Failed to end command buffer\n");
        exit(1);
    }

    // Submit command buffer
    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkSemaphore waitSemaphores[] = {vkCtx->imageAvailableSemaphores[semaphoreIndex]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkCtx->commandBuffers[imageIndex];
    VkSemaphore signalSemaphores[] = {vkCtx->renderFinishedSemaphores[semaphoreIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vkCtx->graphicsQueue, 1, &submitInfo, vkCtx->inFlightFences[imageIndex]) != VK_SUCCESS) {
        printf("Failed to submit draw command buffer\n");
        exit(1);
    }

    // Present the image
    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkCtx->swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VkResult result = vkQueuePresentKHR(vkCtx->graphicsQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Handle swapchain recreation in the main loop
    } else if (result != VK_SUCCESS) {
        printf("Failed to present image: %d\n", result);
        exit(1);
    }
}



// vulkan clean up
void cleanup_vulkan(void) {
    VulkanContext* vkCtx = get_vulkan_context();

    vkDeviceWaitIdle(vkCtx->device);

    // Destroy ImGui descriptor pool
    if (vkCtx->imguiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vkCtx->device, vkCtx->imguiDescriptorPool, NULL);
        vkCtx->imguiDescriptorPool = VK_NULL_HANDLE;
    }

    // Destroy synchronization objects
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        if (vkCtx->imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(vkCtx->device, vkCtx->imageAvailableSemaphores[i], NULL);
        }
        if (vkCtx->renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(vkCtx->device, vkCtx->renderFinishedSemaphores[i], NULL);
        }
        if (vkCtx->inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(vkCtx->device, vkCtx->inFlightFences[i], NULL);
        }
    }
    free(vkCtx->imageAvailableSemaphores);
    free(vkCtx->renderFinishedSemaphores);
    free(vkCtx->inFlightFences);

    // Destroy command buffers
    if (vkCtx->commandBuffers != NULL) {
        vkFreeCommandBuffers(vkCtx->device, vkCtx->commandPool, vkCtx->imageCount, vkCtx->commandBuffers);
        free(vkCtx->commandBuffers);
    }

    // Destroy command pool
    if (vkCtx->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(vkCtx->device, vkCtx->commandPool, NULL);
    }

    // Destroy vertex and index buffers
    if (vkCtx->vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkCtx->device, vkCtx->vertexBuffer, NULL);
    }
    if (vkCtx->vertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkCtx->device, vkCtx->vertexMemory, NULL);
    }
    if (vkCtx->quadBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkCtx->device, vkCtx->quadBuffer, NULL);
    }
    if (vkCtx->quadMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkCtx->device, vkCtx->quadMemory, NULL);
    }
    if (vkCtx->quadIndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkCtx->device, vkCtx->quadIndexBuffer, NULL);
    }
    if (vkCtx->quadIndexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkCtx->device, vkCtx->quadIndexMemory, NULL);
    }

    // Destroy swapchain resources
    for (uint32_t i = 0; i < vkCtx->imageCount; i++) {
        if (vkCtx->swapchainFramebuffers[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(vkCtx->device, vkCtx->swapchainFramebuffers[i], NULL);
        }
        if (vkCtx->swapchainImageViews[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(vkCtx->device, vkCtx->swapchainImageViews[i], NULL);
        }
    }
    free(vkCtx->swapchainFramebuffers);
    free(vkCtx->swapchainImageViews);
    free(vkCtx->swapchainImages);

    if (vkCtx->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vkCtx->device, vkCtx->swapchain, NULL);
    }

    // Destroy pipeline
    if (vkCtx->graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkCtx->device, vkCtx->graphicsPipeline, NULL);
    }
    if (vkCtx->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vkCtx->device, vkCtx->pipelineLayout, NULL);
    }
    if (vkCtx->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(vkCtx->device, vkCtx->renderPass, NULL);
    }

    // Destroy device
    if (vkCtx->device != VK_NULL_HANDLE) {
        vkDestroyDevice(vkCtx->device, NULL);
    }

    // Destroy surface
    if (vkCtx->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(vkCtx->instance, vkCtx->surface, NULL);
    }

    // Destroy debug messenger
    if (vkCtx->debugMessenger != VK_NULL_HANDLE) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vkCtx->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebugMessenger) {
            destroyDebugMessenger(vkCtx->instance, vkCtx->debugMessenger, NULL);
        }
    }

    // Destroy instance
    if (vkCtx->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(vkCtx->instance, NULL);
    }

    free(vkCtx);
}


