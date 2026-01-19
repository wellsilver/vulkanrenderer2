#include "gpu.h"

#include <vulkan/vulkan.h>

VkPipeline creategraphicspipeline(VkDevice device) {
  VkGraphicsPipelineCreateInfo createinfo = {0};

  VkPipeline graphicspipeline;
  vkCreateGraphicsPipelines(device, NULL, 1, &createinfo, NULL, &graphicspipeline);
}