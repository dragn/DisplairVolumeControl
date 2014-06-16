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

#include "SDL.h"
#include "SDL_mixer.h"
#include <cstdlib>
#include <cmath>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include "SDL2_gfxPrimitives.h"
#include "SDL_ttf.h"
#include <string>

int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 768;

char *MUSIC_FILENAME = "sound.wav";
char *FONT_FILENAME = "FreeSans.ttf";

SDL_Window *window;
SDL_Renderer *renderer;

SDL_Texture *volumeTexture;
SDL_Texture *crossTexture;

SDL_Color textColor = { 255, 255, 255 };

TTF_Font *font;

IAudioEndpointVolume *endpointVolume = NULL;
LPWSTR deviceId;
HRESULT hr;

void quit() {
  if (volumeTexture != nullptr) SDL_DestroyTexture(volumeTexture);
  if (crossTexture != nullptr) SDL_DestroyTexture(crossTexture);

  if (renderer != nullptr) SDL_DestroyRenderer(renderer);
  if (window != nullptr) SDL_DestroyWindow(window);

  Mix_CloseAudio();
  SDL_Delay(200);
  
  Mix_Quit();
  TTF_Quit();
  SDL_Quit();

  exit(0);
}

/*
 * Scale 0 to 1
 */
double getCurrentVolume() {
  if (endpointVolume) {
    float volume;
    endpointVolume->GetMasterVolumeLevelScalar(&volume);
    return volume;
  }
  else return 0.;
}

void setCurrentVolume(double vol) {
  if (endpointVolume) {
    endpointVolume->SetMasterVolumeLevelScalar(vol, NULL);
  }
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

bool PickDevice(IMMDevice **DeviceToUse)
{
  HRESULT hr;
  bool retValue = true;
  IMMDeviceEnumerator *deviceEnumerator = NULL;
  IMMDeviceCollection *deviceCollection = NULL;

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
  if (FAILED(hr))
  {
    SDL_Log("Unable to instantiate device enumerator: %x\n", hr);
    retValue = false;
    goto Exit;
  }

  IMMDevice *device = NULL;

  if (device == NULL)
  {
    ERole deviceRole = eMultimedia;
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, deviceRole, &device);
    if (FAILED(hr))
    {
      SDL_Log("Unable to get default device for role %d: %x\n", deviceRole, hr);
      retValue = false;
      goto Exit;
    }
  }

  *DeviceToUse = device;
  retValue = true;
Exit:
  if (deviceCollection) deviceCollection->Release();
  if (deviceEnumerator) deviceEnumerator->Release();

  return retValue;
}

void renderText(SDL_Renderer *renderer, const char *text, int x, int y) {
  if (font == nullptr) return;
  SDL_Surface *surface = TTF_RenderText_Blended(font, text, textColor);
  if (surface != nullptr) {
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst;
    SDL_QueryTexture(texture, NULL, NULL, &dst.w, &dst.h);
    dst.x = x - dst.w / 2;
    dst.y = y - dst.h / 2;
    SDL_RenderCopy(renderer, texture, NULL, &dst);
  }
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

  SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  
  if (renderer == nullptr) {
    SDL_Log("SDL_CreateRenderer error: %s\n", SDL_GetError());
    quit();
  }

  if (TTF_Init() != 0) {
    SDL_Log("SDL_ttf initialization error: %s\n", TTF_GetError());
    quit();
  }

  font = TTF_OpenFont(FONT_FILENAME, 96);
  if (font == nullptr) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Could not load font '%s': %s\n", FONT_FILENAME, TTF_GetError());
  }

  IMMDevice *device = nullptr;
  PickDevice(&device);

  if (device == nullptr) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
      "Unable to initialize audio endpoint!");
    quit();
  }

  hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&endpointVolume));
  if (FAILED(hr) || endpointVolume == nullptr) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, 
      "Unable to activate endpoint volume on output device: %x\n", hr);
    quit();
  }

  int flags = MIX_INIT_MP3;
  int initted = Mix_Init(flags);
  if (initted&flags != flags) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Mix_Init: Failed to init required mp3 support!\n");
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Mix_Init: %s\n", Mix_GetError());
    quit();
  }

  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
    SDL_Log("Mix_OpenAudio error: %s\n", Mix_GetError());
    quit();
  }

  Mix_Music *music = Mix_LoadMUS(MUSIC_FILENAME);
  if (music) {
    Mix_FadeInMusic(music, -1, 1000);
  }
  else {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
      "Unable to load music file: %s, error: %s\n", MUSIC_FILENAME, Mix_GetError());
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
    volumeIconPos.x = (SCREEN_WIDTH - volumeIconPos.w) / 2 - 100;
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
  handleRect.w = 72;
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
          if (dragging) {
            double v = (double)(e.motion.x - barRect.x) / barRect.w;
            if (v < 0) v = 0;
            if (v > 1) v = 1;
            setCurrentVolume(v);
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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &barRect);

    SDL_Rect fillRect = barRect;
    fillRect.w = barRect.w * getCurrentVolume();
    SDL_SetRenderDrawColor(renderer, 215, 120, 10, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &fillRect);

    handleRect.x = barRect.x + barRect.w * getCurrentVolume() - handleRect.w / 2;

    filledEllipseRGBA(renderer, handleRect.x + handleRect.w / 2, handleRect.y + handleRect.h / 2,
      handleRect.w / 2, handleRect.h / 2, 160, 160, 160, SDL_ALPHA_OPAQUE);
    aaellipseRGBA(renderer, handleRect.x + handleRect.w / 2, handleRect.y + handleRect.h / 2,
      handleRect.w / 2, handleRect.h / 2, 160, 160, 160, SDL_ALPHA_OPAQUE);

    filledEllipseRGBA(renderer, handleRect.x + handleRect.w / 2, handleRect.y + handleRect.h / 2,
      handleRect.w / 2 - 5, handleRect.h / 2 - 5, 215, 120, 10, SDL_ALPHA_OPAQUE);
    aaellipseRGBA(renderer, handleRect.x + handleRect.w / 2, handleRect.y + handleRect.h / 2,
      handleRect.w / 2 - 5, handleRect.h / 2 - 5, 215, 120, 10, SDL_ALPHA_OPAQUE);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    renderText(renderer, (std::to_string( (int) round(getCurrentVolume() * 100) )).c_str(),
      volumeIconPos.x + volumeIconPos.w + 100, volumeIconPos.y + volumeIconPos.h / 2);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    SDL_RenderPresent(renderer);
    SDL_Delay(40);
  }

  quit();
}
