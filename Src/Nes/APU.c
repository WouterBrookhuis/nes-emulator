/*
 * APU.c
 *
 *  Created on: 28 Mar 2021
 *      Author: Wouter
 */

#include "APU.h"
#include "Bus.h"
#include "log.h"
#include <string.h>

static inline void SetFrameInterruptFlag(APU_t *apu)
{
  if (!CR8_IsBitSet(apu->FrameCounter, APU_FRAME_FLAG_IRQ_INHIBIT))
  {
    CR8_SetBits(&apu->Status, APU_STATUS_FLAG_FRAME_INT);
  }
}

static void ClockEnvelopes(APU_t *apu)
{

}

static void ClockLengthCounters(APU_t *apu)
{

}

void APU_Initialize(APU_t *apu)
{
  memset(apu, 0, sizeof(APU_t));
}

void APU_Tick(APU_t *apu)
{
  // Frame counter implementation
  if (apu->HalfClockCounter == 7457)
  {
    // Quarter clock
    ClockEnvelopes(apu);
  }
  else if (apu->HalfClockCounter == 14913)
  {
    // Half clock
    ClockEnvelopes(apu);
    ClockLengthCounters(apu);
  }
  else if (apu->HalfClockCounter == 22371)
  {
    // 3/4 clock
    ClockEnvelopes(apu);
  }

  if (CR8_IsBitSet(apu->FrameCounter, APU_FRAME_FLAG_5STEP))
  {
    // 5 step
    if (apu->HalfClockCounter == 37281)
    {
      // 5/4 clock
      ClockEnvelopes(apu);
      ClockLengthCounters(apu);
    }

    apu->HalfClockCounter = (apu->HalfClockCounter + 1) % 37282;
  }
  else
  {
    // 4 step
    if (apu->HalfClockCounter == 29828)
    {
      // Set frame interrupt
      SetFrameInterruptFlag(apu);
    }
    else if (apu->HalfClockCounter == 29829)
    {
      // Set frame interrupt + Clock
      SetFrameInterruptFlag(apu);

      ClockEnvelopes(apu);
      ClockLengthCounters(apu);
    }
    else if (apu->HalfClockCounter == 0)
    {
      // Set frame interrupt?
      SetFrameInterruptFlag(apu);
    }

    apu->HalfClockCounter = (apu->HalfClockCounter + 1) % 29830;
  }

  // IRQ line is tied to the frame interrupt bit
  Bus_IRQ(apu->Bus, CR8_IsBitSet(apu->Status, APU_STATUS_FLAG_FRAME_INT));

  // Clock registers here instead of a separate function
  CR8_Clock(&apu->FrameCounter);
  CR8_Clock(&apu->Status);
}

uint8_t APU_ReadFromCpu(APU_t *apu, uint16_t address)
{
  uint8_t addressByte = (uint8_t) address;

  switch (addressByte)
  {
  case 0x15:
  {
    // STATUS: Reading clears the frame interrupt flag
    // TODO: DNT21 behavior
    uint8_t value = CR8_Read(apu->Status) & (APU_STATUS_FLAG_DMC_INT | APU_STATUS_FLAG_FRAME_INT);
    // TODO: If flag was set at the same moment as the read then it should not be cleared
    CR8_ClearBits(&apu->Status, APU_STATUS_FLAG_FRAME_INT);
    return value;
  }
  default:
    LogError("Not implemented APU read address 0x%04X", address);
    return 0;
  }
}

void APU_WriteFromCpu(APU_t *apu, uint16_t address, uint8_t data)
{
  uint8_t addressByte = (uint8_t) address;

  switch (addressByte)
  {
  case 0x15:
  {
    // STATUS
    // Writing clears the DMC interrupt flag
    // What we do here should preserve the value of the Frame interrupt flag
    apu->Status.newValue &= ~(APU_STATUS_FLAGS_WRITABLE | APU_STATUS_FLAG_DMC_INT);
    apu->Status.newValue |= data & APU_STATUS_FLAGS_WRITABLE;
    break;
  }
  case 0x17:
  {
    // FRAME COUNTER
    // Has some flags
    CR8_Write(&apu->FrameCounter, data & APU_FRAME_FLAG_MASK);
    if (data & APU_FRAME_FLAG_IRQ_INHIBIT)
    {
      CR8_ClearBits(&apu->Status, APU_STATUS_FLAG_FRAME_INT);
    }
    break;
  }
  default:
    LogError("Not implemented APU write address 0x%04X", address);
    break;
  }
}
