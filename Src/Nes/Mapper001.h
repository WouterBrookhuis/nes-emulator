/*
 * Mapper001.h
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_MAPPER001_H_
#define SRC_NES_MAPPER001_H_
#include "Types.h"
#include "INesLoader.h"

typedef struct
{
  u8_t ShiftRegister;    // 4 bit shift register
  u8_t ControlRegister;
  u8_t Char0Register;
  u8_t Char1Register;
  u8_t ProgramRegister;
  u8_t *PrgRam8k;        // Pointer to 8k worth of program RAM
} Mapper001Data_t;

void Mapper001_Initialize(Mapper_t *mapper, INesHeader_t *header);

#endif /* SRC_NES_MAPPER001_H_ */
