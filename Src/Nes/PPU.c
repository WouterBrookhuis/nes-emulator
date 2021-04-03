/*
 * PPU.c
 *
 *  Created on: Sep 25, 2019
 *      Author: wouter
 */

#include "PPU.h"
#include "Bus.h"
#include "Palette.h"
#include <string.h>
#include <SDL2/SDL.h>
#include "SharedSDL.h"
#include "log.h"

#define CTRLFLAG_NAMETABLE_MASK       0x03
#define CTRLFLAG_VRAM_INCREMENT       0x04
#define CTRLFLAG_SPRITE_ADDRESS       0x08
#define CTRLFLAG_BACKGROUND_ADDRESS   0x10
#define CTRLFLAG_SPRITE_SIZE          0x20
#define CTRLFLAG_MASTER_SELECT        0x40
#define CTRLFLAG_VBLANK_NMI           0x80

#define MASKFLAG_GREYSCALE            0x01
#define MASKFLAG_BACKGROUND_LEFT      0x02
#define MASKFLAG_SPRITES_LEFT         0x04
#define MASKFLAG_BACKGROUND           0x08
#define MASKFLAG_SPRITES              0x10
#define MASKFLAG_EMPHASIZE_RED        0x20
#define MASKFLAG_EMPHASIZE_GREEN      0x40
#define MASKFLAG_EMPHASIZE_BLUE       0x80

#define STATFLAG_GARBAGE_MASK         0x1F
#define STATFLAG_SPRITE_OVERFLOW      0x20
#define STATFLAG_SPRITE_0_HIT         0x40
#define STATFLAG_VBLANK               0x80

#define ATTRFLAG_PALLETE_MASK         0x03
#define ATTRFLAG_NOTHING_MASK         0x1C      // TODO: This needs to apply to OAM reads and writes as well
#define ATTRFLAG_PRIORITY             0x20
#define ATTRFLAG_FLIP_HORIZONTAL      0x40
#define ATTRFLAG_FLIP_VERTICAL        0x80

static u8_t BIT_REVERSE_TABLE[16] =
{
    0b0000, 0b1000, 0b0100, 0b1100, 0b0010, 0b1010, 0b0110, 0b1110,
    0b0001, 0b1001, 0b0101, 0b1101, 0b0011, 0b1011, 0b0111, 0b1111
};

static SDL_Surface *_renderSurface;

static inline uint_fast32_t IsInRange(uint_fast32_t low, uint_fast32_t high, uint_fast32_t value)
{
  return (value - low) <= (high - low);
}

static inline u8_t ReverseByte(u8_t byte)
{
  return ((BIT_REVERSE_TABLE[byte & 0xF] << 4) | (BIT_REVERSE_TABLE[byte >> 4]));
}

static inline bool IsRendering(PPU_t *ppu)
{
  return CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND) || CR8_IsBitSet(ppu->Mask, MASKFLAG_SPRITES);
}

void PPU_RenderPixel(const PPU_t *ppu, u16f_t x, u16f_t y, u8_t pixel, u8_t palette)
{
  if (_renderSurface == NULL)
  {
    return;
  }

  if (!IsInRange(0, _renderSurface->w, x) || !IsInRange(0, _renderSurface->h, y))
  {
    return;
  }

  // Palette entry 0 always maps to the universal background of palette 0
  if (pixel == 0)
  {
    palette = 0;
  }

  u8_t colorPaletteIndex = Bus_ReadFromPPU(ppu->Bus, 0x3F00 + (palette << 2) + pixel);
  if (CR8_IsBitSet(ppu->Mask, MASKFLAG_GREYSCALE))
  {
    colorPaletteIndex &= 0x30;
  }

  u8_t r;
  u8_t g;
  u8_t b;
  u8_t *pixelPtr = (u8_t*)_renderSurface->pixels +
                    _renderSurface->pitch * y +
                    _renderSurface->format->BytesPerPixel * x;
  Palette_GetRGB(colorPaletteIndex, &r, &g, &b);
  *(u32_t*)pixelPtr = SDL_MapRGB(_renderSurface->format, r, g, b);
}

