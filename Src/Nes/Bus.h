/*
 * Bus.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_BUS_H_
#define SRC_NES_BUS_H_

#include <stdint.h>

typedef struct _CPU_t CPU_t;
typedef struct _Mapper_t Mapper_t;

typedef struct _Bus_t
{
  CPU_t *CPU;
  Mapper_t *Mapper;
} Bus_t;

void Bus_Initialize(Bus_t *bus, CPU_t *cpu);

void Bus_SetMapper(Bus_t *bus, Mapper_t *mapper);

uint8_t Bus_Read(Bus_t *bus, uint16_t address);

void Bus_Write(Bus_t *bus, uint16_t address, uint8_t data);

#endif /* SRC_NES_BUS_H_ */
