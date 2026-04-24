#include <SDL3/SDL.h>

#include "gpu/gpu.h"

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("Space game", 720, 720, SDL_WINDOW_VULKAN);
  if (window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create window %s\n", SDL_GetError());
    return 3;
  }

  SDL_Thread *gputhread = SDL_CreateThread((SDL_ThreadFunction) &gpu, "renderer", window);

  int status;
  SDL_WaitThread(gputhread, &status);

  SDL_Quit();
  return 0;
}