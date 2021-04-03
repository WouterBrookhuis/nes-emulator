/*
 * InstructionTable.h
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_INSTRUCTIONTABLE_H_
#define SRC_NES_INSTRUCTIONTABLE_H_

#include "Types.h"
#include "AddressingMode.h"

typedef struct _CPU_t CPU_t;

typedef int (*InstructionAction_t)(CPU_t *cpu);

// Use a struct so we can add more data later if required
typedef struct _InstructionTableEntry_t
{
  InstructionAction_t Action;
  AddressingMode_t AddressingMode;
  int BaseCycleCount;
  const char* Name;
} InstructionTableEntry_t;

u8_t InstructionTable_GetInstructionCount(void);

const InstructionTableEntry_t* InstructionTable_GetInstruction(u8_t instruction);

#endif /* SRC_NES_INSTRUCTIONTABLE_H_ */
