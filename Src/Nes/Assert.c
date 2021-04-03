#include "log.h"
#include <stdint.h>
#include <SDL2/SDL.h>

void AssertFailed(const char *file, int32_t line, const char* msg)
{
  LogError("ASSERT FAILED FILE %s LINE %d MSG %s", file, line, msg);
}
