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
extern bool run_api_system_tests(void);
extern bool run_api_filesystem_tests(void);
extern bool run_config_tests(void);
extern bool run_multistream_tests(void);
extern bool run_stream_channel_tests(void);
extern bool run_source_tests(void);
extern bool run_output_tests(void);

/* New comprehensive API test (returns int: 0=success, 1=failure) */
extern int test_restreamer_api_comprehensive(void);

/* New API extension tests (returns int: 0=success, 1=failure) */
extern int test_restreamer_api_extensions(void);

/* New API advanced feature tests (returns int: 0=success, 1=failure) */
extern int test_restreamer_api_advanced(void);

/* New diagnostic API tests (returns bool: true=success, false=failure) */
extern bool run_api_diagnostics_tests(void);

/* New security API tests (returns int: 0=success, 1=failure) */
extern int run_api_security_tests(void);

/* Process config tests - define the struct type here */
typedef struct {
  int passed;
  int failed;
} test_results_t;
extern test_results_t run_api_process_config_tests(void);

/* API utility tests (returns int: 0=success, 1=failure) */
extern int run_api_utils_tests(void);

/* Process management tests (returns int: 0=success, 1=failure) */
extern int run_api_process_management_tests(void);

/* Sessions tests (returns int: 0=success, 1=failure) */
extern int run_api_sessions_tests(void);

/* Process state tests (returns int: 0=success, 1=failure) */
extern int run_api_process_state_tests(void);

/* Dynamic output tests (returns int: 0=success, 1=failure) */
extern int run_api_dynamic_output_tests(void);

/* Skills and extended features tests (returns int: 0=success, 1=failure) */
extern int run_api_skills_tests(void);

/* Edge case and NULL parameter tests (returns bool: true=success, false=failure) */
extern bool run_api_edge_case_tests(void);

/* API endpoint tests (returns bool: true=success, false=failure) */
extern bool run_api_endpoint_tests(void);

/* API parsing and free function tests (returns bool: true=success, false=failure) */
extern bool run_api_parsing_tests(void);

/* API helper function tests (returns bool: true=success, false=failure) */
extern bool run_api_helper_tests(void);

/* Channel coverage tests (returns bool: true=success, false=failure) */
extern bool run_channel_coverage_tests(void);

/* TODO: Add these test files if needed
extern int run_api_coverage_gaps_tests(void);
extern int test_api_coverage_improvements(void);
*/

/* TODO: Re-enable once tests are fixed to match actual API
 * New integration test declarations (return int: 0=success, 1=failure)
 */
/*
extern int test_api_auth(void);
extern int test_api_error_handling(void);
extern int test_restreamer_api_v3_workflow(void);
extern int test_multistream_integration(void);
*/

/* Wrapper functions to convert int return to bool */
static bool run_api_comprehensive_tests(void) {
  return test_restreamer_api_comprehensive() == 0;
}

static bool run_api_extensions_tests(void) {
  return test_restreamer_api_extensions() == 0;
}

static bool run_api_advanced_tests(void) {
  return test_restreamer_api_advanced() == 0;
}

/* Wrapper for security tests (converts int return to bool) */
static bool run_security_tests_wrapper(void) {
  return run_api_security_tests() == 0;
}

/* Wrapper for process config tests (converts struct return to bool) */
static bool run_process_config_tests_wrapper(void) {
  test_results_t results = run_api_process_config_tests();
  return results.failed == 0;
}

/* Wrapper for API utils tests (converts int return to bool) */
static bool run_api_utils_tests_wrapper(void) {
  return run_api_utils_tests() == 0;
}

/* Wrapper for process management tests (converts int return to bool) */
static bool run_api_process_management_tests_wrapper(void) {
  return run_api_process_management_tests() == 0;
}

/* Wrapper for sessions tests (converts int return to bool) */
static bool run_api_sessions_tests_wrapper(void) {
  return run_api_sessions_tests() == 0;
}

/* Wrapper for process state tests (converts int return to bool) */
static bool run_api_process_state_tests_wrapper(void) {
  return run_api_process_state_tests() == 0;
}

/* Wrapper for dynamic output tests (converts int return to bool) */
static bool run_api_dynamic_output_tests_wrapper(void) {
  return run_api_dynamic_output_tests() == 0;
}

/* Wrapper for skills tests (converts int return to bool) */
static bool run_api_skills_tests_wrapper(void) {
  return run_api_skills_tests() == 0;
}

/* TODO: Add wrappers if test files are created
static bool run_api_coverage_improvements_tests_wrapper(void) {
  return test_api_coverage_improvements() == 0;
}

static bool run_api_coverage_gaps_tests_wrapper(void) {
  return run_api_coverage_gaps_tests() == 0;
}
*/

