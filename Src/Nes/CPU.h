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
#include "ClockedRegister.h"

typedef struct _Bus_t Bus_t;

typedef struct _CPU_t
{
  unsigned int ClockPhaseCounter;   // Counter for the external clock, we run at a prescaler of 4
  unsigned int CycleCount;          // Total cycle count, debugging info
  unsigned int CyclesLeftForInstruction;  // Amount of cycles left for the current instruction
  unsigned int InstructionCount;    // Amount of executed instructions
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

  bool IsRisingClockEdge;
  bool NMILineAssertedPrevious;
  bool NMILineAsserted;
  bool IRQLineAsserted;
  cr1_t IRQPendingInternal;
  cr1_t NMIPendingInternal;
  bool NextInstructionIsNMI;
  bool NextInstructionIsIRQ;
} CPU_t;

void CPU_Initialize(CPU_t *cpu);
void CPU_Tick(CPU_t *cpu);
void CPU_Reset(CPU_t *cpu);
void CPU_NMI(CPU_t *cpu, bool assert);
void CPU_IRQ(CPU_t *cpu, bool assert);

#endif /* SRC_NES_CPU_H_ */
