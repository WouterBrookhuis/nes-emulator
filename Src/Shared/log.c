#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "log.h"

void LogMessage(const char* format, ...)
{
    const char* prefix = "[I]  ";
    const char* postfix = "\n";
    size_t newFormatLength = strlen(format) + strlen(prefix) + strlen(postfix);
    char newFormat[newFormatLength + 1];
    memset(newFormat, 0, newFormatLength + 1);
    strcat(newFormat, prefix);
    strcat(newFormat, format);
    strcat(newFormat, postfix);

    va_list args;
    va_start(args, format);
    vprintf(newFormat, args);
    va_end(args);
}
void LogWarning(const char* format, ...)
{
    const char* prefix = "[W]  ";
    const char* postfix = "\n";
    size_t newFormatLength = strlen(format) + strlen(prefix) + strlen(postfix);
    char newFormat[newFormatLength + 1];
    memset(newFormat, 0, newFormatLength + 1);
    strcat(newFormat, prefix);
    strcat(newFormat, format);
    strcat(newFormat, postfix);

    va_list args;
    va_start(args, format);
    vprintf(newFormat, args);
    va_end(args);
}
void LogError(const char* format, ...)
{
    const char* prefix = "[E]  ";
    const char* postfix = "\n";
    size_t newFormatLength = strlen(format) + strlen(prefix) + strlen(postfix);
    char newFormat[newFormatLength + 1];
    memset(newFormat, 0, newFormatLength + 1);
    strcat(newFormat, prefix);
    strcat(newFormat, format);
    strcat(newFormat, postfix);

    va_list args;
    va_start(args, format);
    vprintf(newFormat, args);
    va_end(args);
}
