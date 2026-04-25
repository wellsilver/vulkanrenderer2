#include <SDL3/SDL.h>

struct vertice {
  float x,y,z;
  float r,g,b;
};

struct gpu_threadarguments {
  SDL_Window *window;
  struct vertice *vertices;
  unsigned int lenvertices;
};

int gpu(struct gpu_threadarguments *);