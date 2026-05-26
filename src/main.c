#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>

#include "chip8.h"

static int map_key(SDL_Keycode sym) {
  switch (sym) {
    case SDLK_1:
      return 0x1;
    case SDLK_2:
      return 0x2;
    case SDLK_3:
      return 0x3;
    case SDLK_4:
      return 0xC;
    case SDLK_Q:
      return 0x4;
    case SDLK_W:
      return 0x5;
    case SDLK_E:
      return 0x6;
    case SDLK_R:
      return 0xD;
    case SDLK_A:
      return 0x7;
    case SDLK_S:
      return 0x8;
    case SDLK_D:
      return 0x9;
    case SDLK_F:
      return 0xE;
    case SDLK_Z:
      return 0xA;
    case SDLK_X:
      return 0x0;
    case SDLK_C:
      return 0xB;
    case SDLK_V:
      return 0xF;
    default:
      return -1;
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    SDL_Log("usage: %s <rom>", argv[0]);
    return -1;
  }

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return -1;
  }

  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;

  if (!SDL_CreateWindowAndRenderer("chip-8 emulator", 64, 32, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    SDL_Quit();
    return -1;
  }

  SDL_SetRenderLogicalPresentation(renderer, 64, 32, SDL_LOGICAL_PRESENTATION_LETTERBOX);

  initialize();
  if (loadGame(argv[1]) != 0) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return -1;
  }

  bool running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) running = false;

      if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        int k = map_key(event.key.key);
        if (k >= 0) key[k] = event.type == SDL_EVENT_KEY_DOWN;
      }
    }

    for (int i = 0; i < 10; i++) {
      emulateCycle();
    }

    tickTimers();

    if (drawFlag) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

      for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
          if (gfx[x + y * 64]) {
            SDL_FRect r = {x, y, 1, 1};
            SDL_RenderFillRect(renderer, &r);
          }
        }
      }

      SDL_RenderPresent(renderer);
      drawFlag = false;
    }
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
