/*
 * Text.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_SHARED_TEXT_H_
#define SRC_SHARED_TEXT_H_

#include <SDL2/SDL.h>
#include <stdbool.h>

typedef struct
{
  SDL_Surface *Atlas;
  int NumRows;
  int NumColumns;
  int GlyphWidth;
  int GlyphHeight;
} Font_t;

bool Text_LoadFont(Font_t *font,
                   const char *imageFile,
                   int glyphWidth,
                   int glyphHeight);

void Text_DrawString(SDL_Surface *surface,
                     const char *text,
                     int x,
                     int y,
                     Font_t *font);

void Text_DrawStringWrapping(SDL_Surface *surface,
                             const char *text,
                             int x,
                             int y,
                             int lineLength,
                             Font_t *font);

#endif /* SRC_SHARED_TEXT_H_ */
