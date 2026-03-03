#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "gpu/gpu.h"

VkInstance makeinstance() {
  VkInstance ret;

  VkApplicationInfo appinfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "Space game",
    .applicationVersion = 1,
    .pEngineName = "",
    .engineVersion = 0,
    .apiVersion = VK_API_VERSION_1_3, // TODO Update this to whatever api version the extensions are.. VK_KHR_DYNAMIC_RENDERING is default (and doesnt even suffix with khr) on 1.3
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

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }

  VkInstance instance = makeinstance();
  if (instance == 0) return 2;

  SDL_Window *window = SDL_CreateWindow("Space game", 480, 480, SDL_WINDOW_VULKAN);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create window %s\n", SDL_GetError());
    return 3;
  }

  VkSurfaceKHR windowsurface;
  if (!SDL_Vulkan_CreateSurface(window, instance, NULL, &windowsurface)) {
    SDL_Log("Could not create VkSurface %s\n", SDL_GetError());
    return 5;
  }

  struct selectdeviceret device = selectdevice(instance);
  if (device.device == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot find a Vulkan device\n Requirements:\nAtleast Vulkan 1.3, with a graphic+compute+present queue)\n");
    return 4;
  }

  struct swapchainandformat swapchain = createswapchain(device, windowsurface);
  if (swapchain.swapchain == NULL) {
    SDL_Log("Could not create VkSwapchainKHR %s\n", SDL_GetError());
    return 6;
  }

  VkPipeline graphicspipeline = creategraphicspipeline(device.device, swapchain);
  if (graphicspipeline == NULL) {
    SDL_Log("Could not create VkPipeline %s\n", SDL_GetError());
    return 7;
  }

  SDL_DestroyWindow(window);
  vkDestroySwapchainKHR(device.device, swapchain.swapchain, NULL);
  vkDestroyPipeline(device.device, graphicspipeline, NULL);
  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);

  SDL_Quit();
  return 0;
}