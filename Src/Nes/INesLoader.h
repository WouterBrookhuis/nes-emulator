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

#define INES_FLAGS6_MIRROR_VERTICAL     (0x01)
#define INES_FLAGS6_BATTERY_RAM         (0x02)
#define INES_FLAGS6_TRAINER             (0x04)
#define INES_FLAGS6_FOUR_SCREEN_VRAM    (0x08)
#define INES_FLAGS6_MAPPER_MASK         (0xF0)
#define INES_FLAGS6_MAPPER_SHIFT        (4)

#define INES_FLAGS7_MAPPER_MASK         (0xF0)

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
