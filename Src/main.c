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
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "log.h"
#include "Nes/NES.h"
#include "Nes/INesLoader.h"
#include "Nes/Mapper.h"
#include "Nes/InstructionTable.h"
#include "Nes/Bus.h"
#include "Nes/Palette.h"
#include "Nes/Controllers.h"
#include "Nes/APU.h"

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

#define OAM_VIEW_ENTRIES    22
#define OAM_VIEW_CHARS_PER_ROW  25

typedef enum
{
  DETAIL_MODE_CPU,
  DETAIL_MODE_PPU,
  DETAIL_MODE_APU,
  NR_OF_DETAIL_MODES,
} DetailMode_t;

static Font_t _font;
static char _statusBarBuffer[STATUS_BAR_CHARS_PER_ROW * STATUS_BAR_ROWS + 1];
static char _textBuffer[128];
static char _memTextBuffer[HALF_MEM_WINDOW_SIZE * 2 + 1][128];
static char _memoryViewBuffer[MEMORY_VIEW_CHARS_PER_ROW * MEMORY_VIEW_ROWS + 1];
static char _oamViewBuffer[OAM_VIEW_ENTRIES * OAM_VIEW_CHARS_PER_ROW + 1];
static Mapper_t _mapper;
static bool _stepKeyWasPressed;
static bool _frameStepKeyWasPressed;
static bool _runKeyWasPressed;
static bool _statusKeyWasPressed;
static bool _run;
static DetailMode_t _detailMode;;
static char _lastLoadedFileName[512];
static SDL_Surface *_ppuRenderSurface;
static u8_t _patternTableDrawIndex = 2;
static SDL_Surface *_ppuPatternTableSurfaces[2];

static bool _controller1Buttons[NR_OF_NES_BUTTONS];

int main(int argc, char* argv[])
{
  int sdlReturnCode;

  SharedSDL_Initialize(WINDOW_WIDTH, WINDOW_HEIGHT, "My Nes Emulator Thingy", Initialize, Update, Draw, Event);
  sdlReturnCode = SharedSDL_Start();

  return sdlReturnCode;
}

static bool HandleButton(u8_t controller, NESButton_t button)
{
  return _controller1Buttons[button];
}

static void DrawPalettes(Bus_t *bus, SDL_Surface *surface, int startX, int startY)
{
  // Draw all palette entries
  u8_t r;
  u8_t g;
  u8_t b;
  u16_t address = 0x3F00;
  SDL_Rect rect;
  SDL_Rect bgRect;

  rect.w = 12;
  rect.h = 12;
  rect.x = startX;
  rect.y = startY;

  for (int i = 0; i < 8; i++)
  {
    bgRect.w = rect.w * 4 + 4;
    bgRect.h = rect.h + 4;
    bgRect.x = rect.x - 2;
    bgRect.y = rect.y - 2;
    SDL_FillRect(surface, &bgRect, SDL_MapRGB(surface->format, 255, 255, 255));
    for (int c = 0; c < 4; c++)
    {
      u8_t paletteValue = Bus_ReadFromPPU(bus, address + c);
      Palette_GetRGB(paletteValue, &r, &g, &b);
      SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, r, g, b));
      rect.x += rect.w;
    }
    rect.x += 2;
    address += 4;
  }
}

static void DrawPatternTable(Bus_t *bus, u16_t tableStart, SDL_Surface *surface)
{
  u8_t tileDataLow;
  u8_t tileDataHigh;

  // Hack, hack, hack away
  PPU_SetRenderSurface(surface);

  // Pattern table is 16x16 tiles
  for (int ty = 0; ty < 16; ty++)
  {
    for (int tx = 0; tx < 16; tx++)
    {
      for (int py = 0; py < 8; py++)
      {
        tileDataLow = Bus_ReadFromPPU(bus,
                                      tableStart
                                      + (ty << 8)
                                      + (tx << 4)
                                      + py
                                      + 0);
        tileDataHigh = Bus_ReadFromPPU(bus,
                                       tableStart
                                       + (ty << 8)
                                       + (tx << 4)
                                       + py
                                       + 8);
        for (int px = 0; px < 8; px++)
        {
          u8_t pixel = ((tileDataLow & (0x80 >> px)) > 0)
                          | (((tileDataHigh & (0x80 >> px)) > 0) << 1);
          PPU_RenderPixel(bus->PPU, 8 * tx + px, 8 * ty + py, pixel, 0x00);
        }
      }
    }
  }

  PPU_SetRenderSurface(_ppuRenderSurface);
}

