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

typedef void (*SharedSDL_PreStart)(void);
typedef bool (*SharedSDL_Update)(float);
typedef void (*SharedSDL_Draw)(SDL_Surface*);
typedef void (*SharedSDL_EventHandler)(SDL_Event*);

void SharedSDL_Initialize(int windowWidth,
                          int windowHeight,
                          const char* windowTitle,
                          SharedSDL_PreStart preStart,
                          SharedSDL_Update update,
                          SharedSDL_Draw draw,
                          SharedSDL_EventHandler eventHandler);

int SharedSDL_Start();

SDL_Surface* SharedSDL_LoadImage(const char* filepath);

void SharedSDL_BeginTiming(uint32_t index);

void SharedSDL_EndTiming(uint32_t index);

void SharedSDL_ResetTiming(uint32_t index);

void SharedSDL_PrintTiming(uint32_t index, const char *name);

#endif /* SRC_SHARED_SHAREDSDL_H_ */
