/*
   Copyright 2014 Displair LLC

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Created by: Daniel Sabelnikov <dsabelnikov@gmail.com>
   On: 06/11/2014
 */

#include "SDL2/SDL.h"
#include <cstdlib>
#include "resource.h"

int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 768;

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *volumeTexture;
SDL_Texture *crossTexture;

double volume = 0.75;

void quit() {
  if (volumeTexture != nullptr) SDL_DestroyTexture(volumeTexture);
  if (crossTexture != nullptr) SDL_DestroyTexture(crossTexture);

  if (renderer != nullptr) SDL_DestroyRenderer(renderer);
  if (window != nullptr) SDL_DestroyWindow(window);

  SDL_Quit();

  exit(0);
}

/*
 * Scale 0 to 1
 */
double getCurrentVolume() {
  return volume;
}

void setCurrentVolume(double vol) {
  volume = vol;
}

SDL_Texture* loadTexture(char* name) {
  SDL_Surface* surface = SDL_LoadBMP(name);
  SDL_Texture* texture = nullptr;
  if (surface == nullptr) {
    SDL_Log("SDL_LoadBMP error: %s\n", SDL_GetError());
  }
  else {
    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture == nullptr) {
      SDL_Log("SDL_CreateTextureFromSurface error: %s\n", SDL_GetError());
    }
  }
  return texture;
}

bool isPointInRect(SDL_Rect *rect, int x, int y) {
  return rect->x <= x && rect->y <= y &&
    rect->x + rect->w >= x && rect->y + rect->h >= y;
}

int main(int argc, char **argv) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
    SDL_Log("SDL init error: %s\n", SDL_GetError());
    return 1;
  }

  window = SDL_CreateWindow("Displair Volume Control", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    SDL_Log("SDL_CreateWindow error: %s\n", SDL_GetError());
    quit();
  }

  //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  
  if (renderer == nullptr) {
    SDL_Log("SDL_CreateRenderer error: %s\n", SDL_GetError());
    quit();
  }

  volumeTexture = loadTexture("volume.bmp");
  crossTexture = loadTexture("cross.bmp");

  SDL_Rect barRect;
  barRect.x = 160;
  barRect.y = 660;
  barRect.h = 36;
  barRect.w = 800;

  SDL_Rect volumeIconPos;
  if (volumeTexture != nullptr) {
    SDL_QueryTexture(volumeTexture, NULL, NULL, &volumeIconPos.w, &volumeIconPos.h);
    volumeIconPos.x = (SCREEN_WIDTH - volumeIconPos.w) / 2;
    volumeIconPos.y = (SCREEN_HEIGHT - volumeIconPos.h) / 2;
  }

  SDL_Rect closeIconPos;
  if (crossTexture != nullptr) {
    SDL_QueryTexture(crossTexture, NULL, NULL, &closeIconPos.w, &closeIconPos.h);
    closeIconPos.x = 30;
    closeIconPos.w = closeIconPos.w / 2;
    closeIconPos.h = closeIconPos.h / 2;
    closeIconPos.y = SCREEN_HEIGHT - closeIconPos.h - 30;
  }

  SDL_Rect handleRect;
  handleRect.w = 40;
  handleRect.h = 72;
  handleRect.y = barRect.y + barRect.h / 2 - handleRect.h / 2;

  bool dragging = false;

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
        case SDL_MOUSEBUTTONUP:
          if (e.button.button == SDL_BUTTON_LEFT && e.button.clicks == 1) {
            // single click
            if (isPointInRect(&closeIconPos, e.button.x, e.button.y)) {
              quit();
            }
          }
          if (e.button.button == SDL_BUTTON_LEFT) {
            dragging = false;
          }
          break;
        case SDL_MOUSEBUTTONDOWN:
          if (e.button.button = SDL_BUTTON_LEFT) {
            if (isPointInRect(&handleRect, e.button.x, e.button.y)) {
              dragging = true;
            }
          }
          break;
        case SDL_MOUSEMOTION:
          if (dragging && e.motion.x >= barRect.x && e.motion.x <= barRect.x + barRect.w) {
            setCurrentVolume((double) (e.motion.x - barRect.x) / barRect.w);
          }
          break;
      }
    }

    SDL_RenderClear(renderer);

    if (volumeTexture != nullptr) {
      SDL_RenderCopy(renderer, volumeTexture, NULL, &volumeIconPos);
    }

    if (crossTexture != nullptr) {
      SDL_RenderCopy(renderer, crossTexture, NULL, &closeIconPos);
    }

    SDL_SetRenderDrawColor(renderer, 136, 136, 136, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &barRect);

    handleRect.x = barRect.x + barRect.w * getCurrentVolume() - handleRect.w / 2;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &handleRect);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    SDL_RenderPresent(renderer);
    SDL_Delay(40);
  }

  quit();
}
