#include "device.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

struct idealdeviceRet {
  VkPhysicalDevice pick;
  char *name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
  uint64_t pick_memsize;
  unsigned int pick_queueindex;
};

// Can return NULL, find the supported device with the most memory and return it along with queue family
struct idealdeviceRet idealdevice(VkInstance instance) {
  // Get all devices
  uint32_t devicelen;
  vkEnumeratePhysicalDevices(instance, &devicelen, NULL);
  VkPhysicalDevice devices[devicelen];
  vkEnumeratePhysicalDevices(instance, &devicelen, devices);
  
  struct idealdeviceRet ret;
  ret.pick = NULL;
  ret.pick_memsize = 0;

  for (unsigned int idevice=0;idevice<devicelen;idevice++) {
    // Get the queue families
    uint32_t queuelen;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[idevice], &queuelen, NULL);
    VkQueueFamilyProperties queues[queuelen];
    vkGetPhysicalDeviceQueueFamilyProperties(devices[idevice], &queuelen, queues);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(devices[idevice], &properties);

    uint64_t memsize = 0;
    // Get memory size
    VkPhysicalDeviceMemoryProperties memproperties;
    vkGetPhysicalDeviceMemoryProperties(devices[idevice], &memproperties);
    for (unsigned int heap=0;heap<memproperties.memoryTypeCount;heap++) {
      if (memproperties.memoryTypes[heap].propertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        memsize = memproperties.memoryHeaps[memproperties.memoryTypes[heap].heapIndex].size;
        heap = memproperties.memoryTypeCount; continue; // skip the rest of the entries, this is duplicated.
      }
    }

    // Check for support
    for (unsigned int iqueue=0;iqueue<queuelen;iqueue++) {
      if (queues[iqueue].queueFlags & VK_QUEUE_COMPUTE_BIT && SDL_Vulkan_GetPresentationSupport(instance, devices[idevice], iqueue)) {
        if (ret.pick_memsize < memsize) { // Select the device if it has a supported queue family and greatest local memory size, and the first queue within that device
          ret.pick_memsize = memsize;
          ret.pick = devices[idevice];
          ret.pick_queueindex = iqueue;
          SDL_memcpy(ret.name, properties.deviceName, SDL_strlen(ret.name));
        }
      }
    }
  }

  return ret;
}

VkDevice selectdevice(VkInstance instance) {
  struct idealdeviceRet device = idealdevice(instance);
  if (device.pick == NULL) return NULL;

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Device selected: %s %lu\n", device.name, device.pick_memsize);

  const float one = 1.0f;

  VkDeviceQueueCreateInfo queuecreateinfo = {
    .flags = 0,
    .pNext = 0,
    .pQueuePriorities = &one,
    .queueCount = 1,
    .queueFamilyIndex = device.pick_queueindex,
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
  };

  VkDeviceCreateInfo createinfo = {
    .enabledExtensionCount = 0,
    .enabledLayerCount = 0,
    .flags = 0,
    .pEnabledFeatures = 0,
    .pNext = 0,
    .ppEnabledExtensionNames = 0,
    .ppEnabledLayerNames = 0,
    .pQueueCreateInfos = &queuecreateinfo,
    .queueCreateInfoCount = 1,
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
  };

  VkDevice ret;
  VkResult err = vkCreateDevice(device.pick, &createinfo, NULL, &ret);
  if (err != VK_SUCCESS) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "vkCreateDevice %i\n", err);
    return NULL;
  }
  
  return ret;
}