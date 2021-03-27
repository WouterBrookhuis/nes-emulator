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
#include "ClockedRegister.h"

typedef struct _Bus_t Bus_t;

#pragma pack(push, 1)
typedef struct _OAMEntry_t
{
  uint8_t Y;                // Y coordinate of top of sprite
  uint8_t TileIndex;        // Tile index
  uint8_t Attributes;       // Attributes for the sprite
  uint8_t X;                // X coordinate of left of sprite
} OAMEntry_t;
#pragma pack(pop)

typedef struct
{
  uint8_t X;
  uint8_t SRPatternHigh;
  uint8_t SRPatternLow;
  uint8_t Attributes;
} SpriteData_t;

typedef enum
{
  SPRITE_EVAL_STATE_NEW_SPRITE,
  SPRITE_EVAL_STATE_COPY_SPRITE,
  SPRITE_EVAL_STATE_OVERFLOW,
  SPRITE_EVAL_STATE_END
} SpriteEvalState_t;

typedef struct _PPU_t
{
  unsigned int PhaseCounter;
  unsigned int FrameCount;
  unsigned int CycleCount;          // Total cycle count, debugging info
  unsigned int CyclesSinceReset;    // Amount of cycles since last reset
  Bus_t *Bus;               // The bus this PPU is connected to

  // Registers
  cr8_t Ctrl;               // Control register, various things
  cr8_t Mask;               // Mask register, rendering related flags
  cr8_t Status;             // Status register, has PPU status flags
  cr8_t OAMAddress;         // Address used for OAM access
  cr8_t OAMData;            // Data register used to read or write data to OAM
  cr8_t Scroll;             // Scroll register, used for writing scroll offsets
  cr8_t Data;               // Data register used for read/write via PPU bus from CPU

  // Wonky things
  uint8_t AddressLatch;     // Address latch for '16 bit' registers like Address and Scroll
  uint8_t DataBuffer;       // Buffer for Data register reads (they are delayed)
  uint8_t LatchedData;      // Represents data lines still having the previous values when
                            // reading from non readable registers (it returns this instead)
  uint8_t SuppressNMI;      // Used to suppress automatic VBlank NMI if STATUS is read shortly before VBLANK is set

  // Frame lines
  int VCount;               // Scanline, pre-render is -1
  int HCount;               // Dot (or pixel) horizontally on the scanline, starts at 0
  bool IsEvenFrame;         // Toggle indicating if we are on an odd or even frame

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

  // OAM
  OAMEntry_t OAM[64];       // Object Attribute Memory for storing sprite data
  uint8_t *OAMAsPtr;        // Pointer to OAM for indexing actions

  OAMEntry_t ActiveSpriteOAM[8];      // OAM for sprites on current scanline
  uint8_t *ActiveSpriteOAMAsPtr;      // Pointer to ActiveSpriteOAM
  SpriteData_t ActiveSpriteData[8];   // Other data for sprites on current scanline
  uint8_t SpriteEval_NumberOfSprites;
  uint8_t SpriteEval_OAMSpriteIndex;
  uint8_t SpriteEval_SpriteByteIndex;
  uint8_t SpriteEval_TempSpriteData;
  SpriteEvalState_t SpriteEval_State;
} PPU_t;

void PPU_Initialize(PPU_t *ppu);
void PPU_Tick(PPU_t *ppu);
void PPU_ClockRegisters(PPU_t *ppu);
void PPU_Reset(PPU_t *ppu);
uint8_t PPU_ReadFromCpu(PPU_t *ppu, uint16_t address);
void PPU_WriteFromCpu(PPU_t *ppu, uint16_t address, uint8_t data);
void PPU_SetRenderSurface(SDL_Surface *surface);
void PPU_RenderPixel(PPU_t *ppu, int x, int y, uint8_t pixel, uint8_t palette);
#endif /* SRC_NES_PPU_H_ */
