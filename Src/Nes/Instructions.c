/*
 * Instructions.c
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#include "Instructions.h"
#include "CPU.h"
#include "CPU_Internal.h"

// Add With Carry
int ADC(CPU_t *cpu)
{
  uint8_t mem;
  uint16_t tempResult;
  mem = Read(cpu, cpu->Address);
  tempResult = cpu->A + mem;
  if (IsFlagSet(&cpu->P, PFLAG_CARRY))
  {
    tempResult++;
  }

  // Update flags
  SetFlag(&cpu->P, PFLAG_CARRY, tempResult > 0xFF);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, tempResult & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (tempResult & 0xFF) == 0x00);
  SetFlag(&cpu->P, PFLAG_OVERFLOW, ((cpu->A & 0x80) ^ (tempResult & 0x80)) & ~((cpu->A & 0x80) ^ (mem & 0x80)));

  // Store result
  cpu->A = (uint8_t)tempResult;

  return 1;
}

// Illegal
int AHX(CPU_t *cpu)
{
  // TODO: AHX
  KIL(cpu);
  return 0;
}

// Illegal: And with immediate and shift right
int ALR(CPU_t *cpu)
{
  // TODO: Is broken, pls fix?
  uint16_t temp = cpu->A & Read(cpu, cpu->Address);
  temp >>= 1;
  cpu->A = temp;
  // Set N, Z, C
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
  SetFlag(&cpu->P, PFLAG_CARRY, temp > 255);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0x00);
  return 0;
}

// Illegal: AND but set carry to bit 7 of A
int ANC(CPU_t *cpu)
{
  AND(cpu);
  SetFlag(&cpu->P, PFLAG_CARRY, cpu->A & 0x80);
  return 0;
}

// Illegal: A AND Immediate and then ROR
int ARR(CPU_t *cpu)
{
  // TODO: ALR fails test?
  AND(cpu);
  ROR(cpu);
  return 0;
}

// Illegal: AND X and A, store it in memory
int AXS(CPU_t *cpu)
{
  // TODO: ALR fails test?
  // Just AND and write, no flags to set
  uint8_t temp = cpu->X & cpu->A;
  Write(cpu, cpu->Address, temp);
  return 0;
}

// And with memory
int AND(CPU_t *cpu)
{
  uint8_t mem = Read(cpu, cpu->Address);
  cpu->A = mem & cpu->A;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  return 1;
}

// Shift left A or Memory
int ASL(CPU_t *cpu)
{
  uint8_t temp;
  if (cpu->AddressingMode == ADDR_IMP)
  {
    // Apply to A register
    SetFlag(&cpu->P, PFLAG_CARRY, cpu->A & 0x80);

    cpu->A = cpu->A << 1;

    SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  }
  else
  {
    // Apply to memory
    temp = Read(cpu, cpu->Address);
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x80);

    temp = temp << 1;
    Write(cpu, cpu->Address, temp);

    SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, temp == 0);
  }
  return 0;
}

// Branch on carry clear
int BCC(CPU_t *cpu)
{
  if (!IsFlagSet(&cpu->P, PFLAG_CARRY))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Branch on carry set
int BCS(CPU_t *cpu)
{
  if (IsFlagSet(&cpu->P, PFLAG_CARRY))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Branch on zero set
int BEQ(CPU_t *cpu)
{
  if (IsFlagSet(&cpu->P, PFLAG_ZERO))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Test Bits in Memory with Accumulator
int BIT(CPU_t *cpu)
{
  uint8_t mem = Read(cpu, cpu->Address);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, mem & 0x80);
  SetFlag(&cpu->P, PFLAG_OVERFLOW, mem & 0x40);
  SetFlag(&cpu->P, PFLAG_ZERO, (mem & cpu->A) == 0);
  return 0;
}

// Branch on negative set
int BMI(CPU_t *cpu)
{
  if (IsFlagSet(&cpu->P, PFLAG_NEGATIVE))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Branch on zero clear
int BNE(CPU_t *cpu)
{
  if (!IsFlagSet(&cpu->P, PFLAG_ZERO))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Branch on negative clear
int BPL(CPU_t *cpu)
{
  if (!IsFlagSet(&cpu->P, PFLAG_NEGATIVE))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Force Break
int BRK(CPU_t *cpu)
{
  // BRK is weird because it pushes PC + 2, skipping a byte after the BRK instruction
  uint16_t pcToPush = cpu->InstructionPC + 2;
  // Push PC (hi, then low)
  Push(cpu, pcToPush >> 8);
  Push(cpu, (uint8_t)pcToPush);
  // Push P
  uint8_t statusByte = cpu->P;
  // Set B flag correctly before pushing
  SetFlag(&statusByte, PFLAG_B0, true);   // 1 = BRK
  SetFlag(&statusByte, PFLAG_B1, true);   // Always 1
  Push(cpu, statusByte);
  // Put interrupt vector in PC
  cpu->PC = Read16(cpu, IRQ_VECTOR_LOCATION);
  // Set interrupt disable flag?
  SetFlag(&cpu->P, PFLAG_INTDISABLE, true);
  return 0;
}

// Branch on overflow clear
int BVC(CPU_t *cpu)
{
  if (!IsFlagSet(&cpu->P, PFLAG_OVERFLOW))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Branch on overflow set
int BVS(CPU_t *cpu)
{
  if (IsFlagSet(&cpu->P, PFLAG_OVERFLOW))
  {
    // Add 1 extra cycle for branching
    cpu->CyclesLeftForInstruction++;
    // Relative addressing only, in which case cpu->Address is our new program counter
    cpu->PC = cpu->Address;
    return 1;
  }
  return 0;
}

// Clear carry flag
int CLC(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_CARRY, false);
  return 0;
}

// Clear decimal flag
int CLD(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_DECIMAL, false);
  return 0;
}

// Clear interrupt disable flag
int CLI(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_INTDISABLE, false);
  return 0;
}

// Clear overflow flag
int CLV(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_OVERFLOW, false);
  return 0;
}

// Compare memory with A
int CMP(CPU_t *cpu)
{
  // Do A - M and update Z, N and C
  uint8_t mem = Read(cpu, cpu->Address);
  uint16_t temp = cpu->A + ~mem + 1;
  SetFlag(&cpu->P, PFLAG_CARRY, cpu->A >= mem);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (temp & 0xFF) == 0x00);
  return 1;
}

// Compare memory with X
int CPX(CPU_t *cpu)
{
  // Do X - M and update Z, N and C
  uint8_t mem = Read(cpu, cpu->Address);
  uint16_t temp = cpu->X + ~mem + 1;
  SetFlag(&cpu->P, PFLAG_CARRY, cpu->X >= mem);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (temp & 0xFF) == 0x00);
  return 1;
}

// Compare memory with Y
int CPY(CPU_t *cpu)
{
  // Do Y - M and update Z, N and C
  uint8_t mem = Read(cpu, cpu->Address);
  uint16_t temp = cpu->Y + ~mem + 1;
  SetFlag(&cpu->P, PFLAG_CARRY, cpu->Y >= mem);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (temp & 0xFF) == 0x00);
  return 1;
}

// Illegal: Decrement memory, then compare
int DCP(CPU_t *cpu)
{
  // Basically a DEC followed by CMP
  DEC(cpu);
  CMP(cpu);
  return 0;
}

// Decrement memory
int DEC(CPU_t *cpu)
{
  uint16_t temp = Read(cpu, cpu->Address);
  temp--;
  Write(cpu, cpu->Address, temp);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (temp & 0xFF) == 0x00);
  return 0;
}

// Decrement X
int DEX(CPU_t *cpu)
{
  cpu->X--;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->X & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->X == 0x00);
  return 0;
}

// Decrement Y
int DEY(CPU_t *cpu)
{
  cpu->Y--;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->Y & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->Y == 0x00);
  return 0;
}

// XOR memory with A, store in A
int EOR(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address);
  temp = temp ^ cpu->A;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->A = temp;
  return 1;
}

// Increment memory
int INC(CPU_t *cpu)
{
  uint16_t temp = Read(cpu, cpu->Address);
  temp++;
  Write(cpu, cpu->Address, temp);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (temp & 0xFF) == 0x00);
  return 0;
}

// Increment X
int INX(CPU_t *cpu)
{
  cpu->X++;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->X & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->X == 0x00);
  return 0;
}

// Increment Y
int INY(CPU_t *cpu)
{
  cpu->Y++;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->Y & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->Y == 0x00);
  return 0;
}

// Illegal: Increment memory, then SBC with it
int ISC(CPU_t *cpu)
{
  INC(cpu);
  SBC(cpu);
  return 0;
}

// Jump to location
int JMP(CPU_t *cpu)
{
  // PC is not touched after Instruction action, so this will jump us to address
  cpu->PC = cpu->Address;
  return 0;
}

// Jump to location, save return address on stack
int JSR(CPU_t *cpu)
{
  // Need to push PC + 2, but PC has already been incremented at this point
  // We use InstructionPC instead
  uint16_t pcToPush = cpu->InstructionPC + 2;
  // Push PC (hi, then low)
  Push(cpu, pcToPush >> 8);
  Push(cpu, (uint8_t)pcToPush);
  // PC is not touched after Instruction action, so this will jump us to address
  cpu->PC = cpu->Address;
  return 0;
}

// Illegal: mem AND S -> A,X,S
int LAS(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address) & cpu->S;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->A = temp;
  cpu->X = temp;
  cpu->S = temp;
  return 1;
}

// Illegal: Load A and X from memory
int LAX(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->A = temp;
  cpu->X = temp;
  return 1;
}

// Load A from memory
int LDA(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->A = temp;
  return 1;
}

// Load X from memory
int LDX(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->X = temp;
  return 1;
}

// Load Y from memory
int LDY(CPU_t *cpu)
{
  uint8_t temp = Read(cpu, cpu->Address);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  cpu->Y = temp;
  return 1;
}

// Shift right A or Memory
int LSR(CPU_t *cpu)
{
  uint8_t temp;
  if (cpu->AddressingMode == ADDR_IMP)
  {
    // Apply to A register
    SetFlag(&cpu->P, PFLAG_CARRY, cpu->A & 0x01);

    cpu->A = cpu->A >> 1;

    SetFlag(&cpu->P, PFLAG_NEGATIVE, 0x00);
    SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  }
  else
  {
    // Apply to memory
    temp = Read(cpu, cpu->Address);
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x01);

    temp = temp >> 1;
    Write(cpu, cpu->Address, temp);

    SetFlag(&cpu->P, PFLAG_NEGATIVE, 0x00);
    SetFlag(&cpu->P, PFLAG_ZERO, temp == 0);
  }
  return 0;
}

// No Operation
int NOP(CPU_t *cpu)
{
  // Can actually take longer
  return 1;
}

// JOOOOOOOOJO
// OR memory with A
int ORA(CPU_t *cpu)
{
  uint8_t mem = Read(cpu, cpu->Address);
  cpu->A = mem | cpu->A;
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  return 1;
}

// Push A to stack
int PHA(CPU_t *cpu)
{
  Push(cpu, cpu->A);
  return 0;
}

// Push P (status flags) to stack
int PHP(CPU_t *cpu)
{
  uint8_t pToPush = cpu->P;
  // Both B flags are set when pushed with PHP
  SetFlag(&pToPush, PFLAG_B0, true);
  SetFlag(&pToPush, PFLAG_B1, true);
  Push(cpu, pToPush);
  return 0;
}

// Pull A from stack
int PLA(CPU_t *cpu)
{
  cpu->A = Pop(cpu);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  return 0;
}

// Pull P (status flag) from stack
int PLP(CPU_t *cpu)
{
  // TODO: Interrupt shenanigans
  cpu->P = Pop(cpu);
  // Is this always supposed to be this way?
  SetFlag(&cpu->P, PFLAG_B0, false);
  SetFlag(&cpu->P, PFLAG_B1, true);
  return 0;
}

// Illegal: ROL + AND
int RLA(CPU_t *cpu)
{
  ROL(cpu);
  AND(cpu);
  return 0;
}

// Rotate One Left (but the carry is part of it)
int ROL(CPU_t *cpu)
{
  uint16_t temp;
  if (cpu->AddressingMode == ADDR_IMP)
  {
    // Apply to A register
    // Shift A left
    temp = cpu->A;
    temp <<= 1;
    // Lowest bit is carry
    temp |= IsFlagSet(&cpu->P, PFLAG_CARRY);
    // Bit that was shifted out of LSB becomes carry
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x0100);

    cpu->A = (uint8_t)temp;

    SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  }
  else
  {
    // Apply to memory
    // Shift mem left
    temp = Read(cpu, cpu->Address);
    temp <<= 1;
    // Lowest bit is carry
    temp |= IsFlagSet(&cpu->P, PFLAG_CARRY);
    // Bit that was shifted out of LSB becomes carry
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x0100);
    // Clear high byte so it doesn't mess up flag logic below
    temp &= 0x00FF;
    Write(cpu, cpu->Address, (uint8_t)temp);

    SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  }
  return 0;
}

// Rotate One Right (but the carry is part of it)
int ROR(CPU_t *cpu)
{
  uint16_t temp;
  if (cpu->AddressingMode == ADDR_IMP)
  {
    // Apply to A register
    // Place carry bit above the A bits
    temp = cpu->A | (IsFlagSet(&cpu->P, PFLAG_CARRY) << 8);
    // Lowest bit will be carry
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x0001);
    // Now we can shift it down and store it
    temp >>= 1;

    cpu->A = (uint8_t)temp;

    SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, cpu->A == 0);
  }
  else
  {
    // Apply to memory
    // Place carry bit above the memory bits
    temp = Read(cpu, cpu->Address) | (IsFlagSet(&cpu->P, PFLAG_CARRY) << 8);
    // Lowest bit will be carry
    SetFlag(&cpu->P, PFLAG_CARRY, temp & 0x0001);
    // Now we can shift it down and store it
    temp >>= 1;
    // Clear high byte so it doesn't mess up flag logic below
    temp &= 0x00FF;
    Write(cpu, cpu->Address, (uint8_t)temp);

    SetFlag(&cpu->P, PFLAG_NEGATIVE, temp & 0x80);
    SetFlag(&cpu->P, PFLAG_ZERO, temp == 0x00);
  }
  return 0;
}

//Illegal: ROR mem, then ADC with A
int RRA(CPU_t *cpu)
{
  ROR(cpu);
  ADC(cpu);
  return 0;
}

// Return from interrupt
int RTI(CPU_t *cpu)
{
  uint16_t newPC;
  // Pop status register from stack
  cpu->P = Pop(cpu);
  // Is this always supposed to be this way?
  SetFlag(&cpu->P, PFLAG_B0, false);
  SetFlag(&cpu->P, PFLAG_B1, true);
  // Pop PC from stack
  newPC = Pop(cpu);
  newPC |= (uint16_t)Pop(cpu) << 8;
  cpu->PC = newPC;
  return 0;
}

// Return from subroutine
int RTS(CPU_t *cpu)
{
  uint16_t newPC;
  // Pop PC from stack
  newPC = Pop(cpu);
  newPC |= (uint16_t)Pop(cpu) << 8;
  // Need to increment popped value by 1 according to spec
  cpu->PC = newPC + 1;
  return 0;
}

// Store A & X
int SAX(CPU_t *cpu)
{
  Write(cpu, cpu->Address, cpu->A & cpu->X);
  return 0;
}

// Subtract with borrow
int SBC(CPU_t *cpu)
{
  uint8_t mem;
  uint16_t tempResult;
  // Invert mem and the rest is identical to ADC!
  mem = ~Read(cpu, cpu->Address);
  tempResult = cpu->A + mem + IsFlagSet(&cpu->P, PFLAG_CARRY);

  // Update flags
  SetFlag(&cpu->P, PFLAG_CARRY, tempResult > 0xFF);
  SetFlag(&cpu->P, PFLAG_NEGATIVE, tempResult & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, (tempResult & 0xFF) == 0x00);
  SetFlag(&cpu->P, PFLAG_OVERFLOW, ((cpu->A & 0x80) ^ (tempResult & 0x80)) & ~((cpu->A & 0x80) ^ (mem & 0x80)));

  // Store result
  cpu->A = (uint8_t)tempResult;

  return 1;
}

// Set carry flag
int SEC(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_CARRY, true);
  return 0;
}

// Set decimal flag
int SED(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_DECIMAL, true);
  return 0;
}

// Set interrupt disabled flag
int SEI(CPU_t *cpu)
{
  SetFlag(&cpu->P, PFLAG_INTDISABLE, true);
  return 0;
}

// Illegal: Shift left memory, then OR with A
int SLO(CPU_t *cpu)
{
  ASL(cpu);
  ORA(cpu);
  return 0;
}

// Illegal: LSR memory, then EOR with A
int SRE(CPU_t *cpu)
{
  LSR(cpu);
  EOR(cpu);
  return 0;
}

// Store A in memory
int STA(CPU_t *cpu)
{
  Write(cpu, cpu->Address, cpu->A);
  return 0;
}

// Store X in memory
int STX(CPU_t *cpu)
{
  Write(cpu, cpu->Address, cpu->X);
  return 0;
}

// Store Y in memory
int STY(CPU_t *cpu)
{
  Write(cpu, cpu->Address, cpu->Y);
  return 0;
}

// Transfer A to X
int TAX(CPU_t *cpu)
{
  cpu->X = cpu->A;

  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->X  & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->X  == 0x00);
  return 0;
}

// Transfer A to Y
int TAY(CPU_t *cpu)
{
  cpu->Y = cpu->A;

  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->Y  & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->Y  == 0x00);
  return 0;
}

// Transfer S to X
int TSX(CPU_t *cpu)
{
  cpu->X = cpu->S;

  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->X  & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->X  == 0x00);
  return 0;
}

// Transfer X to A
int TXA(CPU_t *cpu)
{
  cpu->A = cpu->X;

  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A  & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A  == 0x00);
  return 0;
}

// Transfer X to S
int TXS(CPU_t *cpu)
{
  cpu->S = cpu->X;

  return 0;
}

// Transfer Y to A
int TYA(CPU_t *cpu)
{
  cpu->A = cpu->Y;

  SetFlag(&cpu->P, PFLAG_NEGATIVE, cpu->A  & 0x80);
  SetFlag(&cpu->P, PFLAG_ZERO, cpu->A  == 0x00);
  return 0;
}

// Invalid instruction that hangs the CPU
int KIL(CPU_t *cpu)
{
  // TODO: Mimic bus behaviour?
  //Write(cpu, 0x0000, 0xFF);
  cpu->IsKilled = true;
  return 0;
}


// TODO: These unoffical instructions

int XAA(CPU_t *cpu)
{
  KIL(cpu);
  return 0;
}

int TAS(CPU_t *cpu)
{
  KIL(cpu);
  return 0;
}

int SHY(CPU_t *cpu)
{
  KIL(cpu);
  return 0;
}

int SHX(CPU_t *cpu)
{
  KIL(cpu);
  return 0;
}








