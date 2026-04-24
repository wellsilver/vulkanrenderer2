#include <vulkan/vulkan.h>

// The selectdevice function needs to return multiple values so the physical device can be queried for compatible items
struct selectdeviceret {
  VkDevice device;
  VkPhysicalDevice physicaldevice;
  VkQueue queue;
};

struct imageview {
  VkImage image;
  VkImageView view;
  VkImage sampled;
  VkDeviceMemory sampledmemory;
  VkImageView sampledview;
  VkSemaphore finished;
  unsigned int length;
};

// Can return NULL, select a supported device
struct selectdeviceret selectdevice(VkInstance instance);

struct swapchainandformat {
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR format;
};

struct vertice {
  float x,y,z;
  float r,g,b;
};

struct swapchainandformat createswapchain(struct selectdeviceret device, VkSurfaceKHR surface);

VkPipeline creategraphicspipeline(VkDevice device, VkFormat swapchainformat);

struct imageview *createimageviews(struct selectdeviceret device, struct swapchainandformat swappy);
void releaseimageviews(struct selectdeviceret device,struct imageview *images);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};