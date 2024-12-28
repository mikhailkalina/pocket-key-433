#include <Arduino.h>

#include "log.h"

// Local buffer for log output
static char buffer[100];

/**
 * @brief Print formatted string to the log
 *
 * @param format
 * @param ...
 */
void LOG_printf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  Serial.println(buffer);
}
