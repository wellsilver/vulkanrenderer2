#include <vulkan/vulkan.h>
#include "swapchain.h"

// Other functions are inlined and in swapchain.h

// Get swapchain images
void swapchainGetImages(VkDevice device, VkSwapchainKHR swapchain, uint32_t swapchainimagecount, struct swapchainimage *retimages, VkFormat format) {
  VkImage swapchainimages[swapchainimagecount];
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainimagecount, swapchainimages);

  for (unsigned int loop=0;loop<swapchainimagecount;loop++) {
    retimages[loop].image = swapchainimages[loop];

    vkCreateImageView(device, &(VkImageViewCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      // .components all is 0, leave it be
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .components = (VkComponentMapping) {0,0,0,0},
      .flags = 0,
      .format = format,
      .image = swapchainimages[loop],
      .subresourceRange = (VkImageSubresourceRange) {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1
      }
    }, NULL, &retimages[loop].imageview);
    vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    }, NULL, &retimages[loop].frameimageready);
    vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    }, NULL, &retimages[loop].framefinishSem);
    vkCreateFence(device, &(VkFenceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &retimages[loop].framefinishFence);
    retimages[loop].fenceactivated = 0;
  }
}

void swapchainClean(VkDevice device, struct swapchainimage *images, VkSwapchainKHR swapchain, uint32_t swapchainimagecount) {
  for (unsigned int loop=0;loop<swapchainimagecount;loop++) {
    vkDestroyImageView(device, images[loop].imageview, NULL);
    vkDestroySemaphore(device, images[loop].frameimageready, NULL);
    vkDestroySemaphore(device, images[loop].framefinishSem, NULL);
    vkDestroyFence(device, images[loop].framefinishFence, NULL);
  }
  vkDestroySwapchainKHR(device, swapchain, NULL);
}