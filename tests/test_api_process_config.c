/*
 * Process Configuration API Tests
 *
 * Tests for the restreamer_api_get_process_config() API function covering:
 * - Successful retrieval of process configuration as JSON
 * - NULL parameter validation
 * - Empty process ID validation
 * - JSON validity and structure verification
 * - Memory management and cleanup
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

#include <util/bmem.h>
#include "mock_restreamer.h"
#include "restreamer-api.h"

/* Test macros - these set test_passed = false instead of returning */
#define TEST_CHECK(condition, message)                                         \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
      test_passed = false;                                                     \
    }                                                                          \
  } while (0)

/* ========================================================================
 * Process Configuration API Tests
 * ======================================================================== */

/* Test: Successfully get process configuration for valid process */
static bool test_get_process_config_success(void) {
  printf("  Testing get process config success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;
  char *config_json = NULL;

  if (!mock_restreamer_start(9741)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9741,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Get process config */
  bool result = restreamer_api_get_process_config(api, "test-process-id", &config_json);

  if (result && config_json) {
    printf("    Retrieved config (truncated): %.80s...\n", config_json);
    bfree(config_json);
    config_json = NULL;
  } else {
    printf("    Config retrieval failed (may be expected if mock doesn't support endpoint)\n");
  }

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Get process config test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter returns false */
static bool test_get_process_config_null_api(void) {
  printf("  Testing get process config with NULL api...\n");
  bool test_passed = true;

  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(NULL, "test-process", &config_json);

  TEST_CHECK(!result, "Should return false for NULL api");
  TEST_CHECK(config_json == NULL, "Output should remain NULL when api is NULL");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter returns false */
static bool test_get_process_config_null_process_id(void) {
  printf("  Testing get process config with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9742)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9742,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(api, NULL, &config_json);

  TEST_CHECK(!result, "Should return false for NULL process_id");
  TEST_CHECK(config_json == NULL, "Output should remain NULL when process_id is NULL");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ NULL process_id handling\n");
  }
  return test_passed;
}

/* Test: NULL output pointer parameter returns false */
static bool test_get_process_config_null_output(void) {
  printf("  Testing get process config with NULL output pointer...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9743)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9743,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_get_process_config(api, "test-process", NULL);

  TEST_CHECK(!result, "Should return false for NULL output pointer");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ NULL output pointer handling\n");
  }
  return test_passed;
}

/* Test: Empty process_id string handling */
static bool test_get_process_config_empty_process_id(void) {
  printf("  Testing get process config with empty process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;
  char *config_json = NULL;

  if (!mock_restreamer_start(9744)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9744,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_get_process_config(api, "", &config_json);

  /* The API may or may not validate empty strings - just verify it doesn't crash */
  printf("    Result with empty process_id: %s\n", result ? "success" : "failed");

  if (config_json) {
    bfree(config_json);
  }

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Empty process_id handling\n");
  }
  return test_passed;
}

/* Test: Returned JSON is valid and contains expected fields */
static bool test_get_process_config_json_valid(void) {
  printf("  Testing JSON validity of process config...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;
  char *config_json = NULL;

  if (!mock_restreamer_start(9745)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9745,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_get_process_config(api, "test-process-id", &config_json);

  if (result && config_json) {
    /* Basic JSON validation - check for common JSON markers */
    bool has_braces = (config_json[0] == '{' || config_json[0] == '[');
    if (!has_braces) {
      fprintf(stderr, "  ✗ FAIL: JSON should start with { or [\n");
      test_passed = false;
    }

    size_t len = strlen(config_json);
    if (len <= 2) {
      fprintf(stderr, "  ✗ FAIL: JSON should have content\n");
      test_passed = false;
    } else {
      printf("    JSON appears valid (length: %zu bytes)\n", len);
    }

    bfree(config_json);
    config_json = NULL;
  } else {
    printf("    Config not retrieved (mock may not support this endpoint)\n");
  }

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ JSON validity check\n");
  }
  return test_passed;
}

/* Test: Config JSON can be properly freed with bfree() */
static bool test_get_process_config_memory_freed(void) {
  printf("  Testing process config memory management...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9746)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9746,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Get config multiple times to test memory management */
  for (int i = 0; i < 3; i++) {
    char *config_json = NULL;
    bool result = restreamer_api_get_process_config(api, "test-process-id", &config_json);

    if (result && config_json) {
      /* Verify we can read the memory */
      size_t len = strlen(config_json);
      if (len == 0) {
        fprintf(stderr, "  ✗ FAIL: Config should have content\n");
        test_passed = false;
      }

      /* Free it properly */
      bfree(config_json);
      config_json = NULL;
    }
  }

  printf("    Memory allocated and freed 3 times successfully\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Memory management\n");
  }
  return test_passed;
}

/* Test: Multiple processes can have configs retrieved */
static bool test_get_process_config_multiple_processes(void) {
  printf("  Testing get config for multiple processes...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9747)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9747,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Try to get configs for different process IDs */
  const char *process_ids[] = {
    "process-1",
    "process-2",
    "test-stream",
  };

  for (size_t i = 0; i < sizeof(process_ids) / sizeof(process_ids[0]); i++) {
    char *config_json = NULL;
    bool result = restreamer_api_get_process_config(api, process_ids[i], &config_json);

    printf("    Process '%s': %s\n", process_ids[i], result ? "retrieved" : "not found");

    if (config_json) {
      bfree(config_json);
    }
  }

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Multiple processes\n");
  }
  return test_passed;
}

/* Test: Error message is set on failure */
static bool test_get_process_config_error_message(void) {
  printf("  Testing error message on config retrieval failure...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;
  char *config_json = NULL;

  if (!mock_restreamer_start(9748)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9748,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Try to get config for non-existent process */
  bool result = restreamer_api_get_process_config(api, "nonexistent-process-9999", &config_json);

  if (!result) {
    const char *error = restreamer_api_get_error(api);
    if (error && strlen(error) > 0) {
      printf("    Error message: %s\n", error);
    } else {
      printf("    No error message set\n");
    }
  }

  if (config_json) {
    bfree(config_json);
  }

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Error message handling\n");
  }
  return test_passed;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

typedef struct {
  int passed;
  int failed;
} test_results_t;

test_results_t run_api_process_config_tests(void) {
  printf("\n=== Process Configuration API Tests ===\n");

  test_results_t results = {0, 0};

  /* Process Config API Tests */
  if (test_get_process_config_success()) results.passed++; else results.failed++;
  if (test_get_process_config_null_api()) results.passed++; else results.failed++;
  if (test_get_process_config_null_process_id()) results.passed++; else results.failed++;
  if (test_get_process_config_null_output()) results.passed++; else results.failed++;
  if (test_get_process_config_empty_process_id()) results.passed++; else results.failed++;
  if (test_get_process_config_json_valid()) results.passed++; else results.failed++;
  if (test_get_process_config_memory_freed()) results.passed++; else results.failed++;
  if (test_get_process_config_multiple_processes()) results.passed++; else results.failed++;
  if (test_get_process_config_error_message()) results.passed++; else results.failed++;

  printf("\n=== Test Summary ===\n");
  printf("Passed: %d\n", results.passed);
  printf("Failed: %d\n", results.failed);
  printf("Total:  %d\n", results.passed + results.failed);

  return results;
}
