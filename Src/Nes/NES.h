/*
 * NES.h
 *
 *  Created on: Sep 20, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_NES_H_
#define SRC_NES_NES_H_

#include "CPU.h"
#include "Bus.h"
#include "PPU.h"

void NES_Initialize(void);
void NES_TickClock(void);
void NES_TickUntilCPUComplete(void);

PPU_t *NES_GetPPU(void);
CPU_t *NES_GetCPU(void);
Bus_t *NES_GetBus(void);


#endif /* SRC_NES_NES_H_ */
