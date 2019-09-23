/*
 * AddressingMode.c
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#include "AddressingMode.h"

const char* AddressingMode_GetName(AddressingMode_t mode)
{
  switch(mode)
  {
  case ADDR_ABS:
    return "ABS";
  case ADDR_ABX:
    return "ABX";
  case ADDR_ABY:
    return "ABY";
  case ADDR_IMM:
    return "IMM";
  case ADDR_IMP:
    return "IMP";
  case ADDR_IND:
    return "IND";
  case ADDR_IZX:
    return "IZX";
  case ADDR_IZY:
    return "IZY";
  case ADDR_REL:
    return "REL";
  case ADDR_ZP0:
    return "ZP ";
  case ADDR_ZPX:
    return "ZPX";
  case ADDR_ZPY:
    return "ZPY";
  }
  return "UNK";
}

uint8_t AddressingMode_GetInstructionLength(AddressingMode_t mode)
{
  switch(mode)
  {
  case ADDR_ABS:
    return 3;
  case ADDR_ABX:
    return 3;
  case ADDR_ABY:
    return 3;
  case ADDR_IMM:
    return 2;
  case ADDR_IMP:
    return 1;
  case ADDR_IND:
    return 3;
  case ADDR_IZX:
    return 2;
  case ADDR_IZY:
    return 2;
  case ADDR_REL:
    return 2;
  case ADDR_ZP0:
    return 2;
  case ADDR_ZPX:
    return 2;
  case ADDR_ZPY:
    return 2;
  }
  return 0;
}
