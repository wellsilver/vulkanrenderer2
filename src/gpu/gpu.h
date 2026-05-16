#include <SDL3/SDL.h>

struct vertice {
  float x,y,z; // 3D Position, relative to mesh
  float color;
};

struct mesh {
  float x,y,z; // 3D Position
  struct vertice *vertices;
};

struct gpu_threadarguments {
  SDL_Window *window;
  int *active;
};

struct renderobjects {
  uint64_t meshlen;
  struct mesh *meshes;
};

int gpu(struct gpu_threadarguments *);

static const char shadercode[] = {
#embed "../../out/shaders.spv"
};