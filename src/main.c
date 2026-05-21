#include <SDL3/SDL.h>

#include "gpu/gpu.h"

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }

  int active = 1; // Master active value, when this is 0 the program is shutting down.

  SDL_Window *window = SDL_CreateWindow("Space Game", 480, 480, SDL_WINDOW_VULKAN);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create window %s\n", SDL_GetError());
    return 3;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  
  struct gpu_threadarguments gpudata;
  gpudata.window = window;
  gpudata.active = &active;
  SDL_Thread *gputhread = SDL_CreateThread((SDL_ThreadFunction) &gpu, "renderer", &gpudata);
  
  SDL_Event event;
  while (active) {
    SDL_WaitEvent(&event);
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) active = 0;
  }

  SDL_WaitThread(gputhread, NULL);

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}