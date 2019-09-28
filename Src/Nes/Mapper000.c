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

static uint8_t _mapper000Ram[SIZE_8KB];

uint8_t Mapper000_Read(Mapper_t *mapper,
                       uint16_t address)
{
  Mapper000Data_t *customData = (Mapper000Data_t*) mapper->CustomData;
  if (address >= 0x0000 && address <= 0x1FFF)
  {
    // PPU CHR ROM
    return mapper->Memory[address + mapper->ChrOffset];
  }
  else if (address >= 0x2000 && address <= 0x3EFF)
  {
    // Nametables
    // We don't have any custom nametable logic, so forward it back to the bus for
    // a default implementation
    return Bus_ReadNametableDefault(mapper->Bus, address);
  }
  else if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    return customData->PrgRam8k[address - 0x6000];
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be two 16k banks or a mirrored 16k bank
    uint16_t externalBankBaseAddress;
    uint32_t internalBankBaseAddress;

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

    uint32_t index = address - externalBankBaseAddress
        + internalBankBaseAddress;
    return mapper->Memory[index];
  }

  // TODO: PPU
  // TODO: What to do for lower addresses?
  return 0;
}

void Mapper000_Write(Mapper_t *mapper,
                     uint16_t address,
                     uint8_t data)
{
  Mapper000Data_t *customData = (Mapper000Data_t*) mapper->CustomData;
  if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    customData->PrgRam8k[address - 0x6000] = data;
  }
  else if (address >= 0x2000 && address <= 0x3EFF)
  {
    // Nametables
    // We don't have any custom nametable logic, so forward it back to the bus for
    // a default implementation
    Bus_WriteNametableDefault(mapper->Bus, address, data);
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be two 16k banks or a mirrored 16k bank
    uint16_t externalBankBaseAddress;
    uint32_t internalBankBaseAddress;

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

    uint32_t index = address - externalBankBaseAddress
        + internalBankBaseAddress;
    mapper->Memory[index] = data;
  }

  // TODO: PPU
  // TODO: What to do for lower addresses?
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
  mapper->ReadFn = Mapper000_Read;
  mapper->WriteFn = Mapper000_Write;

  Mapper000Data_t *customData;
  customData = malloc(sizeof(Mapper000Data_t));
  customData->PrgRam8k = _mapper000Ram;// malloc(SIZE_8KB);
  mapper->CustomData = customData;

  // Init RAM for debugging purposes
  memset(customData->PrgRam8k, 0x55, SIZE_8KB);
}