void PPU_SetRenderSurface(SDL_Surface *surface)
{
  _renderSurface = surface;
}

void PPU_Initialize(PPU_t *ppu)
{
  memset(ppu, 0, sizeof(*ppu));

  CR8_WriteImmediate(&ppu->Ctrl, 0x00);
  CR8_WriteImmediate(&ppu->Mask, 0x00);
  CR8_WriteImmediate(&ppu->Status, 0xA0);
  CR8_WriteImmediate(&ppu->OAMAddress, 0x00);
  CR8_WriteImmediate(&ppu->Scroll, 0x00);
  CR8_WriteImmediate(&ppu->Data, 0x00);

  ppu->VCount = PPU_PRE_RENDER_SCANLINE;

  ppu->OAMAsPtr = (u8_t *)&ppu->OAM;
  ppu->ActiveSpriteOAMAsPtr = (u8_t *)&ppu->ActiveSpriteOAM;
}

void PPU_Reset(PPU_t *ppu)
{
  CR8_Reset(&ppu->Ctrl);
  CR8_Reset(&ppu->Mask);
  CR8_WriteImmediate(&ppu->Status, CR8_Read(ppu->Status));
  CR8_Reset(&ppu->Scroll);
  CR8_Reset(&ppu->Data);
}

static void IncrementCoarseX(PPU_t *ppu)
{
  if ((ppu->V & 0x001F) == 31)
  {
    // Coarse X is 31, reset it to 0 and toggle H nametable
    ppu->V &= ~0x001F;
    ppu->V ^= 0x0400;
  }
  else
  {
    // Increment coarse X
    ppu->V++;
  }
}

static void IncrementY(PPU_t *ppu)
{
  if ((ppu->V & 0x7000) != 0x7000)
  {
    // Increment fine Y as long as it isn't 7 yet
    ppu->V += 0x1000;
  }
  else
  {
    // Reset fine Y to 0
    ppu->V &= ~0x7000;
    u16f_t y = (ppu->V & 0x03E0) >> 5;
    if (y == 29)
    {
      // Reset coarse Y and toggle V nametable
      y = 0;
      ppu->V ^= 0x0800;
    }
    else if (y == 31)
    {
      // Only reset coarse Y
      y = 0;
    }
    else
    {
      // Increment coarse Y
      y++;
    }
    // Write back Y to V
    ppu->V = (ppu->V & ~0x03E0) | (y << 5);
  }
}

static u16_t CalculateSpriteAddress(u16f_t vCount, u16f_t ctrl, const OAMEntry_t *spriteOAM, bool flipVertical)
{
  // If CTRLFLAG_SPRITE_ADDRESS (0x80) is set we use base address 0x1000 (0x80 << 5)
  u16f_t address = (ctrl & CTRLFLAG_SPRITE_ADDRESS) << 5;
  // Add the tile offset
  address |= ((u16f_t) spriteOAM->TileIndex << 4);
  // Add the row offset
  u16f_t rowOffset = (vCount - (u16f_t) spriteOAM->Y);
  if (flipVertical)
  {
    rowOffset = ~rowOffset;
  }
  address |= rowOffset & 0x7;
  return address;
}

void PPU_ClockRegisters(PPU_t *ppu)
{
  if (ppu->PhaseCounter != 1)
  {
    return;
  }

  CR8_Clock(&ppu->Ctrl);
  CR8_Clock(&ppu->Mask);
  CR8_Clock(&ppu->Status);
  CR8_Clock(&ppu->OAMAddress);
  CR8_Clock(&ppu->OAMData);
  CR8_Clock(&ppu->Scroll);
  CR8_Clock(&ppu->Data);
}

