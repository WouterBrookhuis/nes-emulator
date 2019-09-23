/*
 * Mapper.h
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_MAPPER_H_
#define SRC_NES_MAPPER_H_

#include <stddef.h>

typedef struct _Bus_t Bus_t;
typedef struct _Mapper_t Mapper_t;

typedef uint8_t (*Mapper_Read)(Mapper_t *mapper, uint16_t address);
typedef void (*Mapper_Write)(Mapper_t *mapper, uint16_t address, uint8_t data);

typedef struct _Mapper_t
{
  uint8_t MapperId;     // iNES mapper ID
  Bus_t *Bus;           // The bus we are connected to
  uint8_t NumPrgBanks;
  uint8_t NumChrBanks;
  uint8_t *Memory;      // This mapper's backing memory, used internally
  size_t MemorySize;    // The size of the mapper's memory, used internally
  Mapper_Read ReadFn;   // The mapper read function
  Mapper_Write WriteFn; // The mapper write function
  void *CustomData;     // Pointer to custom data for the mapper implementation
} Mapper_t;

#endif /* SRC_NES_MAPPER_H_ */
