#include <vulkan/vulkan.h>

// The selectdevice function needs to return multiple values so the physical device can be queried for compatible items
struct selectdeviceret {
  VkDevice device;
  VkPhysicalDevice physicaldevice;
};

// Can return NULL, select a supported device
struct selectdeviceret selectdevice(VkInstance instance);

VkSwapchainKHR createswapchain(struct selectdeviceret device, VkSurfaceKHR surface);

VkPipeline creategraphicspipeline(VkDevice device);

static const char vertexcode[] = {
#embed "../../out/vertex.spv"
};

static const char fragmentcode[] = {
#embed "../../out/fragment.spv"
};