void PPU_Tick(PPU_t *ppu)
{
  SharedSDL_BeginTiming(PERF_INDEX_PPU);

  bool isPreRenderScanline = (PPU_PRE_RENDER_SCANLINE == ppu->VCount);
  bool isVisibleScanline = IsInRange(0, 239, ppu->VCount);

  ppu->PhaseCounter = 1;

  // Increment cycles
  ppu->CycleCount++;
  ppu->CyclesSinceReset++;

  // Do PPU things

  SharedSDL_BeginTiming(PERF_INDEX_PPU_BG_FETCH);

  // Visible scanlines and pre-render scanline
  if (isPreRenderScanline || isVisibleScanline)
  {
    // Memory accessing to get background data
    if (IsInRange(1, 257, ppu->HCount) || IsInRange(321, 337, ppu->HCount))
    {
      u8_t pixelCycle = ppu->HCount & 0x07;

      if (pixelCycle == 1)
      {
        // Reload shift registers, but not on tick 1 for this scanline
        if (ppu->HCount > 1)
        {
          ppu->SRPatternLow =     (ppu->SRPatternLow & 0xFF00)    | (ppu->NextBgTileLow);
          ppu->SRPatternHigh =    (ppu->SRPatternHigh & 0xFF00)   | (ppu->NextBgTileHigh);
          ppu->SRAttributeLow =   (ppu->SRAttributeLow & 0xFF00)  | (ppu->NextBgAttribute & 0x01 ? 0xFF : 0x00);
          ppu->SRAttributeHigh =  (ppu->SRAttributeHigh & 0xFF00) | (ppu->NextBgAttribute & 0x02 ? 0xFF : 0x00);
        }
        // Fetch NT
        ppu->NextBgTileId = Bus_ReadFromPPU(ppu->Bus, 0x2000 | (ppu->V & 0x0FFF));
      }
      else if (pixelCycle == 3)
      {
        // Fetch AT
        ppu->NextBgAttribute = Bus_ReadFromPPU(ppu->Bus,
                                           0x23C0
                                           | (ppu->V & 0x0C00)
                                           | ((ppu->V >> 4) & 0x0038)
                                           | ((ppu->V >> 2) & 0x0007));
        if (ppu->V & 0x40)
        {
          // Bit 1 of coarse Y is set
          ppu->NextBgAttribute >>= 4;
        }
        if (ppu->V & 0x02)
        {
          // Bit 1 of coarse X is set
          ppu->NextBgAttribute >>= 2;
        }
        ppu->NextBgAttribute &= 0x03;
      }
      else if (pixelCycle == 5)
      {
        // Fetch low BG tile byte
        ppu->NextBgTileLow = Bus_ReadFromPPU(ppu->Bus,
                                             (CR8_IsBitSet(ppu->Ctrl, CTRLFLAG_BACKGROUND_ADDRESS) ? 0x1000 : 0x000)
                                             + ((u16f_t)ppu->NextBgTileId << 4)
                                             + ((ppu->V >> 12) & 0x07) + 0);
      }
      else if (pixelCycle == 7)
      {
        // Fetch high BG tile byte
        ppu->NextBgTileHigh = Bus_ReadFromPPU(ppu->Bus,
                                              (CR8_IsBitSet(ppu->Ctrl, CTRLFLAG_BACKGROUND_ADDRESS) ? 0x1000 : 0x000)
                                              + ((u16f_t)ppu->NextBgTileId << 4)
                                              + ((ppu->V >> 12) & 0x07) + 8);
      }
    }
  }

  SharedSDL_EndTiming(PERF_INDEX_PPU_BG_FETCH);

  SharedSDL_BeginTiming(PERF_INDEX_PPU_SPRITE_EVAL);

  if (isVisibleScanline)
  {
    if (IsInRange(1, 256, ppu->HCount))
    {
      if (ppu->HCount < 65)
      {
        // Sprite evaluation: Clear secondary OAM
        u8f_t byte = (ppu->HCount - 1) / 2;

        ppu->ActiveSpriteOAMAsPtr[byte] = 0xFF;
      }
      else
      {
        // Sprite evaluation: Actually evaluating
        if (ppu->HCount == 65)
        {
          // Initialize ourselves
          ppu->SpriteEval_SpriteByteIndex = 0;
          ppu->SpriteEval_OAMSpriteIndex = 0;
          ppu->SpriteEval_NumberOfSprites = 0;
          ppu->SpriteEval_TempSpriteData = 0;
          ppu->SpriteEval_State = SPRITE_EVAL_STATE_NEW_SPRITE;
        }
        if (ppu->HCount & 1)
        {
          // Read OAM on uneven cycles
          ppu->SpriteEval_TempSpriteData = ppu->OAMAsPtr[ppu->SpriteEval_OAMSpriteIndex * 4 + ppu->SpriteEval_SpriteByteIndex];
        }
        else
        {
          // Write secondary OAM
          if (ppu->SpriteEval_NumberOfSprites < 8)
          {
            ppu->ActiveSpriteOAMAsPtr[ppu->SpriteEval_NumberOfSprites * 4 + ppu->SpriteEval_SpriteByteIndex] = ppu->SpriteEval_TempSpriteData;
          }
          else
          {
            // Read from secondary OAM, so just do nothing here
          }

          // Do logic after write
          switch (ppu->SpriteEval_State)
          {
          case SPRITE_EVAL_STATE_NEW_SPRITE:
            // New sprite, check if it is visible based on Y
            // Note that we evaluate it as if it is displayed THIS line, even though
            // it will be shown from the next line onwards
            if (IsInRange(ppu->ActiveSpriteOAM[ppu->SpriteEval_NumberOfSprites].Y, ppu->ActiveSpriteOAM[ppu->SpriteEval_NumberOfSprites].Y + 7, ppu->VCount))
            {
              // It is visible, copy the rest of the data
              ppu->SpriteEval_SpriteByteIndex++;
              ppu->SpriteEval_State = SPRITE_EVAL_STATE_COPY_SPRITE;
            }
            else
            {
              // Not visible, check next sprite
              ppu->SpriteEval_OAMSpriteIndex++;
              if (ppu->SpriteEval_OAMSpriteIndex >= 64)
              {
                // Looped trough all sprites
                ppu->SpriteEval_State = SPRITE_EVAL_STATE_END;
              }
            }
            break;
          case SPRITE_EVAL_STATE_COPY_SPRITE:
            // Wait until everything has been copied
            ppu->SpriteEval_SpriteByteIndex++;
            if (ppu->SpriteEval_SpriteByteIndex >= 4)
            {
              ppu->SpriteEval_SpriteByteIndex = 0;

              // All bytes copied, increment
              ppu->SpriteEval_NumberOfSprites++;
              ppu->SpriteEval_OAMSpriteIndex++;
              if (ppu->SpriteEval_OAMSpriteIndex >= 64)
              {
                // Looped trough all sprites
                ppu->SpriteEval_State = SPRITE_EVAL_STATE_END;
              }
              else if (ppu->SpriteEval_NumberOfSprites < 8)
              {
                // Find more sprites
                ppu->SpriteEval_State = SPRITE_EVAL_STATE_NEW_SPRITE;
              }
              else
              {
                // Full
                ppu->SpriteEval_State = SPRITE_EVAL_STATE_OVERFLOW;
              }
            }
            break;
          case SPRITE_EVAL_STATE_OVERFLOW:
            // Overflow logic
            // TODO: Sprite overflow
            ppu->SpriteEval_State = SPRITE_EVAL_STATE_END;
            break;
          case SPRITE_EVAL_STATE_END:
            // Ending state
            break;
          }
        }
      }
    }
  }

  SharedSDL_EndTiming(PERF_INDEX_PPU_SPRITE_EVAL);

  SharedSDL_BeginTiming(PERF_INDEX_PPU_SPRITE_FETCH);

  if (isVisibleScanline || isPreRenderScanline)
  {
    if (IsInRange(257, 320, ppu->HCount))
    {
      // Sprite Evaluation: Sprite data loading for the next scanline
      u8_t pixelCycle = ppu->HCount & 0x7;
      u8_t spriteIndex = (ppu->HCount - 257) >> 3;

      // Reset OAM address
      CR8_Write(&ppu->OAMAddress, 0);

      SpriteData_t *activeSprite = &ppu->ActiveSpriteData[spriteIndex];

      switch (pixelCycle)
      {
      case 1:
        // Fetch garbage NT
        Bus_ReadFromPPU(ppu->Bus, 0x2000 | (ppu->V & 0x0FFF));
        break;
      case 2:
        // Move attribute to 'latch'
        activeSprite->Attributes = ppu->ActiveSpriteOAM[spriteIndex].Attributes;
        break;
      case 3:
        // Fetch garbage AT
        Bus_ReadFromPPU(ppu->Bus,
                        0x23C0
                        | (ppu->V & 0x0C00)
                        | ((ppu->V >> 4) & 0x0038)
                        | ((ppu->V >> 2) & 0x0007));
        // Load X coordinate into latch
        activeSprite->X = ppu->ActiveSpriteOAM[spriteIndex].X;
        break;
      case 5:
      {
        // Fetch low sprite tile byte
        u16_t address = CalculateSpriteAddress(ppu->VCount, CR8_Read(ppu->Ctrl), &ppu->ActiveSpriteOAM[spriteIndex], activeSprite->Attributes & ATTRFLAG_FLIP_VERTICAL);
        activeSprite->SRPatternLow = Bus_ReadFromPPU(ppu->Bus, address);
        break;
      }
      case 7:
      {
        u16_t address = CalculateSpriteAddress(ppu->VCount, CR8_Read(ppu->Ctrl), &ppu->ActiveSpriteOAM[spriteIndex], activeSprite->Attributes & ATTRFLAG_FLIP_VERTICAL) + 8;
        activeSprite->SRPatternHigh = Bus_ReadFromPPU(ppu->Bus, address);

        // Do horizontal mirroring
        if (ppu->ActiveSpriteData[spriteIndex].Attributes & ATTRFLAG_FLIP_HORIZONTAL)
        {
          ppu->ActiveSpriteData[spriteIndex].SRPatternLow = ReverseByte(ppu->ActiveSpriteData[spriteIndex].SRPatternLow);
          ppu->ActiveSpriteData[spriteIndex].SRPatternHigh = ReverseByte(ppu->ActiveSpriteData[spriteIndex].SRPatternHigh);
        }
        break;
      }
      default:
        break;
      }
    }
  }

  SharedSDL_EndTiming(PERF_INDEX_PPU_SPRITE_FETCH);

  SharedSDL_BeginTiming(PERF_INDEX_PPU_ORDERING);

  // Try to render a pixel
  u8_t bgPixel = 0;
  u8_t bgPalette = 0;
  u8_t spPixel = 0;
  u8_t spPalette = 0;
  bool bgPriority;

  u16f_t minBackgroundX = CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND_LEFT) ? 0 : 8;
  u16f_t minSpriteX = CR8_IsBitSet(ppu->Mask, MASKFLAG_SPRITES_LEFT) ? 0 : 8;

  if (CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND) && ppu->HCount >= minBackgroundX)
  {
    // Try to render a pixel, RenderPixel will deal with any out of bounds write attempts
    u16_t pixelBit = (0x8000 >> ppu->X);
    u8_t pixel = ((ppu->SRPatternLow  & pixelBit) > 0) |
                    (((ppu->SRPatternHigh &  pixelBit) > 0) << 1);
    u8_t palette = ((ppu->SRAttributeLow  & pixelBit) > 0) |
                      (((ppu->SRAttributeHigh &  pixelBit) > 0) << 1);

    bgPixel = pixel;
    bgPalette = palette;
  }

  bool isSpriteZero = false;

  if (CR8_IsBitSet(ppu->Mask, MASKFLAG_SPRITES) && ppu->HCount >= minSpriteX)
  {
    // Find a sprite pixel to draw
    for (u8f_t i = 0; i < 8; i++)
    {
      if (ppu->ActiveSpriteData[i].X == 0)
      {
        u8f_t pixel = ((ppu->ActiveSpriteData[i].SRPatternLow & 0x80) > 0) |
            (((ppu->ActiveSpriteData[i].SRPatternHigh &  0x80) > 0) << 1);

        u8f_t palette = (ppu->ActiveSpriteData[i].Attributes & ATTRFLAG_PALLETE_MASK) + 4;

        if (pixel != 0)
        {
          // Non-transparent pixel
          spPixel = pixel;
          spPalette = palette;
          // TODO: Attributes
          bgPriority = (ppu->ActiveSpriteData[i].Attributes & ATTRFLAG_PRIORITY) > 0;
          isSpriteZero = i == 0;
          break;
        }
      }
    }
  }

  // Find when we should display sprite instead of background
  // I'm reusing the bgXXX variables for the final pixel and palette
  if (bgPixel == 0 && spPixel != 0)
  {
    bgPixel = spPixel;
    bgPalette = spPalette;
  }
  else if (bgPixel != 0 && spPixel != 0)
  {
    if (isSpriteZero && CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND))
    {
      // Sprite zero hit wooo
      // TODO: Partially hidden logic
      if (ppu->HCount != 255 && ppu->HCount >= 2 && ppu->HCount <= 257)
      {
        CR8_SetBits(&ppu->Status, STATFLAG_SPRITE_0_HIT);
      }
    }

    if (!bgPriority)
    {
      bgPixel = spPixel;
      bgPalette = spPalette;
    }
  }

  // Flag updating
  if (ppu->HCount == 1)
  {
    // Pre-render scanline
    if (isPreRenderScanline)
    {
      // Some flags are cleared here
      CR8_ClearBits(&ppu->Status, STATFLAG_VBLANK | STATFLAG_SPRITE_0_HIT | STATFLAG_SPRITE_OVERFLOW);
    }

    // Post render scanline + 1
    if (ppu->VCount == 241)
    {
      // Set VBLANK flag here
      CR8_SetBits(&ppu->Status, STATFLAG_VBLANK);
    }
  }

  // NMI line is enabled iff it's enabled in CTRL and STATUS has VBLANK active
  Bus_NMI(ppu->Bus, CR8_IsBitSet(ppu->Ctrl, CTRLFLAG_VBLANK_NMI) && CR8_IsBitSet(ppu->Status, STATFLAG_VBLANK));

  SharedSDL_EndTiming(PERF_INDEX_PPU_ORDERING);

  SharedSDL_BeginTiming(PERF_INDEX_PPU_INCREMENTS);

  // Address increment things
  if ((isPreRenderScanline || isVisibleScanline))
  {
    // Update shifters and counters
    bool isVisibleHCount = IsInRange(2, 257, ppu->HCount);

    if (isVisibleHCount || IsInRange(322, 337, ppu->HCount))
    {
      if (CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND))
      {
        // Shift the shift registers
        ppu->SRAttributeHigh <<= 1;
        ppu->SRAttributeLow <<= 1;
        ppu->SRPatternHigh <<= 1;
        ppu->SRPatternLow <<= 1;
      }
    }

    if (isVisibleHCount && CR8_IsBitSet(ppu->Mask, MASKFLAG_SPRITES))
    {
      for (unsigned int i =  8; i--;)
      {
        if (ppu->ActiveSpriteData[i].X > 0)
        {
          // Decrement
          ppu->ActiveSpriteData[i].X--;
        }
        else
        {
          // Shift registers to the side
          // TODO: When a sprite is drawn at X = 0, this will happen 1 cycle too soon
          ppu->ActiveSpriteData[i].SRPatternHigh <<= 1;
          ppu->ActiveSpriteData[i].SRPatternLow <<= 1;
        }
      }
    }

    if (IsRendering(ppu))
    {
      if (ppu->HCount == 256)
      {
        IncrementY(ppu);
      }
      else if (ppu->HCount == 257)
      {
        // Copy horizontal position from T to V
        ppu->V = (ppu->T & 0x041F) | (ppu->V & ~0x041F);
      }
      else if (isPreRenderScanline && IsInRange(280, 304, ppu->HCount))
      {
        // Copy vertical bits from T to V
        ppu->V = (ppu->T & ~0x041F) | (ppu->V & 0x041F);
      }
      if (ppu->HCount != 0 && (ppu->HCount <= 256 || ppu->HCount >= 328))
      {
        // Increment horizontal of V every 8 dots (except at dot 0)
        if ((ppu->HCount & 7) == 0)
        {
          IncrementCoarseX(ppu);
        }
      }
    }
  }

  SharedSDL_EndTiming(PERF_INDEX_PPU_INCREMENTS);

  SharedSDL_BeginTiming(PERF_INDEX_PPU_PIXEL_OUT);

  // Render the pixel to the screen
  PPU_RenderPixel(ppu, ppu->HCount, ppu->VCount, bgPixel, bgPalette);

  SharedSDL_EndTiming(PERF_INDEX_PPU_PIXEL_OUT);

  // Calculate next scanline position
  ppu->HCount++;
  // 340 is the last cycle, so go to the next line when we hit 341
  // However, on uneven frames with rendering enabled we skip the last cycle of
  // the pre-render scanline (-1 here)
  if (ppu->HCount == 341 ||
      (isPreRenderScanline && ppu->HCount == 340 && !ppu->IsEvenFrame && CR8_IsBitSet(ppu->Mask, MASKFLAG_BACKGROUND)))
  {
    ppu->HCount = 0;
    ppu->VCount++;
    if (ppu->VCount == PPU_NUM_SCANLINES)
    {
      ppu->VCount = 0;
      ppu->IsEvenFrame = !ppu->IsEvenFrame;
      ppu->FrameCount++;
    }
  }

  SharedSDL_EndTiming(PERF_INDEX_PPU);
}

