#include "gpu.h"

#include <SDL3/SDL.h>

VkSwapchainKHR createswapchain(struct selectdeviceret device, VkSurfaceKHR surface) {
  VkSwapchainCreateInfoKHR createinfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,0};

  // Going to assume that the first format returned is the ideal one (and it does give the ideal one, atleast on my linux RTX 2050 590.44.01)
  VkSurfaceFormatKHR idealformat;
  unsigned int count = 1;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicaldevice, surface, &count, &idealformat);

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicaldevice, surface, &capabilities);

  createinfo.surface = surface;
  createinfo.minImageCount = 2;
  createinfo.imageFormat = idealformat.format;
  createinfo.imageColorSpace = idealformat.colorSpace;
  createinfo.imageExtent = capabilities.currentExtent;
  createinfo.imageArrayLayers = 1;
  createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createinfo.queueFamilyIndexCount = 1;
  uint32_t queuefamily = 1; // only one queue family is used at the moment
  createinfo.pQueueFamilyIndices = &queuefamily;
  createinfo.preTransform = capabilities.currentTransform;
  createinfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  createinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createinfo.clipped = VK_TRUE;

  VkSwapchainKHR swapchain;
  vkCreateSwapchainKHR(device.device, &createinfo, NULL, &swapchain);
}