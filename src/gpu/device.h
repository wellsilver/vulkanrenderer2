#include <vulkan/vulkan.h>

// The selectdevice function needs to return multiple values so the physical device can be queried for compatible items
struct selectdeviceret {
  VkDevice device;
  VkPhysicalDevice physicaldevice;
  VkQueue queue;
};

// Can return NULL, select a supported device
struct selectdeviceret selectdevice(VkInstance instance);