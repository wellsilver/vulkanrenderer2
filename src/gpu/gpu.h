#include <SDL3/SDL.h>
#include <cglm/cglm.h>

struct gpuparams {
  float camx, camy, camz;
  float camdir_yaw, camdir_pitch;
};

#include "displaylist.h"

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
  // Camera
  struct displaylist *thelist;
  
  // Performance counters
  bool counters;
  uint64_t counterFrametimeNS;
};

struct vertice {
  float x,y,z;
};

unsigned int addmesh(void *data, unsigned int verticelen, struct vertice *vertices);

struct camera {
  mat4 proj;
  mat4 view;
  mat4 model;
};

int gpu(struct gpu_threadarguments *);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};