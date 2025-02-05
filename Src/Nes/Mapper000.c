/*
 * Mapper000.c
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#include "Mapper000.h"
#include "INesLoader.h"
#include "Bus.h"
#include <string.h>
#include <stdlib.h>
#include "log.h"

static u8_t _mapper000Ram[SIZE_8KB];

bool Mapper000_ReadFromCpu(Mapper_t *mapper,
                           u16_t address,
                           u8_t *data)
{
  Mapper000Data_t *customData = (Mapper000Data_t*) mapper->CustomData;
  if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    *data = customData->PrgRam8k[address - 0x6000];
    return true;
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be two 16k banks or a mirrored 16k bank
    u16_t externalBankBaseAddress;
    u32_t internalBankBaseAddress;

    if (mapper->NumPrgBanks == 1)
    {
      // Just 1 bank that is mirrored
      internalBankBaseAddress = 0x0000;
      if (address < 0xC000)
      {
        externalBankBaseAddress = 0x8000;
      }
      else
      {
        externalBankBaseAddress = 0xC000;
      }
    }
    else
    {
      // Two banks
      if (address < 0xC000)
      {
        externalBankBaseAddress = 0x8000;
        internalBankBaseAddress = 0x0000;
      }
      else
      {
        externalBankBaseAddress = 0xC000;
        internalBankBaseAddress = 0x4000;
      }
    }

    u32_t index = address - externalBankBaseAddress
        + internalBankBaseAddress;
    NES_ASSERT(index < mapper->MemorySize);
    *data = mapper->Memory[index];
    return true;
  }

  return false;
}

bool Mapper000_WriteFromCpu(Mapper_t *mapper,
                            u16_t address,
                            u8_t data)
{
  Mapper000Data_t *customData = (Mapper000Data_t*) mapper->CustomData;
  if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    customData->PrgRam8k[address - 0x6000] = data;
    return true;
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be two 16k banks or a mirrored 16k bank
    u16_t externalBankBaseAddress;
    u32_t internalBankBaseAddress;

    if (mapper->NumPrgBanks == 1)
    {
      // Just 1 bank that is mirrored
      internalBankBaseAddress = 0x0000;
      if (address < 0xC000)
      {
        externalBankBaseAddress = 0x8000;
      }
      else
      {
        externalBankBaseAddress = 0xC000;
      }
    }
    else
    {
      // Two banks
      if (address < 0xC000)
      {
        externalBankBaseAddress = 0x8000;
        internalBankBaseAddress = 0x0000;
      }
      else
      {
        externalBankBaseAddress = 0xC000;
        internalBankBaseAddress = 0x4000;
      }
    }

    u32_t index = address - externalBankBaseAddress
        + internalBankBaseAddress;
    NES_ASSERT(index < mapper->MemorySize);
    mapper->Memory[index] = data;
    return true;
  }

  return false;
}

bool Mapper000_ReadFromPpu(Mapper_t *mapper,
                           u16_t address,
                           u8_t *data)
{
  if (address >= 0x0000 && address <= 0x1FFF)
  {
    if (mapper->NumChrBanks > 0)
    {
      // PPU CHR ROM
      u32_t offset = address + mapper->ChrOffset;
      NES_ASSERT(offset < mapper->MemorySize);
      *data = mapper->Memory[offset];
      return true;
    }
  }

  return false;
}

bool Mapper000_WriteFromPpu(Mapper_t *mapper,
                            u16_t address,
                            u8_t data)
{
  return false;
}

void Mapper000_Initialize(Mapper_t *mapper,
                          INesHeader_t *header)
{
  memset(mapper, 0, sizeof(*mapper));

  mapper->MapperId = 0x00;
  mapper->Mirror = header->Flags6 & 0x01 ? MIRROR_MODE_VERTICAL : MIRROR_MODE_HORIZONTAL;
  mapper->MemorySize = header->PrgRomSize * SIZE_16KB + header->ChrRomSize * SIZE_8KB;
  // TODO: Check for malloc failure
  mapper->Memory = malloc((size_t) mapper->MemorySize);
  mapper->ChrOffset = header->PrgRomSize * SIZE_16KB;
  mapper->NumPrgBanks = header->PrgRomSize;
  mapper->NumChrBanks = header->ChrRomSize;
  mapper->ReadFromCpu = Mapper000_ReadFromCpu;
  mapper->ReadFromPpu = Mapper000_ReadFromPpu;
  mapper->WriteFromCpu = Mapper000_WriteFromCpu;
  mapper->WriteFromPpu = Mapper000_WriteFromPpu;

  Mapper000Data_t *customData;
  customData = malloc(sizeof(Mapper000Data_t));
  customData->PrgRam8k = _mapper000Ram;// malloc(SIZE_8KB);
  mapper->CustomData = customData;

  // Init RAM for debugging purposes
  memset(customData->PrgRam8k, 0x55, SIZE_8KB);
}
