/*
 * PPU.h
 *
 *  Created on: Sep 25, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_PPU_H_
#define SRC_NES_PPU_H_

#include <SDL2/SDL.h>
#include "Types.h"
#include "ClockedRegister.h"

#define PPU_NUM_SCANLINES         (262)
#define PPU_PRE_RENDER_SCANLINE   (PPU_NUM_SCANLINES - 1)

typedef struct _Bus_t Bus_t;

#pragma pack(push, 1)
typedef struct _OAMEntry_t
{
  u8_t Y;                // Y coordinate of top of sprite
  u8_t TileIndex;        // Tile index
  u8_t Attributes;       // Attributes for the sprite
  u8_t X;                // X coordinate of left of sprite
} OAMEntry_t;
#pragma pack(pop)

typedef struct
{
  u8_t X;
  u8_t SRPatternHigh;
  u8_t SRPatternLow;
  u8_t Attributes;
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
  u8_t AddressLatch;     // Address latch for '16 bit' registers like Address and Scroll
  u8_t DataBuffer;       // Buffer for Data register reads (they are delayed)
  u8_t LatchedData;      // Represents data lines still having the previous values when
                            // reading from non readable registers (it returns this instead)

  // Frame lines
  u16f_t VCount;     // Scanline
  u16f_t HCount;     // Dot (or pixel) horizontally on the scanline, starts at 0
  bool IsEvenFrame;         // Toggle indicating if we are on an odd or even frame

  // VRAM Address Registers
  // V and T have the same internal structure
  //  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
  //  y   y   y   N   N   Y   Y   Y   Y   Y   X   X   X   X   X
  // y = Fine Y scroll
  // N = Nametable
  // Y = Coarse Y scroll
  // X = Coarse X scroll
  u16f_t V;       // The VRAM address that is actually used
  u16f_t T;       // A temporary VRAM address, most CPU actions modify this one
  u8f_t X;        // Fine X offset, 3 bits, 1 MSB = 1 pixel

  // Rendering shift registers
  u16_t SRPatternHigh;   // High bit of pattern
  u16_t SRPatternLow;    // Low bit of pattern
  u16_t SRAttributeHigh; // High bit of attribute
  u16_t SRAttributeLow;  // Low bit of attribute

  u8f_t NextBgTileId;
  u8f_t NextBgAttribute;
  u8f_t NextBgTileLow;
  u8f_t NextBgTileHigh;

  // OAM
  OAMEntry_t OAM[64];       // Object Attribute Memory for storing sprite data
  u8_t *OAMAsPtr;        // Pointer to OAM for indexing actions

  OAMEntry_t ActiveSpriteOAM[8];      // OAM for sprites on current scanline
  u8_t *ActiveSpriteOAMAsPtr;      // Pointer to ActiveSpriteOAM
  SpriteData_t ActiveSpriteData[8];   // Other data for sprites on current scanline
  u8_t SpriteEval_NumberOfSprites;
  u8_t SpriteEval_OAMSpriteIndex;
  u8_t SpriteEval_SpriteByteIndex;
  u8_t SpriteEval_TempSpriteData;
  SpriteEvalState_t SpriteEval_State;
} PPU_t;

void PPU_Initialize(PPU_t *ppu);
void PPU_Tick(PPU_t *ppu);
void PPU_ClockRegisters(PPU_t *ppu);
void PPU_Reset(PPU_t *ppu);
u8_t PPU_ReadFromCpu(PPU_t *ppu, u16_t address);
void PPU_WriteFromCpu(PPU_t *ppu, u16_t address, u8_t data);
void PPU_SetRenderSurface(SDL_Surface *surface);
void PPU_RenderPixel(const PPU_t *ppu, u16f_t x, u16f_t y, u8_t pixel, u8_t palette);
#endif /* SRC_NES_PPU_H_ */
