#include <SDL3/SDL_mutex.h>

#include <vulkan/vulkan.h>

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

void createDisplaylist(struct displaylist *create) {
  create->access = SDL_CreateSemaphore(1);
  create->camhorizontal = 0;
  create->camvertical = 0;
  create->camx = 0;
  create->camy = 0;
  create->camz = 0;
  // create a small buffer which can be resized when required.
  create->lenmodels = 1;
  create->models = SDL_malloc(sizeof(struct model));
}

// Creates the command buffer, run after vkcmdbeginrendering all draw calls in here
void renderDisplaylist(VkCommandBuffer buffer, struct displaylist *render) {
  
}