static void Initialize()
{
  //const char * romFile = "Resources/instr_test-v5/all_instrs.nes";
  //const char * romFile = "Resources/instr_test-v5/rom_singles/03-immediate.nes";
  //const char * romFile = "Resources/instr_test-v5/rom_singles/07-abs_xy.nes";
  //const char * romFile = "Resources/instr_timing/instr_timing.nes";
  //const char * romFile = "Resources/official_only.nes";
  //const char * romFile = "Resources/nestest.nes";
  //const char * romFile = "Resources/games/donkey kong.nes";
  const char * romFile = "Resources/games/super mario bros.nes";
  //const char * romFile = "Resources/games/super mario bros 3.nes";
  //const char * romFile = "Resources/ppu_sprite_hit/ppu_sprite_hit.nes";
  //const char *romFile = "Resources/apu_test/apu_test.nes";
  //const char * romFile = "Resources/power_up_palette.nes";
  //const char * romFile = "Resources/instr_test-v5/rom_singles/02-implied.nes";
  //const char *romFile = "Resources/nmi_sync/demo_ntsc.nes";
  //const char *romFile = "Resources/ppu_sprite_hit/ppu_sprite_hit.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/01-vbl_basics.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/02-vbl_set_time.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/03-vbl_clear_time.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/04-nmi_control.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/05-nmi_timing.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/06-suppression.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/07-nmi_on_timing.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/08-nmi_off_timing.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/09-even_odd_frames.nes";
  //const char *romFile = "Resources/ppu_vbl_nmi/rom_singles/10-even_odd_timing.nes";
  //const char *romFile = "Resources/cpu_timing_test6/cpu_timing_test.nes";
  //const char *romFile = "Resources/cpu_interrupts_v2/rom_singles/2-nmi_and_brk.nes";
  //const char *romFile = "Resources/ntsc_torture.nes";
  const char *paletteFile = "Resources/ntscpalette.pal";
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
    exit(-1);
  }

  Palette_LoadFrom(paletteFile);

  _ppuRenderSurface = SDL_CreateRGBSurfaceWithFormat(0, NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
  PPU_SetRenderSurface(_ppuRenderSurface);

  _ppuPatternTableSurfaces[0] = SDL_CreateRGBSurfaceWithFormat(0, 16 * 8, 16 * 8, 32, SDL_PIXELFORMAT_RGBA32);
  _ppuPatternTableSurfaces[1] = SDL_CreateRGBSurfaceWithFormat(0, 16 * 8, 16 * 8, 32, SDL_PIXELFORMAT_RGBA32);

  // Hook up controllers to SDL
  Controllers_SetButtonHandler(0, HandleButton);

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
  bool keyDown = event->type == SDL_KEYDOWN;

  if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
  {
    if (event->key.keysym.sym == SDLK_z)
    {
      _controller1Buttons[NES_BUTTON_A] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_x)
    {
      _controller1Buttons[NES_BUTTON_B] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_BACKSPACE)
    {
      _controller1Buttons[NES_BUTTON_SELECT] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_RETURN)
    {
      _controller1Buttons[NES_BUTTON_START] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_UP)
    {
      _controller1Buttons[NES_BUTTON_UP] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_DOWN)
    {
      _controller1Buttons[NES_BUTTON_DOWN] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_LEFT)
    {
      _controller1Buttons[NES_BUTTON_LEFT] = keyDown;
    }
    else if (event->key.keysym.sym == SDLK_RIGHT)
    {
      _controller1Buttons[NES_BUTTON_RIGHT] = keyDown;
    }
  }

  if (event->type == SDL_KEYDOWN)
  {
    if (event->key.keysym.sym == SDLK_SPACE)
    {
      _stepKeyWasPressed = true;
    }
    else if (event->key.keysym.sym == SDLK_f)
    {
      _frameStepKeyWasPressed = true;
    }
    else if (event->key.keysym.sym == SDLK_r)
    {
      _runKeyWasPressed = true;
    }
    else if (event->key.keysym.sym == SDLK_s)
    {
      _statusKeyWasPressed = true;
    }
    else if (event->key.keysym.sym == SDLK_p)
    {
      _patternTableDrawIndex++;
      if (_patternTableDrawIndex > 2)
      {
        _patternTableDrawIndex = 0;
      }
    }
  }
}

