/*
 * Palette.c
 *
 *  Created on: Sep 29, 2019
 *      Author: wouter
 */

#include "Palette.h"
#include "log.h"

#include <stdio.h>

#define PALETTE_ENTRIES   0x40
uint8_t _palette[PALETTE_ENTRIES * 3];

void Palette_LoadFrom(const char* file)
{
  FILE *f;

  f = fopen(file, "rb");
  if (f == NULL)
  {
    LogError("Unable to open file %s", file);
    return;
  }

  if (fread(_palette, 1, PALETTE_ENTRIES * 3, f) != PALETTE_ENTRIES * 3)
  {
    LogError("Unable to read from file");
    fclose(f);
    return;
  }

  fclose(f);
  LogMessage("Loaded palette from %s", file);
}

void Palette_GetRGB(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b)
{
  if (index >= PALETTE_ENTRIES)
  {
    LogError("Palette index out of bounds: %d", index);
    index = PALETTE_ENTRIES;
  }
  *r = _palette[index * 3];
  *g = _palette[index * 3 + 1];
  *b = _palette[index * 3 + 2];
}