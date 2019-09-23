/*
 * Instructions.h
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_INSTRUCTIONS_H_
#define SRC_NES_INSTRUCTIONS_H_

#include "CPU.h"

typedef int (*InstructionAction_t)(CPU_t *cpu);

int BRK(CPU_t *cpu);
int ORA(CPU_t *cpu);
int KIL(CPU_t *cpu);
int SLO(CPU_t *cpu);
int NOP(CPU_t *cpu);
int ASL(CPU_t *cpu);
int PHP(CPU_t *cpu);
int ANC(CPU_t *cpu);

int BPL(CPU_t *cpu);
int CLC(CPU_t *cpu);

int JSR(CPU_t *cpu);
int AND(CPU_t *cpu);
int RLA(CPU_t *cpu);
int BIT(CPU_t *cpu);
int ROL(CPU_t *cpu);
int PLP(CPU_t *cpu);

int BMI(CPU_t *cpu);
int SEC(CPU_t *cpu);

int RTI(CPU_t *cpu);
int FOR(CPU_t *cpu);
int SRE(CPU_t *cpu);
int EOR(CPU_t *cpu);
int LSR(CPU_t *cpu);
int PHA(CPU_t *cpu);
int ALR(CPU_t *cpu);
int JMP(CPU_t *cpu);

int BVC(CPU_t *cpu);
int CLI(CPU_t *cpu);

int RTS(CPU_t *cpu);
int ADC(CPU_t *cpu);
int RRA(CPU_t *cpu);
int ROR(CPU_t *cpu);
int PLA(CPU_t *cpu);
int ARR(CPU_t *cpu);

int BVS(CPU_t *cpu);
int SEI(CPU_t *cpu);

int STA(CPU_t *cpu);
int SAX(CPU_t *cpu);
int STY(CPU_t *cpu);
int STX(CPU_t *cpu);
int DEY(CPU_t *cpu);
int TXA(CPU_t *cpu);
int XAA(CPU_t *cpu);

int BCC(CPU_t *cpu);
int AHX(CPU_t *cpu);
int TYA(CPU_t *cpu);
int TXS(CPU_t *cpu);
int TAS(CPU_t *cpu);
int SHY(CPU_t *cpu);
int SHX(CPU_t *cpu);
int AHX(CPU_t *cpu);

int LDY(CPU_t *cpu);
int LDA(CPU_t *cpu);
int LDX(CPU_t *cpu);
int LAX(CPU_t *cpu);
int TAY(CPU_t *cpu);
int TAX(CPU_t *cpu);

int BCS(CPU_t *cpu);
int CLV(CPU_t *cpu);
int TSX(CPU_t *cpu);
int LAS(CPU_t *cpu);

int CPY(CPU_t *cpu);
int CMP(CPU_t *cpu);
int DCP(CPU_t *cpu);
int DEC(CPU_t *cpu);
int INY(CPU_t *cpu);
int DEX(CPU_t *cpu);
int AXS(CPU_t *cpu);
int DCP(CPU_t *cpu);

int BNE(CPU_t *cpu);
int CLD(CPU_t *cpu);
int TAY(CPU_t *cpu);

int CPX(CPU_t *cpu);
int SBC(CPU_t *cpu);
int ISC(CPU_t *cpu);
int CPX(CPU_t *cpu);
int INC(CPU_t *cpu);
int INX(CPU_t *cpu);

int BEQ(CPU_t *cpu);
int SED(CPU_t *cpu);

#endif /* SRC_NES_INSTRUCTIONS_H_ */
