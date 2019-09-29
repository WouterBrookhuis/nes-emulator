/*
 * PPU.h
 *
 *  Created on: Sep 25, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_PPU_H_
#define SRC_NES_PPU_H_

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

typedef struct _Bus_t Bus_t;

typedef struct _PPU_t
{
  unsigned int CycleCount;          // Total cycle count, debugging info
  unsigned int CyclesSinceReset;    // Amount of cycles since last reset
  Bus_t *Bus;

  // Registers
  uint8_t Ctrl;
  uint8_t Mask;
  uint8_t Status;
  uint8_t OAMAddress;
  uint8_t OAMData;
  uint8_t Scroll;
  //uint8_t Address;
  uint8_t Data;
  uint8_t OAMDma;

  // Wonky things
  uint8_t AddressLatch;
  uint8_t DataBuffer;
  uint8_t LatchedData;

  // Frame lines
  int VCount;
  int HCount;
  bool IsEvenFrame;

  // VRAM Address Registers
  // V and T have the same internal structure
  //  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
  //  y   y   y   N   N   Y   Y   Y   Y   Y   X   X   X   X   X
  // y = Fine Y scroll
  // N = Nametable
  // Y = Coarse Y scroll
  // X = Coarse X scroll
  uint16_t V;       // The VRAM address that is actually used
  uint16_t T;       // A temporary VRAM address, most CPU actions modify this one
  uint8_t X;        // Fine X offset, 3 bits, 1 MSB = 1 pixel

  // Rendering shift registers
  uint16_t SRPatternHigh;   // High bit of pattern
  uint16_t SRPatternLow;    // Low bit of pattern
  uint16_t SRAttributeHigh; // High bit of attribute
  uint16_t SRAttributeLow;  // Low bit of attribute

  uint8_t NextBgTileId;
  uint8_t NextBgAttribute;
  uint8_t NextBgTileLow;
  uint8_t NextBgTileHigh;

} PPU_t;

void PPU_Initialize(PPU_t *ppu);
void PPU_Tick(PPU_t *ppu);
void PPU_Reset(PPU_t *ppu);
uint8_t PPU_ReadFromCpu(PPU_t *ppu, uint16_t address);
void PPU_WriteFromCpu(PPU_t *ppu, uint16_t address, uint8_t data);
void PPU_SetRenderSurface(SDL_Surface *surface);
#endif /* SRC_NES_PPU_H_ */
