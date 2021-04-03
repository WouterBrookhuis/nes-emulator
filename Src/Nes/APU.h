/*
 * APU.h
 *
 *  Created on: 28 Mar 2021
 *      Author: Wouter
 */

#ifndef SRC_NES_APU_H_
#define SRC_NES_APU_H_

#include "Types.h"
#include "ClockedRegister.h"

typedef struct _Bus_t Bus_t;

typedef enum
{
  APU_STATUS_FLAG_NONE = 0,             // No flags enabled
  APU_STATUS_FLAG_PC1_ENABLE = 0x01,    // Enable Pulse 1
  APU_STATUS_FLAG_PC2_ENABLE = 0x02,    // Enable Pulse 2
  APU_STATUS_FLAG_T_ENABLE = 0x04,      // Enable Triangle
  APU_STATUS_FLAG_N_ENABLE = 0x08,      // Enable Noise
  APU_STATUS_FLAG_D_ENABLE = 0x10,      // Enable DMC
  APU_STATUS_FLAG_FRAME_INT = 0x40,     // Frame interrupt status flag
  APU_STATUS_FLAG_DMC_INT = 0x80,       // DMC interrupt status flag
} APU_StatusFlags_t;

#define APU_STATUS_FLAGS_WRITABLE   (0x1F)    // The status register bits that can be written by CPU

typedef enum
{
  APU_FRAME_FLAG_MASK = 0xC0,           // Mask for the flags in the FrameCounter register
  APU_FRAME_FLAG_5STEP = 0x80,          // Use 5 step mode instead of 4 step mode
  APU_FRAME_FLAG_IRQ_INHIBIT = 0x40,    // Inhibit IRQ generation
} APU_FrameFlags_t;

typedef struct _APU_t
{
  Bus_t *Bus;       // The bus we are attached to

  uint_fast16_t HalfClockCounter;   // Counts half APU clock cycles (1 APU clock = 2 CPU clock)



  cr8_t Status;             // The status register at 0x4015
  cr8_t FrameCounter;       // Frame counter register at 0x4017
  cr1_t FrameCounterWritten;    // ??
} APU_t;

void APU_Initialize(APU_t *apu);
void APU_Tick(APU_t *apu);
u8_t APU_ReadFromCpu(APU_t *apu, u16_t address);
void APU_WriteFromCpu(APU_t *apu, u16_t address, u8_t data);

#endif /* SRC_NES_APU_H_ */
