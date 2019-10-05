/*
 * Controllers.h
 *
 *  Created on: Oct 5, 2019
 *      Author: wouter
 */

#ifndef SRC_NES_CONTROLLERS_H_
#define SRC_NES_CONTROLLERS_H_

#include <stdint.h>
#include <stdbool.h>

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

typedef bool (*IsButtonPressed_t)(uint8_t controller, NESButton_t button);

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
  uint8_t Data;
} Controller_t;

void Controllers_Initialize(uint8_t numberOfControllers);

void Controllers_SetButtonHandler(uint8_t controllerIndex, IsButtonPressed_t handler);

void Controllers_Write(uint8_t controllerIndex, uint8_t data);

uint8_t Controllers_ReadAndShiftState(uint8_t controllerIndex);

#endif /* SRC_NES_CONTROLLERS_H_ */
