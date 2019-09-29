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
    return false;
  }

  if (fread(&header, 1, 16, f) != 16)
  {
    fclose(f);
    return false;
  }

  if (header.Flags6 & 0x01)
  {
    // Mirroring
  }

  if (header.Flags6 & 0x02)
  {
    // Battery
  }

  if (header.Flags6 & 0x04)
  {
    // 512 byte trainer for 0x7000 - 0x71FF
    if (fseek(f, 512, SEEK_CUR) != 0)
    {
      fclose(f);
      return false;
    }
  }

  if (header.Flags6 & 0x08)
  {
    // Ignore mirroring
  }

  uint8_t mapperId = ((header.Flags6 >> 4) & 0x0F) | (header.Flags7 & 0xF0);

  switch (mapperId)
  {
  case 0x00:
    Mapper000_Initialize(mapper, &header);
    break;
//  case 0x01:
//    Mapper001_Initialize(mapper, &header);
//    break;
  default:
    fclose(f);
    return false;
  }

  size_t bytesToRead = mapper->MemorySize;
  if (fread(mapper->Memory, 1, bytesToRead, f) != bytesToRead)
  {
    // TODO: Free mapper memory
    fclose(f);
    return false;
  }

  fclose(f);
  return true;
}
