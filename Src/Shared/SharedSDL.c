/*
 * SharedSDL.c
 *
 *  Created on: Sep 19, 2019
 *      Author: wouter
 */
#include "SharedSDL.h"

#include <SDL2/SDL.h>
#include "log.h"

typedef struct
{
  int windowWidth;
  int windowHeight;
  const char* windowTitle;
  int targetFrameTime_ms;
  SharedSDL_PreStart preStart;
  SharedSDL_Update update;
  SharedSDL_Draw draw;
  SharedSDL_EventHandler userEventHandler;
} ControlBlock_t;

static ControlBlock_t _controlBlock;

void SharedSDL_Initialize(int windowWidth,
                          int windowHeight,
                          const char* windowTitle,
                          SharedSDL_PreStart preStart,
                          SharedSDL_Update update,
                          SharedSDL_Draw draw,
                          SharedSDL_EventHandler eventHandler)
{
  _controlBlock.windowWidth = windowWidth;
  _controlBlock.windowHeight = windowHeight;
  _controlBlock.windowTitle = windowTitle;
  _controlBlock.draw = draw;
  _controlBlock.preStart = preStart;
  _controlBlock.update = update;
  _controlBlock.userEventHandler = eventHandler;
  _controlBlock.targetFrameTime_ms = 16;
}

int SharedSDL_Start()
{
  SDL_Window *window = NULL;
  SDL_Surface *windowSurface = NULL;
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
  {
    LogError("Unable to Init SDL, Error: %s", SDL_GetError());
    return -1;
  }

  window = SDL_CreateWindow(_controlBlock.windowTitle,
                            SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED,
                            _controlBlock.windowWidth,
                            _controlBlock.windowHeight,
                            SDL_WINDOW_SHOWN);
  if (window == NULL)
  {
    LogError("Unable to create window, Error: %s", SDL_GetError());
    goto quit_on_error;
  }
  windowSurface = SDL_GetWindowSurface(window);

  if (_controlBlock.preStart != NULL)
  {
    _controlBlock.preStart();
  }

  SDL_Event event;
  bool run = true;
  float deltaTime = 1.0 / 60;
  uint32_t frameStartTicks;
  uint32_t frameEndTicks;
  uint32_t frameTickDuration;

  while (run)
  {
    frameStartTicks = SDL_GetTicks();

    while (SDL_PollEvent(&event))
    {
      switch (event.type)
      {
      case SDL_QUIT:
        run = false;
        break;
      default:
        break;
      }
      if (_controlBlock.userEventHandler != NULL)
      {
        _controlBlock.userEventHandler(&event);
      }
    }

    if (_controlBlock.update != NULL)
    {
      if (false == _controlBlock.update(deltaTime))
      {
        run = false;
      }
    }

    SDL_FillRect(windowSurface, NULL,
                 SDL_MapRGB(windowSurface->format, 0x33, 0x99, 0xFF));

    if (_controlBlock.draw != NULL)
    {
      _controlBlock.draw(windowSurface);
    }

    frameEndTicks = SDL_GetTicks();
    frameTickDuration = frameEndTicks - frameStartTicks;
    if (frameTickDuration < _controlBlock.targetFrameTime_ms)
    {
      deltaTime = _controlBlock.targetFrameTime_ms / 1000.0;
      SDL_Delay(_controlBlock.targetFrameTime_ms - frameTickDuration);
    }
    else
    {
      deltaTime = frameTickDuration / 1000.0;
    }

    SDL_UpdateWindowSurface(window);
  }

  //close_window_on_error:
  SDL_DestroyWindow(window);
  quit_on_error:
  SDL_Quit();

  return 0;
}

SDL_Surface* SharedSDL_LoadImage(const char* filepath)
{
    SDL_Surface* image = SDL_LoadBMP(filepath);
    if (image == NULL)
    {
        LogError("Unable to load image %s, Error: %s", filepath, SDL_GetError());
        return NULL;
    }
    return image;
}