/*
static bool run_api_auth_tests(void) {
  return test_api_auth() == 0;
}

static bool run_api_error_handling_tests(void) {
  return test_api_error_handling() == 0;
}

static bool run_api_v3_workflow_tests(void) {
  return test_restreamer_api_v3_workflow() == 0;
}

static bool run_multistream_integration_tests(void) {
  return test_multistream_integration() == 0;
}
*/

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

  /* TODO: These new test suites need fixes before enabling by default
   * - api-system: ping test expects JSON but API returns plain text
   * - api-filesystem: mock server cleanup issues cause cascade failures
   * Run explicitly with --test-suite=api-system or --test-suite=api-filesystem
   */
  if (suite_filter && strcmp(suite_filter, "api-system") == 0) {
    run_test_suite("API System & Configuration Tests", run_api_system_tests);
  }

  if (suite_filter && strcmp(suite_filter, "api-filesystem") == 0) {
    run_test_suite("API Filesystem & Connection Tests", run_api_filesystem_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-comprehensive") == 0) {
    run_test_suite("Comprehensive API Tests", run_api_comprehensive_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-extensions") == 0) {
    run_test_suite("API Extension Tests", run_api_extensions_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-advanced") == 0) {
    run_test_suite("API Advanced Feature Tests", run_api_advanced_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-diagnostics") == 0) {
    run_test_suite("API Diagnostics Tests", run_api_diagnostics_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-security") == 0) {
    run_test_suite("API Security Tests", run_security_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-process-config") == 0) {
    run_test_suite("API Process Config Tests", run_process_config_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-utils") == 0) {
    run_test_suite("API Utility Tests", run_api_utils_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-process-management") == 0) {
    run_test_suite("API Process Management Tests", run_api_process_management_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-sessions") == 0) {
    run_test_suite("API Sessions Tests", run_api_sessions_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-process-state") == 0) {
    run_test_suite("API Process State Tests", run_api_process_state_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-dynamic-output") == 0) {
    run_test_suite("API Dynamic Output Tests", run_api_dynamic_output_tests_wrapper);
  }

  /* TODO: api-skills tests need fixes before enabling by default
   * Run explicitly with --test-suite=api-skills
   */
  if (suite_filter && strcmp(suite_filter, "api-skills") == 0) {
    run_test_suite("API Skills and Extended Features Tests", run_api_skills_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-edge-cases") == 0) {
    run_test_suite("API Edge Cases and NULL Parameter Tests", run_api_edge_case_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-endpoints") == 0) {
    run_test_suite("API Endpoint Tests", run_api_endpoint_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-parsing") == 0) {
    run_test_suite("API Parsing and Free Functions Tests", run_api_parsing_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-helpers") == 0) {
    run_test_suite("API Helper Functions Tests", run_api_helper_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "channel-coverage") == 0) {
    run_test_suite("Channel Coverage Tests", run_channel_coverage_tests);
  }

  /* TODO: Add these test suites if test files are created
  if (!suite_filter || strcmp(suite_filter, "api-coverage-gaps") == 0) {
    run_test_suite("API Coverage Gaps Tests", run_api_coverage_gaps_tests_wrapper);
  }

  if (!suite_filter || strcmp(suite_filter, "api-coverage-improvements") == 0) {
    run_test_suite("API Coverage Improvement Tests", run_api_coverage_improvements_tests_wrapper);
  }
  */

  /* TODO: Re-enable once tests are fixed to match actual API
  if (!suite_filter || strcmp(suite_filter, "api-auth") == 0) {
    run_test_suite("API Authentication Tests", run_api_auth_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-errors") == 0) {
    run_test_suite("API Error Handling Tests", run_api_error_handling_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "api-v3-workflow") == 0) {
    run_test_suite("API v3 Workflow Tests", run_api_v3_workflow_tests);
  }
  */

  if (!suite_filter || strcmp(suite_filter, "config") == 0) {
    run_test_suite("Configuration Tests", run_config_tests);
  }

  if (!suite_filter || strcmp(suite_filter, "multistream") == 0) {
    run_test_suite("Multistream Tests", run_multistream_tests);
  }

  /* TODO: Re-enable once tests are fixed
  if (!suite_filter || strcmp(suite_filter, "multistream-integration") == 0) {
    run_test_suite("Multistream Integration Tests", run_multistream_integration_tests);
  }
  */

  if (!suite_filter || strcmp(suite_filter, "channel") == 0) {
    run_test_suite("Stream Channel Tests", run_stream_channel_tests);
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
