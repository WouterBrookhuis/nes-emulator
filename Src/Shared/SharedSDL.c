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
  SharedSDL_GetAudioSamples getAudioSamples;
} ControlBlock_t;

uint64_t _perfTimerStack[NR_OF_PERF_COUNTERS];
uint64_t _perfTimerAccum[NR_OF_PERF_COUNTERS];
uint32_t _perfTimerCount[NR_OF_PERF_COUNTERS];
unsigned int _perfTimerStackIndex;

static SDL_AudioDeviceID _audioDevice;
static SDL_AudioSpec _audioSpec;

static ControlBlock_t _controlBlock;

static void AudioCallback(void *userdata, uint8_t *stream, int len);

void SharedSDL_Initialize(int windowWidth,
                          int windowHeight,
                          const char* windowTitle,
                          SharedSDL_PreStart preStart,
                          SharedSDL_Update update,
                          SharedSDL_Draw draw,
                          SharedSDL_EventHandler eventHandler,
                          SharedSDL_GetAudioSamples audioCallback)
{
  _controlBlock.windowWidth = windowWidth;
  _controlBlock.windowHeight = windowHeight;
  _controlBlock.windowTitle = windowTitle;
  _controlBlock.draw = draw;
  _controlBlock.preStart = preStart;
  _controlBlock.update = update;
  _controlBlock.userEventHandler = eventHandler;
  _controlBlock.getAudioSamples = audioCallback;
  _controlBlock.targetFrameTime_ms = 16;
}

int SharedSDL_Start()
{
  SDL_Window *window = NULL;
  SDL_Surface *windowSurface = NULL;
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
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

  int numAudioDevices = SDL_GetNumAudioDevices(0);
  LogMessage("Found %d audio devices", numAudioDevices);
  for (int i = 0; i < numAudioDevices; i++)
  {
    const char* name = SDL_GetAudioDeviceName(i, 0);
    LogMessage("[%d] %s", i, name);
  }

  SDL_AudioSpec want;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_S32;
  want.channels = 1;
  want.samples = 1024;
  want.callback = AudioCallback;

  _audioDevice = SDL_OpenAudioDevice(NULL, 0, &want, &_audioSpec, 0);
  if (0 == _audioDevice)
  {
    LogError("Unable to open audio device, Error: %s", SDL_GetError());
    goto close_window_on_error;
  }

  SharedSDL_StartAudio();

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

  LogMessage("PerfCounterFrequency = %lu", SDL_GetPerformanceFrequency());

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

    memset(windowSurface->pixels, 0x00, windowSurface->h * windowSurface->pitch);

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

#if ENABLE_PERF_TIMING
    for(uint_fast8_t i = 0; i < NR_OF_PERF_COUNTERS; i++)
    {
      printf("%I64u, ", _perfTimerAccum[i]);
      SharedSDL_ResetTiming(i);
    }

    printf("\n");
#endif

    fflush(stdout);
  }

  SDL_CloseAudioDevice(_audioDevice);
  close_window_on_error:
  SDL_DestroyWindow(window);
  quit_on_error:
  SDL_Quit();

  return 0;
}

void SharedSDL_StopAudio()
{
  SDL_PauseAudioDevice(_audioDevice, 1);
}

void SharedSDL_StartAudio()
{
  SDL_PauseAudioDevice(_audioDevice, 0);
}

#if ENABLE_PERF_TIMING
void SharedSDL_BeginTiming(unsigned int index)
{
  _perfTimerStack[index] = SDL_GetPerformanceCounter();
}

void SharedSDL_EndTiming(unsigned int index)
{
  uint64_t delta = SDL_GetPerformanceCounter() - _perfTimerStack[index];
  _perfTimerAccum[index] += delta;
  _perfTimerCount[index]++;
}

void SharedSDL_ResetTiming(unsigned int index)
{
  _perfTimerAccum[index] = 0;
  _perfTimerCount[index] = 0;
}

void SharedSDL_PrintTiming(unsigned int index, const char *name)
{
  uint64_t avg = 0;
  if (_perfTimerCount[index] != 0)
  {
    avg = _perfTimerAccum[index] / _perfTimerCount[index];
  }

  LogMessage("[%s] Avg: %lu Total: %lu", name, avg, _perfTimerAccum[index]);
}
#endif

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

static void AudioCallback(void *userdata, uint8_t *stream, int len)
{
  _controlBlock.getAudioSamples(_audioDevice, (int32_t*) stream, len / sizeof(int32_t), _audioSpec.freq);
}
