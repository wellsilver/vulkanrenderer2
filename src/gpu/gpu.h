#include <vulkan/vulkan.h>

// The selectdevice function needs to return multiple values so the physical device can be queried for compatible items
struct selectdeviceret {
  VkDevice device;
  VkPhysicalDevice physicaldevice;
};

// Can return NULL, select a supported device
struct selectdeviceret selectdevice(VkInstance instance);

struct swapchainandformat {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR format;
};

struct swapchainandformat createswapchain(struct selectdeviceret device, VkSurfaceKHR surface);

VkPipeline creategraphicspipeline(VkDevice device, struct swapchainandformat);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};