/*
 * SharedSDL.c
 *
 *  Created on: Sep 19, 2019
 *      Author: wouter
 */
#include "SharedSDL.h"

#include <SDL2/SDL.h>
#include "log.h"
#include <stdio.h>
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

#define MAX_PERF_TIMER_STACK_DEPTH    (20)

uint64_t _perfTimerStack[MAX_PERF_TIMER_STACK_DEPTH];
uint64_t _perfTimerAccum[MAX_PERF_TIMER_STACK_DEPTH];
uint32_t _perfTimerCount[MAX_PERF_TIMER_STACK_DEPTH];
unsigned int _perfTimerStackIndex;


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

  LogMessage("PerfCounterFrequency = %ul", SDL_GetPerformanceFrequency());

  while (run)
  {
    frameStartTicks = SDL_GetTicks();

    SharedSDL_BeginTiming(0);

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

    SharedSDL_EndTiming(0);

    SharedSDL_BeginTiming(1);

    memset(windowSurface->pixels, 0x00, windowSurface->h * windowSurface->pitch);

    if (_controlBlock.draw != NULL)
    {
      _controlBlock.draw(windowSurface);
    }

    SharedSDL_EndTiming(1);

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

//    SharedSDL_PrintTiming(0, "Update");
//    SharedSDL_PrintTiming(1, "Draw");
//    SharedSDL_PrintTiming(2, "PPU");
//    SharedSDL_PrintTiming(3, "CPU");

    for(uint_fast8_t i = 0; i < 5; i++)
    {
      printf("%I64u, ", _perfTimerAccum[i]);
      _perfTimerAccum[i] = 0;
    }

    printf("\n");
  }

  //close_window_on_error:
  SDL_DestroyWindow(window);
  quit_on_error:
  SDL_Quit();

  return 0;
}

void SharedSDL_BeginTiming(uint32_t index)
{
  _perfTimerStack[index] = SDL_GetPerformanceCounter();
}

void SharedSDL_EndTiming(uint32_t index)
{
  uint64_t delta = SDL_GetPerformanceCounter() - _perfTimerStack[index];
  _perfTimerAccum[index] += delta;
  _perfTimerCount[index]++;
}

void SharedSDL_ResetTiming(uint32_t index)
{
  _perfTimerAccum[index] = 0;
  _perfTimerCount[index] = 0;
}

void SharedSDL_PrintTiming(uint32_t index, const char *name)
{
  uint64_t avg = 0;
  if (_perfTimerCount[index] != 0)
  {
    avg = _perfTimerAccum[index] / _perfTimerCount[index];
  }

  LogMessage("[%s] Avg: %lu Total: %lu", name, avg, _perfTimerAccum[index]);
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
