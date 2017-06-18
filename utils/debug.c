#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

int kbar_err_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}
