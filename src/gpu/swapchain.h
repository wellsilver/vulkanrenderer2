#include <vulkan/vulkan.h>

struct swapchainimage {
  VkImage image;
  VkImageView imageview;
  VkSemaphore framefinishSem;
  VkFence framefinishFence;
  VkSemaphore frameimageready;
  unsigned int fenceactivated;
};

// Get images from swapchain
void swapchainGetImages(VkDevice device, VkSwapchainKHR swapchain, uint32_t swapchainimagecount, struct swapchainimage *retimages, VkFormat format);
// Destroy a swapchain
void swapchainClean(VkDevice device, struct swapchainimage *images, VkSwapchainKHR swapchain, uint32_t swapchainimagecount);

/// Inlines

// Create swapchain
static inline VkSwapchainKHR swapchainCreate(VkDevice device, VkSurfaceKHR windowsurface, VkColorSpaceKHR colorspace, VkExtent2D extent, VkFormat surfaceformat, uint32_t minimagecount) {
  VkResult err;

  VkSwapchainKHR swapchain;
  err = vkCreateSwapchainKHR(device, &(VkSwapchainCreateInfoKHR) {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .clipped = VK_FALSE,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .flags = 0,
    .imageArrayLayers = 1,
    .imageColorSpace = colorspace,
    .imageExtent = extent,
    .imageFormat = surfaceformat,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
    .minImageCount = minimagecount,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &(uint32_t) {0},
    .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .surface = windowsurface,
  }, NULL, &swapchain);

  return swapchain;
}

// Get how many images are in the swapchain
static inline uint32_t swapchainGetImageCount(VkSwapchainKHR swapchain, VkDevice device) {
  uint32_t swapchainimagecount;
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainimagecount, NULL);
  
  return swapchainimagecount;
}