/*
 * CPU_Internal.h
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_CPU_INTERNAL_H_
#define SRC_NES_CPU_INTERNAL_H_

#include "CPU.h"
#include <stdint.h>
#include <stdbool.h>
#include "Bus.h"

#define PFLAG_CARRY       0x01
#define PFLAG_ZERO        0x02
#define PFLAG_INTDISABLE  0x04
#define PFLAG_DECIMAL     0x08
#define PFLAG_B0          0x10
#define PFLAG_B1          0x20
#define PFLAG_OVERFLOW    0x40
#define PFLAG_NEGATIVE    0x80

#define NMI_VECTOR_LOCATION        0xFFFA
#define RESET_VECTOR_LOCATION      0xFFFC
#define IRQ_VECTOR_LOCATION        0xFFFE
#define STACK_OFFSET      0x0100

static inline bool IsFlagSet(const uint8_t *P, uint8_t flag)
{
  return ((*P) & flag) > 0;
}

static inline void SetFlag(uint8_t *P, uint8_t flag, bool set)
{
  if (set)
  {
    (*P) |= flag;
  }
  else
  {
    (*P) &= ~flag;
  }
}

static inline uint16_t Read16(CPU_t *cpu, uint16_t address)
{
  uint8_t lowByte;
  lowByte = Bus_ReadFromCPU(cpu->Bus, address);
  return (Bus_ReadFromCPU(cpu->Bus, address + 1) << 8) | lowByte;
}

static inline uint8_t Read(CPU_t *cpu, uint16_t address)
{
  return Bus_ReadFromCPU(cpu->Bus, address);
}

static inline void Write(CPU_t *cpu, uint16_t address, uint8_t data)
{
  Bus_WriteFromCPU(cpu->Bus, address, data);
}

static inline void Push(CPU_t *cpu, uint8_t data)
{
  Write(cpu, cpu->S + STACK_OFFSET, data);
  cpu->S--;
}

static inline uint8_t Pop(CPU_t *cpu)
{
  cpu->S++;
  return Read(cpu, cpu->S + STACK_OFFSET);
}

#endif /* SRC_NES_CPU_INTERNAL_H_ */
