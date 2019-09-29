/*
 * Bus.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#include "Bus.h"
#include "CPU.h"
#include "PPU.h"
#include "log.h"

#include "Mapper.h"
#include <string.h>

// TODO: No shared ram, what is this, a multiprocessor system?
uint8_t _testRam[UINT16_MAX];
uint8_t _palette[256];
uint8_t _vram[2048];

static uint8_t ReadNametableDefault(Bus_t *bus, uint16_t address);

static void WriteNametableDefault(Bus_t *bus, uint16_t address, uint8_t data);

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

uint8_t Bus_ReadFromCPU(Bus_t *bus, uint16_t address)
{
  uint8_t data;

  if (bus->Mapper != NULL && bus->Mapper->ReadFn(bus->Mapper, address, &data))
  {
    // Handled by mapper
  }
  else if (address < 0x2000)
  {
    data = _testRam[address & 0x7FF];
  }
  else if (address < 0x4000)
  {
    // PPU
    data = PPU_ReadFromCpu(bus->PPU, address);
  }
  else if (address < 0x4018)
  {
    // APU + IO
    data = 0;
  }
  else if (address < 0x4020)
  {
    // Test APU + IO
    data = 0;
  }
  else
  {
    LogError("Unmapped CPU read @ 0x%04X", address);
  }

  return data;
}

void Bus_WriteFromCPU(Bus_t *bus, uint16_t address, uint8_t data)
{
  if (bus->Mapper != NULL && bus->Mapper->WriteFn(bus->Mapper, address, data))
  {
    // Handled by mapper
  }
  else if (address < 0x2000)
  {
    _testRam[address & 0x7FF] = data;
  }
  else if (address < 0x4000)
  {
    // PPU access
    PPU_WriteFromCpu(bus->PPU, address, data);
  }
  else if (address < 0x4018)
  {
    // APU + IO
  }
  else if (address < 0x4020)
  {
    // Test APU + IO
  }
  else
  {
    LogError("Unmapped CPU write @ 0x%04X", address);
  }
}

uint8_t Bus_ReadFromPPU(Bus_t *bus, uint16_t address)
{
  uint8_t data;
  address &= 0x3FFF;

  if (bus->Mapper != NULL && bus->Mapper->ReadFn(bus->Mapper, address, &data))
  {
    // Handled by mapper
  }
  else if (address >= 0x2000 && address <= 0x3EFF)
  {
    // Default nametable implementation
    data = ReadNametableDefault(bus, address);
  }
  else if (address >= 0x3F00 && address <= 0x3FFF)
  {
    uint16_t localAddress = address & 0x001F;
    if (localAddress == 0x10
        || localAddress == 0x14
        || localAddress == 0x18
        || localAddress == 0x1C)
    {
      localAddress -= 0x10;
    }
    data = _palette[localAddress];
  }
  else
  {
    LogError("Unmapped PPU read @ 0x%04X", address);
  }

  return data;
}

void Bus_WriteFromPPU(Bus_t *bus, uint16_t address, uint8_t data)
{
  address &= 0x3FFF;

  if (bus->Mapper != NULL && bus->Mapper->WriteFn(bus->Mapper, address, data))
  {
    // Handled by mapper
  }
  else if (address >= 0x2000 && address <= 0x3EFF)
  {
    // Default nametable implementation
    WriteNametableDefault(bus, address, data);
  }
  else if (address >= 0x3F00 && address <= 0x3FFF)
  {
    uint16_t localAddress = address & 0x001F;
    if (localAddress == 0x10
        || localAddress == 0x14
        || localAddress == 0x18
        || localAddress == 0x1C)
    {
      localAddress -= 0x10;
    }
    _palette[localAddress] = data;
  }
  else
  {
    LogError("Unmapped PPU write @ 0x%04X", address);
  }
}

static uint8_t ReadNametableDefault(Bus_t *bus, uint16_t address)
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

static void WriteNametableDefault(Bus_t *bus, uint16_t address, uint8_t data)
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
