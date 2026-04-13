#include "gpu.h"

#include <SDL3/SDL.h>

struct swapchainandformat createswapchain(struct selectdeviceret device, VkSurfaceKHR surface) {
  VkResult err;

  struct swapchainandformat ret = {0};

  // Going to assume that the first format returned is the ideal one (and it does give the ideal one, atleast on my linux RTX 2050 590.44.01)
  VkSurfaceFormatKHR idealformat;
  unsigned int count = 1;
  err = vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicaldevice, surface, &count, &idealformat);
  if (err < VK_SUCCESS) { // This function can return a greater than 0 value in success
    SDL_Log("vkGetPhysicalDeviceSurfaceFormatsKHR Failed - %i\n", err);
    return ret;
  }

  //SDL_Log("Begin\n");
  //SDL_Log("%p, %p\n", device.physicaldevice, surface);
  VkSurfaceCapabilitiesKHR capabilities;
  err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicaldevice, surface, &capabilities);
  //SDL_Log("Done\n");
  if (err != VK_SUCCESS) {
    SDL_Log("vkGetPhysicalDeviceSurfaceCapabilitiesKHR Failed - %i\n", err);
    return ret;
  }
  
  VkSwapchainCreateInfoKHR createinfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,0};

  createinfo.surface = surface;
  createinfo.minImageCount = 3;
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
  err = vkCreateSwapchainKHR(device.device, &createinfo, NULL, &swapchain);
  if (err != VK_SUCCESS) {
    SDL_Log("vkCreateSwapchainKHR Failed - %i\n", err);
    return ret;
  }
  return (struct swapchainandformat) {.format = idealformat,.swapchain = swapchain};
}

struct imageview *createimageviews(struct selectdeviceret device, struct swapchainandformat swappy) {
  // Retrieve the images
  uint32_t swapchainimagecount;
  vkGetSwapchainImagesKHR(device.device, swappy.swapchain, &swapchainimagecount, NULL);
  VkImage imagebuffer[swapchainimagecount];
  struct imageview *ret = SDL_malloc(sizeof(struct imageview)*(swapchainimagecount));
  vkGetSwapchainImagesKHR(device.device, swappy.swapchain, &swapchainimagecount, imagebuffer);

  for (unsigned int loop=0;loop<swapchainimagecount;loop++) {
    ret[loop].image = imagebuffer[loop];
    
    VkResult err = vkCreateImageView(device.device, &(VkImageViewCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = ret[loop].image,
      .format = swappy.format.format,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .components = (VkComponentMapping) {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1
    }, NULL, &ret[loop].view);
    if (err != VK_SUCCESS) {
      SDL_Log("vkCreateImageView Failed - %i\n", err);
      SDL_free(ret);
      return NULL;
    }

    vkCreateImage(device.device, &(VkImageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .samples = VK_SAMPLE_COUNT_4_BIT,
      .format = swappy.format.format,
      .extent = (VkExtent3D) {.width=720,.height=720, .depth=1},
      .imageType = VK_IMAGE_TYPE_2D,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .arrayLayers = 1,
      .mipLevels = 1,
      .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL
    }, NULL, &ret[loop].sampled);

    VkMemoryRequirements memrequirements;
    vkGetImageMemoryRequirements(device.device, ret[loop].sampled, &memrequirements); 
    
    vkAllocateMemory(device.device, &(VkMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memrequirements.size,
      .memoryTypeIndex = 0,
    }, NULL, &ret[loop].sampledmemory);
    vkBindImageMemory(device.device, ret[loop].sampled, ret[loop].sampledmemory, 0);

    vkCreateImageView(device.device, &(VkImageViewCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = ret[loop].sampled,
      .format = swappy.format.format,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .components = (VkComponentMapping) {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1
    }, NULL, &ret[loop].sampledview);

    vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    }, NULL, &ret[loop].finished);

    ret[loop].length = swapchainimagecount;
  }

  return ret;
}

void releaseimageviews(struct selectdeviceret device, struct imageview *images) {
  for (unsigned int loop=0;loop<3;loop++) {
    // swapchain
    vkDestroyImageView(device.device, images[loop].view, NULL);
    vkDestroySemaphore(device.device, images[loop].finished, NULL);
    // Sampled images
    vkDestroyImage(device.device, images[loop].sampled, NULL);
    vkDestroyImageView(device.device, images[loop].sampledview, NULL);
    vkFreeMemory(device.device, images[loop].sampledmemory, NULL);
  }
  SDL_free(images);
}