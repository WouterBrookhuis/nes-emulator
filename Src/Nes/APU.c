/*
 * APU.c
 *
 *  Created on: 28 Mar 2021
 *      Author: Wouter
 */

#include "APU.h"
#include "Bus.h"
#include <string.h>

void APU_Initialize(APU_t *apu)
{
  memset(apu, 0, sizeof(APU_t));
}

void APU_Tick(APU_t *apu)
{

}

uint8_t APU_ReadFromCpu(APU_t *apu, uint16_t address)
{
  uint8_t addressByte = (uint8_t) address;

  switch (addressByte)
  {
  case 0x15:
    return apu->Status;
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
    apu->Status = data;
    break;
  default:
    LogError("Not implemented APU write address 0x%04X", address);
    break;
  }
}
