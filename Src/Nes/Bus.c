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
}

void Bus_TriggerNMI(Bus_t *bus)
{
  // TODO: NMI stuff
}

uint8_t Bus_Read(Bus_t *bus, uint16_t address)
{
  if (address < 0x2000)
  {
    return _testRam[address & 0x7FF];
  }
  else if (address < 0x4000)
  {
    // PPU
    return 0;
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

void Bus_Write(Bus_t *bus, uint16_t address, uint8_t data)
{
  if (address < 0x2000)
  {
    _testRam[address & 0x7FF] = data;
  }
  else if (address < 0x4000)
  {
    // PPU
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
