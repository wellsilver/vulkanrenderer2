#include "gpu.h"

#include <vulkan/vulkan.h>

VkShaderModule createshadermodule(VkDevice device, const char *code, uint32_t size) {
  VkShaderModule ret;
  vkCreateShaderModule(device, &(VkShaderModuleCreateInfo) {
    .codeSize = size,
    .flags = 0,
    .pCode = (const uint32_t *) code,
    .pNext = NULL,
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
  }, NULL, &ret);
  return ret;
}

VkPipeline creategraphicspipeline(VkDevice device) {
  VkShaderModule vertex = createshadermodule(device, vertexcode, sizeof(vertexcode));
  VkShaderModule fragment = createshadermodule(device, fragmentcode, sizeof(vertexcode));

  VkGraphicsPipelineCreateInfo createinfo = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stageCount = 2,
    .pStages = (VkPipelineShaderStageCreateInfo[]) {
      (VkPipelineShaderStageCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = vertex,
        .pName = "vertex",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
      },
      (VkPipelineShaderStageCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = fragment,
        .pName = "fragment",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
    },


  };

  VkPipeline graphicspipeline;
  vkCreateGraphicsPipelines(device, NULL, 1, &createinfo, NULL, &graphicspipeline);

  vkDestroyShaderModule(device, vertex, NULL);
  vkDestroyShaderModule(device, fragment, NULL);

  return graphicspipeline;
}