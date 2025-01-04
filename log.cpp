#include "log.h"

#include <stdio.h>

#include <Arduino.h>

using namespace Log;

namespace
{
#define LOG_ENABLE // Uncomment to enable log printing

#ifdef LOG_ENABLE
  // Local buffer for log output
  char buffer[60];
#endif // LOG_ENABLE
} // namespace

/**
 * @brief Print formatted string to the log
 *
 * @param format
 * @param ...
 */
void Log::printf(const char *format, ...)
{
#ifdef LOG_ENABLE
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
#endif // LOG_ENABLE
}
