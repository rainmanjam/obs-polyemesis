/*
 * OBS Function Stubs for Testing
 *
 * Provides minimal stub implementations of OBS functions that are referenced
 * by the source code but not needed for unit tests.
 */

#include <stdarg.h>
#include <stdio.h>

/* Stub implementation of obs_log for testing */
void obs_log(int log_level, const char *format, ...) {
  /* For tests, we can just suppress OBS logging or print to stdout */
  (void)log_level; /* Unused in tests */

  va_list args;
  va_start(args, format);
  /* Optionally print to stdout for debugging */
  /* vprintf(format, args); */
  /* printf("\n"); */
  va_end(args);
}
