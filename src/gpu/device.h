#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

// The selectdevice function needs to return multiple values so the physical device can be queried for compatible items
struct selectdeviceret {
  VkDevice device;
  VkPhysicalDevice physicaldevice;
  VkQueue queue;
  VmaAllocator allocator;
};

// Can return NULL, select a supported device
struct selectdeviceret selectdevice(VkInstance instance);