#ifndef displaylist_h
#define displaylist_h

#include <SDL3/SDL_misc.h>

#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

struct vertice {
  float x,y,z;
  float r,g,b;
  uint32_t mesh;
};

static VkPipelineVertexInputStateCreateInfo vertexinputstateinfo = {
  .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  .vertexAttributeDescriptionCount = 3,
  .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
    {
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .location = 0,
      .offset = 0
    },
    {
      .binding = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .location = 1,
      .offset = sizeof(float)*3,
    },
    {
      .binding = 0,
      .format = VK_FORMAT_R32_UINT,
      .location = 2,
      .offset = sizeof(float)*3+sizeof(float)*3,
    },
  },
  .vertexBindingDescriptionCount = 1,
  .pVertexBindingDescriptions = (VkVertexInputBindingDescription[]) {
    {
      .binding = 0,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      .stride = sizeof(struct vertice),
    }
  }
};

struct displaylist {
  float camx,camy,camz;
  float camhorizontal,camvertical;
  // Triangle points len
  unsigned int lenvertices;
  // Triangle points
  struct vertice *vertices;
  // Model matrices len
  unsigned int lenmodels;
  // model matrices
  float *models[4*4];
};

// Thread safe returns a identifier
uint64_t addobject(struct displaylist *render);

struct displaylist *createDisplaylist();
void renderDisplaylist(VkCommandBuffer buffer, VmaAllocator allocator, struct displaylist *render);

#endif