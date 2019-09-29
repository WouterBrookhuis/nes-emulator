/*
 * Palette.h
 *
 *  Created on: Sep 29, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_PALETTE_H_
#define SRC_NES_PALETTE_H_

#include <stdint.h>

void Palette_LoadFrom(const char* file);
void Palette_GetRGB(uint8_t index, uint8_t *r, uint8_t *g, uint8_t *b);

#endif /* SRC_NES_PALETTE_H_ */
