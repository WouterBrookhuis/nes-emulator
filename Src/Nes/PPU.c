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


static SDL_Surface *_renderSurface;

static inline bool IsFlagSet(const uint8_t *P, uint8_t flag)
{
  return ((*P) & flag) > 0;
}

static inline void SetFlag(uint8_t *P, uint8_t flag, bool set)
{
  if (set)
  {
    (*P) |= flag;
  }
  else
  {
    (*P) &= ~flag;
  }
}

static inline bool IsRendering(PPU_t *ppu)
{
  return IsFlagSet(&ppu->Mask, MASKFLAG_BACKGROUND) || IsFlagSet(&ppu->Mask, MASKFLAG_SPRITES);
}

static void RenderPixel(int x, int y, uint8_t paletteIndex)
{
  if (_renderSurface == NULL)
  {
    return;
  }

  if (x < 0 || x >= _renderSurface->w || y < 0 || y >= _renderSurface->h)
  {
    return;
  }

  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t *pixel = (uint8_t*)_renderSurface->pixels +
                    _renderSurface->w * _renderSurface->format->BytesPerPixel * y +
                    _renderSurface->format->BytesPerPixel * x;
  Palette_GetRGB(paletteIndex, &r, &g, &b);
  *(uint32_t*)pixel = SDL_MapRGB(_renderSurface->format, r, g, b);
}

void PPU_SetRenderSurface(SDL_Surface *surface)
{
  _renderSurface = surface;
}

void PPU_Initialize(PPU_t *ppu)
{
  memset(ppu, 0, sizeof(*ppu));

  ppu->Ctrl = 0x00;
  ppu->Mask = 0x00;
  ppu->Status = 0xA0;
  ppu->OAMAddress = 0x00;
  ppu->Scroll = 0x00;
  ppu->Data = 0x00;

  ppu->VCount = -1;
}

