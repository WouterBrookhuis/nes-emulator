/*
 * InstructionTable.c
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#include "InstructionTable.h"
#include "Instructions.h"
#include <stdint.h>
#include <stddef.h>

const static InstructionTableEntry_t TABLE[] =
{
    { BRK, ADDR_IMP, 7, "BRK" },
    { ORA, ADDR_IZX, 6, "ORA" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { SLO, ADDR_IZX, 8, "SLO" },
    { NOP, ADDR_ZP0, 3, "NOP" },
    { ORA, ADDR_ZP0, 3, "ORA" },
    { ASL, ADDR_ZP0, 5, "ASL" },
    { SLO, ADDR_ZP0, 5, "SLO" },
    { PHP, ADDR_IMP, 3, "PHP" },
    { ORA, ADDR_IMM, 2, "ORA" },
    { ASL, ADDR_IMP, 2, "ASL" },
    { ANC, ADDR_IMM, 2, "ANC" },
    { NOP, ADDR_ABS, 4, "NOP" },
    { ORA, ADDR_ABS, 4, "ORA" },
    { ASL, ADDR_ABS, 6, "ASL" },
    { SLO, ADDR_ABS, 6, "SLO" },

    { BPL, ADDR_REL, 2, "BPL" },
    { ORA, ADDR_IZY, 5, "ORA" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { SLO, ADDR_IZY, 8, "SLO" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { ORA, ADDR_ZPX, 4, "ORA" },
    { ASL, ADDR_ZPX, 6, "ASL" },
    { SLO, ADDR_ZPX, 6, "SLO" },
    { CLC, ADDR_IMP, 2, "CLC" },
    { ORA, ADDR_ABY, 4, "ORA" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { SLO, ADDR_ABY, 7, "SLO" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { ORA, ADDR_ABX, 4, "ORA" },
    { ASL, ADDR_ABX, 7, "ASL" },
    { SLO, ADDR_ABX, 7, "SLO" },

    { JSR, ADDR_ABS, 6, "JSR" },
    { AND, ADDR_IZX, 6, "AND" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { RLA, ADDR_IZX, 8, "RLA" },
    { BIT, ADDR_ZP0, 3, "BIT" },
    { AND, ADDR_ZP0, 3, "AND" },
    { ROL, ADDR_ZP0, 5, "ROL" },
    { RLA, ADDR_ZP0, 5, "RLA" },
    { PLP, ADDR_IMP, 4, "PLP" },
    { AND, ADDR_IMM, 2, "AND" },
    { ROL, ADDR_IMP, 2, "ROL" },
    { ANC, ADDR_IMM, 2, "ANC" },
    { BIT, ADDR_ABS, 4, "BIT" },
    { AND, ADDR_ABS, 4, "AND" },
    { ROL, ADDR_ABS, 6, "ROL" },
    { RLA, ADDR_ABS, 6, "RLA" },

    { BMI, ADDR_REL, 2, "BMI" },
    { AND, ADDR_IZY, 5, "AND" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { RLA, ADDR_IZY, 8, "RLA" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { AND, ADDR_ZPX, 4, "AND" },
    { ROL, ADDR_ZPX, 6, "ROL" },
    { RLA, ADDR_ZPX, 6, "RLA" },
    { SEC, ADDR_IMP, 2, "SEC" },
    { AND, ADDR_ABY, 4, "AND" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { RLA, ADDR_ABY, 7, "RLA" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { AND, ADDR_ABX, 4, "AND" },
    { ROL, ADDR_ABX, 7, "ROL" },
    { RLA, ADDR_ABX, 7, "RLA" },

    { RTI, ADDR_IMP, 6, "RTI" },
    { EOR, ADDR_IZX, 6, "EOR" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { SRE, ADDR_IZX, 8, "SRE" },
    { NOP, ADDR_ZP0, 3, "NOP" },
    { EOR, ADDR_ZP0, 3, "EOR" },
    { LSR, ADDR_ZP0, 5, "LSR" },
    { SRE, ADDR_ZP0, 5, "SRE" },
    { PHA, ADDR_IMP, 3, "PHA" },
    { EOR, ADDR_IMM, 2, "EOR" },
    { LSR, ADDR_IMP, 2, "LSR" },
    { ALR, ADDR_IMM, 2, "ALR" },
    { JMP, ADDR_ABS, 3, "JMP" },
    { EOR, ADDR_ABS, 4, "EOR" },
    { LSR, ADDR_ABS, 6, "LSR" },
    { SRE, ADDR_ABS, 6, "SRE" },

    { BVC, ADDR_REL, 2, "BVC" },
    { EOR, ADDR_IZY, 5, "EOR" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { SRE, ADDR_IZY, 8, "SRE" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { EOR, ADDR_ZPX, 4, "EOR" },
    { LSR, ADDR_ZPX, 6, "LSR" },
    { SRE, ADDR_ZPX, 6, "SRE" },
    { CLI, ADDR_IMP, 2, "CLI" },
    { EOR, ADDR_ABY, 4, "EOR" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { SRE, ADDR_ABY, 7, "SRE" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { EOR, ADDR_ABX, 4, "EOR" },
    { LSR, ADDR_ABX, 7, "LSR" },
    { SRE, ADDR_ABX, 7, "SRE" },

    { RTS, ADDR_IMM, 6, "RTS" },
    { ADC, ADDR_IZX, 6, "ADC" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { RRA, ADDR_IZX, 8, "RRA" },
    { NOP, ADDR_ZP0, 3, "NOP" },
    { ADC, ADDR_ZP0, 3, "ADC" },
    { ROR, ADDR_ZP0, 5, "ROR" },
    { RRA, ADDR_ZP0, 5, "RRA" },
    { PLA, ADDR_IMP, 4, "PLA" },
    { ADC, ADDR_IMM, 2, "ADC" },
    { ROR, ADDR_IMP, 2, "ROR" },
    { ARR, ADDR_IMM, 2, "ARR" },
    { JMP, ADDR_IND, 5, "JMP" },
    { ADC, ADDR_ABS, 4, "ADC" },
    { ROR, ADDR_ABS, 6, "ROR" },
    { RRA, ADDR_ABS, 6, "RRA" },

    { BVS, ADDR_REL, 2, "BVS" },
    { ADC, ADDR_IZY, 5, "ADC" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { RRA, ADDR_IZY, 8, "RRA" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { ADC, ADDR_ZPX, 4, "ADC" },
    { ROR, ADDR_ZPX, 6, "ROR" },
    { RRA, ADDR_ZPX, 6, "RRA" },
    { SEI, ADDR_IMP, 2, "SEI" },
    { ADC, ADDR_ABY, 4, "ADC" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { RRA, ADDR_ABY, 7, "RRA" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { ADC, ADDR_ABX, 4, "ADC" },
    { ROR, ADDR_ABX, 7, "ROR" },
    { RRA, ADDR_ABX, 7, "RRA" },

    { NOP, ADDR_IMM, 2, "NOP" },
    { STA, ADDR_IZX, 6, "STA" },
    { NOP, ADDR_IMM, 2, "NOP" },
    { SAX, ADDR_IZX, 6, "SAX" },
    { STY, ADDR_ZP0, 3, "STY" },
    { STA, ADDR_ZP0, 3, "STA" },
    { STX, ADDR_ZP0, 3, "STX" },
    { SAX, ADDR_ZP0, 3, "SAX" },
    { DEY, ADDR_IMP, 2, "DEY" },
    { NOP, ADDR_IMM, 2, "NOP" },
    { TXA, ADDR_IMP, 2, "TXA" },
    { XAA, ADDR_IMM, 2, "XAA" },
    { STY, ADDR_ABS, 4, "STY" },
    { STA, ADDR_ABS, 4, "STA" },
    { STX, ADDR_ABS, 4, "STX" },
    { SAX, ADDR_ABS, 4, "SAX" },

    { BCC, ADDR_REL, 2, "BCC" },
    { STA, ADDR_IZY, 6, "STA" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { AHX, ADDR_IZY, 6, "AHX" },
    { STY, ADDR_ZPX, 4, "STY" },
    { STA, ADDR_ZPX, 4, "STA" },
    { STX, ADDR_ZPY, 4, "STX" },
    { SAX, ADDR_ZPY, 4, "SAX" },
    { TYA, ADDR_IMP, 2, "TYA" },
    { STA, ADDR_ABY, 5, "STA" },
    { TXS, ADDR_IMP, 2, "TXS" },
    { TAS, ADDR_ABY, 5, "TAS" },
    { SHY, ADDR_ABX, 5, "SHY" },
    { STA, ADDR_ABX, 5, "STA" },
    { SHX, ADDR_ABY, 5, "SHX" },
    { AHX, ADDR_ABY, 5, "AHX" },

    { LDY, ADDR_IMM, 2, "LDY" },
    { LDA, ADDR_IZX, 6, "LDA" },
    { LDX, ADDR_IMM, 2, "LDX" },
    { LAX, ADDR_IZX, 6, "LAX" },
    { LDY, ADDR_ZP0, 3, "LDY" },
    { LDA, ADDR_ZP0, 3, "LDA" },
    { LDX, ADDR_ZP0, 3, "LDX" },
    { LAX, ADDR_ZP0, 3, "LAX" },
    { TAY, ADDR_IMP, 2, "TAY" },
    { LDA, ADDR_IMM, 2, "LDA" },
    { TAX, ADDR_IMP, 2, "TAX" },
    { LAX, ADDR_IMM, 2, "LAX" },
    { LDY, ADDR_ABS, 4, "LDY" },
    { LDA, ADDR_ABS, 4, "LDA" },
    { LDX, ADDR_ABS, 4, "LDX" },
    { LAX, ADDR_ABS, 4, "LAX" },

    { BCS, ADDR_REL, 2, "BCS" },
    { LDA, ADDR_IZY, 5, "LDA" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { LAX, ADDR_IZY, 5, "LAX" },
    { LDY, ADDR_ZPX, 4, "LDY" },
    { LDA, ADDR_ZPX, 4, "LDA" },
    { LDX, ADDR_ZPY, 4, "LDX" },
    { LAX, ADDR_ZPY, 4, "LAX" },
    { CLV, ADDR_IMP, 2, "CLV" },
    { LDA, ADDR_ABY, 4, "LDA" },
    { TSX, ADDR_IMP, 2, "TSX" },
    { LAS, ADDR_ABY, 4, "LAS" },
    { LDY, ADDR_ABX, 4, "LDY" },
    { LDA, ADDR_ABX, 4, "LDA" },
    { LDX, ADDR_ABY, 4, "LDX" },
    { LAX, ADDR_ABY, 4, "LAX" },

    { CPY, ADDR_IMM, 2, "CPY" },
    { CMP, ADDR_IZX, 6, "CMP" },
    { NOP, ADDR_IMM, 2, "NOP" },
    { DCP, ADDR_IZX, 8, "DCP" },
    { CPY, ADDR_ZP0, 3, "CPY" },
    { CMP, ADDR_ZP0, 3, "CMP" },
    { DEC, ADDR_ZP0, 5, "DEC" },
    { DCP, ADDR_ZP0, 5, "DCP" },
    { INY, ADDR_IMP, 2, "INY" },
    { CMP, ADDR_IMM, 2, "CMP" },
    { DEX, ADDR_IMP, 2, "DEX" },
    { AXS, ADDR_IMM, 2, "AXS" },
    { CPY, ADDR_ABS, 4, "CPY" },
    { CMP, ADDR_ABS, 4, "CMP" },
    { DEC, ADDR_ABS, 6, "DEC" },
    { DCP, ADDR_ABS, 6, "DCP" },

    { BNE, ADDR_REL, 2, "BNE" },
    { CMP, ADDR_IZY, 5, "CMP" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { DCP, ADDR_IZY, 8, "DCP" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { CMP, ADDR_ZPX, 4, "CMP" },
    { DEC, ADDR_ZPX, 6, "DEC" },
    { DCP, ADDR_ZPX, 6, "DCP" },
    { CLD, ADDR_IMP, 2, "CLD" },
    { CMP, ADDR_ABY, 4, "CMP" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { DCP, ADDR_ABY, 7, "DCP" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { CMP, ADDR_ABX, 4, "CMP" },
    { DEC, ADDR_ABX, 7, "DEC" },
    { DCP, ADDR_ABX, 7, "DCP" },

    { CPX, ADDR_IMM, 2, "CPX" },
    { SBC, ADDR_IZX, 6, "SBC" },
    { NOP, ADDR_IMM, 2, "NOP" },
    { ISC, ADDR_IZX, 8, "ISC" },
    { CPX, ADDR_ZP0, 3, "CPX" },
    { SBC, ADDR_ZP0, 3, "SBC" },
    { INC, ADDR_ZP0, 5, "INC" },
    { ISC, ADDR_ZP0, 5, "ISC" },
    { INX, ADDR_IMP, 2, "INX" },
    { SBC, ADDR_IMM, 2, "SBC" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { SBC, ADDR_IMM, 2, "SBC" },
    { CPX, ADDR_ABS, 4, "CPX" },
    { SBC, ADDR_ABS, 4, "SBC" },
    { INC, ADDR_ABS, 6, "INC" },
    { ISC, ADDR_ABS, 6, "ISC" },

    { BEQ, ADDR_REL, 2, "BEQ" },
    { SBC, ADDR_IZY, 5, "SBC" },
    { KIL, ADDR_IMM, 1, "KIL" },
    { ISC, ADDR_IZY, 8, "ISC" },
    { NOP, ADDR_ZPX, 4, "NOP" },
    { SBC, ADDR_ZPX, 4, "SBC" },
    { INC, ADDR_ZPX, 6, "INC" },
    { ISC, ADDR_ZPX, 6, "ISC" },
    { SED, ADDR_IMP, 2, "SED" },
    { SBC, ADDR_ABY, 4, "SBC" },
    { NOP, ADDR_IMP, 2, "NOP" },
    { ISC, ADDR_ABY, 7, "ISC" },
    { NOP, ADDR_ABX, 4, "NOP" },
    { SBC, ADDR_ABX, 4, "SBC" },
    { INC, ADDR_ABX, 7, "INC" },
    { ISC, ADDR_ABX, 7, "ISC" },
};

const size_t TABLE_SIZE = (sizeof(TABLE) / sizeof(TABLE[0]));

uint8_t InstructionTable_GetInstructionCount(void)
{
  return (uint8_t)TABLE_SIZE;
}

const InstructionTableEntry_t* InstructionTable_GetInstruction(uint8_t instruction)
{
  return &TABLE[instruction];
}