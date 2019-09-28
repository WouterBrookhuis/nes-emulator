/*
 * CPU.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_CPU_H_
#define SRC_NES_CPU_H_

#include <stdint.h>
#include <stdbool.h>

#include "AddressingMode.h"

typedef struct _Bus_t Bus_t;

typedef struct _CPU_t
{
  unsigned int CycleCount;          // Total cycle count, debugging info
  unsigned int CyclesLeftForInstruction;  // Amount of cycles left for the current instruction
  Bus_t *Bus;                       // Pointer to Bus structure the CPU is connected to
  bool IsKilled;                    // Flag to check if processor is killed by invalid instruction
  AddressingMode_t AddressingMode;  // Addressing mode of current instruction
  uint16_t Address;                 // Current address for the bus
  uint8_t Instruction;              // Current instruction byte
  uint16_t InstructionPC;           // The PC value where this instruction came from

  uint8_t A;      // Accumulator register
  uint8_t X;      // X addressing register
  uint8_t Y;      // Y addressing register
  uint16_t PC;    // Program counter
  uint8_t S;      // Stack pointer
  uint8_t P;      // Status register

  bool PendingNMI;
  bool PendingIRQ;

} CPU_t;

void CPU_Initialize(CPU_t *cpu);
void CPU_Tick(CPU_t *cpu);
void CPU_Reset(CPU_t *cpu);
void CPU_NMI(CPU_t *cpu);

#endif /* SRC_NES_CPU_H_ */
