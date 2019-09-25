/*
 * PPU.c
 *
 *  Created on: Sep 25, 2019
 *      Author: wouter
 */

#include "PPU.h"
#include "Bus.h"
#include <string.h>

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

void PPU_Initialize(PPU_t *ppu)
{
  memset(ppu, 0, sizeof(*ppu));

  ppu->Ctrl = 0x00;
  ppu->Mask = 0x00;
  ppu->Status = 0xA0;
  ppu->OAMAddress = 0x00;
  ppu->Scroll = 0x00;
  ppu->Address = 0x00;
  ppu->Data = 0x00;
}

void PPU_Reset(PPU_t *ppu)
{
  ppu->Ctrl = 0x00;
  ppu->Mask = 0x00;
  ppu->Status = ppu->Status & 0x80;
  ppu->Scroll = 0x00;
  ppu->Data = 0x00;
}

void PPU_Tick(PPU_t *ppu)
{
  ppu->CycleCount++;
  ppu->CyclesSinceReset++;
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
    result = (ppu->Status & STATFLAG_GARBAGE_MASK) | ppu->LatchedData;
    // Reading causes the address latch and the vblank flag to reset
    SetFlag(&ppu->Status, STATFLAG_VBLANK, false);
    ppu->AddressLatch = 0;
    // Update latched data since this is a proper read
    ppu->LatchedData = result;
    return result;
  case 0x0003:
    // OMAAddress
    return ppu->LatchedData;
  case 0x0004:
    // OMAData
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
    // TODO: Data read
    break;
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
    // OMAAddress
    ppu->OAMAddress = data;
    break;
  case 0x0004:
    // OMAData
    // TODO: Implement glitches
    break;
  case 0x0005:
    // Scroll
    // TODO: Scroll write
    if (ppu->AddressLatch == 1)
    {
      ppu->AddressLatch = 0;
    }
    else
    {
      ppu->AddressLatch = 1;
    }
    break;
  case 0x0006:
    // Address
    // TODO: Addr write
    if (ppu->AddressLatch == 1)
    {
      ppu->AddressLatch = 0;
    }
    else
    {
      ppu->AddressLatch = 1;
    }
    break;
  case 0x0007:
    // Data
    // TODO: Data write
    break;
  default:
    // TODO: Error
    break;
  }
}
