#include <SDL3/SDL.h>

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
  uint64_t frametimeMS;
};

struct vertice {
  float x,y,z;
};

int gpu(struct gpu_threadarguments *);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};