/*
 * Mapper000.h
 *
 *  Created on: Sep 22, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_MAPPER000_H_
#define SRC_NES_MAPPER000_H_
#include "Types.h"
#include "INesLoader.h"

typedef struct
{
  u8_t *PrgRam8k;        // Pointer to 8k worth of program RAM
} Mapper000Data_t;

void Mapper000_Initialize(Mapper_t *mapper, INesHeader_t *header);

#endif /* SRC_NES_MAPPER000_H_ */
