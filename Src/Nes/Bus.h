/*
 * Bus.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_BUS_H_
#define SRC_NES_BUS_H_

#include "Types.h"

typedef struct _PPU_t PPU_t;
typedef struct _CPU_t CPU_t;
typedef struct _APU_t APU_t;
typedef struct _Mapper_t Mapper_t;

typedef enum
{
  DMA_STATE_IDLE,
  DMA_STATE_WAITING,
  DMA_STATE_RUNNING
} DMA_State_t;

typedef struct _DMA_t
{
  DMA_State_t State;
  u16_t CPUBaseAddress;
  u8_t NumTransfersComplete;
  u8_t Data;
} DMA_t;

typedef struct _Bus_t
{
  CPU_t *CPU;
  PPU_t *PPU;
  APU_t *APU;
  Mapper_t *Mapper;
  DMA_t DMA;
} Bus_t;

void Bus_TriggerDMA(Bus_t *bus, u8_t cpuPage);

void Bus_NMI(const Bus_t *bus, bool assert);

void Bus_IRQ(const Bus_t *bus, bool assert);

void Bus_Initialize(Bus_t *bus, CPU_t *cpu, PPU_t *ppu, APU_t *apu);

void Bus_SetMapper(Bus_t *bus, Mapper_t *mapper);

u8_t Bus_ReadFromCPU(const Bus_t *bus, u16_t address);

void Bus_WriteFromCPU(Bus_t *bus, u16_t address, u8_t data);

u8_t Bus_ReadFromPPU(const Bus_t *bus, u16_t address);

void Bus_WriteFromPPU(const Bus_t *bus, u16_t address, u8_t data);


#endif /* SRC_NES_BUS_H_ */