void PPU_Reset(PPU_t *ppu)
{
  ppu->Ctrl = 0x00;
  ppu->Mask = 0x00;
  ppu->Status = ppu->Status & 0x80;
  ppu->Scroll = 0x00;
  ppu->Data = 0x00;
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
    int y = (ppu->V & 0x03E0) >> 5;
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

void PPU_Tick(PPU_t *ppu)
{
  // Increment cycles
  ppu->CycleCount++;
  ppu->CyclesSinceReset++;

  // Do PPU things
  // Address increment things
  if (IsRendering(ppu))
  {
    if (ppu->HCount == 256)
    {
      IncrementY(ppu);
    }
    if (ppu->HCount == 257)
    {
      // Copy horizontal position from T to V
      ppu->V = (ppu->T & 0x041F) | (ppu->V & ~0x041F);
    }
    if (ppu->VCount == -1 && ppu->HCount >= 280 && ppu->HCount <= 304)
    {
      // Copy vertical bits from T to V
      ppu->V = (ppu->T & ~0x041F) | (ppu->V & 0x041F);
    }
    if (ppu->HCount != 0 && (ppu->HCount <= 256 || ppu->HCount >= 328))
    {
      // Increment horizontal of V every 8 dots (except at dot 0)
      if (ppu->HCount % 8 == 0)
      {
        IncrementCoarseX(ppu);
      }
    }
  }

  // Pre-render scanline
  if (ppu->VCount == -1)
  {
    if (ppu->HCount == 1)
    {
      // Some flags are cleared here
      SetFlag(&ppu->Status, STATFLAG_VBLANK, false);
      SetFlag(&ppu->Status, STATFLAG_SPRITE_0_HIT, false);
      SetFlag(&ppu->Status, STATFLAG_SPRITE_OVERFLOW, false);
    }
  }
  // Visible scanlines
  if (ppu->VCount >= 0 && ppu->VCount <= 239)
  {
    // TODO: Pre-render line and visible lines have the same memory accesses, ensure we do that too

    if (ppu->HCount >= 1 && ppu->HCount <= 256)
    {
      uint16_t tileAddress = 0x2000 | (ppu->V & 0x0FFF);
      uint16_t attributeAddress = 0x23C0 | (ppu->V & 0x0C00)
          | ((ppu->V >> 4) & 0x0038) | ((ppu->V >> 2) & 0x0007);
      uint8_t pixelCycle = ppu->HCount % 8;

      if (pixelCycle == 1)
      {
        // Fetch NT
      }
      else if (pixelCycle == 3)
      {
        // Fetch AT
      }
      else if (pixelCycle == 5)
      {
        // Fetch low BG tile byte
      }
      else if (pixelCycle == 7)
      {
        // Fetch high BG tile byte
      }
      // TODO: Render properly
      RenderPixel(ppu->HCount, ppu->VCount, ppu->HCount % 0x40);
    }
  }
  // Post render scanline + 1
  if (ppu->VCount == 241)
  {
    if (ppu->HCount == 1)
    {
      // Set VBLANK flag here
      SetFlag(&ppu->Status, STATFLAG_VBLANK, true);
      if (IsFlagSet(&ppu->Ctrl, CTRLFLAG_VBLANK_NMI))
      {
        // Trigger the NMI here as well
        Bus_TriggerNMI(ppu->Bus);
      }
    }
  }

  // Calculate next scanline position
  ppu->HCount++;
  // 340 is the last cycle, so go to the next line when we hit 341
  // However, on uneven frames with rendering disabled we skip the last cycle of
  // the pre-render scanline (-1 here)
  if (ppu->HCount == 341 ||
      (!ppu->IsEvenFrame && IsRendering(ppu) && ppu->HCount == 340 && ppu->VCount == -1))
  {
    ppu->HCount = 0;
    ppu->VCount++;
    if (ppu->VCount == 261)
    {
      ppu->VCount = -1;
      ppu->IsEvenFrame = !ppu->IsEvenFrame;
    }
  }
}

uint8_t PPU_Read(PPU_t *ppu, uint16_t address)
{
  uint8_t result;
  uint16_t wrappedAddress = address & 0x0007;

  if (address == 0x4014)
  {
    // TODO: Do we want this here?
    return 0xFF;
  }

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
    result = (ppu->Status & ~STATFLAG_GARBAGE_MASK) | ppu->LatchedData;
    // Reading causes the address latch and the vblank flag to reset
    SetFlag(&ppu->Status, STATFLAG_VBLANK, false);
    ppu->AddressLatch = 0;
    // Update latched data since this is a proper read
    ppu->LatchedData = result;
    return result;
  case 0x0003:
    // OAMAddress
    return ppu->LatchedData;
  case 0x0004:
    // OAMData
    return ppu->OAMData;
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
    result = ppu->DataBuffer;
    // DataBuffer only gets updated by reading this register
    ppu->DataBuffer = Bus_ReadPPU(ppu->Bus, ppu->V & 0x3FFF);
    // Check if we are reading palette memory and update result accordingly
    if (ppu->V >= 0x3F00 && ppu->V <= 0x3FFF)
    {
      result = ppu->DataBuffer;
      // TODO: DataBuffer should be filled with nametable data 'underneath' palette
    }
    // The vram address always gets incremented on reading
    ppu->V += IsFlagSet(&ppu->Ctrl, CTRLFLAG_VRAM_INCREMENT) ? 32 : 1;
    return result;
  default:
    // TODO: Error
    break;
  }

  return 0x55;
}

void PPU_Write(PPU_t *ppu, uint16_t address, uint8_t data)
{
  uint16_t wrappedAddress = address & 0x0007;

  if (address == 0x4014)
  {
    // TODO: Do we want this here?
    return;
  }

  // Update the latched data on any write
  ppu->LatchedData = data;

  switch (wrappedAddress)
  {
  case 0x0000:
    // Ctrl
    ppu->Ctrl = data;
    // Update temp register with nametable info
    // Bits 10-11 are the ones we need
    ppu->T = (ppu->T & ~0x0C00) | ((data << 10) & 0x0C00);

    // If we are in VBLANK and the vblank status flag is still set, then enabling the NMI
    // here will instantly trigger it
    if (IsFlagSet(&ppu->Status, STATFLAG_VBLANK) && IsFlagSet(&ppu->Ctrl, CTRLFLAG_VBLANK_NMI))
    {
      Bus_TriggerNMI(ppu->Bus);
    }
    break;
  case 0x0001:
    // Mask
    ppu->Mask = data;
    break;
  case 0x0002:
    // Status
    break;
  case 0x0003:
    // OAMAddress
    ppu->OAMAddress = data;
    break;
  case 0x0004:
    // OAMData
    // TODO: Implement glitches
    break;
  case 0x0005:
    // Scroll
    // TODO: Write while rendering corruption?
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
    Bus_WritePPU(ppu->Bus, ppu->V & 0x3FFF, data);
    ppu->V += IsFlagSet(&ppu->Ctrl, CTRLFLAG_VRAM_INCREMENT) ? 32 : 1;
    break;
  default:
    // TODO: Error
    break;
  }
}
