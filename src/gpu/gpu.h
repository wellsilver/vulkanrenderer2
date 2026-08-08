#include <SDL3/SDL.h>
#include <cglm/cglm.h>

struct gpuparams {
  float camx, camy, camz;
  float camdir_yaw, camdir_pitch;
};

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
  // Camera
  float camx,camy,camz;
  
  // Performance counters
  bool counters;
  uint64_t counterFrametimeNS;
};

struct vertice {
  float x,y,z;
};

struct camera {
  mat4 proj;
  mat4 view;
  mat4 model;
};

int gpu(struct gpu_threadarguments *);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};