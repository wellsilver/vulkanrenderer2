#include "gpu.h"

#include <SDL3/SDL.h>

struct imageview *createimageviews(struct selectdeviceret device, struct swapchainandformat swappy) {
  // Retrieve the images
  uint32_t swapchainimagecount;
  vkGetSwapchainImagesKHR(device.device, swappy.swapchain, &swapchainimagecount, NULL);
  VkImage imagebuffer[swapchainimagecount];
  struct imageview *ret = SDL_malloc(sizeof(struct imageview)*(swapchainimagecount+1));
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
  }

  ret[swapchainimagecount+1].image = NULL;

  return ret;
}

void releaseimageviews(struct selectdeviceret device, struct imageview *images) {
  for (unsigned int loop=0;images[loop].image != NULL;loop++) {
    vkDestroyImageView(device.device, images[loop].view, NULL);
  }
  SDL_free(images);
}