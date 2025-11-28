/*
 * API Edge Cases and NULL Parameter Tests
 *
 * Comprehensive tests for NULL parameter handling, empty strings, and edge cases
 * for restreamer-api.c functions to improve code coverage.
 *
 * This file focuses on testing error paths and boundary conditions that don't
 * require a mock server.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "restreamer-api.h"

/* Test macros */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
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

/* ========================================================================
 * Process State API - Edge Cases
 * ======================================================================== */

/* Test: get_process_state with all NULL parameters */
static bool test_get_process_state_all_null(void) {
  printf("  Testing get_process_state with all NULL parameters...\n");

  bool result = restreamer_api_get_process_state(NULL, NULL, NULL);
  TEST_ASSERT(!result, "Should return false for all NULL parameters");

  printf("  ✓ get_process_state all NULL handling\n");
  return true;
}

/* Test: get_process_state with empty process_id string */
static bool test_get_process_state_empty_id(void) {
  printf("  Testing get_process_state with empty process_id...\n");

  /* Create a minimal API structure for testing
   * Note: This will fail without a valid connection, but we're testing
   * parameter validation which should happen before any network calls */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true; /* Skip test if API creation fails */
  }

  restreamer_process_state_t state = {0};
  bool result = restreamer_api_get_process_state(api, "", &state);
  /* Empty string may or may not be validated - just verify it doesn't crash */
  (void)result; /* Intentionally unused - testing for crashes */

  restreamer_api_destroy(api);

  printf("  ✓ get_process_state empty process_id handling\n");
  return true;
}

/* Test: free_process_state with already-freed structure */
static bool test_free_process_state_double_free(void) {
  printf("  Testing free_process_state with already-freed structure...\n");

  restreamer_process_state_t state = {0};
  /* Simulate a freed state */
  restreamer_api_free_process_state(&state);
  /* Free again - should be safe */
  restreamer_api_free_process_state(&state);

  printf("  ✓ free_process_state double free handling\n");
  return true;
}

/* ========================================================================
 * Probe Info API - Edge Cases
 * ======================================================================== */

/* Test: probe_input with all NULL parameters */
static bool test_probe_input_all_null(void) {
  printf("  Testing probe_input with all NULL parameters...\n");

  bool result = restreamer_api_probe_input(NULL, NULL, NULL);
  TEST_ASSERT(!result, "Should return false for all NULL parameters");

  printf("  ✓ probe_input all NULL handling\n");
  return true;
}

