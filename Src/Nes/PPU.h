/*
 * PPU.h
 *
 *  Created on: Sep 25, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_PPU_H_
#define SRC_NES_PPU_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct _Bus_t Bus_t;

typedef struct _PPU_t
{
  unsigned int CycleCount;          // Total cycle count, debugging info
  unsigned int CyclesSinceReset;    // Amount of cycles since last reset
  Bus_t *Bus;

  // Registers
  uint8_t Ctrl;
  uint8_t Mask;
  uint8_t Status;
  uint8_t OAMAddress;
  uint8_t OAMData;
  uint8_t Scroll;
  //uint8_t Address;
  uint8_t Data;
  uint8_t OAMDma;

  // Wonky things
  uint8_t AddressLatch;
  uint8_t DataBuffer;
  uint8_t LatchedData;

  // Frame lines
  int VCount;
  int HCount;
  bool IsEvenFrame;

  // VRAM Address Registers
  uint16_t V;
  uint16_t T;
  uint8_t X;

} PPU_t;

void PPU_Initialize(PPU_t *ppu);
void PPU_Tick(PPU_t *ppu);
void PPU_Reset(PPU_t *ppu);
uint8_t PPU_Read(PPU_t *ppu, uint16_t address);
void PPU_Write(PPU_t *ppu, uint16_t address, uint8_t data);

#endif /* SRC_NES_PPU_H_ */
