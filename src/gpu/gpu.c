#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "common.h"
#include "device.h"
#include "gpu.h"

VkInstance makeinstance() {
  VkInstance ret;

  VkApplicationInfo appinfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "Space game",
    .applicationVersion = 1,
    .pEngineName = "wellsilver_VURender2",
    .engineVersion = 0,
    .apiVersion = VK_API_VERSION_1_4, // TODO Update this to whatever api version the extensions are..
  };

  unsigned int count;
  const char *const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);
  const char *layers = "VK_LAYER_KHRONOS_validation";

  VkInstanceCreateInfo instanceinfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .pApplicationInfo = &appinfo,
    .enabledLayerCount = 1,
    .ppEnabledLayerNames = &layers,
    .enabledExtensionCount = count,
    .ppEnabledExtensionNames = extensions,
  };
  
  VkResult err = vkCreateInstance(&instanceinfo, NULL, &ret);
  if (err != VK_SUCCESS) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "vkCreateInstance failed %i\n", err);
    return 0;
  }
  return ret;
}

struct graphicSettings {

};

/*

*/
void graphics3D(VkSurfaceKHR windowsurface, struct selectdeviceret device, int *active, struct graphicSettings *settings) {
  VkResult err;
  
  // This is assuming the first format is the best format (which it is on every tested implementation)
  VkSurfaceFormatKHR surfaceformat;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicaldevice, windowsurface, &(uint32_t) {1}, &surfaceformat);
  VkSurfaceCapabilitiesKHR surfacecapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicaldevice, windowsurface, &surfacecapabilities);

  VkSwapchainKHR swapchain;
  if (VK_SUCCESS != vkCreateSwapchainKHR(device.device, &(VkSwapchainCreateInfoKHR) {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .clipped = VK_FALSE,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .flags = 0,
    .imageArrayLayers = 1,
    .imageColorSpace = surfaceformat.colorSpace,
    .imageExtent = surfacecapabilities.currentExtent,
    .imageFormat = surfaceformat.format,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .minImageCount = surfacecapabilities.minImageCount,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &(uint32_t) {0},
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .surface = windowsurface,
  }, NULL, &swapchain)) {
    SDL_LogError(SDL_LOG_CATEGORY_GPU, "vkCreateSwapchainKHR Error - %i\n", err);
    *active = 0;
    return; 
  }

  *active = 0;
  vkDestroySwapchainKHR(device.device, swapchain, NULL);
}

/*
GPU Thread entry point
setup vulkan and buffers transparently then start render
*/
int gpu(struct gpu_threadarguments *args) {
  VkInstance instance = makeinstance();
  if (instance == 0) return 1;

  VkSurfaceKHR windowsurface;
  if (!SDL_Vulkan_CreateSurface(args->window, instance, NULL, &windowsurface)) {
    SDL_LogError(SDL_LOG_CATEGORY_GPU, "Could not create VkSurface %s\n", SDL_GetError());
    return 2;
  }

  struct selectdeviceret device = selectdevice(instance);
  if (device.device == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_GPU, "Cannot find a Vulkan device\nRequirements:\n Atleast Vulkan 1.4, with a graphic+compute+present queue)\n");
    return 3;
  }

  struct graphicSettings settings;

  while (*args->active)
    graphics3D(windowsurface, device, args->active, &settings);


  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
}