/*
 * main.c
 *
 *  Created on: Sep 19, 2019
 *      Author: wouter
 */

#include <SDL2/SDL.h>
#include "SharedSDL.h"
#include "Text.h"
#include <stdint.h>
#include <stdbool.h>
#include "log.h"
#include "Nes/NES.h"
#include "Nes/INesLoader.h"
#include "Nes/Mapper.h"
#include "Nes/InstructionTable.h"
#include "Nes/Bus.h"

static void Initialize(void);

static bool Update(float deltaTime);

static void Draw(SDL_Surface* surface);

static void Event(SDL_Event* event);

#define HALF_MEM_WINDOW_SIZE  7
#define MEM2_WINDOW_SIZE      16

static Font_t _font;
static char _logBuffer[2048];
static char _textBuffer[128];
static char _memTextBuffer[HALF_MEM_WINDOW_SIZE * 2 + 1][128];
static char _memTextBuffer2[MEM2_WINDOW_SIZE][128];
static Mapper_t _mapper;
static bool _stepKeyWasPressed;
static bool _runKeyWasPressed;
static bool _run;

int main(int argc, const char* argv[])
{
  int sdlReturnCode;

  SharedSDL_Initialize(640, 480, "My Nes Emulator Thingy", Initialize, Update, Draw, Event);
  sdlReturnCode = SharedSDL_Start();

  return sdlReturnCode;
}

static void Initialize()
{
  //const char * romFile = "Resources/all_instrs.nes";
  const char * romFile = "Resources/official_only.nes";
  //const char * romFile = "Resources/nestest.nes";
  //const char * romFile = "Resources/rom_singles/01-basics.nes";
  Bus_t *bus;
  CPU_t *cpu;
  NES_Initialize();
  if (INesLoader_Load(romFile, &_mapper))
  {
    bus = NES_GetBus();
    Bus_SetMapper(bus, &_mapper);
  }

  // Run first instruction
  cpu = NES_GetCPU();
  CPU_Reset(cpu);
  //cpu->PC = 0xC000; // nestest.nes auto mode
  NES_TickClock();
  NES_TickUntilCPUComplete();

  Text_LoadFont(&_font, "Resources/monofont.bmp", 16, 16);
}

static void Event(SDL_Event* event)
{
  if (event->type == SDL_KEYDOWN)
  {
    if (event->key.keysym.sym == SDLK_SPACE)
    {
      _stepKeyWasPressed = true;
    }
    else if (event->key.keysym.sym == SDLK_r)
    {
      _runKeyWasPressed = true;
    }
  }
}

static void FormatInstruction(CPU_t *cpu, char* textBuffer)
{
  uint8_t instructionBytes[3];
  const InstructionTableEntry_t *instr;
  instructionBytes[0] = Bus_Read(cpu->Bus, cpu->PC);
  instr = InstructionTable_GetInstruction(instructionBytes[0]);
  uint8_t instructionLength = AddressingMode_GetInstructionLength(instr->AddressingMode);

  for (uint8_t i = 1; i < instructionLength; i++)
  {
    instructionBytes[i] = Bus_Read(cpu->Bus, cpu->PC + i);
  }

  sprintf(textBuffer, "%04X  %02X ", cpu->PC, instructionBytes[0]);
  if (instructionLength > 1)
  {
    sprintf(textBuffer + strlen(textBuffer), "%02X ", instructionBytes[1]);
    if (instructionLength > 2)
    {
      sprintf(textBuffer + strlen(textBuffer), "%02X  ", instructionBytes[2]);
    }
    else
    {
      sprintf(textBuffer + strlen(textBuffer), "    ");
    }
  }
  else
  {
    sprintf(textBuffer + strlen(textBuffer), "       ");
  }

  sprintf(textBuffer + strlen(textBuffer), "%3s ", instr->Name);

  sprintf(textBuffer + strlen(textBuffer),
          "    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%u",
          cpu->A,
          cpu->X,
          cpu->Y,
          cpu->P,
          cpu->S,
          cpu->CycleCount
          );
  //LogMessage(textBuffer);
}

