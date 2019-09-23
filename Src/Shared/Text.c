/*
 * Text.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#include "Text.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "SharedSDL.h"
#include "log.h"


static SDL_Rect GetRectOnFontAtlas(Font_t *font,
                                   unsigned char charToFind);

bool Text_LoadFont(Font_t *font,
                   const char *imageFile,
                   int glyphWidth,
                   int glyphHeight)
{
  SDL_Surface *atlas;

  if (font == NULL)
  {
    LogError("Can not load font into NULL font struct");
    return false;
  }

  if (font->Atlas != NULL)
  {
    LogError("Trying to load font into non empty font struct");
    return false;
  }

  atlas = SharedSDL_LoadImage(imageFile);
  if (atlas == NULL)
  {
    LogError("Unable to load image for font");
    return false;
  }

  memset(font, 0, sizeof(*font));

  font->Atlas = atlas;
  font->GlyphWidth = glyphWidth;
  font->GlyphHeight = glyphHeight;
  font->NumColumns = atlas->w / glyphWidth;
  font->NumRows = atlas->h / glyphHeight;

  LogMessage("Loaded font %s", imageFile);
  return true;
}

void Text_DrawString(SDL_Surface *surface,
                     const char *text,
                     int x,
                     int y,
                     Font_t *font)
{
  char* currentChar = (char *)text;
  SDL_Rect dstRect =
  {
      w: font->GlyphWidth,
      h: font->GlyphHeight,
      x: x,
      y: y
  };

  if (font->Atlas == NULL)
  {
    return;
  }

  while (*currentChar != 0x00)
  {
    SDL_Rect rect = GetRectOnFontAtlas(font, (unsigned char)*currentChar);
    SDL_BlitSurface(font->Atlas, &rect,
                    surface, &dstRect);
    dstRect.x += font->GlyphWidth;
    currentChar++;
  }
}

void Text_DrawStringWrapping(SDL_Surface *surface,
                             const char *text,
                             int x,
                             int y,
                             int lineLength,
                             Font_t *font)
{
  int offset = 0;
  char* currentChar = (char *)text;
  SDL_Rect dstRect =
  {
      w: font->GlyphWidth,
      h: font->GlyphHeight,
      x: x,
      y: y
  };

  if (font->Atlas == NULL)
  {
    return;
  }

  while (*currentChar != 0x00)
  {
    if (*currentChar == 0x0D || *currentChar == 0x0A)
    {
      // CR or LF
      dstRect.x = x;
      offset = 0;
      dstRect.y += font->GlyphHeight;
    }
    else
    {
      SDL_Rect rect = GetRectOnFontAtlas(font, (unsigned char)*currentChar);
      SDL_BlitSurface(font->Atlas, &rect,
                      surface, &dstRect);
      dstRect.x += font->GlyphWidth;
      offset++;
      if (offset == lineLength)
      {
        dstRect.y += font->GlyphHeight;
        dstRect.x = x;
        offset = 0;
      }
    }

    currentChar++;
  }
}

static SDL_Rect GetRectOnFontAtlas(Font_t *font,
                                   unsigned char charToFind)
{
  SDL_Rect rect;
  int numberOfGlyphs;

  rect.w = font->GlyphWidth;
  rect.h = font->GlyphHeight;

  numberOfGlyphs = font->NumColumns * font->NumRows;
  if (charToFind >= numberOfGlyphs)
  {
    rect.x = 0;
    rect.y = 0;
    return rect;
  }

  rect.x = (charToFind % font->NumColumns) * font->GlyphWidth;
  rect.y = (charToFind / font->NumColumns) * font->GlyphHeight;

  return rect;
}
