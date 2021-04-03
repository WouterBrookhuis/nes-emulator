/*
 * Mapper.h
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_MAPPER_H_
#define SRC_NES_MAPPER_H_

#include "Types.h"

typedef enum _MirrorMode_t
{
  MIRROR_MODE_HORIZONTAL,
  MIRROR_MODE_VERTICAL,
  MIRROR_MODE_SINGLE,
  MIRROR_MODE_FOUR,
} MirrorMode_t;

typedef struct _Bus_t Bus_t;
typedef struct _Mapper_t Mapper_t;

typedef bool (*Mapper_Read)(Mapper_t *mapper, u16_t address, u8_t *data);
typedef bool (*Mapper_Write)(Mapper_t *mapper, u16_t address, u8_t data);

typedef struct _Mapper_t
{
  u8_t MapperId;     // iNES mapper ID
  MirrorMode_t Mirror;  // Mirroring mode
  Bus_t *Bus;           // The bus we are connected to
  u8_t NumPrgBanks;
  u8_t NumChrBanks;
  u8_t *Memory;      // This mapper's backing memory, used internally
  size_t MemorySize;    // The size of the mapper's memory, used internally
  size_t ChrOffset;     // Offset of CHR rom/ram in Memory
  Mapper_Read ReadFromCpu;   // The mapper read function
  Mapper_Write WriteFromCpu; // The mapper write function
  Mapper_Read ReadFromPpu;   // The mapper read function
  Mapper_Write WriteFromPpu; // The mapper write function
  void *CustomData;     // Pointer to custom data for the mapper implementation
} Mapper_t;

#endif /* SRC_NES_MAPPER_H_ */
