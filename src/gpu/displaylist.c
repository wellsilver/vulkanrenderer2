#include <SDL3/SDL_mutex.h>

#include <vulkan/vulkan.h>

#define VMA_VULKAN_VERSION 1004000 // Vulkan 1.4
#include <vk_mem_alloc.h>

#include "displaylist.h"

// Thread safe, adds
struct model *addmodel(struct displaylist *render, unsigned int lenvertices, struct vertice *vertices) {
  struct model setup;
  setup.lenvertices = lenvertices;
  setup.vertices = SDL_malloc(lenvertices*sizeof(struct vertice));
  SDL_memcpy(setup.vertices, vertices, sizeof(struct vertice)*lenvertices);
  setup.access = SDL_CreateSemaphore(1);

  SDL_WaitSemaphore(render->access);
  
  // Get space to place the new model
  render->lenmodels++;
  SDL_realloc(render->models, sizeof(struct model)*render->lenmodels);  
  // Copy into models
  render->models[render->lenmodels-1] = setup;

  SDL_SignalSemaphore(render->access);

  return &render->models[render->lenmodels-1];
}

struct displaylist createDisplaylist(VmaAllocator allocator) {
  struct displaylist ret;
  ret.access = SDL_CreateSemaphore(1);
  ret.camhorizontal = 0;
  ret.camvertical = 0;
  ret.camx = 0;
  ret.camy = 0;
  ret.camz = 0;
  // create a small buffer which can be resized when required.
  ret.lenmodels = 1;
  ret.models = SDL_malloc(sizeof(struct model));
  return ret;
}

// Creates the command buffer, run after vkcmdbeginrendering all draw calls in here
void renderDisplaylist(VkCommandBuffer buffer, struct displaylist *render) {
  
}