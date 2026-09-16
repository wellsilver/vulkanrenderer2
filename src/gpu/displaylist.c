#include <SDL3/SDL_mutex.h>

#include <vulkan/vulkan.h>

#include "displaylist.h"

// Thread safe returns an identifier
uint64_t addobject(struct displaylist *render) {

}

void createDisplaylist(struct displaylist *create) {
  create->access = SDL_CreateSemaphore(1);
  create->camhorizontal = 0;
  create->camvertical = 0;
  create->camx = 0;
  create->camy = 0;
  create->camz = 0;
  // zero a small vertice buffer.
  create->lenvertices = 3;
  create->vertices = malloc(sizeof(struct vertice)*3);
  SDL_memset(create->vertices, 0, sizeof(struct vertice)*3);
}

// Creates the command buffer, run after vkcmdbeginrendering all draw calls in here
void renderDisplaylist(VkCommandBuffer buffer, struct displaylist *render) {
  
}