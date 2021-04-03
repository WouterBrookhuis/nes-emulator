/*
 * ClockedRegister.h
 *
 *  Created on: 27 Mar 2021
 *      Author: Wouter
 */

#ifndef SRC_NES_CLOCKEDREGISTER_H_
#define SRC_NES_CLOCKEDREGISTER_H_

#include "Types.h"

typedef struct
{
  bool currentValue;
  bool newValue;
} cr1_t;

typedef struct
{
  u8_t currentValue;
  u8_t newValue;
} cr8_t;

static inline void CR1_Clock(cr1_t *reg)
{
  reg->currentValue = reg->newValue;
}

static inline bool CR1_Read(cr1_t reg)
{
  return reg.currentValue;
}

static inline void CR1_Write(cr1_t *reg, bool value)
{
  reg->newValue = value;
}

static inline void CR8_Clock(cr8_t *reg)
{
  reg->currentValue = reg->newValue;
}

static inline u8_t CR8_Read(cr8_t reg)
{
  return reg.currentValue;
}

static inline bool CR8_IsBitSet(cr8_t reg, u8_t mask)
{
  return reg.currentValue & mask;
}

static inline void CR8_Write(cr8_t *reg, u8_t value)
{
  reg->newValue = value;
}

static inline void CR8_SetBits(cr8_t *reg, u8_t value)
{
  reg->newValue |= value;
}

static inline void CR8_ClearBits(cr8_t *reg, u8_t value)
{
  reg->newValue &= ~value;
}

static inline void CR8_Reset(cr8_t *reg)
{
  reg->newValue = 0;
  reg->currentValue = 0;
}

static inline void CR8_WriteImmediate(cr8_t *reg, u8_t value)
{
  CR8_Write(reg, value);
  CR8_Clock(reg);
}

#endif /* SRC_NES_CLOCKEDREGISTER_H_ */
