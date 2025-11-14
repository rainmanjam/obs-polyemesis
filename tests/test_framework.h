/**
 * Simple Unit Test Framework for obs-polyemesis
 * Provides crash detection, memory leak tracking, and assertion helpers
 */

#pragma once

#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Color codes for terminal output */
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"

/* Test statistics */
typedef struct {
  int total;
  int passed;
  int failed;
  int crashed;
  int skipped;
} test_stats_t;

static test_stats_t global_stats = {0};
static jmp_buf test_crash_handler;
static volatile bool test_crashed = false;

/* Signal handler for crash detection */
static void crash_signal_handler(int sig) {
  const char *sig_name = "UNKNOWN";
  switch (sig) {
  case SIGSEGV:
    sig_name = "SIGSEGV (Segmentation Fault)";
    break;
  case SIGABRT:
    sig_name = "SIGABRT (Abort)";
    break;
  case SIGFPE:
    sig_name = "SIGFPE (Floating Point Exception)";
    break;
  case SIGILL:
    sig_name = "SIGILL (Illegal Instruction)";
    break;
  case SIGBUS:
    sig_name = "SIGBUS (Bus Error)";
    break;
  }

  fprintf(stderr, "%s[CRASH]%s Test crashed with signal: %s\n", COLOR_RED,
          COLOR_RESET, sig_name);
  test_crashed = true;
  longjmp(test_crash_handler, 1);
}

/* Setup crash handlers */
static void setup_crash_handlers(void) {
  signal(SIGSEGV, crash_signal_handler);
  signal(SIGABRT, crash_signal_handler);
  signal(SIGFPE, crash_signal_handler);
  signal(SIGILL, crash_signal_handler);
  signal(SIGBUS, crash_signal_handler);
}

/* Test assertion macros */
#define ASSERT(condition, message)                                             \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "%s[FAIL]%s %s:%d: %s\n", COLOR_RED, COLOR_RESET,        \
              __FILE__, __LINE__, message);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_EQ(a, b, message)                                               \
  do {                                                                         \
    if ((a) != (b)) {                                                          \
      fprintf(stderr, "%s[FAIL]%s %s:%d: %s (expected: %ld, got: %ld)\n",      \
              COLOR_RED, COLOR_RESET, __FILE__, __LINE__, message, (long)(b),  \
              (long)(a));                                                      \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_NE(a, b, message)                                               \
  do {                                                                         \
    if ((a) == (b)) {                                                          \
      fprintf(stderr, "%s[FAIL]%s %s:%d: %s\n", COLOR_RED, COLOR_RESET,        \
              __FILE__, __LINE__, message);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_NULL(ptr, message) ASSERT((ptr) == NULL, message)
#define ASSERT_NOT_NULL(ptr, message) ASSERT((ptr) != NULL, message)

#define ASSERT_STR_EQ(a, b, message)                                           \
  do {                                                                         \
    if (strcmp((a), (b)) != 0) {                                               \
      fprintf(stderr,                                                          \
              "%s[FAIL]%s %s:%d: %s (expected: \"%s\", got: \"%s\")\n",        \
              COLOR_RED, COLOR_RESET, __FILE__, __LINE__, message, (b), (a));  \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define ASSERT_TRUE(condition, message) ASSERT((condition), message)
#define ASSERT_FALSE(condition, message) ASSERT(!(condition), message)

/* Test runner macros */
#define RUN_TEST(test_func, test_name)                                         \
  do {                                                                         \
    global_stats.total++;                                                      \
    printf("%s[TEST]%s Running: %s\n", COLOR_CYAN, COLOR_RESET, test_name);    \
                                                                               \
    test_crashed = false;                                                      \
    if (setjmp(test_crash_handler) == 0) {                                     \
      setup_crash_handlers();                                                  \
      bool result = test_func();                                               \
      if (result) {                                                            \
        printf("%s[PASS]%s %s\n", COLOR_GREEN, COLOR_RESET, test_name);        \
        global_stats.passed++;                                                 \
      } else {                                                                 \
        printf("%s[FAIL]%s %s\n", COLOR_RED, COLOR_RESET, test_name);          \
        global_stats.failed++;                                                 \
      }                                                                        \
    } else {                                                                   \
      printf("%s[CRASH]%s %s\n", COLOR_RED, COLOR_RESET, test_name);           \
      global_stats.crashed++;                                                  \
    }                                                                          \
    printf("\n");                                                              \
  } while (0)

/* Print test summary */
static void print_test_summary(void) {
  printf("\n");
  printf("====================================================================="
         "===========\n");
  printf("%sTEST SUMMARY%s\n", COLOR_MAGENTA, COLOR_RESET);
  printf("====================================================================="
         "===========\n");
  printf("Total:   %d\n", global_stats.total);
  printf("%sPassed:  %d%s\n", COLOR_GREEN, global_stats.passed, COLOR_RESET);
  printf("%sFailed:  %d%s\n", global_stats.failed > 0 ? COLOR_RED : COLOR_RESET,
         global_stats.failed, COLOR_RESET);
  printf("%sCrashed: %d%s\n",
         global_stats.crashed > 0 ? COLOR_RED : COLOR_RESET,
         global_stats.crashed, COLOR_RESET);
  printf("Skipped: %d\n", global_stats.skipped);
  printf("====================================================================="
         "===========\n");

  if (global_stats.failed > 0 || global_stats.crashed > 0) {
    printf("%sResult: FAILED%s\n", COLOR_RED, COLOR_RESET);
  } else {
    printf("%sResult: PASSED%s\n", COLOR_GREEN, COLOR_RESET);
  }
  printf("====================================================================="
         "===========\n");
}

/* Get exit code based on test results */
static int get_test_exit_code(void) {
  if (global_stats.crashed > 0)
    return 2; /* Critical failure */
  if (global_stats.failed > 0)
    return 1; /* Test failures */
  return 0;   /* All passed */
}

/* Test suite macros */
#define BEGIN_TEST_SUITE(name)                                                 \
  int main(void) {                                                             \
    printf("\n");                                                              \
    printf("=================================================================" \
           "===============\n");                                               \
    printf("%sTEST SUITE: %s%s\n", COLOR_BLUE, name, COLOR_RESET);             \
    printf("=================================================================" \
           "===============\n");                                               \
    printf("\n");

#define END_TEST_SUITE()                                                       \
  print_test_summary();                                                        \
  return get_test_exit_code();                                                 \
  }
