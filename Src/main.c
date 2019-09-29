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
#include "Nes/Palette.h"

static void Initialize(void);

static bool Update(float deltaTime);

static void Draw(SDL_Surface* surface);

static void Event(SDL_Event* event);

#define HALF_MEM_WINDOW_SIZE  7
#define MEM2_WINDOW_SIZE      16


#define NES_SCREEN_WIDTH    256         // Width of the NES screen output
#define NES_SCREEN_HEIGHT   240         // Height of the NES screen output
#define NES_SCREEN_SCALE    2           // Scaling done to NES screen output before displaying

#define FONT_SIZE           16          // Size of the font in pixels
#define STATUS_BAR_ROWS     2
#define STATUS_BAR_HEIGHT   (FONT_SIZE * STATUS_BAR_ROWS) // Height of the top of screen status bar

#define DEBUG_VIEW_CHARS    32          // Number of chars that fit in the debug view
#define DEBUG_VIEW_WIDTH    (FONT_SIZE * DEBUG_VIEW_CHARS)  // Width of debug view next to NES screen

#define WINDOW_WIDTH        (NES_SCREEN_WIDTH * NES_SCREEN_SCALE + DEBUG_VIEW_WIDTH)
#define WINDOW_HEIGHT       (NES_SCREEN_HEIGHT * NES_SCREEN_SCALE + STATUS_BAR_HEIGHT)

#define STATUS_BAR_CHARS_PER_ROW    (WINDOW_WIDTH / FONT_SIZE)  // Number of characters in the status bar

#define MEMORY_VIEW_COLUMNS 8
#define MEMORY_VIEW_ROWS    8
#define MEMORY_VIEW_CHARS_PER_ROW   ((MEMORY_VIEW_COLUMNS - 1) * 3 + 2 + 6)

static Font_t _font;
static char _statusBarBuffer[STATUS_BAR_CHARS_PER_ROW * STATUS_BAR_ROWS + 1];
static char _logBuffer[2048];
static char _textBuffer[128];
static char _memTextBuffer[HALF_MEM_WINDOW_SIZE * 2 + 1][128];
static char _memoryViewBuffer[MEMORY_VIEW_CHARS_PER_ROW * MEMORY_VIEW_ROWS + 1];
static Mapper_t _mapper;
static bool _stepKeyWasPressed;
static bool _runKeyWasPressed;
static bool _statusKeyWasPressed;
static bool _run;
static bool _showCpu;
static char _lastLoadedFileName[512];
static SDL_Surface *_ppuRenderSurface;


int main(int argc, const char* argv[])
{
  int sdlReturnCode;

  SharedSDL_Initialize(WINDOW_WIDTH, WINDOW_HEIGHT, "My Nes Emulator Thingy", Initialize, Update, Draw, Event);
  sdlReturnCode = SharedSDL_Start();

  return sdlReturnCode;
}

static void Initialize()
{
  //const char * romFile = "Resources/all_instrs.nes";
  //const char * romFile = "Resources/official_only.nes";
  const char * romFile = "Resources/nestest.nes";
  //const char * romFile = "Resources/donkey kong.nes";
  //const char * romFile = "Resources/super mario bros.nes";
  //const char * romFile = "Resources/palette_ram.nes";
  //const char * romFile = "Resources/rom_singles/01-basics.nes";
  const char * paletteFile = "Resources/ntscpalette.pal";
  Bus_t *bus;
  CPU_t *cpu;
  NES_Initialize();
  if (INesLoader_Load(romFile, &_mapper))
  {
    strncpy(_lastLoadedFileName, romFile, sizeof(_lastLoadedFileName));
    bus = NES_GetBus();
    Bus_SetMapper(bus, &_mapper);
  }
  else
  {
    LogError("Unable to load NES ROM, do not run system!");
  }

  Palette_LoadFrom(paletteFile);

  _ppuRenderSurface = SDL_CreateRGBSurfaceWithFormat(0, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
  PPU_SetRenderSurface(_ppuRenderSurface);

  // Run first instruction
  cpu = NES_GetCPU();
  CPU_Reset(cpu);
  //cpu->PC = 0xC000; // nestest.nes auto mode
  NES_TickClock();
  NES_TickUntilCPUComplete();

  Text_LoadFont(&_font, "Resources/monofont.bmp", FONT_SIZE, FONT_SIZE);
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
    else if (event->key.keysym.sym == SDLK_s)
    {
      _statusKeyWasPressed = true;
    }
  }
}

