#include <SDL3/SDL.h>
#include <cglm/cglm.h>

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
  uint64_t frametimeMS;
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