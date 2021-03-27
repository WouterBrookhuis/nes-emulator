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
#include "Controllers.h"
#include "log.h"

static int _clockCycleCount;
static int _cpuClockDivisor;
static int _ppuClockDivisor;
static CPU_t _cpu;
static Bus_t _bus;
static PPU_t _ppu;
static bool _ppuLastFrameEven;
static bool _isEvenCpuCycle;

static uint8_t _ppuTicker;
static uint8_t _cpuTicker;

void NES_Initialize(void)
{
  _cpuClockDivisor = 12;  // NTSC mode
  _clockCycleCount = 0;

  CPU_Initialize(&_cpu);
  PPU_Initialize(&_ppu);
  Bus_Initialize(&_bus, &_cpu, &_ppu);
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

  if (_cpuTicker  == 0)
  {
    // Handle DMA
    switch (_bus.DMA.State)
    {
    case DMA_STATE_IDLE:
      CPU_Tick(&_cpu);
      break;
    case DMA_STATE_WAITING:
      if (!_isEvenCpuCycle)
      {
        // DMA can only start on even cycles, so if this was an odd cycle the next one is even
        _bus.DMA.State = DMA_STATE_RUNNING;
      }
      break;
    case DMA_STATE_RUNNING:
      if (_isEvenCpuCycle)
      {
        // Read from cpu
        _bus.DMA.Data = Bus_ReadFromCPU(&_bus, _bus.DMA.CPUBaseAddress + _bus.DMA.NumTransfersComplete);
      }
      else
      {
        // Write to PPU OAM via OAMDATA register
        PPU_WriteFromCpu(&_ppu, 0x2004, _bus.DMA.Data);

        _bus.DMA.NumTransfersComplete++;

        if (_bus.DMA.NumTransfersComplete == 256)
        {
          _bus.DMA.State = DMA_STATE_IDLE;
        }
      }
      break;
    }

    _isEvenCpuCycle = !_isEvenCpuCycle;
  }

  if (_ppuTicker == 0)
  {
    PPU_ClockRegisters(&_ppu);
  }

  _ppuTicker = (_ppuTicker + 1) & 0x1;
  _cpuTicker = (_cpuTicker + 1) & 0x3;

  _clockCycleCount++;
}

void NES_TickUntilCPUComplete(void)
{
  // Tick until the last CPU cycle for the current instruction
  while (_cpu.CyclesLeftForInstruction != 0)
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
