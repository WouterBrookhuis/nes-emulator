/*
 * Controllers.h
 *
 *  Created on: Oct 5, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_CONTROLLERS_H_
#define SRC_NES_CONTROLLERS_H_

#include "Types.h"

typedef enum _NESButton_t
{
  NES_BUTTON_A,
  NES_BUTTON_B,
  NES_BUTTON_SELECT,
  NES_BUTTON_START,
  NES_BUTTON_UP,
  NES_BUTTON_DOWN,
  NES_BUTTON_LEFT,
  NES_BUTTON_RIGHT,
  NR_OF_NES_BUTTONS,
} NESButton_t;

typedef bool (*IsButtonPressed_t)(u8_t controller, NESButton_t button);

typedef struct _Controller_t
{
  IsButtonPressed_t ButtonHandler;
  bool IsReadingButtons;
  // Button order from 0 - 7
  // A
  // B
  // Select
  // Start
  // Up
  // Down
  // Left
  // Right
  u8_t Data;
} Controller_t;

void Controllers_Initialize(u8_t numberOfControllers);

void Controllers_SetButtonHandler(u8_t controllerIndex, IsButtonPressed_t handler);

void Controllers_Write(u8_t controllerIndex, u8_t data);

u8_t Controllers_ReadAndShiftState(u8_t controllerIndex);

#endif /* SRC_NES_CONTROLLERS_H_ */
