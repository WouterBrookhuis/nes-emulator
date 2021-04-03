/*
 * SharedSDL.h
 *
 *  Created on: Sep 19, 2019
 *      Author: wouter
 */

#ifndef SRC_SHARED_SHAREDSDL_H_
#define SRC_SHARED_SHAREDSDL_H_

#include <stdbool.h>
#include <SDL2/SDL.h>

#define ENABLE_PERF_TIMING      (0)

typedef enum
{
  PERF_INDEX_PPU,
  PERF_INDEX_CPU,
  PERF_INDEX_EMULATE,
  PERF_INDEX_PPU_BG_FETCH,
  PERF_INDEX_PPU_SPRITE_EVAL,
  PERF_INDEX_PPU_SPRITE_FETCH,
  PERF_INDEX_PPU_ORDERING,
  PERF_INDEX_PPU_INCREMENTS,
  PERF_INDEX_PPU_PIXEL_OUT,
  NR_OF_PERF_COUNTERS
} PerfIndex_t;

typedef void (*SharedSDL_PreStart)(void);
typedef bool (*SharedSDL_Update)(float);
typedef void (*SharedSDL_Draw)(SDL_Surface*);
typedef void (*SharedSDL_EventHandler)(SDL_Event*);
typedef void (*SharedSDL_GetAudioSamples)(SDL_AudioDeviceID device, int32_t *samples, uint32_t numSamples, uint32_t sampleRate);

void SharedSDL_Initialize(int windowWidth,
                          int windowHeight,
                          const char* windowTitle,
                          SharedSDL_PreStart preStart,
                          SharedSDL_Update update,
                          SharedSDL_Draw draw,
                          SharedSDL_EventHandler eventHandler,
                          SharedSDL_GetAudioSamples audioCallback);

int SharedSDL_Start();

void SharedSDL_StopAudio();

void SharedSDL_StartAudio();

SDL_Surface* SharedSDL_LoadImage(const char* filepath);

#if ENABLE_PERF_TIMING
void SharedSDL_BeginTiming(unsigned int index);

void SharedSDL_EndTiming(unsigned int index);

void SharedSDL_ResetTiming(unsigned int index);

void SharedSDL_PrintTiming(unsigned int index, const char *name);
#else
#define SharedSDL_BeginTiming(index)

#define SharedSDL_EndTiming(index)

#define SharedSDL_ResetTiming(index)

#define SharedSDL_PrintTiming(index, name)
#endif
#endif /* SRC_SHARED_SHAREDSDL_H_ */
