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
void graphics3D(VkSurfaceKHR windowsurface, struct selectdeviceret device, int *active, uint64_t *frametime, struct graphicSettings *settings) {
  VkResult err;
  
  VkPhysicalDeviceProperties deviceproperties;
  vkGetPhysicalDeviceProperties(device.physicaldevice, &deviceproperties);

  // This is assuming the first format is the best format (which it is on every tested implementation)
  VkSurfaceFormatKHR surfaceformat;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicaldevice, windowsurface, &(uint32_t) {1}, &surfaceformat);
  VkSurfaceCapabilitiesKHR surfacecapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicaldevice, windowsurface, &surfacecapabilities);

  VkSwapchainKHR swapchain;
  err = vkCreateSwapchainKHR(device.device, &(VkSwapchainCreateInfoKHR) {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .clipped = VK_FALSE,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .flags = 0,
    .imageArrayLayers = 1,
    .imageColorSpace = surfaceformat.colorSpace,
    .imageExtent = surfacecapabilities.currentExtent,
    .imageFormat = surfaceformat.format,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
    .minImageCount = surfacecapabilities.minImageCount,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &(uint32_t) {0},
    .presentMode = VK_PRESENT_MODE_FIFO_KHR,
    .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    .surface = windowsurface,
  }, NULL, &swapchain);

  uint32_t swapchainimagecount;
  vkGetSwapchainImagesKHR(device.device, swapchain, &swapchainimagecount, NULL);
  VkImage swapchainimages[swapchainimagecount];
  VkImageView swapchainimageviews[swapchainimagecount];
  VkSemaphore framefinishrendersemaphore[swapchainimagecount];
  VkFence framefinishrenderfence[swapchainimagecount];
  VkSemaphore frameimagereadysemaphore[swapchainimagecount];
  bool framefenceactivated[swapchainimagecount];
  //VkSemaphore swapchainrenderfinishsemaphore[swapchainimagecount];
  vkGetSwapchainImagesKHR(device.device, swapchain, &swapchainimagecount, swapchainimages);

  for (unsigned int loop=0;loop<swapchainimagecount;loop++) {
    vkCreateImageView(device.device, &(VkImageViewCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      // .components all is 0, leave it be
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .components = (VkComponentMapping) {0,0,0,0},
      .flags = 0,
      .format = surfaceformat.format,
      .image = swapchainimages[loop],
      .subresourceRange = (VkImageSubresourceRange) {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1
      }
    }, NULL, &swapchainimageviews[loop]);
    vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    }, NULL, &framefinishrendersemaphore[loop]);
    vkCreateSemaphore(device.device, &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    }, NULL, &frameimagereadysemaphore[loop]);
    vkCreateFence(device.device, &(VkFenceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &framefinishrenderfence[loop]);
    framefenceactivated[loop] = 0;
  }

  VkShaderModule shadermodule;
  vkCreateShaderModule(device.device, &(VkShaderModuleCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .flags = 0,
    .pCode = (uint32_t *) shadercode,
    .codeSize = sizeof(shadercode),
    .pNext = NULL
  }, NULL, &shadermodule);

  VkDescriptorSetLayout imagedescriptorset;
  vkCreateDescriptorSetLayout(device.device, &(VkDescriptorSetLayoutCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
    .bindingCount = 1,
    .pBindings = &(VkDescriptorSetLayoutBinding) {
      .binding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .pImmutableSamplers = 0
    }
  }, NULL, &imagedescriptorset);

  VkPipelineLayout computelayout;
  vkCreatePipelineLayout(device.device, &(VkPipelineLayoutCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .flags = 0,
    .setLayoutCount = 1,
    .pSetLayouts = (VkDescriptorSetLayout[]) {
      imagedescriptorset
    },
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = (VkPushConstantRange[]) {
      {.offset = 0,.size = 8,.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT}
    },
  }, NULL, &computelayout);

  VkPipeline computepipeline;
  vkCreateComputePipelines(device.device, NULL, 1, &(VkComputePipelineCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = (VkPipelineShaderStageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = shadermodule,
      .pName = "main",
      .pSpecializationInfo = NULL,
    },
    .layout = computelayout,
    .basePipelineHandle = NULL,
    .basePipelineIndex = 0
  }, NULL, &computepipeline);

  VkCommandPool commandpool;
  vkCreateCommandPool(device.device, &(VkCommandPoolCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = 0
  }, NULL, &commandpool);

  VkCommandBuffer commandbuffers[swapchainimagecount];
  vkAllocateCommandBuffers(device.device, &(VkCommandBufferAllocateInfo) {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = commandpool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = swapchainimagecount,
  }, commandbuffers);

  VkQueryPool querypool;
  vkCreateQueryPool(device.device, &(VkQueryPoolCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .pipelineStatistics = 0,
    .flags = 0,
    .queryCount = 2,
    .queryType = VK_QUERY_TYPE_TIMESTAMP
  }, NULL, &querypool);

  uint32_t frameindex = 0;

  while (*active) {
    if (framefenceactivated[frameindex]) {
      if (vkGetFenceStatus(device.device, framefinishrenderfence[frameindex]) == VK_NOT_READY)
        vkWaitForFences(device.device, 1, &framefinishrenderfence[frameindex], 1, UINT64_MAX);
      vkResetFences(device.device, 1, &framefinishrenderfence[frameindex]);
      framefenceactivated[frameindex] = 0;
      if (frameindex == 0) { // Tally performance stats
        uint64_t timestamps[2];
        vkGetQueryPoolResults(device.device, querypool, 0, 2, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

        *frametime = (timestamps[1] - timestamps[0]) * deviceproperties.limits.timestampPeriod;
      }
    }

    uint32_t imageindex;
    err = vkAcquireNextImageKHR(device.device, swapchain, UINT64_MAX, frameimagereadysemaphore[frameindex], 0, &imageindex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
      break; // Exit the loop and remake everything

    VkCommandBuffer commandbuffer = commandbuffers[imageindex];

    vkResetCommandBuffer(commandbuffer, 0);
    vkBeginCommandBuffer(commandbuffer, &(VkCommandBufferBeginInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pInheritanceInfo = NULL,
      .flags = 0
    });

    if (frameindex == 0) vkCmdResetQueryPool(commandbuffer, querypool, 0, 2);

    vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = swapchainimages[imageindex],
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_GENERAL,
      .dstAccessMask = 0,
      .dstQueueFamilyIndex = 0,
      .srcAccessMask = 0,
      .srcQueueFamilyIndex = 0,
      .subresourceRange = (VkImageSubresourceRange) {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1
      },
    });

    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computepipeline);
    vkCmdPushDescriptorSet(commandbuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computelayout, 0, 1, &(VkWriteDescriptorSet) {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      .dstArrayElement = 0,
      .dstBinding = 0,
      .dstSet = 0,
      .pBufferInfo = NULL,
      .pImageInfo = &(VkDescriptorImageInfo) {
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        .imageView = swapchainimageviews[imageindex],
        .sampler = NULL,
      },
      .pTexelBufferView = 0
    });
    uint32_t imagesize[2] = {surfacecapabilities.currentExtent.width, surfacecapabilities.currentExtent.height};
    vkCmdPushConstants(commandbuffer, computelayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, imagesize);

    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, querypool, 0);
    vkCmdDispatch(commandbuffer, surfacecapabilities.currentExtent.width, surfacecapabilities.currentExtent.height, 1);
    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, querypool, 1);

    vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = swapchainimages[imageindex],
      .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .dstAccessMask = 0,
      .dstQueueFamilyIndex = 0,
      .srcAccessMask = 0,
      .srcQueueFamilyIndex = 0,
      .subresourceRange = (VkImageSubresourceRange) {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseArrayLayer = 0,
        .baseMipLevel = 0,
        .layerCount = 1,
        .levelCount = 1
      },
    });

    vkEndCommandBuffer(commandbuffer);

    vkQueueSubmit(device.queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandbuffer,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &frameimagereadysemaphore[frameindex],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &framefinishrendersemaphore[frameindex],
      .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    }, framefinishrenderfence[frameindex]);
    framefenceactivated[frameindex] = 1;

    err = vkQueuePresentKHR(device.queue, &(VkPresentInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .swapchainCount = 1,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &framefinishrendersemaphore[frameindex],
      .pImageIndices = &imageindex,
      .pSwapchains = &swapchain,
    });
    if (err == VK_SUBOPTIMAL_KHR || err == VK_ERROR_OUT_OF_DATE_KHR) break;

    frameindex++;
    frameindex %= swapchainimagecount;
  }

  vkDeviceWaitIdle(device.device);

  // Destroy everything

  vkDestroyShaderModule(device.device, shadermodule, NULL);
  vkDestroyPipeline(device.device, computepipeline, NULL);
  vkDestroyDescriptorSetLayout(device.device, imagedescriptorset, NULL);
  vkDestroyPipelineLayout(device.device, computelayout, NULL);
  vkFreeCommandBuffers(device.device, commandpool, swapchainimagecount, commandbuffers);
  vkDestroyCommandPool(device.device, commandpool, NULL);
  vkDestroyQueryPool(device.device, querypool, NULL);
  for (unsigned int loop=0;loop<swapchainimagecount;loop++) {
    vkDestroyImageView(device.device, swapchainimageviews[loop], NULL);
    vkDestroySemaphore(device.device, framefinishrendersemaphore[loop], NULL);
    vkDestroyFence(device.device, framefinishrenderfence[loop], NULL);
    vkDestroySemaphore(device.device, frameimagereadysemaphore[loop], NULL);
  }
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
    graphics3D(windowsurface, device, args->active, &args->frametimeMS, &settings);


  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
}