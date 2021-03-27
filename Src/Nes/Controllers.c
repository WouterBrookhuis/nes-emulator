/*
 * Controllers.c
 *
 *  Created on: Oct 5, 2019
 *      Author: wouter
 */

#include "Controllers.h"
#include "log.h"

#include <stdbool.h>
#include <stddef.h>

#define MAX_NUM_CONTROLLERS     4

static Controller_t _controllers[MAX_NUM_CONTROLLERS];
static int _numControllers;

void Controllers_Initialize(uint8_t numberOfControllers)
{
  if (numberOfControllers > MAX_NUM_CONTROLLERS)
  {
    return;
  }

  for (int i = 0; i < numberOfControllers; i++)
  {
    _controllers[i].ButtonHandler = NULL;
    _controllers[i].IsReadingButtons = false;
    _controllers[i].Data = 0x00;
  }
  _numControllers = numberOfControllers;
}

void Controllers_Write(uint8_t controllerIndex, uint8_t data)
{
  controllerIndex &= MAX_NUM_CONTROLLERS;

  if ((data & 0x01) == 1)
  {
    _controllers[controllerIndex].IsReadingButtons = true;
  }
  else if ((data & 0x01) == 0 && _controllers[controllerIndex].IsReadingButtons)
  {
    if (_controllers[controllerIndex].ButtonHandler != NULL)
    {
      _controllers[controllerIndex].Data = 0;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_A)) << 0;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_B)) << 1;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_SELECT)) << 2;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_START)) << 3;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_UP)) << 4;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_DOWN)) << 5;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_LEFT)) << 6;
      _controllers[controllerIndex].Data |= (_controllers[controllerIndex].ButtonHandler(controllerIndex, NES_BUTTON_RIGHT)) << 7;
    }
    else
    {
      _controllers[controllerIndex].Data = 0x00;
    }
    _controllers[controllerIndex].IsReadingButtons = false;
  }
}

uint8_t Controllers_ReadAndShiftState(uint8_t controllerIndex)
{
  uint8_t result;

  controllerIndex &= MAX_NUM_CONTROLLERS;

  result = _controllers[controllerIndex].Data & 0x01;
  _controllers[controllerIndex].Data >>= 1;
  // TODO: Why is this commented out?
  // NES controllers will return 1 after reading all bits, so set the MSB
  //_controllers[controllerIndex].Data |= 0x80;

  return result;
}

void Controllers_SetButtonHandler(uint8_t controllerIndex, IsButtonPressed_t handler)
{
  if (controllerIndex >= _numControllers)
  {
    LogError("Invalid controller index %d", controllerIndex);
    return;
  }

  _controllers[controllerIndex].ButtonHandler = handler;
}