u8_t PPU_ReadFromCpu(PPU_t *ppu, u16_t address)
{
  u8_t result;
  u16_t wrappedAddress = address & 0x0007;

  switch (wrappedAddress)
  {
  case 0x0000:
    // Ctrl
    return ppu->LatchedData;
  case 0x0001:
    // Mask
    return ppu->LatchedData;
  case 0x0002:
    // Status
    result = (CR8_Read(ppu->Status) & ~STATFLAG_GARBAGE_MASK) | (ppu->LatchedData & STATFLAG_GARBAGE_MASK);
    // Reading causes the address latch and the vblank flag to reset
    CR8_ClearBits(&ppu->Status, STATFLAG_VBLANK);
    ppu->AddressLatch = 0;
    // Update latched data since this is a proper read
    ppu->LatchedData = result;
    // TODO: Verify VBLANK suppression?
    return result;
  case 0x0003:
    // OAMAddress
    return ppu->LatchedData;
  case 0x0004:
    // OAMData
    // Just read the OAM from the current address
    // TODO: Clock OAM?
    return ppu->OAMAsPtr[CR8_Read(ppu->OAMAddress)];
    break;
  case 0x0005:
    // Scroll
    return ppu->LatchedData;
  case 0x0006:
    // Address
    return ppu->LatchedData;
  case 0x0007:
    // Data
    // Reading data returns the last byte that was stored in DataBuffer
    // UNLESS we are reading from palette memory, in which case it is
    // returned immediately
    // TODO: Clock?
    result = ppu->DataBuffer;
    // DataBuffer only gets updated by reading this register
    ppu->DataBuffer = Bus_ReadFromPPU(ppu->Bus, ppu->V);
    // Check if we are reading palette memory and update result accordingly
    if (ppu->V >= 0x3F00)
    {
      result = ppu->DataBuffer;
      ppu->DataBuffer = Bus_ReadFromPPU(ppu->Bus, ppu->V - 0x1000);
    }
    // The vram address always gets incremented on reading
    ppu->V += CR8_IsBitSet(ppu->Ctrl, CTRLFLAG_VRAM_INCREMENT) ? 32 : 1;
    return result;
  default:
    // TODO: Error
    break;
  }

  return 0x55;
}