static bool Update(float deltaTime)
{
  CPU_t *cpu;
  Bus_t *bus;
  const InstructionTableEntry_t *instr;
  cpu = NES_GetCPU();
  bus = NES_GetBus();

  instr = InstructionTable_GetInstruction(cpu->Instruction);

  sprintf(_textBuffer,
          "%04X: %3s %3s A:%02X X:%02X Y:%02X S:%02X C:%04d",
          cpu->InstructionPC,
          instr->Name,
          AddressingMode_GetName(instr->AddressingMode),
          cpu->A,
          cpu->X,
          cpu->Y,
          cpu->S,
          cpu->CycleCount / 1000
          );
  uint16_t memAddress = cpu->InstructionPC - HALF_MEM_WINDOW_SIZE;
  for (int i = 0; i < HALF_MEM_WINDOW_SIZE * 2 + 1; i++)
  {
    uint8_t memData = Bus_Read(bus, memAddress);
    if (i == HALF_MEM_WINDOW_SIZE)
    {
      sprintf(_memTextBuffer[i],
              "%04X: %02X <",
              memAddress,
              memData
              );
    }
    else
    {
      sprintf(_memTextBuffer[i],
              "%04X: %02X",
              memAddress,
              memData
              );
    }

    memAddress++;
  }

  memAddress = 0x6000;
  for (int i = 0; i < MEM2_WINDOW_SIZE; i++)
  {
    uint8_t memData = Bus_Read(bus, memAddress);
    if (i == 0)
    {
      sprintf(_memTextBuffer2[i],
              "%04X: %02X <",
              memAddress,
              memData
              );
    }
    else
    {
      sprintf(_memTextBuffer2[i],
              "%04X: %02X",
              memAddress,
              memData
              );
    }

    memAddress++;
  }

  if (_runKeyWasPressed)
  {
    _run = !_run;
    _runKeyWasPressed = false;
  }

  if (_stepKeyWasPressed)
  {
    if (cpu->CyclesLeftForInstruction == 0)
    {
      NES_TickClock();
    }
    NES_TickUntilCPUComplete();
    _stepKeyWasPressed = false;
  }

  if (_run)
  {
    // Realtime-ish speed
    for (int i = 0; i < 29830; i++)
    {
      FormatInstruction(cpu, _textBuffer);
      NES_TickClock();
      NES_TickUntilCPUComplete();
      if (cpu->IsKilled)
      {
        _run = false;
        break;
      }
    }
  }

  if (Bus_Read(bus, 0x6001) == 0xDE &&
      Bus_Read(bus, 0x6002) == 0xB0 &&
      Bus_Read(bus, 0x6003) == 0x61)
  {
    // Cpu test text is available to read
    memset(_logBuffer, 0, sizeof(_logBuffer));
    memAddress = 0x6004;
    uint8_t data;
    uint16_t offset = 0;
    while ((data = Bus_Read(bus, memAddress + offset)) != 0x00 && offset < sizeof(_logBuffer))
    {
      _logBuffer[offset] = (char)data;
      offset++;
    }
    _logBuffer[sizeof(_logBuffer) - 1] = 0x00;
  }

  return true;
}

static void Draw(SDL_Surface* surface)
{
  SDL_Rect rect;
  uint32_t color;

  rect.w = 100;
  rect.h = 50;
  rect.x = 40;
  rect.y = 40;
  color = SDL_MapRGB(surface->format, 0xFF, 0x00, 0x00);

  SDL_FillRect(surface, &rect, color);

  Text_DrawString(surface, _textBuffer, 0, 0, &_font);
  Text_DrawString(surface, "Memory view", 0, 16, &_font);
  for (int i = 0; i < HALF_MEM_WINDOW_SIZE * 2 + 1; i++)
  {
    Text_DrawString(surface, _memTextBuffer[i], 0, 32 + _font.GlyphHeight * i, &_font);
  }

  for (int i = 0; i < MEM2_WINDOW_SIZE; i++)
  {
    Text_DrawString(surface, _memTextBuffer2[i], 300, 32 + _font.GlyphHeight * i, &_font);
  }

  Text_DrawStringWrapping(surface, _logBuffer, 0, 288, surface->w / _font.GlyphWidth, &_font);

  Text_DrawString(surface, _run ? "Running" : "Stopped", 0, surface->h - _font.GlyphHeight, &_font);
}


