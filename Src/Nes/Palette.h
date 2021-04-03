/*
 * Palette.h
 *
 *  Created on: Sep 29, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_PALETTE_H_
#define SRC_NES_PALETTE_H_

#include "Types.h"

void Palette_LoadFrom(const char* file);
void Palette_GetRGB(u8_t index, u8_t *r, u8_t *g, u8_t *b);

#endif /* SRC_NES_PALETTE_H_ */
