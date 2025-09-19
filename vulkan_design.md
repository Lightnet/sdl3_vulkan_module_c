# Design:
  Review design and logic of vulkan.

 Well doing some sample build to understand some logic code for understand how code works. To break down the code to see where it start and end for setup and render.

# vulkan context:
```c
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
    VkBuffer vertexBuffer;                          // For triangle
    VkDeviceMemory vertexMemory;
    VkBuffer quadBuffer;                            // For quad vertices
    VkDeviceMemory quadMemory;
    VkBuffer quadIndexBuffer;                       // For quad indices
    VkDeviceMemory quadIndexMemory;
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    uint32_t imageCount;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkFramebuffer* swapchainFramebuffers;
    VkDescriptorPool imguiDescriptorPool;           // imgui
    uint32_t width;
    uint32_t height;
} VulkanContext;
```

# vulkan set up reference.
 - window set up
 - graphic setup

## graphic:
```
- vkCreateInstance
- SDL_Vulkan_CreateSurface
- vkEnumeratePhysicalDevices
- vkGetPhysicalDeviceQueueFamilyProperties
- vkCreateDevice
- vkCreateSwapchainKHR
- vkCreateRenderPass
- vkCreateCommandPool
- vkAllocateCommandBuffers
- vkCreateSemaphore
```

## pipeline:
  Required shader to render object. It can be empty with minimal shader. Else it will fail to start render screen.
```
- vkCreateShaderModule
- vkCreateShaderModule
- Vertex input configuration
- Set dynamic states for viewport and scissor
- Viewport state with dynamic viewport and scissor
- vkCreatePipelineLayout
- vkCreateGraphicsPipelines
- vkDestroyShaderModule
- vkDestroyShaderModule

```
## command buffer:

```
- vkBeginCommandBuffer
- vkCmdBeginRenderPass
- vkCmdSetViewport
- vkCmdSetScissor
- //draw triangle
- // Draw text
- vkCmdEndRenderPass
- vkEndCommandBuffer
```

## recreate_swapchain:

```
- vkDeviceWaitIdle
- imageCount
  - vkDestroyFramebuffer
  - vkDestroyImageView
- 
- SDL_GetWindowSize
- 
- vkCreateSwapchainKHR
- vkDestroySwapchainKHR
- vkGetSwapchainImagesKHR
- vkGetSwapchainImagesKHR
- imageCount
  - vkCreateImageView
- imageCount
  - vkCreateFramebuffer

```


## render:

```
- imageIndex
  - vkAcquireNextImageKHR

- vkResetCommandBuffer
- record_command_buffer

- vkQueueSubmit
- vkQueuePresentKHR
- if recreate_swapchain
- 

```