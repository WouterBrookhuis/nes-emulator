/*
 * Bus.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#include "Bus.h"
#include "CPU.h"
#include "PPU.h"

#include "Mapper.h"
#include <string.h>

// TODO: No shared ram, what is this, a multiprocessor system?
uint8_t _testRam[UINT16_MAX];
uint8_t _palette[256];
uint8_t _vram[2048];

void Bus_Initialize(Bus_t *bus, CPU_t *cpu, PPU_t *ppu)
{
  memset(bus, 0, sizeof(*bus));
  // Link CPU and bus together
  bus->CPU = cpu;
  cpu->Bus = bus;
  // Link PPU and bus together
  bus->PPU = ppu;
  ppu->Bus = bus;
}

void Bus_SetMapper(Bus_t *bus, Mapper_t *mapper)
{
  bus->Mapper = mapper;
  mapper->Bus = bus;
}

void Bus_TriggerNMI(Bus_t *bus)
{
  CPU_NMI(bus->CPU);
}

uint8_t Bus_ReadCPU(Bus_t *bus, uint16_t address)
{
  if (address < 0x2000)
  {
    return _testRam[address & 0x7FF];
  }
  else if (address < 0x4000)
  {
    // PPU
    return PPU_Read(bus->PPU, address);
  }
  else if (address < 0x4018)
  {
    // APU + IO
    return 0;
  }
  else if (address < 0x4020)
  {
    // Test APU + IO
    return 0;
  }
  else if (bus->Mapper != NULL)
  {
    // Cartridge
    return bus->Mapper->ReadFn(bus->Mapper, address);
  }
  // TODO: Log cartridge missing?
  return 0;
}

void Bus_WriteCPU(Bus_t *bus, uint16_t address, uint8_t data)
{
  if (address < 0x2000)
  {
    _testRam[address & 0x7FF] = data;
  }
  else if (address < 0x4000)
  {
    // PPU access
    PPU_Write(bus->PPU, address, data);
  }
  else if (address < 0x4018)
  {
    // APU + IO
  }
  else if (address < 0x4020)
  {
    // Test APU + IO
  }
  else if (bus->Mapper != NULL)
  {
    // Cartridge
    bus->Mapper->WriteFn(bus->Mapper, address, data);
  }
}

uint8_t Bus_ReadPPU(Bus_t *bus, uint16_t address)
{
  if (address > 0x3FFF)
  {
    LogError("Invalid address for PPU read: 0x%04X", address);
    return 0x00;
  }
  if (address >= 0x3F00 && address <= 0x3FFF)
  {
    return _palette[address & 0x00FF];
  }
  else if (bus->Mapper != NULL)
  {
    // Cartridge
    return bus->Mapper->ReadFn(bus->Mapper, address);
  }
  // TODO: Log cartridge missing?
  return 0;
}

void Bus_WritePPU(Bus_t *bus, uint16_t address, uint8_t data)
{
  if (address > 0x3FFF)
  {
    LogError("Invalid address for PPU write: 0x%04X", address);
    return;
  }
  if (address >= 0x3F00 && address <= 0x3FFF)
  {
    _palette[address & 0x00FF] = data;
  }
  else if (bus->Mapper != NULL)
  {
    // Cartridge
    bus->Mapper->WriteFn(bus->Mapper, address, data);
  }
}

uint8_t Bus_ReadNametableDefault(Bus_t *bus, uint16_t address)
{
  if (address >= 0x3000)
  {
    address -= 0x1000;
  }
  if (bus->Mapper->Mirror == MIRROR_MODE_HORIZONTAL)
  {
    if (address >= 0x2000 && address <= 0x23FF)
    {
      return _vram[address & 0x3FF];
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
      return _vram[address & 0x3FF];
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
      return _vram[(address & 0x3FF) + 0x400];
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
      return _vram[(address & 0x3FF) + 0x400];
    }
  }
  else if (bus->Mapper->Mirror == MIRROR_MODE_VERTICAL)
  {
    if (address >= 0x2000 && address <= 0x23FF)
    {
      return _vram[address & 0x3FF];
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
      return _vram[(address & 0x3FF) + 0x400];
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
      return _vram[(address & 0x3FF) + 0x400];
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
      return _vram[address & 0x3FF];
    }
  }
  // TODO: Log error
  return 0x55;
}

void Bus_WriteNametableDefault(Bus_t *bus, uint16_t address, uint8_t data)
{
  if (address >= 0x3000)
  {
    address -= 0x1000;
  }
  if (bus->Mapper->Mirror == MIRROR_MODE_HORIZONTAL)
  {
    if (address >= 0x2000 && address <= 0x23FF)
    {
      _vram[address & 0x3FF] = data;
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
      _vram[address & 0x3FF] = data;
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
      _vram[(address & 0x3FF) + 0x400] = data;
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
      _vram[(address & 0x3FF) + 0x400] = data;
    }
  }
  else if (bus->Mapper->Mirror == MIRROR_MODE_VERTICAL)
  {
    if (address >= 0x2000 && address <= 0x23FF)
    {
      _vram[address & 0x3FF] = data;
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
      _vram[(address & 0x3FF) + 0x400] = data;
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
      _vram[(address & 0x3FF) + 0x400] = data;
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
      _vram[address & 0x3FF] = data;
    }
  }
}
