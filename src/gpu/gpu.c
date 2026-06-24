#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

#include "common.h"
#include "device.h"
#include "gpu.h"
#include "swapchain.h"

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
void graphics3D(VkSurfaceKHR windowsurface, struct selectdeviceret device, int *active, uint64_t *frametime, struct graphicSettings *settings, VkPipelineCache cache, VkBuffer triangles) {
  VkResult err;
  
  VkPhysicalDeviceProperties deviceproperties;
  vkGetPhysicalDeviceProperties(device.physicaldevice, &deviceproperties);

  // This is assuming the first format is the best format (which it is on every tested implementation)
  VkSurfaceFormatKHR surfaceformat;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device.physicaldevice, windowsurface, &(uint32_t) {1}, &surfaceformat);
  VkSurfaceCapabilitiesKHR surfacecapabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicaldevice, windowsurface, &surfacecapabilities);

  VkSwapchainKHR swapchain = swapchainCreate(device.device, windowsurface, surfaceformat.colorSpace, surfacecapabilities.currentExtent, surfaceformat.format, surfacecapabilities.minImageCount);
  if (swapchain == NULL) {SDL_Log("Failed to create swapchain\n"); *active=0; return;}
  
  uint32_t swapchainimagecount = swapchainGetImageCount(swapchain, device.device);
  struct swapchainimage images[swapchainimagecount];
  swapchainGetImages(device.device, swapchain, swapchainimagecount, images, surfaceformat.format);

  VkShaderModule shadermodule;
  vkCreateShaderModule(device.device, &(VkShaderModuleCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .flags = 0,
    .pCode = (uint32_t *) shadercode,
    .codeSize = sizeof(shadercode),
    .pNext = NULL
  }, NULL, &shadermodule);

  VkPipelineRenderingCreateInfoKHR rfInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .pNext = NULL,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &surfaceformat.format,
    .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT,
    .stencilAttachmentFormat = VK_FORMAT_D32_SFLOAT_S8_UINT
  };

  VkPipelineLayout layout;
  vkCreatePipelineLayout(device.device, &(VkPipelineLayoutCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = NULL,
    .setLayoutCount = 0,
    .pSetLayouts = NULL,
  }, NULL, &layout);

  VkPipeline graphicspipeline;
  vkCreateGraphicsPipelines(device.device, cache, 1, &(VkGraphicsPipelineCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = &rfInfo,
    .flags = 0,
    .stageCount = 2,
    .pStages = (VkPipelineShaderStageCreateInfo[]) {
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = shadermodule,
        .pName = "vertexsmasher",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
      },
      {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = shadermodule,
        .pName = "fragger",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
    },
    .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexAttributeDescriptionCount = 1,
      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
        {
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .location = 0,
          .offset = 0
        }
      },
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
        {
          .binding = 0,
          .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
          .stride = sizeof(struct vertice),
        }
      }
    },
    .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .flags = 0,
      .primitiveRestartEnable = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    },
    .pTessellationState = &(VkPipelineTessellationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
      .patchControlPoints = 0,
    },
    .pViewportState = &(VkPipelineViewportStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .flags = 0,
      .scissorCount = 1,
      .pScissors = &(VkRect2D) {
        .extent = surfacecapabilities.currentExtent,
        .offset = {0,0},
      },
      .viewportCount = 1,
      .pViewports = &(VkViewport) {
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
        .width = surfacecapabilities.currentExtent.width,
        .height = surfacecapabilities.currentExtent.height,
        .x = 0,
        .y = 0,
      }
    },
    .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .flags = 0,
      .depthClampEnable = 0,
      .rasterizerDiscardEnable = 0,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = 0,
      .lineWidth = 1.0f,
    },
    .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = 0,
      .minSampleShading = 0,
      .pSampleMask = NULL
    },
    .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .depthTestEnable = 0,
      .depthWriteEnable = 0,
      .depthCompareOp = 0,
      .depthBoundsTestEnable = 0,
      .stencilTestEnable = 0,
    },
    .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .logicOpEnable = 0,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = (VkPipelineColorBlendAttachmentState[]) {
        {
          .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
          .blendEnable = VK_FALSE,
        }
      },
      .blendConstants = {0, 0, 0, 0}
    },
    .pDynamicState = NULL,
    .layout = layout,
    .renderPass = NULL,
    .subpass = 0,
    .basePipelineHandle = 0,
    .basePipelineIndex = 0,
  }, NULL, &graphicspipeline);

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