static void FormatInstruction(CPU_t *cpu, char* textBuffer)
{
  uint8_t instructionBytes[3];
  const InstructionTableEntry_t *instr;
  instructionBytes[0] = Bus_ReadFromCPU(cpu->Bus, cpu->PC);
  instr = InstructionTable_GetInstruction(instructionBytes[0]);
  uint8_t instructionLength = AddressingMode_GetInstructionLength(instr->AddressingMode);

  for (uint8_t i = 1; i < instructionLength; i++)
  {
    instructionBytes[i] = Bus_ReadFromCPU(cpu->Bus, cpu->PC + i);
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
  PPU_t *ppu;
  const InstructionTableEntry_t *instr;
  cpu = NES_GetCPU();
  bus = NES_GetBus();
  ppu = NES_GetPPU();

  instr = InstructionTable_GetInstruction(cpu->Instruction);

  // First row: Meta info
  const char* firstRowTemplate = "Map: %02X File: %-*s";
  snprintf(_statusBarBuffer,
           STATUS_BAR_CHARS_PER_ROW + 1,
           firstRowTemplate,
           _mapper.MapperId,
           STATUS_BAR_CHARS_PER_ROW,
           _lastLoadedFileName
           );

  if (_statusKeyWasPressed)
  {
    _statusKeyWasPressed = false;
    _showCpu = !_showCpu;
  }

  // Second row: CPU status
  if (_showCpu)
  {
    snprintf(&_statusBarBuffer[STATUS_BAR_CHARS_PER_ROW],
            STATUS_BAR_CHARS_PER_ROW + 1,
            "%04X: %3s %3s A:%02X X:%02X Y:%02X S:%02X P:%02X, C:%010d",
            cpu->InstructionPC,
            instr->Name,
            AddressingMode_GetName(instr->AddressingMode),
            cpu->A,
            cpu->X,
            cpu->Y,
            cpu->S,
            cpu->P,
            cpu->CycleCount
            );
  }
  else
  {
    // Second row: PPU status
    snprintf(&_statusBarBuffer[STATUS_BAR_CHARS_PER_ROW],
            STATUS_BAR_CHARS_PER_ROW + 1,
            "H:%03d V:%03d CTRL:%02X STAT:%02X",
            ppu->HCount,
            ppu->VCount,
            ppu->Ctrl,
            ppu->Status
            );
  }


  uint16_t memAddress = cpu->InstructionPC - HALF_MEM_WINDOW_SIZE;
  for (int i = 0; i < HALF_MEM_WINDOW_SIZE * 2 + 1; i++)
  {
    uint8_t memData = Bus_ReadFromCPU(bus, memAddress);
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
  memset(_memoryViewBuffer, 0, sizeof(_memoryViewBuffer));
  for (int i = 0; i < MEMORY_VIEW_ROWS; i++)
  {
    snprintf(_memoryViewBuffer + strlen(_memoryViewBuffer),
             sizeof(_memoryViewBuffer) - strlen(_memoryViewBuffer),
             "%04X:",
             memAddress
             );
    for (int j = 0; j < MEMORY_VIEW_COLUMNS; j++)
    {
      uint8_t memData = Bus_ReadFromCPU(bus, memAddress);
      snprintf(_memoryViewBuffer + strlen(_memoryViewBuffer),
               sizeof(_memoryViewBuffer) - strlen(_memoryViewBuffer),
               " %02X",
               memData
               );
      memAddress++;
    }
  }

  if (_runKeyWasPressed)
  {
    _run = !_run;
    _runKeyWasPressed = false;
  }

  if (_stepKeyWasPressed)
  {
//    if (cpu->CyclesLeftForInstruction == 0)
//    {
//      NES_TickClock();
//    }
//    NES_TickUntilCPUComplete();
    NES_TickUntilFrameComplete();
    _stepKeyWasPressed = false;
  }

  if (_run)
  {
    // Realtime-ish speed
    for (int i = 0; i < 1; i++)
    {
      FormatInstruction(cpu, _textBuffer);
      //NES_TickClock();
      //NES_TickUntilCPUComplete();
      NES_TickUntilFrameComplete();
      if (cpu->IsKilled)
      {
        _run = false;
        break;
      }
    }
  }

  if (Bus_ReadFromCPU(bus, 0x6001) == 0xDE &&
      Bus_ReadFromCPU(bus, 0x6002) == 0xB0 &&
      Bus_ReadFromCPU(bus, 0x6003) == 0x61)
  {
    // Cpu test text is available to read
    memset(_logBuffer, 0, sizeof(_logBuffer));
    memAddress = 0x6004;
    uint8_t data;
    uint16_t offset = 0;
    while ((data = Bus_ReadFromCPU(bus, memAddress + offset)) != 0x00 && offset < sizeof(_logBuffer))
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
  SDL_Rect nesInternalRect;
  SDL_Rect nesScreenRect;
  uint32_t color;

  // Nes screen output
  nesInternalRect.w = _ppuRenderSurface->w;
  nesInternalRect.h = _ppuRenderSurface->h;
  nesInternalRect.x = 0;
  nesInternalRect.y = 0;
  nesScreenRect.w = NES_SCREEN_WIDTH * NES_SCREEN_SCALE;
  nesScreenRect.h = NES_SCREEN_HEIGHT * NES_SCREEN_SCALE;
  nesScreenRect.x = 0;
  nesScreenRect.y = STATUS_BAR_HEIGHT;
  //color = SDL_MapRGB(surface->format, 0xFF, 0x00, 0x00);
  //SDL_FillRect(surface, &nesScreenRect, color);
  SDL_BlitScaled(_ppuRenderSurface, &nesInternalRect, surface, &nesScreenRect);

  // Status bar
  Text_DrawStringWrapping(surface, _statusBarBuffer, 0, 0, STATUS_BAR_CHARS_PER_ROW, &_font);

  // Debug view
  // Memory view
  Text_DrawString(surface, "Memory view", nesScreenRect.w, STATUS_BAR_HEIGHT, &_font);
  Text_DrawStringWrapping(surface,
                           _memoryViewBuffer,
                           nesScreenRect.w,
                           STATUS_BAR_HEIGHT + _font.GlyphHeight,
                           MEMORY_VIEW_CHARS_PER_ROW,
                           &_font);
  // PC memory area
  Text_DrawString(surface, "Disassembler (WIP)", nesScreenRect.w, STATUS_BAR_HEIGHT + (2 + MEMORY_VIEW_ROWS) * _font.GlyphHeight, &_font);
  for (int i = 0; i < HALF_MEM_WINDOW_SIZE * 2 + 1; i++)
  {
    Text_DrawString(surface,
                    _memTextBuffer[i],
                    nesScreenRect.w,
                    STATUS_BAR_HEIGHT + _font.GlyphHeight * (i + 3 + MEMORY_VIEW_ROWS),
                    &_font);
  }




  Text_DrawStringWrapping(surface, _logBuffer, 0, 288, surface->w / _font.GlyphWidth, &_font);

  // Debug: state
  Text_DrawString(surface, _run ? "Running" : "Stopped", 0, surface->h - _font.GlyphHeight, &_font);




}


