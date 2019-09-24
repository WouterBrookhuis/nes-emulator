/*
 * NES.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */


#include "NES.h"
#include "CPU.h"
#include "Bus.h"

static int _clockCycleCount;
static int _cpuClockDivisor;
static CPU_t _cpu;
static Bus_t _bus;

void NES_Initialize(void)
{
  _cpuClockDivisor = 12;  // NTSC mode
  _clockCycleCount = 0;

  CPU_Initialize(&_cpu);
  Bus_Initialize(&_bus, &_cpu);
}

void NES_TickClock(void)
{
  if (_clockCycleCount % _cpuClockDivisor == 0)
  {
    CPU_Tick(&_cpu);
  }
  _clockCycleCount++;
}

void NES_TickUntilCPUComplete(void)
{
  // Tick until the last CPU cycle for the current instruction
  while (_cpu.CyclesLeftForInstruction != 0)
  {
    NES_TickClock();
  }
  // Complete all master clock cycles BEFORE triggering the CPU again
  while (_clockCycleCount % _cpuClockDivisor != 0)
  {
    NES_TickClock();
  }
}

CPU_t *NES_GetCPU(void)
{
  return &_cpu;
}

Bus_t *NES_GetBus(void)
{
  return &_bus;
}