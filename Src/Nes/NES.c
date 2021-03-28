/*
 * NES.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */


#include "NES.h"
#include "CPU.h"
#include "Bus.h"
#include "PPU.h"
#include "APU.h"
#include "Controllers.h"
#include "log.h"

static int _clockCycleCount;
static CPU_t _cpu;
static Bus_t _bus;
static PPU_t _ppu;
static APU_t _apu;
static bool _ppuLastFrameEven;
static bool _isEvenCpuCycle;

static uint8_t _ppuTicker;
static uint8_t _cpuTicker;

void NES_Initialize(void)
{
  _clockCycleCount = 0;

  CPU_Initialize(&_cpu);
  PPU_Initialize(&_ppu);
  APU_Initialize(&_apu);
  Bus_Initialize(&_bus, &_cpu, &_ppu, &_apu);
  Controllers_Initialize(2);

  _ppuLastFrameEven = _ppu.IsEvenFrame;
  _isEvenCpuCycle = true;
}

void NES_TickClock(void)
{
  if (_ppuTicker == 0)
  {
    PPU_Tick(&_ppu);
  }

  if ((_cpuTicker == 0) || (_cpuTicker == 3))
  {
    // Handle DMA
    if (_bus.DMA.State == DMA_STATE_IDLE)
    {
      CPU_Tick(&_cpu);
    }
    else if ((_bus.DMA.State == DMA_STATE_WAITING) && (_cpuTicker == 3))
    {
      // DMA can only start on even cycles, so if this was an odd cycle the next one is even
      _bus.DMA.State = DMA_STATE_RUNNING;
    }
    else if (_cpuTicker == 0)
    {
      // Read from cpu
      _bus.DMA.Data = Bus_ReadFromCPU(&_bus, _bus.DMA.CPUBaseAddress + _bus.DMA.NumTransfersComplete);
    }
    else
    {
      // Write to PPU OAM via OAMDATA register
      PPU_WriteFromCpu(&_ppu, 0x2004, _bus.DMA.Data);

      _bus.DMA.NumTransfersComplete++;

      if (_bus.DMA.NumTransfersComplete == 0)
      {
        _bus.DMA.State = DMA_STATE_IDLE;
      }
    }
  }

  if (_ppuTicker == 0)
  {
    PPU_ClockRegisters(&_ppu);
  }

  _ppuTicker = (_ppuTicker + 1) & 0x1;
  _cpuTicker = (_cpuTicker + 1) % 6;

  _clockCycleCount++;
}

void NES_TickUntilCPUComplete(void)
{
  // Tick until the last CPU cycle for the current instruction
  unsigned int currentCount = _cpu.InstructionCount;
  while (_cpu.InstructionCount == currentCount)
  {
    NES_TickClock();
  }
}

void NES_TickUntilFrameComplete(void)
{
  while (_ppu.IsEvenFrame == _ppuLastFrameEven)
  {
    NES_TickClock();
  }
  _ppuLastFrameEven = _ppu.IsEvenFrame;
}

PPU_t *NES_GetPPU(void)
{
  return &_ppu;
}

CPU_t *NES_GetCPU(void)
{
  return &_cpu;
}

Bus_t *NES_GetBus(void)
{
  return &_bus;
}
