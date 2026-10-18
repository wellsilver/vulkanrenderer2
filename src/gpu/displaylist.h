#ifndef displaylist_h
#define displaylist_h

#include <SDL3/SDL_mutex.h>
#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

struct vertice {
  float x,y,z;
  float u,v; // as in YUV
  unsigned int texture; // I believe these are 32 bits, so its aligned with yuv's UV
};

static VkPipelineVertexInputStateCreateInfo vertexinputstateinfo = {
  .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  .vertexAttributeDescriptionCount = 2,
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
};

// Every object made of triangles
struct model {
  float x,y,z;
  float ro,pi,ya; // Roll Pitch Yaw

  // this is the pointer to inside of a vkbuffer
  struct vertice *vertices;
  unsigned int lenvertices;

  SDL_Semaphore *access;
};

struct displaylist {
  float camx,camy,camz;
  float camhorizontal,camvertical;

  // Triangle models, has all the needed information per model
  unsigned int lenmodels;
  struct model *models;

  // Triangle buffers
  unsigned int lenbuffers;
  VkBuffer *buffers;
  VmaAllocation *allocations;

  SDL_Semaphore *access;
};

// Thread safe returns an identifier
struct model *addmodel(struct displaylist *render, unsigned int lenvertices, struct vertice *vertices);

struct displaylist createDisplaylist(VmaAllocator allocator);

// Creates the command buffer, run after vkcmdbeginrendering all draw calls in here
void renderDisplaylist(VkCommandBuffer buffer, struct displaylist *render);

#endif