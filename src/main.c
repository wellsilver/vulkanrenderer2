#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "gpu/gpu.h"

#define FRAMES_IN_FLIGHT 2

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
    .image = images[*imageindex].sampled,
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
    .imageView = images[*imageindex].sampledview,
    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
    .resolveImageView = images[*imageindex].view, // TODO
    .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
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

  VkPipeline graphicspipeline = creategraphicspipeline(device.device, swapchain.format.format);
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
  
  VkCommandBuffer imagebuffer[FRAMES_IN_FLIGHT];
  vkAllocateCommandBuffers(device.device, &(VkCommandBufferAllocateInfo) {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,.commandPool=pool,.commandBufferCount=FRAMES_IN_FLIGHT,.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY}, imagebuffer);

  VkSemaphore imagesem[FRAMES_IN_FLIGHT];
  for (unsigned int loop=0;loop<FRAMES_IN_FLIGHT;loop++)
    vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    }, NULL, &imagesem[loop]);
  
  bool imagefencecheck[FRAMES_IN_FLIGHT];
  VkFence imagefence[FRAMES_IN_FLIGHT];
  for (unsigned int loop=0;loop<FRAMES_IN_FLIGHT;loop++) {
    vkCreateFence(device.device, &(VkFenceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    }, NULL, &imagefence[loop]);
    imagefencecheck[loop] = false;
  }

  bool active = true;
  SDL_Event currentevent;
  int width=480, height=480;
  uint32_t imageindex = 0;
  uint32_t frame = 0;

  while (active) {
    if (imagefencecheck[frame]) { // Wait for the frame before last to finish before starting on the next one
      vkWaitForFences(device.device, 1, &imagefence[frame], 1, UINT64_MAX);
      vkResetFences(device.device, 1, &imagefence[frame]);
      imagefencecheck[frame] = false;
    }

    // Get the image for rendering
    VkResult err = vkAcquireNextImageKHR(device.device, swapchain.swapchain, UINT64_MAX, imagesem[frame], NULL, &imageindex);
    if (err != VK_SUCCESS) return 1;

    vkResetCommandBuffer(imagebuffer[frame], 0);
    recordcommandbuffer(imagebuffer[frame], device, swapchain, graphicspipeline, images, imagesem[frame], &imageindex, width, height);

    vkQueueSubmit(device.queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &imagebuffer[frame],
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &imagesem[frame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &images[imageindex].finished,
      .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    }, imagefence[frame]);
    imagefencecheck[frame] = true;
    
    err = vkQueuePresentKHR(device.queue, &(VkPresentInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &images[imageindex].finished,
      .swapchainCount = 1,
      .pSwapchains = &swapchain.swapchain,
      .pImageIndices = &imageindex,
    });


    frame = (frame+1) % FRAMES_IN_FLIGHT; // two frames in flight at a time

    while (SDL_PollEvent(&currentevent)) {
      if (currentevent.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) active = false;
    }
  }

  vkDeviceWaitIdle(device.device);

  releaseimageviews(device, images);
  vkFreeCommandBuffers(device.device, pool, FRAMES_IN_FLIGHT, imagebuffer);
  vkDestroyCommandPool(device.device, pool, NULL);
  vkDestroySemaphore(device.device, imagesem[0], NULL);
  vkDestroySemaphore(device.device, imagesem[1], NULL);
  vkDestroyFence(device.device, imagefence[0], NULL);
  vkDestroyFence(device.device, imagefence[1], NULL);
  vkDestroySwapchainKHR(device.device, swapchain.swapchain, NULL);
  vkDestroyPipeline(device.device, graphicspipeline, NULL);
  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}