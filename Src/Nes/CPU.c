/*
 * CPU.c
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */


#include "CPU.h"
#include "Bus.h"
#include "stdint.h"
#include "SharedSDL.h"

#include <string.h>
#include <stdbool.h>

#include "CPU_Internal.h"
#include "InstructionTable.h"

void CPU_Initialize(CPU_t *cpu)
{
  memset(cpu, 0, sizeof(*cpu));

  cpu->CycleCount = 0;
  cpu->IsRisingClockEdge = true;
}

void CPU_Reset(CPU_t *cpu)
{
  cpu->PC = Read16(cpu, RESET_VECTOR_LOCATION);
  // Reset normally takes 7 cycles
  // TODO: Proper reset, irq and nmi implementation
  cpu->CycleCount = 7;
  cpu->S = 0xFD;
  cpu->P = 0x24;
  cpu->IsRisingClockEdge = true;
}

void CPU_NMI(CPU_t *cpu, bool assert)
{
  cpu->NMILineAsserted = assert;
}

void CPU_IRQ(CPU_t *cpu, bool assert)
{
  cpu->IRQLineAsserted = assert;
}

void CPU_Tick(CPU_t *cpu)
{
  const InstructionTableEntry_t *newInstruction;
  u16_t readAddress;
  bool addressingCanHaveExtraCycle = false;
  bool instructionCanHaveExtraCycle = false;

  if (!cpu->IsRisingClockEdge)
  {
    // NMI edge detection on falling edges
    if (cpu->NMILineAsserted && !cpu->NMILineAssertedPrevious)
    {
      // NMI was asserted, pend it
      CR1_Write(&cpu->NMIPendingInternal, true);
    }

    cpu->NMILineAssertedPrevious = cpu->NMILineAsserted;

    // IRQ level detection
    CR1_Write(&cpu->IRQPendingInternal, cpu->IRQLineAsserted);

    // Next tick will be a rising clock edge
    cpu->IsRisingClockEdge = true;
    return;
  }

  // Next tick will be a falling clock edge
  cpu->IsRisingClockEdge = false;

  if (cpu->IsKilled)
  {
    // We be dead, exit before doing anything
    return;
  }

  SharedSDL_BeginTiming(PERF_INDEX_CPU);

  cpu->CycleCount++;

  if (cpu->CyclesLeftForInstruction == 0)
  {
    if (cpu->NextInstructionIsNMI)
    {
      // NMI takes priority over other things
      // Push PC (hi, then low)
      Push(cpu, cpu->PC >> 8);
      Push(cpu, (u8_t)cpu->PC);
      // Push P
      u8_t statusByte = cpu->P;
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
    // TODO: Implement IRQ handling
//    else if (cpu->NextInstructionIsIRQ && ((cpu->P & PFLAG_INTDISABLE) == 0))
//    {
//      // IRQ takes priority over other things, but not an NMI
//      // Push PC (hi, then low)
//      Push(cpu, cpu->PC >> 8);
//      Push(cpu, (u8_t)cpu->PC);
//      // Push P
//      u8_t statusByte = cpu->P;
//      // Set B flag correctly before pushing
//      SetFlag(&statusByte, PFLAG_B0, false);  // 1 = BRK, 0 = NMI/IRQ
//      SetFlag(&statusByte, PFLAG_B1, true);   // Always 1
//      Push(cpu, statusByte);
//      // Put IRQ vector in PC
//      cpu->PC = Read16(cpu, IRQ_VECTOR_LOCATION);
//      // Set interrupt disable flag
//      SetFlag(&cpu->P, PFLAG_INTDISABLE, true);
//      // IRQ takes 7 cycles
//      cpu->CyclesLeftForInstruction = 7;
//
//      cpu->NextInstructionIsIRQ = false;
//    }
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

    cpu->InstructionCount++;
  }

  if (cpu->CyclesLeftForInstruction == 1)
  {
    if (CR1_Read(cpu->NMIPendingInternal))
    {
      cpu->NextInstructionIsNMI = true;
      CR1_Write(&cpu->NMIPendingInternal, false);
    }

    cpu->NextInstructionIsIRQ = CR1_Read(cpu->IRQPendingInternal);
  }

  // Clock registers
  CR1_Clock(&cpu->NMIPendingInternal);
  CR1_Clock(&cpu->IRQPendingInternal);

  // Always decrement cycle counter
  cpu->CyclesLeftForInstruction--;

  SharedSDL_EndTiming(PERF_INDEX_CPU);
}
