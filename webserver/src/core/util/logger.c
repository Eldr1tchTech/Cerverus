#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

// Define the arrays
#define X_COLOR(name, color, tag) color,
const char *log_colors[] = {LOG_LEVELS(X_COLOR)};
#undef X_COLOR

#define X_TAG(name, color, tag) tag,
const char *log_tags[] = {LOG_LEVELS(X_TAG)};
#undef X_TAG

void logger_log(LOG_LEVEL log_level, char *format, ...) {
  va_list args;
  va_start(args, format);

  // pre-append
  printf("%s%s ", log_colors[log_level], log_tags[log_level]);

  // print content
  vprintf(format, args);

  // reset
  printf(ANSI_RESET "\n");

  va_end(args);

#if defined(_DEBUG) &&                                                         \
    !defined(_TESSERACT_UNIT_TESTS) // TODO: Get rid of tesserat legacy code
  if (log_level == LOG_LEVEL_FATAL ||
      log_level == LOG_LEVEL_ERROR) /* breakpoint()*/
    ;
#endif
}