/* Test: probe_input with empty process_id string */
static bool test_probe_input_empty_id(void) {
  printf("  Testing probe_input with empty process_id...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  restreamer_probe_info_t info = {0};
  bool result = restreamer_api_probe_input(api, "", &info);
  /* Empty string may or may not be validated - just verify it doesn't crash */
  (void)result; /* Intentionally unused - testing for crashes */

  restreamer_api_destroy(api);

  printf("  ✓ probe_input empty process_id handling\n");
  return true;
}

/* Test: free_probe_info with already-freed structure */
static bool test_free_probe_info_double_free(void) {
  printf("  Testing free_probe_info with already-freed structure...\n");

  restreamer_probe_info_t info = {0};
  /* Free once */
  restreamer_api_free_probe_info(&info);
  /* Free again - should be safe */
  restreamer_api_free_probe_info(&info);

  printf("  ✓ free_probe_info double free handling\n");
  return true;
}

/* ========================================================================
 * Config API - Edge Cases
 * ======================================================================== */

/* Test: get_config with NULL api and NULL output */
static bool test_get_config_all_null(void) {
  printf("  Testing get_config with all NULL parameters...\n");

  bool result = restreamer_api_get_config(NULL, NULL);
  TEST_ASSERT(!result, "Should return false for all NULL parameters");

  printf("  ✓ get_config all NULL handling\n");
  return true;
}

/* Test: set_config with NULL api and NULL config */
static bool test_set_config_all_null(void) {
  printf("  Testing set_config with all NULL parameters...\n");

  bool result = restreamer_api_set_config(NULL, NULL);
  TEST_ASSERT(!result, "Should return false for all NULL parameters");

  printf("  ✓ set_config all NULL handling\n");
  return true;
}

/* Test: set_config with empty config string */
static bool test_set_config_empty_string(void) {
  printf("  Testing set_config with empty string...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  bool result = restreamer_api_set_config(api, "");
  /* Empty string may or may not be accepted - just verify it doesn't crash */
  (void)result; /* Intentionally unused - testing for crashes */

  restreamer_api_destroy(api);

  printf("  ✓ set_config empty string handling\n");
  return true;
}

/* Test: reload_config with NULL api */
static bool test_reload_config_null_api(void) {
  printf("  Testing reload_config with NULL api...\n");

  bool result = restreamer_api_reload_config(NULL);
  TEST_ASSERT(!result, "Should return false for NULL api");

  printf("  ✓ reload_config NULL api handling\n");
  return true;
}

/* ========================================================================
 * Additional API Function Edge Cases
 * ======================================================================== */

/* Test: get_processes with NULL list */
static bool test_get_processes_null_list(void) {
  printf("  Testing get_processes with NULL list...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  bool result = restreamer_api_get_processes(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL list");

  restreamer_api_destroy(api);

  printf("  ✓ get_processes NULL list handling\n");
  return true;
}

/* Test: get_process with NULL output */
static bool test_get_process_null_output(void) {
  printf("  Testing get_process with NULL output...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  bool result = restreamer_api_get_process(api, "test-id", NULL);
  TEST_ASSERT(!result, "Should return false for NULL output");

  restreamer_api_destroy(api);

  printf("  ✓ get_process NULL output handling\n");
  return true;
}

/* Test: create_process with NULL parameters */
static bool test_create_process_null_params(void) {
  printf("  Testing create_process with NULL parameters...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  /* Test NULL reference */
  bool result = restreamer_api_create_process(api, NULL, "input", NULL, 0, NULL);
  TEST_ASSERT(!result, "Should return false for NULL reference");

  /* Test NULL input */
  result = restreamer_api_create_process(api, "test-ref", NULL, NULL, 0, NULL);
  TEST_ASSERT(!result, "Should return false for NULL input");

  /* Test empty reference */
  result = restreamer_api_create_process(api, "", "input", NULL, 0, NULL);
  TEST_ASSERT(!result, "Should return false for empty reference");

  restreamer_api_destroy(api);

  printf("  ✓ create_process NULL parameter handling\n");
  return true;
}

/* Test: delete_process with NULL/empty process_id */
static bool test_delete_process_invalid_params(void) {
  printf("  Testing delete_process with invalid parameters...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  /* Test NULL process_id */
  bool result = restreamer_api_delete_process(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL process_id");

  /* Test empty process_id */
  result = restreamer_api_delete_process(api, "");
  TEST_ASSERT(!result, "Should return false for empty process_id");

  restreamer_api_destroy(api);

  printf("  ✓ delete_process invalid parameter handling\n");
  return true;
}

/* Test: get_process_logs with NULL parameters */
static bool test_get_process_logs_null_params(void) {
  printf("  Testing get_process_logs with NULL parameters...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  /* Test NULL process_id */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, NULL, &logs);
  TEST_ASSERT(!result, "Should return false for NULL process_id");

  /* Test NULL logs output */
  result = restreamer_api_get_process_logs(api, "test-id", NULL);
  TEST_ASSERT(!result, "Should return false for NULL logs output");

  restreamer_api_destroy(api);

  printf("  ✓ get_process_logs NULL parameter handling\n");
  return true;
}

/* Test: get_sessions with NULL list */
static bool test_get_sessions_null_list(void) {
  printf("  Testing get_sessions with NULL list...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  bool result = restreamer_api_get_sessions(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL list");

  restreamer_api_destroy(api);

  printf("  ✓ get_sessions NULL list handling\n");
  return true;
}

/* Test: API creation with NULL connection */
static bool test_api_create_null_connection(void) {
  printf("  Testing API creation with NULL connection...\n");

  restreamer_api_t *api = restreamer_api_create(NULL);
  TEST_ASSERT_NULL(api, "Should return NULL for NULL connection");

  printf("  ✓ API creation NULL connection handling\n");
  return true;
}

/* Test: API creation with NULL host */
static bool test_api_create_null_host(void) {
  printf("  Testing API creation with NULL host...\n");

  restreamer_connection_t conn = {
      .host = NULL,
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NULL(api, "Should return NULL for NULL host");

  printf("  ✓ API creation NULL host handling\n");
  return true;
}

/* Test: API destroy with NULL */
static bool test_api_destroy_null(void) {
  printf("  Testing API destroy with NULL...\n");

  /* Should not crash */
  restreamer_api_destroy(NULL);

  printf("  ✓ API destroy NULL handling\n");
  return true;
}

/* Test: get_error with NULL api */
static bool test_get_error_null_api(void) {
  printf("  Testing get_error with NULL api...\n");

  const char *error = restreamer_api_get_error(NULL);
  /* May return NULL or empty string - just verify it doesn't crash */
  (void)error;

  printf("  ✓ get_error NULL api handling\n");
  return true;
}

/* Test: free_process_list with NULL */
static bool test_free_process_list_null(void) {
  printf("  Testing free_process_list with NULL...\n");

  /* Should not crash */
  restreamer_api_free_process_list(NULL);

  printf("  ✓ free_process_list NULL handling\n");
  return true;
}

/* Test: free_process with NULL */
static bool test_free_process_null(void) {
  printf("  Testing free_process with NULL...\n");

  /* Should not crash */
  restreamer_api_free_process(NULL);

  printf("  ✓ free_process NULL handling\n");
  return true;
}

/* Test: process control functions with empty process_id */
static bool test_process_control_empty_id(void) {
  printf("  Testing process control with empty process_id...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ⚠ Could not create API for testing\n");
    return true;
  }

  /* Test start_process with empty ID */
  bool result = restreamer_api_start_process(api, "");
  TEST_ASSERT(!result, "start_process should fail with empty ID");

  /* Test stop_process with empty ID */
  result = restreamer_api_stop_process(api, "");
  TEST_ASSERT(!result, "stop_process should fail with empty ID");

  /* Test restart_process with empty ID */
  result = restreamer_api_restart_process(api, "");
  TEST_ASSERT(!result, "restart_process should fail with empty ID");

  restreamer_api_destroy(api);

  printf("  ✓ Process control empty ID handling\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

/* Run all edge case tests */
bool run_api_edge_case_tests(void) {
  bool all_passed = true;

  printf("\nAPI Edge Cases and NULL Parameter Tests\n");
  printf("========================================\n");

  /* Process State API */
  all_passed &= test_get_process_state_all_null();
  all_passed &= test_get_process_state_empty_id();
  all_passed &= test_free_process_state_double_free();

  /* Probe Info API */
  all_passed &= test_probe_input_all_null();
  all_passed &= test_probe_input_empty_id();
  all_passed &= test_free_probe_info_double_free();

  /* Config API */
  all_passed &= test_get_config_all_null();
  all_passed &= test_set_config_all_null();
  all_passed &= test_set_config_empty_string();
  all_passed &= test_reload_config_null_api();

  /* General API Functions */
  all_passed &= test_get_processes_null_list();
  all_passed &= test_get_process_null_output();
  all_passed &= test_create_process_null_params();
  all_passed &= test_delete_process_invalid_params();
  all_passed &= test_get_process_logs_null_params();
  all_passed &= test_get_sessions_null_list();

  /* API Creation/Destruction */
  all_passed &= test_api_create_null_connection();
  all_passed &= test_api_create_null_host();
  all_passed &= test_api_destroy_null();

  /* Utility Functions */
  all_passed &= test_get_error_null_api();
  all_passed &= test_free_process_list_null();
  all_passed &= test_free_process_null();
  all_passed &= test_process_control_empty_id();

  return all_passed;
}
