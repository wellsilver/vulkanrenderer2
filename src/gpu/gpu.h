#include <SDL3/SDL.h>

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
  uint64_t frametimeMS;
};

struct vertice {
  float x,y,z;
};

struct camera {
  float proj[4*4];
  float view[4*4];
  float model[4*4];
};

int gpu(struct gpu_threadarguments *);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};