/*
Query Pool: Every 0'th frame, it gets the time for

Begin
Vertex
Fragment
End
*/

  VkQueryPool querypool;
  vkCreateQueryPool(device.device, &(VkQueryPoolCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .pipelineStatistics = 0,
    .flags = 0,
    .queryCount = 4,
    .queryType = VK_QUERY_TYPE_TIMESTAMP
  }, NULL, &querypool);

  uint32_t frameindex = 0;

  while (*active) {
    if (images[frameindex].fenceactivated) {
      if (vkGetFenceStatus(device.device, images[frameindex].framefinishFence) == VK_NOT_READY)
        vkWaitForFences(device.device, 1, &images[frameindex].framefinishFence, 1, UINT64_MAX);
      vkResetFences(device.device, 1, &images[frameindex].framefinishFence);
      images[frameindex].fenceactivated = 0;
      if (frameindex == 0) { // Tally performance stats
        uint64_t timestamps[4];
        vkGetQueryPoolResults(device.device, querypool, 0, 4, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

        *frametime = (timestamps[3] - timestamps[0]) * deviceproperties.limits.timestampPeriod;
        //SDL_Log("%f ms\n", (float) (*frametime)/1000000.0f);
      }
    }

    uint32_t imageindex;
    err = vkAcquireNextImageKHR(device.device, swapchain, UINT64_MAX, images[frameindex].frameimageready, 0, &imageindex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
      break; // Exit the loop and remake everything

    VkCommandBuffer commandbuffer = commandbuffers[imageindex];

    vkResetCommandBuffer(commandbuffer, 0);
    vkBeginCommandBuffer(commandbuffer, &(VkCommandBufferBeginInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pInheritanceInfo = NULL,
      .flags = 0
    });

    vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, 0, 0, 0, 1, &(VkImageMemoryBarrier) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = images[frameindex].image,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    });

    if (frameindex == 0) {
      vkCmdResetQueryPool(commandbuffer, querypool, 0, 4);
    }

    vkCmdBeginRendering(commandbuffer, &(VkRenderingInfo) {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea=(VkRect2D) {.extent = surfacecapabilities.currentExtent,.offset=(VkOffset2D) {.x=0,.y=0}},
      .layerCount=1,
      .colorAttachmentCount=1,
      .pColorAttachments=&(VkRenderingAttachmentInfo) {.sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = images[frameindex].imageview,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .resolveMode = VK_RESOLVE_MODE_NONE,
      .resolveImageView = images[frameindex].imageview,
      .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    },
    });

    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline);

    vkCmdBindVertexBuffers(commandbuffer, 0, 1, &triangles, (VkDeviceSize[]) {0});
    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, querypool, 0); // Write time before start
    vkCmdDraw(commandbuffer, 3, 1, 0, 0);
    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, querypool, 1); // End of vertex shader
    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, querypool, 2); // End of fragment shader

    vkCmdEndRendering(commandbuffer);

    vkCmdPipelineBarrier(commandbuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &(VkImageMemoryBarrier) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .image = images[frameindex].image,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount = 1,
    });

    if (frameindex==0) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, querypool, 3); // End of graphics

    vkEndCommandBuffer(commandbuffer);

    vkQueueSubmit(device.queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandbuffer,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &images[frameindex].frameimageready,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &images[frameindex].framefinishSem,
      .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    }, images[frameindex].framefinishFence);
    images[frameindex].fenceactivated = 1;

    err = vkQueuePresentKHR(device.queue, &(VkPresentInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .swapchainCount = 1,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &images[frameindex].framefinishSem,
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
  vkDestroyPipeline(device.device, graphicspipeline, NULL);
  //vkDestroyDescriptorSetLayout(device.device, imagedescriptorset, NULL);
  vkDestroyPipelineLayout(device.device, layout, NULL);
  vkFreeCommandBuffers(device.device, commandpool, swapchainimagecount, commandbuffers);
  vkDestroyCommandPool(device.device, commandpool, NULL);
  vkDestroyQueryPool(device.device, querypool, NULL);
  swapchainClean(device.device, images, swapchain, swapchainGetImageCount(swapchain, device.device));
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

  VkPipelineCache cache;
  vkCreatePipelineCache(device.device, &(VkPipelineCacheCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    .initialDataSize = 0
  }, NULL, &cache);

  VmaAllocation trianglesallocation;
  VkBuffer triangles;
  vmaCreateBuffer(device.allocator, &(VkBufferCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = (uint32_t[]) {0},
    .flags = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .size = (4*3)*3, // 3 3D vertices
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  }, &(VmaAllocationCreateInfo) {
    .usage = VMA_MEMORY_USAGE_AUTO,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  }, &triangles, &trianglesallocation, NULL);
  
  struct vertice *data;
  vmaMapMemory(device.allocator, trianglesallocation, (void **) &data);
  data[0] = (struct vertice) {0, -1, 0};
  data[1] = (struct vertice) {1, 1, 0};
  data[2] = (struct vertice) {-1, 1, 0};
  vmaUnmapMemory(device.allocator, trianglesallocation);

  struct graphicSettings settings;

  while (*args->active)
    graphics3D(windowsurface, device, args->active, &args->frametimeMS, &settings, cache, triangles);

  vmaDestroyBuffer(device.allocator, triangles, trianglesallocation);
  vmaDestroyAllocator(device.allocator);
  vkDestroyPipelineCache(device.device, cache, NULL);
  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
}