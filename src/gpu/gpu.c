#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>
#include <cglm/cam.h>
#include <cglm/clipspace/persp_rh_zo.h>
#include <cglm/clipspace/view_rh_zo.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

#include "common.h"
#include "device.h"
#include "gpu.h"
#include "swapchain.h"

struct graphicspersistdata {
  VkPipelineCache pipelinecache;
  VmaAllocator allocator;

  VkBuffer indirectbuffer;
  VkBuffer trianglecachebuffer;
  VkBuffer *trianglebuffers;
  unsigned int *trianglebufferscount;
  unsigned int lentrianglebuffers;
  SDL_Semaphore *writesem;
};

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

unsigned int addmesh(void *data_v, unsigned int verticelen, struct vertice *vertices) {
  struct graphicspersistdata *data = data_v;

  VmaAllocation trianglebuffer1allocation;
  VkBuffer trianglebuffer1;
  vmaCreateBuffer(data->allocator, &(VkBufferCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = (uint32_t[]) {0},
    .flags = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .size = sizeof(struct vertice)*verticelen,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  }, &(VmaAllocationCreateInfo) {
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  }, &trianglebuffer1, &trianglebuffer1allocation, NULL);
  vmaCopyMemoryToAllocation(data->allocator, vertices, trianglebuffer1allocation, 0, sizeof(struct vertice)*verticelen);

  SDL_WaitSemaphore(data->writesem);
  data->lentrianglebuffers++;
  data->trianglebuffers = SDL_realloc(data->trianglebuffers, data->lentrianglebuffers);
  data->trianglebufferscount = SDL_realloc(data->trianglebuffers, data->lentrianglebuffers);

  data->trianglebuffers[data->lentrianglebuffers-1] = trianglebuffer1;
  data->trianglebufferscount[data->lentrianglebuffers-1] = verticelen; 

  SDL_SignalSemaphore(data->writesem);
}

