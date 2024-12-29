#include "log.h"

#include <Arduino.h>

using namespace Log;

namespace
{
  // Local buffer for log output
  char buffer[50];
} // namespace

/**
 * @brief Print formatted string to the log
 *
 * @param format
 * @param ...
 */
void Log::printf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
}