static void FormatInstruction(CPU_t *cpu, char* textBuffer)
{
  u8_t instructionBytes[3];
  const InstructionTableEntry_t *instr;
  instructionBytes[0] = Bus_ReadFromCPU(cpu->Bus, cpu->PC);
  instr = InstructionTable_GetInstruction(instructionBytes[0]);
  u8_t instructionLength = AddressingMode_GetInstructionLength(instr->AddressingMode);

  for (u8_t i = 1; i < instructionLength; i++)
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
  APU_t *apu;
  const InstructionTableEntry_t *instr;
  cpu = NES_GetCPU();
  bus = NES_GetBus();
  ppu = NES_GetPPU();
  apu = bus->APU;

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
    _detailMode = (_detailMode + 1) % NR_OF_DETAIL_MODES;
  }

  // Second row: CPU status
  switch (_detailMode)
  {
  case DETAIL_MODE_CPU:
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
    break;
  }
  case DETAIL_MODE_PPU:
  {
    // Second row: PPU status
    snprintf(&_statusBarBuffer[STATUS_BAR_CHARS_PER_ROW],
            STATUS_BAR_CHARS_PER_ROW + 1,
            "H:%03d V:%03d CTRL:%02X STAT:%02X MASK:%02X FRAME:%d",
            ppu->HCount,
            ppu->VCount,
            ppu->Ctrl.currentValue,
            ppu->Status.currentValue,
            ppu->Mask.currentValue,
            ppu->FrameCount
            );
    break;
  }
  case DETAIL_MODE_APU:
  {
    // Second row: APU status
    snprintf(&_statusBarBuffer[STATUS_BAR_CHARS_PER_ROW],
            STATUS_BAR_CHARS_PER_ROW + 1,
            "CNT:%08u S:%02X F:%02X",
            apu->HalfClockCounter,
            apu->Status.currentValue,
            apu->FrameCounter.currentValue
            );
    break;
  }
  case NR_OF_DETAIL_MODES:
  default:
    break;
  }


  u16_t memAddress = cpu->InstructionPC - HALF_MEM_WINDOW_SIZE;
  for (int i = 0; i < HALF_MEM_WINDOW_SIZE * 2 + 1; i++)
  {
    u8_t memData = Bus_ReadFromCPU(bus, memAddress);
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
      u8_t memData = Bus_ReadFromCPU(bus, memAddress);
      snprintf(_memoryViewBuffer + strlen(_memoryViewBuffer),
               sizeof(_memoryViewBuffer) - strlen(_memoryViewBuffer),
               " %02X",
               memData
               );
      memAddress++;
    }
  }

  memset(_oamViewBuffer, 0, sizeof(_oamViewBuffer));
  for (int i = 0; i < OAM_VIEW_ENTRIES; i++)
  {
    OAMEntry_t *entry;

    entry = &(ppu->OAM[i]);

    snprintf(_oamViewBuffer + strlen(_oamViewBuffer),
             sizeof(_oamViewBuffer) - strlen(_oamViewBuffer),
             "%02d: %3d %3d T:0x%02X A:0x%02X",
             i,
             entry->X,
             entry->Y,
             entry->TileIndex,
             entry->Attributes
             );
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

  if (_frameStepKeyWasPressed)
  {
    NES_TickUntilFrameComplete();
    _frameStepKeyWasPressed = false;
  }

  SharedSDL_BeginTiming(4);
  if (_run)
  {
    // Realtime-ish speed
    NES_TickUntilFrameComplete();
    if (cpu->IsKilled)
    {
      _run = false;
    }
  }
  SharedSDL_EndTiming(4);

  FormatInstruction(cpu, _textBuffer);

  // Draw pattern tables AFTER rendering
  if (_patternTableDrawIndex < 2)
  {
    DrawPatternTable(bus, _patternTableDrawIndex * 0x1000, _ppuPatternTableSurfaces[_patternTableDrawIndex]);
  }

  return true;
}

static uint64_t prevPerformanceCounter;
static uint64_t performanceCounterFrequency;

static void Draw(SDL_Surface* surface)
{
  SDL_Rect nesInternalRect;
  SDL_Rect nesScreenRect;

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
  if ((_detailMode == DETAIL_MODE_CPU) || (_detailMode == DETAIL_MODE_APU))
  {
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
  }
  else
  {
    // OAM view
    Text_DrawString(surface, "OAM view", nesScreenRect.w, STATUS_BAR_HEIGHT, &_font);
    Text_DrawStringWrapping(surface,
                            _oamViewBuffer,
                            nesScreenRect.w,
                            STATUS_BAR_HEIGHT + _font.GlyphHeight,
                            OAM_VIEW_CHARS_PER_ROW,
                            &_font);
  }

  // Pattern table output
  if (_patternTableDrawIndex < 2)
  {
    SDL_Rect srcRect =
    {
        w: 128,
        h: 128,
        x: 0,
        y: 0
    };
    SDL_Rect dstRect =
    {
        w: 256,
        h: 256,
        x: nesScreenRect.w,
        y: STATUS_BAR_HEIGHT
    };

    SDL_BlitScaled(_ppuPatternTableSurfaces[_patternTableDrawIndex], &srcRect,
                    surface, &dstRect);
  }

  // Palette output
  DrawPalettes(NES_GetBus(), surface, nesScreenRect.w, FONT_SIZE * 29 + 3);

  // Debug: state
  Text_DrawString(surface, _run ? "Running" : "Stopped", 0, surface->h - _font.GlyphHeight, &_font);
  Text_DrawString(surface, "- P: Pattern, Space: Step, R: Run, S: Status", 8 * _font.GlyphWidth, surface->h - _font.GlyphHeight, &_font);

  // Debug: FPS
  if (0 == performanceCounterFrequency)
  {
    performanceCounterFrequency = SDL_GetPerformanceFrequency();
  }

  uint64_t perfCounter = SDL_GetPerformanceCounter();
  u32_t fps = performanceCounterFrequency / (perfCounter - prevPerformanceCounter);
  snprintf(_statusBarBuffer, sizeof(_statusBarBuffer), "FPS: %u", fps);
  Text_DrawString(surface, _statusBarBuffer, 0, surface->h - 2 * _font.GlyphHeight, &_font);
  prevPerformanceCounter = perfCounter;
}


