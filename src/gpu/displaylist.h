#ifndef displaylist_h
#define displaylist_h

#include <SDL3/SDL_mutex.h>

struct vertice {
  float x,y,z;
  float r,g,b;
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

struct displaylist {
  float camx,camy,camz;
  float camhorizontal,camvertical;

  // Triangle points len
  unsigned int lenvertices;
  // Triangle points
  struct vertice *vertices;

  SDL_Semaphore *access;
};

// Thread safe returns an identifier
uint64_t addobject(struct displaylist *render);

void createDisplaylist(struct displaylist *create);

// Creates the command buffer, run after vkcmdbeginrendering all draw calls in here
void renderDisplaylist(VkCommandBuffer buffer, struct displaylist *render);

#endif