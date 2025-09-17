


To use the Vulkan context in another module, include vulkan_module.h and call get_vulkan_context() to access the VulkanContext. For example:
```
#include "vulkan_module.h"

void some_other_module_function(void) {
    VulkanContext* vkCtx = get_vulkan_context();
    // Use vkCtx->device, vkCtx->graphicsQueue, etc.
}
```

To add ImGui functionality in another module, include imgui_module.h and call ImGui functions via cimgui. For example:
```
#include "imgui_module.h"
#include "cimgui.h"

void custom_ui(void) {
    igBegin("Custom Window", NULL, 0);
    igText("Hello from another module!");
    igEnd();
}
```