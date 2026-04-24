#include "gpu.h"

#include <SDL3/SDL.h>

VkShaderModule createshadermodule(VkDevice device, const char *code, uint32_t size) {
  VkShaderModule ret;
  VkResult err = vkCreateShaderModule(device, &(VkShaderModuleCreateInfo) {
    .codeSize = size,
    .flags = 0,
    .pCode = (const uint32_t *) code,
    .pNext = NULL,
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
  }, NULL, &ret);
  if (err != VK_SUCCESS) {
    SDL_Log("vkCreateShaderModule Failed - %i\n", err);
    return NULL;
  }
  return ret;
}

VkPipeline creategraphicspipeline(VkDevice device, VkFormat swapchainformat) {
  VkShaderModule shader = createshadermodule(device, shadercode, sizeof(shadercode));
  if (shader == NULL) return NULL;

  VkPipelineLayout layout;
  vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pushConstantRangeCount = 0,
  }, NULL, &layout);

  VkGraphicsPipelineCreateInfo createinfo = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stageCount = 2,
    .pStages = (VkPipelineShaderStageCreateInfo[]) {
      (VkPipelineShaderStageCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = shader,
        .pName = "vertexsmasher",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
      },
      (VkPipelineShaderStageCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .module = shader,
        .pName = "fragger",
        .pNext = NULL,
        .pSpecializationInfo = NULL,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
    },
    .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .vertexAttributeDescriptionCount = 2,
      .vertexBindingDescriptionCount = 1,
      .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
        { // Position
          .binding = 0,
          .location = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 0,
        },
        { // Color
          .binding = 0,
          .location = 1,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = 4*3,
        }
      },
      .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
        .binding = 0,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        .stride = sizeof(struct vertice)
      },
    },
    .pTessellationState = NULL, // Not using tessellation shaders
    .pViewportState = &(VkPipelineViewportStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
    },
    .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthBiasEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
    },
    .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_TRUE,
      .rasterizationSamples = VK_SAMPLE_COUNT_4_BIT,
    },
    .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &(VkPipelineColorBlendAttachmentState) {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
      },
      .blendConstants = {0, 0, 0, 0},
    },
    .pDepthStencilState = NULL,
    .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = (VkDynamicState[]) {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR},
    },
    .pNext = &(VkPipelineRenderingCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapchainformat,
      .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
      .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    },
    .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .primitiveRestartEnable = VK_FALSE,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    },
    .layout = layout,
    .renderPass = NULL, // dynamic rendering
  };

  VkPipeline graphicspipeline;
  vkCreateGraphicsPipelines(device, NULL, 1, &createinfo, NULL, &graphicspipeline);

  vkDestroyShaderModule(device, shader, NULL);
  vkDestroyPipelineLayout(device, layout, NULL);

  return graphicspipeline;
}