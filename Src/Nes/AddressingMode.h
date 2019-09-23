/*
 * AddressingMode.h
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_ADDRESSINGMODE_H_
#define SRC_NES_ADDRESSINGMODE_H_

#include <stdint.h>

typedef enum _AddressingMode_t
{
  ADDR_IMP,     // Implied addressing, no extra bytes
  ADDR_IMM,     // Immediate addressing, operand follows (1 byte)
  ADDR_ZP0,     // Zero page addressing, 1 extra byte
  ADDR_ZPX,     // Zero page + X, 1 extra byte
  ADDR_ZPY,     // Zero page + Y, 1 extra byte
  ADDR_IZX,     // Indirect zero page X, 1 extra byte
  ADDR_IZY,     // Indirect zero page Y, 1 extra byte
  ADDR_ABS,     // Absolute addressing, 2 extra bytes
  ADDR_ABX,     // Absolute + X, 2 extra bytes
  ADDR_ABY,     // Absolute + Y, 2 extra bytes
  ADDR_IND,     // Indirect addressing, 2 extra bytes
  ADDR_REL,     // Relative addressing, 1 extra byte
} AddressingMode_t;

const char* AddressingMode_GetName(AddressingMode_t mode);

uint8_t AddressingMode_GetInstructionLength(AddressingMode_t mode);

#endif /* SRC_NES_ADDRESSINGMODE_H_ */
