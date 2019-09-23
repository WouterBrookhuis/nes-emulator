/*
 * Mapper001.c
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#include "Mapper001.h"
#include "INesLoader.h"
#include <string.h>
#include <stdlib.h>

uint8_t Mapper001_Read(Mapper_t *mapper, uint16_t address)
{
  Mapper001Data_t* customData = (Mapper001Data_t*)mapper->CustomData;
  if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    return customData->PrgRam8k[address - 0x6000];
  }
  else if (address >= 0x8000 && address <= 0xFFFF)
  {
    // Program ROM, may be a single 32k or two 16k banks
    uint8_t bankMode = (customData->ControlRegister >> 2) & 0x03;
    uint16_t externalBankBaseAddress;
    uint32_t internalBankBaseAddress;
    uint8_t selectedBank = (customData->ProgramRegister) & 0x0F;

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
    uint32_t index = address - externalBankBaseAddress + internalBankBaseAddress;
    return mapper->Memory[index];
  }

  // TODO: PPU
  // TODO: What to do for lower addresses?
  return 0;
}

void Mapper001_Write(Mapper_t *mapper, uint16_t address, uint8_t data)
{
  Mapper001Data_t* customData = (Mapper001Data_t*)mapper->CustomData;
  if (address >= 0x6000 && address <= 0x7FFF)
  {
    // Optional RAM bank, we always provide it
    customData->PrgRam8k[address - 0x6000] = data;
  }
  else if (address >= 0x8000)
  {
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
      customData->ShiftRegister |= (data << 4);

      // Write to location, register is bits 14 and 13 of address
      uint8_t regAddress = (address >> 13) & 0x03;
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
      customData->ShiftRegister |= (data << 4);
    }
  }
}


void Mapper001_Initialize(Mapper_t *mapper, INesHeader_t *header)
{
  memset(mapper, 0, sizeof(*mapper));

  mapper->MapperId = 0x01;
  mapper->MemorySize = header->PrgRomSize * SIZE_16KB;
  // TODO: Check for malloc failure
  mapper->Memory = malloc((size_t)mapper->MemorySize);
  mapper->NumPrgBanks = header->PrgRomSize;
  mapper->NumChrBanks = header->ChrRomSize;
  mapper->ReadFn = Mapper001_Read;
  mapper->WriteFn = Mapper001_Write;

  Mapper001Data_t* customData;
  customData = malloc(sizeof(Mapper001Data_t));
  customData->ShiftRegister = 0x10; // bit 5 is set
  customData->ControlRegister = 0x00;
  customData->Char0Register = 0x00;
  customData->Char1Register = 0x00;
  customData->ProgramRegister = 0x0F;
  customData->PrgRam8k = malloc(SIZE_8KB);
  mapper->CustomData = customData;
}