void PPU_WriteFromCpu(PPU_t *ppu, u16_t address, u8_t data)
{
  u16_t wrappedAddress = address & 0x0007;

  // Update the latched data on any write
  ppu->LatchedData = data;

  switch (wrappedAddress)
  {
  case 0x0000:
    // TODO: Does this need clocking?
    // Update temp register with nametable info
    // Bits 10-11 are the ones we need
    ppu->T = (ppu->T & ~0x0C00) | (((u16_t)data << 10) & 0x0C00);

    CR8_Write(&ppu->Ctrl, data);
    break;
  case 0x0001:
    // Mask
    CR8_Write(&ppu->Mask, data);
    break;
  case 0x0002:
    // Status
    break;
  case 0x0003:
    // OAMAddress
    CR8_Write(&ppu->OAMAddress, data);
    break;
  case 0x0004:
    // OAMData
    if (IsRendering(ppu) && ((ppu->VCount == PPU_PRE_RENDER_SCANLINE) || IsInRange(0, 239, ppu->VCount)))
    {
      // TODO: Behaviour emulation for writes during rendering
      LogWarning("Ignoring OAM Data write during rendering");
    }
    else
    {
      // TODO: Clock OAM
      // Just write to the OAM at the current address
      ppu->OAMAsPtr[CR8_Read(ppu->OAMAddress)] = data;
      // Writing also increments OAM Address by one, reading does not
      CR8_Write(&ppu->OAMAddress, CR8_Read(ppu->OAMAddress) + 1);
    }
    break;
  case 0x0005:
    // Scroll
    // TODO: Write while rendering corruption?
    // TODO: Clock the X and T registers?
    if (ppu->AddressLatch == 0)
    {
      // Lower 3 bits are for X
      ppu->X = data & 0x07;
      // High 5 bits are for 0-4 of T
      ppu->T = ((data >> 3) & 0x001F) | (ppu->T & ~0x001F);

      ppu->AddressLatch = 1;
    }
    else
    {
      // Lowest 3 bits need to go to 12-14 of T
      ppu->T = ((data << 12) & 0x7000) | (ppu->T & ~0x7000);
      // High 5 bits need to go to 5-9 of T
      ppu->T = ((data << 2) & 0x03E0) | (ppu->T & ~0x03E0);

      ppu->AddressLatch = 0;
    }
    break;
  case 0x0006:
    // Address
    // TODO: Check weirdness for Addr
    // TODO: Clock T?
    if (ppu->AddressLatch == 0)
    {
      // Only lower 6 bits are needed, they go to 8-13 of T
      // Bit 15 of T also gets cleared
      ppu->T = ((data << 8) & 0x3F00) | (ppu->T & 0x00FF);

      ppu->AddressLatch = 1;
    }
    else
    {
      // Set low byte of T to data
      ppu->T = (data & 0x00FF) | (ppu->T & 0xFF00);
      // V gets updated with contents of T
      ppu->V = ppu->T;

      ppu->AddressLatch = 0;
    }
    break;
  case 0x0007:
    // Data
    // TODO: Weird glitches for writing to 0x2007 during rendering
    if (ppu->VCount >= 0 && ppu->VCount <= 241)
    {
      //LogMessage("Writing to PPU memory at line %d, 0x%04X (0x%04X) = 0x%02X", ppu->VCount, ppu->V, ppu->V & 0x3FFF, data);
    }
    // TODO: Clock clock clock?
    Bus_WriteFromPPU(ppu->Bus, ppu->V, data);
    ppu->V += CR8_IsBitSet(ppu->Ctrl, CTRLFLAG_VRAM_INCREMENT) ? 32 : 1;
    break;
  default:
    // TODO: Error
    break;
  }
}
