/*
 * INesLoader.c
 *
 *  Created on: Sep 21, 2019
 *      Author: wouter
 */

#include "INesLoader.h"
#include "Mapper.h"
#include "Mapper000.h"
#include "Mapper001.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


bool INesLoader_Load(const char* file, Mapper_t* mapper)
{
  FILE *f;
  INesHeader_t header;

  f = fopen(file, "rb");
  if (f == NULL)
  {
    LogError("Unable to open file %s", file);
    return false;
  }

  if (fread(&header, 1, 16, f) != 16)
  {
    fclose(f);
    LogError("Unable to read file %s", file);
    return false;
  }

  LogMessage("Loading file %s", file);
  LogMessage("Mapper PRG ROM: %u x 16k", header.PrgRomSize);
  LogMessage("Mapper CHR ROM: %u x 8k", header.ChrRomSize);
  LogMessage("Mapper flags6: 0x%02X", header.Flags6);
  LogMessage("Mapper flags7: 0x%02X", header.Flags7);
  LogMessage("Mapper flags8: 0x%02X", header.Flags8);
  LogMessage("Mapper flags9: 0x%02X", header.Flags9);
  LogMessage("Mapper flagsA: 0x%02X", header.Flags10);

  if (header.Flags6 & 0x04)
  {
    // 512 byte trainer for 0x7000 - 0x71FF
    if (fseek(f, 512, SEEK_CUR) != 0)
    {
      fclose(f);
      LogError("Unable to read file %s", file);
      return false;
    }
  }

  u8_t mapperId = ((header.Flags6 & INES_FLAGS6_MAPPER_MASK) >> INES_FLAGS6_MAPPER_SHIFT) | (header.Flags7 & INES_FLAGS7_MAPPER_MASK);

  switch (mapperId)
  {
  case 0x00:
    Mapper000_Initialize(mapper, &header);
    break;
  case 0x01:
    Mapper001_Initialize(mapper, &header);
    break;
  default:
    fclose(f);
    LogError("Mapper id %u in file %s is not supported", mapperId, file);
    return false;
  }

  size_t bytesToRead = mapper->MemorySize;
  if (fread(mapper->Memory, 1, bytesToRead, f) != bytesToRead)
  {
    // TODO: Free mapper memory
    fclose(f);
    LogError("Unable to fully read file %s", file);
    return false;
  }

  fclose(f);
  return true;
}
