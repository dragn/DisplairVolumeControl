/**
 * 
 */

#include "SDL2/SDL.h"
#include <cstdio>
#include <cstdlib>

int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 768;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *volumeTexture;

void quit() {
  if (volumeTexture != nullptr) SDL_DestroyTexture(volumeTexture);
  if (renderer != nullptr) SDL_DestroyRenderer(renderer);
  if (window != nullptr) SDL_DestroyWindow(window);

  SDL_Quit();

  exit(0);
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
    printf("SDL init error: %s\n", SDL_GetError());
    return 1;
  }

  window = SDL_CreateWindow("Displair Volume Control", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    printf("SDL_CreateWindow error: %s\n", SDL_GetError());
    quit();
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  
  if (renderer == nullptr) {
    printf("SDL_CreateRenderer error: %s\n", SDL_GetError());
    quit();
  }

  SDL_Surface* volumeBMP = SDL_LoadBMP("./volume.bmp");
  if (volumeBMP == nullptr) {
    printf("SDL_LoadBMP error: %s\n", SDL_GetError());
  } else {
    volumeTexture = SDL_CreateTextureFromSurface(renderer, volumeBMP);
    SDL_FreeSurface(volumeBMP);
    if (volumeTexture == nullptr) {
      printf("SDL_CreateTextureFromSurface error: %s\n", SDL_GetError());
    }
  }


  SDL_Event e;

  while (true) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          quit();
          break;
        case SDL_KEYDOWN:
          if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) quit();
          break;
      }
    }
    SDL_RenderClear(renderer);

    if (volumeTexture != nullptr) {
      SDL_Rect dst;
      SDL_QueryTexture(volumeTexture, NULL, NULL, &dst.w, &dst.h);
      dst.x = ( SCREEN_WIDTH - dst.w ) / 2;
      dst.y = ( SCREEN_HEIGHT - dst.h ) / 2;
      SDL_RenderCopy(renderer, volumeTexture, NULL, &dst);
    }

    SDL_RenderPresent(renderer);
    SDL_Delay(40);
  }

  quit();
}
