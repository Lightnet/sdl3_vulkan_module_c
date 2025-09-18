




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