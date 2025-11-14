/*
 * Test runner for OBS Polyemesis
 *
 * Simple test framework that runs all test suites
 */

#include <curl/curl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test framework macros and helpers */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Alias for consistency with new tests */
#define test_assert(condition, message) TEST_ASSERT(condition, message)

/* Test section markers (for readability) */
static void test_section_start(const char *name) {
  (void)name; /* Unused - reserved for future use */
  /* Optional: could print section header */
}

static void test_section_end(const char *name) {
  (void)name; /* Unused - reserved for future use */
  /* Optional: could print section footer */
}

/* Test start/end markers */
static void test_start(const char *name) {
  printf("  Testing %s...\n", name);
}

static void test_end(void) {
  /* Optional: could print test completion */
}

/* Test suite start/end */
static void test_suite_start(const char *name) {
  printf("\n%s\n", name);
  printf("========================================\n");
}

static void test_suite_end(const char *name, bool result) {
  if (result) {
    printf("✓ %s: PASSED\n", name);
  } else {
    printf("✗ %s: FAILED\n", name);
  }
}

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message)                       \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: \"%s\", Actual: \"%s\"\n    at "    \
              "%s:%d\n",                                                       \
              message, (expected), (actual), __FILE__, __LINE__);              \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NULL(ptr, message)                                         \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected NULL but got %p\n    at %s:%d\n",    \
              message, (void *)(ptr), __FILE__, __LINE__);                     \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_NOT_NULL(ptr, message)                                     \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected non-NULL pointer\n    at %s:%d\n",   \
              message, __FILE__, __LINE__);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test statistics */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test suite declarations */
extern bool run_api_client_tests(void);
extern bool run_config_tests(void);
extern bool run_multistream_tests(void);
extern bool run_output_profile_tests(void);
extern bool run_source_tests(void);
extern bool run_output_tests(void);

/* Test runner */
static bool run_test_suite(const char *name, bool (*suite_func)(void)) {
  printf("\n%s\n", name);
  printf("========================================\n");

  bool result = suite_func();

  if (result) {
    printf("✓ %s: PASSED\n", name);
    tests_passed++;
  } else {
    printf("✗ %s: FAILED\n", name);
    tests_failed++;
  }

  tests_run++;
  return result;
}

int main(int argc, char **argv) {
  const char *suite_filter = NULL;

  /* Initialize curl globally (required for Windows) */
  curl_global_init(CURL_GLOBAL_DEFAULT);

  /* Parse command line arguments */
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--test-suite=", 13) == 0) {
      suite_filter = argv[i] + 13;
    }
  }

  printf("========================================\n");
  printf("  OBS Polyemesis Test Suite\n");
  printf("========================================\n");

  /* Run test suites */
  if (!suite_filter || strcmp(suite_filter, "api") == 0) {
    run_test_suite("API Client Tests", run_api_client_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "config") == 0) {
    run_test_suite("Configuration Tests", run_config_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "multistream") == 0) {
    run_test_suite("Multistream Tests", run_multistream_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "profile") == 0) {
    run_test_suite("Output Profile Tests", run_output_profile_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "source") == 0) {
    run_test_suite("Source Plugin Tests", run_source_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "output") == 0) {
    run_test_suite("Output Plugin Tests", run_output_tests);
  }

  /* Print summary */
  printf("\n========================================\n");
  printf("  Test Summary\n");
  printf("========================================\n");
  printf("  Total:  %d\n", tests_run);
  printf("  ✓ Passed: %d\n", tests_passed);
  printf("  ✗ Failed: %d\n", tests_failed);
  printf("========================================\n");

  /* Cleanup curl */
  curl_global_cleanup();

  return (tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
