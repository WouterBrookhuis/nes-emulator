/*
 * Mapper001.c
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#include "Mapper001.h"
#include "INesLoader.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

static bool Mapper001_ReadFromCpu(Mapper_t *mapper, u16_t address, u8_t *data)
{
  Mapper001Data_t *customData = (Mapper001Data_t*) mapper->CustomData;

  if (customData->PrgRam8k != NULL && address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank
    *data = customData->PrgRam8k[address - 0x6000];
    return true;
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be a single 32k or two 16k banks
    u8_t bankMode = (customData->ControlRegister >> 2) & 0x03;
    u16_t externalBankBaseAddress;
    u32_t internalBankBaseAddress;
    u8_t selectedBank = (customData->ProgramRegister) & 0x0F;

    if (bankMode == 0 || bankMode == 1)
    {
      // Single 32k bank
      // Clear lowest bit since it is ignored in 32k mode
      selectedBank &= ~0x01;
      externalBankBaseAddress = 0x8000;
      internalBankBaseAddress = selectedBank * SIZE_16KB;
    }
    else if (bankMode == 2)
    {
      // Fixed first bank, second bank is variable
      if (address < 0xC000)
      {
        // First bank
        externalBankBaseAddress = 0x8000;
        internalBankBaseAddress = 0x0000;
      }
      else
      {
        // Second bank, find mapping
        externalBankBaseAddress = 0xC000;
        internalBankBaseAddress = selectedBank * SIZE_16KB;
      }
    }
    else
    {
      // First bank is variable, Last bank is fixed
      if (address < 0xC000)
      {
        // Switchable bank
        externalBankBaseAddress = 0x8000;
        internalBankBaseAddress = selectedBank * SIZE_16KB;
      }
      else
      {
        // Fixed last bank
        externalBankBaseAddress = 0xC000;
        // Stats at 256k-16k
        internalBankBaseAddress = 0x3C000;
      }
    }
    u32_t index = address - externalBankBaseAddress + internalBankBaseAddress;
    index &= (mapper->NumPrgBanks * SIZE_16KB - 1);
    *data = mapper->Memory[index];
    return true;
  }

  return false;
}

static bool Mapper001_WriteFromCpu(Mapper_t *mapper, u16_t address, u8_t data)
{
  Mapper001Data_t *customData = (Mapper001Data_t*) mapper->CustomData;
  if (customData->PrgRam8k != NULL && address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank
    customData->PrgRam8k[address - 0x6000] = data;
    return true;
  }
  else if (address >= 0x8000)
  {
    LogMessage("CPU Mapper Write: 0x%04X = 0x%02X", address, data);
    // Shift register time
    if (data & 0x80)
    {
      // Clear register
      customData->ShiftRegister = 0x10;
    }
    else if (customData->ShiftRegister & 0x01)
    {
      // Write it to memory
      // Push last data bit into register
      customData->ShiftRegister >>= 1;
      customData->ShiftRegister |= ((data & 1) << 4);

      // Write to location, register is bits 14 and 13 of address
      u8_t regAddress = (address >> 13) & 0x03;
      switch (regAddress)
      {
      case 0:
        customData->ControlRegister = customData->ShiftRegister & 0x1F;
        break;
      case 1:
        customData->Char0Register = customData->ShiftRegister & 0x1F;
        break;
      case 2:
        customData->Char1Register = customData->ShiftRegister & 0x1F;
        break;
      case 3:
        customData->ProgramRegister = customData->ShiftRegister & 0x1F;
        break;
      default:
        break;
      }
      // Reset register
      customData->ShiftRegister = 0x10;
    }
    else
    {
      // Push data bit into register
      customData->ShiftRegister >>= 1;
      customData->ShiftRegister |= ((data & 1) << 4);
    }

    LogMessage("MMC1-SR: 0x%02X", customData->ShiftRegister);
    LogMessage("MMC1-CR: 0x%02X", customData->ControlRegister);
    LogMessage("MMC1-C0: 0x%02X", customData->Char0Register);
    LogMessage("MMC1-C1: 0x%02X", customData->Char1Register);
    LogMessage("MMC1-PR: 0x%02X", customData->ProgramRegister);

    return true;
  }

  return false;
}

static bool Mapper001_ReadFromPpu(Mapper_t *mapper, u16_t address, u8_t *data)
{
  // TODO: This is supposed to be banked!
  //Mapper000Data_t *customData = (Mapper000Data_t*) mapper->CustomData;
  if (address >= 0x0000 && address <= 0x1FFF)
  {
    if (mapper->NumChrBanks > 0)
    {
      // PPU CHR ROM
      *data = mapper->Memory[address + mapper->ChrOffset];
      return true;
    }
  }

  return false;
}

static bool Mapper001_WriteFromPpu(Mapper_t *mapper, u16_t address, u8_t data)
{
  return false;
}

void Mapper001_Initialize(Mapper_t *mapper, INesHeader_t *header)
{
  memset(mapper, 0, sizeof(*mapper));

  mapper->MapperId = 0x01;
  mapper->Mirror = header->Flags6 & INES_FLAGS6_MIRROR_VERTICAL ? MIRROR_MODE_VERTICAL : MIRROR_MODE_HORIZONTAL;
  mapper->MemorySize = header->PrgRomSize * SIZE_16KB + header->ChrRomSize * SIZE_8KB;
  // TODO: Check for malloc failure
  mapper->Memory = malloc((size_t) mapper->MemorySize);
  mapper->ChrOffset = header->PrgRomSize * SIZE_16KB;
  mapper->NumPrgBanks = header->PrgRomSize;
  mapper->NumChrBanks = header->ChrRomSize;
  mapper->ReadFromCpu = Mapper001_ReadFromCpu;
  mapper->ReadFromPpu = Mapper001_ReadFromPpu;
  mapper->WriteFromCpu = Mapper001_WriteFromCpu;
  mapper->WriteFromPpu = Mapper001_WriteFromPpu;

  Mapper001Data_t *customData;
  customData = malloc(sizeof(Mapper001Data_t));
  customData->ShiftRegister = 0x10; // bit 5 is set
  customData->ControlRegister = 0x00;
  customData->Char0Register = 0x00;
  customData->Char1Register = 0x00;
  customData->ProgramRegister = 0x0F;
  // TODO: For some reason the CPU tries to read from this RAM on some test ROMs that don't include it
  if (header->Flags6 & INES_FLAGS6_BATTERY_RAM || true)
  {
    LogMessage("Mapper001: Using battery backed RAM");
    customData->PrgRam8k = malloc(SIZE_8KB);
  }
  else
  {
    LogMessage("Mapper001: No battery backed RAM");
    customData->PrgRam8k = 0;
  }
  mapper->CustomData = customData;
}
