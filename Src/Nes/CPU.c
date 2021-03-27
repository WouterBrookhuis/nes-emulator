/*
 * CPU.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */


#include "CPU.h"
#include "Bus.h"
#include "stdint.h"

#include <string.h>
#include <stdbool.h>

#include "CPU_Internal.h"
#include "InstructionTable.h"

void CPU_Initialize(CPU_t *cpu)
{
  memset(cpu, 0, sizeof(*cpu));

  cpu->CycleCount = 0;
}

void CPU_Reset(CPU_t *cpu)
{
  cpu->PC = Read16(cpu, RESET_VECTOR_LOCATION);
  // Reset normally takes 7 cycles
  // TODO: Proper reset, irq and nmi implementation
  cpu->CycleCount = 7;
  cpu->S = 0xFD;
  cpu->P = 0x24;
}

void CPU_NMI(CPU_t *cpu, bool assert)
{
  CR1_Write(&cpu->NMILineAsserted, assert);
}

void CPU_Tick(CPU_t *cpu)
{
  const InstructionTableEntry_t *newInstruction;
  uint16_t readAddress;
  bool addressingCanHaveExtraCycle = false;
  bool instructionCanHaveExtraCycle = false;

  if (cpu->IsKilled)
  {
    // We be dead, exit before doing anything
    return;
  }

  // Clock prescaler of 12
  if (cpu->ClockPhaseCounter == 0)
  {
    cpu->ClockPhaseCounter++;
  }
  else if (cpu->ClockPhaseCounter == 6)
  {
    // NMI edge detection
    CR1_Clock(&cpu->NMILineAsserted);
    if (CR1_Read(cpu->NMILineAsserted) && !cpu->NMILineAssertedPrevious)
    {
      // NMI was asserted, pend it
      CR1_Write(&cpu->NMIPendingInternal, true);
    }

    cpu->NMILineAssertedPrevious = CR1_Read(cpu->NMILineAsserted);

    cpu->ClockPhaseCounter++;
    return;
  }
  else if (cpu->ClockPhaseCounter == 11)
  {
    cpu->ClockPhaseCounter = 0;
    return;
  }
  else
  {
    cpu->ClockPhaseCounter++;
    return;
  }

  cpu->CycleCount++;

  if (cpu->CyclesLeftForInstruction == 0)
  {
    if (cpu->NextInstructionIsNMI)
    {
      // NMI takes priority over other things
      // Push PC (hi, then low)
      Push(cpu, cpu->PC >> 8);
      Push(cpu, (uint8_t)cpu->PC);
      // Push P
      uint8_t statusByte = cpu->P;
      // Set B flag correctly before pushing
      SetFlag(&statusByte, PFLAG_B0, false);  // 1 = BRK, 0 = NMI/IRQ
      SetFlag(&statusByte, PFLAG_B1, true);   // Always 1
      Push(cpu, statusByte);
      // Put NMI vector in PC
      cpu->PC = Read16(cpu, NMI_VECTOR_LOCATION);
      // Set interrupt disable flag
      SetFlag(&cpu->P, PFLAG_INTDISABLE, true);
      // NMI takes 7 cycles
      cpu->CyclesLeftForInstruction = 7;

      cpu->NextInstructionIsNMI = false;
    }
    else
    {
      // Time for a new instruction!
      cpu->Address = cpu->PC;
      cpu->Instruction = Bus_ReadFromCPU(cpu->Bus, cpu->PC);
      cpu->InstructionPC = cpu->PC;

      newInstruction = InstructionTable_GetInstruction(cpu->Instruction);

      cpu->AddressingMode = newInstruction->AddressingMode;
      cpu->CyclesLeftForInstruction = newInstruction->BaseCycleCount;

      // Calculate address for addressing mode and increment program counter past instruction
      switch (cpu->AddressingMode)
      {
      case ADDR_ABS:
        // Absolute
        cpu->Address = Read16(cpu, cpu->PC + 1);
        cpu->PC += 3;
        break;
      case ADDR_ABX:
        // Absolute + X
        readAddress = Read16(cpu, cpu->PC + 1);
        cpu->Address = readAddress + cpu->X;
        if ((cpu->Address & 0xFF00) != (readAddress & 0xFF00))
        {
          // Crossed page boundary, add extra cycle
          //cpu->CyclesLeftForInstruction++;
          addressingCanHaveExtraCycle = true;
        }
        cpu->PC += 3;
        break;
      case ADDR_ABY:
        // Absolute + Y
        readAddress = Read16(cpu, cpu->PC + 1);
        cpu->Address = readAddress + cpu->Y;
        if ((cpu->Address & 0xFF00) != (readAddress & 0xFF00))
        {
          // Crossed page boundary, add extra cycle
          //cpu->CyclesLeftForInstruction++;
          addressingCanHaveExtraCycle = true;
        }
        cpu->PC += 3;
        break;
      case ADDR_IMM:
        // Single byte operand, no addressing
        cpu->Address = cpu->PC + 1;
        cpu->PC += 2;
        break;
      case ADDR_IMP:
        // Implied, single byte instruction
        cpu->PC += 1;
        break;
      case ADDR_IND:
        // Indirect, pointer to actual address basically
        readAddress = Read16(cpu, cpu->PC + 1);
        // Errata: When lower byte is stored at 0x..FF higher byte is grabbed from the same page
        // as the lower byte instead of from the following page
        if ((readAddress & 0x00FF) == 0xFF)
        {
          cpu->Address = (Read(cpu, readAddress & 0xFF00) << 8) | Read(cpu, readAddress);
        }
        else
        {
          cpu->Address = Read16(cpu, readAddress);
        }
        cpu->PC += 3;
        break;
      case ADDR_IZX:
        // Indirect zero page with X
        readAddress = Read(cpu, cpu->PC + 1);
        readAddress += cpu->X;
        readAddress &= 0x00FF;
        // Errata: When lower bits are stored at 0xFF higher bits are grabbed from 0x00 instead of 0x100
        if (readAddress == 0xFF)
        {
          readAddress = (Read(cpu, 0x00) << 8) | Read(cpu, 0xFF);
        }
        else
        {
          readAddress = Read16(cpu, readAddress);
        }
        cpu->Address = readAddress;
        cpu->PC += 2;
        break;
      case ADDR_IZY:
        // Indirect zero page with Y
        readAddress = Read(cpu, cpu->PC + 1);

        // Errata: When lower bits are stored at 0xFF higher bits are grabbed from 0x00 instead of 0x100
        if (readAddress == 0xFF)
        {
          readAddress = (Read(cpu, 0x00) << 8) | Read(cpu, 0xFF);
        }
        else
        {
          readAddress = Read16(cpu, readAddress);
        }

        cpu->Address = readAddress + cpu->Y;
        if ((cpu->Address & 0xFF00) != (readAddress & 0xFF00))
        {
          // Crossed page boundary, add extra cycle
          //cpu->CyclesLeftForInstruction++;
          addressingCanHaveExtraCycle = true;
        }
        cpu->PC += 2;
        break;
      case ADDR_REL:
        // Relative, 1 byte operand, treat as two's complement signed offset
        // It is added to the INCREMENTED program counter, so add 2 first
        readAddress = cpu->Address + 2;
        cpu->Address = readAddress + (int8_t)Read(cpu, cpu->PC + 1);
        if ((cpu->Address & 0xFF00) != (readAddress & 0xFF00))
        {
          // Crossed page boundary, add extra cycle
          //cpu->CyclesLeftForInstruction++;
          addressingCanHaveExtraCycle = true;
        }
        cpu->PC += 2;
        break;
      case ADDR_ZP0:
        // Zero page
        cpu->Address = Read(cpu, cpu->PC + 1);
        cpu->PC += 2;
        break;
      case ADDR_ZPX:
        // Zero page + X
        cpu->Address = Read(cpu, cpu->PC + 1) + cpu->X;
        cpu->Address &= 0x00FF;
        cpu->PC += 2;
        break;
      case ADDR_ZPY:
        // Zero page + Y
        cpu->Address = Read(cpu, cpu->PC + 1) + cpu->Y;
        cpu->Address &= 0x00FF;
        cpu->PC += 2;
        break;
      default:
        // TODO: Error?
        break;
      }

      // Execute instruction and add any extra cycles needed
      instructionCanHaveExtraCycle = newInstruction->Action(cpu);

      if (addressingCanHaveExtraCycle && instructionCanHaveExtraCycle)
      {
        cpu->CyclesLeftForInstruction++;
      }
    }
  }
  else if (cpu->CyclesLeftForInstruction == 1)
  {
    if (CR1_Read(cpu->NMIPendingInternal))
    {
      cpu->NextInstructionIsNMI = true;
      CR1_Write(&cpu->NMIPendingInternal, false);
    }
  }

  // Clock registers
  CR1_Clock(&cpu->NMIPendingInternal);

  // Always decrement cycle counter
  cpu->CyclesLeftForInstruction--;
}
