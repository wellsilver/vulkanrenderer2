#include <vulkan/vulkan.h>

#include "device.h"

struct imageview {
  VkImage image;
  VkImageView view;
  VkImage sampled;
  VkDeviceMemory sampledmemory;
  VkImageView sampledview;
  VkSemaphore finished;
  unsigned int length;
};

struct swapchainandformat {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR format;
};

struct swapchainandformat createswapchain(struct selectdeviceret device, VkSurfaceKHR surface);

struct imageview *createimageviews(struct selectdeviceret device, struct swapchainandformat swappy, uint32_t width, uint32_t height);
void releaseimageviews(struct selectdeviceret device, struct imageview *images);