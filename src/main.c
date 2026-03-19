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

void recordcommandbuffer(VkCommandBuffer buffer, struct selectdeviceret device, struct swapchainandformat swapchain, VkPipeline graphicspipeline, struct imageview *images, VkSemaphore imagesem, uint32_t *imageindex, int width, int height) {
  vkBeginCommandBuffer(buffer, &(VkCommandBufferBeginInfo) {.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,.flags = 0});

  vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 1, &(VkImageMemoryBarrier) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .image = images[*imageindex].image,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
  });

  vkCmdBeginRendering(buffer, &(VkRenderingInfo) {.sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
    .renderArea=(VkRect2D) {.extent=(VkExtent2D){.width=width,.height=height},.offset=(VkOffset2D) {.x=0,.y=0}},
    .layerCount=1,
    .colorAttachmentCount=1,
    .pColorAttachments=&(VkRenderingAttachmentInfo) {.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .imageView = images[*imageindex].view,
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    },
  });

  vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline);

  vkCmdSetViewport(buffer, 0, 1, &(VkViewport) {.width = width,.height=height,.x=0,.y=0,.minDepth=0.0f,.maxDepth=1.0f});
  vkCmdSetScissor(buffer, 0, 1, &(VkRect2D) {.extent=(VkExtent2D){.width=width,.height=height},.offset=(VkOffset2D) {.x=0,.y=0}});

  vkCmdDraw(buffer, 3, 1, 0, 0);

  vkCmdEndRendering(buffer);

  vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, 0, 0, 0, 1, &(VkImageMemoryBarrier) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .image = images[*imageindex].image,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel = 0,
    .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0,
    .subresourceRange.layerCount = 1,
  });

  vkEndCommandBuffer(buffer);
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }

  VkInstance instance = makeinstance();
  if (instance == 0) return 2;

  SDL_Window *window = SDL_CreateWindow("Space game", 480, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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
    SDL_Log("Could not create VkPipeline (graphics) %s\n", SDL_GetError());
    return 7;
  }

  struct imageview *images = createimageviews(device, swapchain);
  if (images == NULL) {
    SDL_Log("Could not create imageviews\n");
    return 7;
  }

  VkCommandPool pool;
  vkCreateCommandPool(device.device, &(struct VkCommandPoolCreateInfo) {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT}, NULL, &pool);
  
  VkCommandBuffer buffer;
  vkAllocateCommandBuffers(device.device, &(VkCommandBufferAllocateInfo) {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.commandBufferCount=1,.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY}, &buffer);

  VkSemaphore finishedrender; // triggered when rendering is finished.
  vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  }, NULL, &finishedrender);

  VkSemaphore imagesem;
  vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  }, NULL, &imagesem);

  bool active = true;
  SDL_Event currentevent;
  int width=480, height=480;
  SDL_GetWindowSizeInPixels(window, &width, &height);

  while (active) {
    uint32_t imageindex;
    
    // Get the image for rendering
    VkResult err = vkAcquireNextImageKHR(device.device, swapchain.swapchain, 0, imagesem, NULL, &imageindex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR) {
      releaseimageviews(device, images);
      vkDestroySwapchainKHR(device.device, swapchain.swapchain, NULL);
      swapchain = createswapchain(device, windowsurface);
      SDL_GetWindowSizeInPixels(window, &width, &height);
      if (swapchain.swapchain == NULL) {
        SDL_Log("Couldnt recreate swapchain\n");
        return 7;
      }
      images = createimageviews(device, swapchain);
      if (images == NULL) {
        SDL_Log("Couldnt recreate imageviews\n");
        return 7;
      }
      continue;
    }

    vkResetCommandBuffer(buffer, 0);
    recordcommandbuffer(buffer, device, swapchain, graphicspipeline, images, imagesem, &imageindex, width, height);

    vkQueueSubmit(device.queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &buffer,
      .waitSemaphoreCount=1,
      .pWaitSemaphores = &imagesem,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &finishedrender,
      .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    }, NULL);
    
    vkQueuePresentKHR(device.queue, &(VkPresentInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &finishedrender,
      .swapchainCount = 1,
      .pSwapchains = &swapchain.swapchain,
      .pImageIndices = &imageindex,
    });

    vkQueueWaitIdle(device.queue);
    
    while (SDL_PollEvent(&currentevent)) {
      if (currentevent.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) active = false;
    }
  }


  releaseimageviews(device, images);
  vkFreeCommandBuffers(device.device, pool, 1, &buffer);
  vkDestroyCommandPool(device.device, pool, NULL);
  vkDestroySemaphore(device.device, imagesem, NULL);
  vkDestroySemaphore(device.device, finishedrender, NULL);
  vkDestroySwapchainKHR(device.device, swapchain.swapchain, NULL);
  vkDestroyPipeline(device.device, graphicspipeline, NULL);
  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}