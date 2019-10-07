/*
 * Bus.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_BUS_H_
#define SRC_NES_BUS_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct _PPU_t PPU_t;
typedef struct _CPU_t CPU_t;
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
  uint16_t CPUBaseAddress;
  uint16_t NumTransfersComplete;
  uint8_t Data;
} DMA_t;

typedef struct _Bus_t
{
  CPU_t *CPU;
  PPU_t *PPU;
  Mapper_t *Mapper;
  DMA_t DMA;
} Bus_t;

void Bus_TriggerDMA(Bus_t *bus, uint8_t cpuPage);

void Bus_TriggerNMI(Bus_t *bus);

void Bus_Initialize(Bus_t *bus, CPU_t *cpu, PPU_t *ppu);

void Bus_SetMapper(Bus_t *bus, Mapper_t *mapper);

uint8_t Bus_ReadFromCPU(Bus_t *bus, uint16_t address);

void Bus_WriteFromCPU(Bus_t *bus, uint16_t address, uint8_t data);

uint8_t Bus_ReadFromPPU(Bus_t *bus, uint16_t address);

void Bus_WriteFromPPU(Bus_t *bus, uint16_t address, uint8_t data);


#endif /* SRC_NES_BUS_H_ */