/*

*/
void graphics3D(VkSurfaceKHR windowsurface, struct gpu_threadarguments *args, struct selectdeviceret device, struct graphicspersistdata *data) {
  VkResult err;
  
  int *active = args->active;

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
    .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
    .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
  };

  VkPipelineLayout layout;
  vkCreatePipelineLayout(device.device, &(VkPipelineLayoutCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext = 0,
    .flags = 0,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &(VkPushConstantRange) {
      .offset = 0,
      .size = sizeof(struct camera),
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    },
    .setLayoutCount = 0,
    .pSetLayouts = NULL,
  }, NULL, &layout);

  VkPipeline graphicspipeline;
  vkCreateGraphicsPipelines(device.device, data->pipelinecache, 1, &(VkGraphicsPipelineCreateInfo) {
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
      .cullMode = VK_CULL_MODE_NONE,
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
      .depthTestEnable = 1,
      .depthWriteEnable = 1,
      .depthCompareOp = VK_COMPARE_OP_LESS,
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

  VkImage zbuffer;
  VkImageView zbufferview;
  VmaAllocation zbufferallocation;
  vmaCreateImage(device.allocator, &(VkImageCreateInfo) {
    .arrayLayers = 1,
    .extent = (VkExtent3D) {.depth = 1,.width=surfacecapabilities.currentExtent.width,.height= surfacecapabilities.currentExtent.height},
    .flags = 0,
    .format = VK_FORMAT_D32_SFLOAT,
    .imageType = VK_IMAGE_TYPE_2D,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .mipLevels = 1,
    .pQueueFamilyIndices = &(uint32_t) {0},
    .queueFamilyIndexCount = 1,
    .samples = 1,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
  }, &(VmaAllocationCreateInfo) {
    .flags = VMA_MEMORY_USAGE_GPU_ONLY
  }, &zbuffer, &zbufferallocation, NULL);
  vkCreateImageView(device.device, &(VkImageViewCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .image = zbuffer,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = VK_FORMAT_D32_SFLOAT,
    .components = (VkComponentMapping) {0,0,0,0},
    .subresourceRange = (VkImageSubresourceRange) {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
      .baseArrayLayer = 0,
      .baseMipLevel = 0,
      .layerCount = 1,
      .levelCount = 1
    }
  }, NULL, &zbufferview);

  VkQueryPool querypool;
  vkCreateQueryPool(device.device, &(VkQueryPoolCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
    .pipelineStatistics = 0,
    .flags = 0,
    .queryCount = 4,
    .queryType = VK_QUERY_TYPE_TIMESTAMP
  }, NULL, &querypool);

  struct camera camMatrices;

/*
ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
ubo.proj[1][1] *= -1;
*/
  
  uint32_t frameindex = 0;
  bool performancecounter = 0;

  while (*active) {
    if (images[frameindex].fenceactivated) {
      if (vkGetFenceStatus(device.device, images[frameindex].framefinishFence) == VK_NOT_READY)
        vkWaitForFences(device.device, 1, &images[frameindex].framefinishFence, 1, UINT64_MAX);
      vkResetFences(device.device, 1, &images[frameindex].framefinishFence);
      images[frameindex].fenceactivated = 0;
      if (performancecounter) { // Tally performance stats
        uint64_t timestamps[2];
        vkGetQueryPoolResults(device.device, querypool, 0, 2, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

        args->counterFrametimeNS = (timestamps[1] - timestamps[0]) * deviceproperties.limits.timestampPeriod;
        //SDL_Log("%f ms\n", (float) (*frametime)/1000000.0f);
      }
    }

    uint32_t imageindex;
    err = vkAcquireNextImageKHR(device.device, swapchain, UINT64_MAX, images[frameindex].frameimageready, 0, &imageindex);
    if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
      break; // Exit the loop and remake everything

    vec3 position = {args->camx, args->camy, args->camz};
    vec3 front = {0, 0, 0};
    vec3 positionplusfront;
    vec3 up = {0, 1, 0};
    glm_vec3_add(position, front, positionplusfront);
    glm_lookat_rh_zo(positionplusfront, front, up, camMatrices.view);
    glm_perspective_rh_zo(glm_rad(90.0f), (float)surfacecapabilities.currentExtent.width / (float) surfacecapabilities.currentExtent.height, 0.1f, 100.0f, camMatrices.proj);
    camMatrices.proj[1][1] *= -1.0f;

    VkCommandBuffer commandbuffer = commandbuffers[imageindex];

    vkResetCommandBuffer(commandbuffer, 0);
    vkBeginCommandBuffer(commandbuffer, &(VkCommandBufferBeginInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pInheritanceInfo = NULL,
      .flags = 0
    });

    // The NVIDIA performance guide says to use synchronization2, and to do it in one command.
    vkCmdPipelineBarrier2(commandbuffer, &(VkDependencyInfo) {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers = (VkImageMemoryBarrier2[2]) {
        { // Color image
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .image = images[frameindex].image,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .srcAccessMask = VK_ACCESS_2_NONE,
          .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
          .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .subresourceRange.baseMipLevel = 0,
          .subresourceRange.levelCount = 1,
          .subresourceRange.baseArrayLayer = 0,
          .subresourceRange.layerCount = 1,
        },
        { // Depth buffer
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .image = zbuffer,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .srcAccessMask = VK_ACCESS_2_NONE,
          .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
          .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .subresourceRange.baseMipLevel = 0,
          .subresourceRange.levelCount = 1,
          .subresourceRange.baseArrayLayer = 0,
          .subresourceRange.layerCount = 1,
        }
      }
    });

    if (performancecounter) {
      vkCmdResetQueryPool(commandbuffer, querypool, 0, 2);
    }

    if (performancecounter) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, querypool, 0); // Write time before start

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
        .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE
      },
      .pDepthAttachment = &(VkRenderingAttachmentInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = zbufferview,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = zbufferview,
        .resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .clearValue = 100,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      }
    });
    
    vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicspipeline);
    
    vkCmdPushConstants(commandbuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(camMatrices), &camMatrices);
    for (unsigned int loop=0;loop<data->lentrianglebuffers;loop++) {
      if (data->trianglebufferscount[loop] == 0) continue;
      vkCmdBindVertexBuffers(commandbuffer, 0, 1, &data->trianglebuffers[loop], &(VkDeviceSize) {0});
      vkCmdDraw(commandbuffer, data->trianglebufferscount[loop]*3, 1, 0, 0);
    }

    if (performancecounter) vkCmdWriteTimestamp(commandbuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, querypool, 1); // End of graphics

    vkCmdEndRendering(commandbuffer);

    vkCmdPipelineBarrier2(commandbuffer, &(VkDependencyInfo) {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &(VkImageMemoryBarrier2) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .image = images[frameindex].image,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
      }
    });
    vkEndCommandBuffer(commandbuffer);

    vkQueueSubmit(device.queue, 1, &(VkSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandbuffer,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &images[frameindex].frameimageready,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &images[frameindex].framefinishSem,
      .pWaitDstStageMask = &(VkPipelineStageFlags) {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
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

  vkDestroyImageView(device.device, zbufferview, NULL);
  vmaDestroyImage(device.allocator, zbuffer, zbufferallocation);
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

  struct graphicspersistdata data;

  VkPipelineCache cache;
  vkCreatePipelineCache(device.device, &(VkPipelineCacheCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    .initialDataSize = 0
  }, NULL, &data.pipelinecache);

  VmaAllocation vertexTempallocation;
  vmaCreateBuffer(device.allocator, &(VkBufferCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = (uint32_t[]) {0},
    .flags = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .size = (4*3)*6, // 3 3D vertices
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  }, &(VmaAllocationCreateInfo) {
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  }, &data.trianglecachebuffer, &vertexTempallocation, NULL);

  VmaAllocation drawindirectbufferallocation;
  VkBuffer drawindirectbuffer;
  vmaCreateBuffer(device.allocator, &(VkBufferCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = (uint32_t[]) {0},
    .flags = 0,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .size = sizeof(VkDrawIndirectCommand), // 3 3D vertices
    .usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
  }, &(VmaAllocationCreateInfo) {
    .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
  }, &data.indirectbuffer, &drawindirectbufferallocation, NULL);

  data.lentrianglebuffers = 1,
  data.trianglebuffers = SDL_malloc(sizeof(VkBuffer *)*1);
  data.trianglebuffers[0] = NULL;
  data.trianglebufferscount = SDL_malloc(sizeof(unsigned int)*1);
  data.trianglebufferscount[0] = 0;

  data.allocator = device.allocator;
  data.writesem = SDL_CreateSemaphore(1);

  struct vertice trianglebuffer1data[3];
  trianglebuffer1data[0] = (struct vertice) {0, 1, 0};
  trianglebuffer1data[1] = (struct vertice) {-1, -1, 0};
  trianglebuffer1data[2] = (struct vertice) {1, -1, 0};
  addmesh(&data, 3, trianglebuffer1data);

  while (*args->active)
    graphics3D(windowsurface, args, device, &data);

  //vmaUnmapMemory(device.allocator, trianglebuffer1allocation);
  //vmaDestroyBuffer(device.allocator, trianglebuffer1, trianglebuffer1allocation);
  vmaDestroyBuffer(device.allocator, data.trianglecachebuffer, vertexTempallocation);
  vmaDestroyBuffer(device.allocator, data.indirectbuffer, drawindirectbufferallocation);
  vmaDestroyAllocator(device.allocator);
  vkDestroyPipelineCache(device.device, cache, NULL);
  vkDestroyDevice(device.device, NULL);
  vkDestroySurfaceKHR(instance, windowsurface, NULL);
  vkDestroyInstance(instance, NULL);
}