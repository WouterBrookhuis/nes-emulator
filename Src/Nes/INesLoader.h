/*
 * INesLoader.h
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_INESLOADER_H_
#define SRC_NES_INESLOADER_H_

#include <stdint.h>
#include <stdbool.h>

#include "Mapper.h"

#define SIZE_16KB   16384
#define SIZE_8KB    8192

#pragma pack(push, 1)
typedef struct
{
  uint8_t Magic[4];
  uint8_t PrgRomSize;
  uint8_t ChrRomSize;
  uint8_t Flags6;
  uint8_t Flags7;
  uint8_t Flags8;
  uint8_t Flags9;
  uint8_t Flags10;
  uint8_t Padding[5];
} INesHeader_t;
#pragma pack(pop)

bool INesLoader_Load(const char* file, Mapper_t *mapper);


#endif /* SRC_NES_INESLOADER_H_ */
