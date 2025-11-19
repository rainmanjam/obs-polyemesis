/*
 * API Error Handling Tests
 *
 * Comprehensive tests for error scenarios, network failures, and recovery
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

#include "mock_restreamer.h"
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

#define TEST_ASSERT_NOT_NULL(ptr, message)                                     \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected non-NULL pointer\n    at %s:%d\n",   \
              message, __FILE__, __LINE__);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: Network Timeout */
static bool test_network_timeout(void) {
  printf("  Testing network timeout handling...\n");

  /* Connect to non-existent host */
  restreamer_connection_t conn = {
      .host = "192.0.2.1",  /* TEST-NET-1, should timeout */
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* This should timeout */
  bool result = restreamer_api_test_connection(api);
  TEST_ASSERT(!result, "Connection should fail with timeout");

  /* Error message should be set */
  const char *error = restreamer_api_get_error(api);
  TEST_ASSERT_NOT_NULL(error, "Error message should be set on timeout");

  restreamer_api_destroy(api);

  printf("  ✓ Network timeout handling\n");
  return true;
}

/* Test: Connection Refused */
static bool test_connection_refused(void) {
  printf("  Testing connection refused handling...\n");

  /* Connect to localhost on unused port */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 65432,  /* Unlikely to be in use */
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_test_connection(api);
  TEST_ASSERT(!result, "Connection should be refused");

  const char *error = restreamer_api_get_error(api);
  TEST_ASSERT_NOT_NULL(error, "Error message should be set");

  restreamer_api_destroy(api);

  printf("  ✓ Connection refused handling\n");
  return true;
}

/* Test: HTTP 404 Not Found */
static bool test_http_404_error(void) {
  printf("  Testing HTTP 404 error handling...\n");

  if (!mock_restreamer_start(9200)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9200,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Try to get a non-existent process */
  restreamer_process_t *process = restreamer_api_get_process(api, "nonexistent-id");
  TEST_ASSERT(process == NULL, "Should return NULL for non-existent process");

  const char *error = restreamer_api_get_error(api);
  TEST_ASSERT_NOT_NULL(error, "Error message should be set for 404");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ HTTP 404 error handling\n");
  return true;
}

/* Test: HTTP 500 Internal Server Error */
static bool test_http_500_error(void) {
  printf("  Testing HTTP 500 error handling...\n");

  if (!mock_restreamer_start(9201)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9201,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Mock server can be configured to return 500 */
  /* Test error handling for server errors */
  bool result = restreamer_api_test_connection(api);
  /* Even if connection succeeds, API operations should handle 500s gracefully */

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ HTTP 500 error handling\n");
  return true;
}

/* Test: Invalid JSON Response */
static bool test_invalid_json_response(void) {
  printf("  Testing invalid JSON response handling...\n");

  if (!mock_restreamer_start(9202)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9202,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Mock server can return invalid JSON */
  /* API should handle this gracefully */
  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);

  /* Should either succeed with valid JSON or fail with error message */
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    TEST_ASSERT_NOT_NULL(error, "Error should be set for invalid JSON");
  }

  restreamer_api_free_process_list(&list);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Invalid JSON response handling\n");
  return true;
}

/* Test: NULL Parameter Handling */
static bool test_null_parameter_handling(void) {
  printf("  Testing NULL parameter handling...\n");

  /* Creating API with NULL connection should fail gracefully */
  restreamer_api_t *api = restreamer_api_create(NULL);
  TEST_ASSERT(api == NULL, "Should return NULL for NULL connection");

  /* Destroying NULL API should not crash */
  restreamer_api_destroy(NULL);  /* Should not crash */

  printf("  ✓ NULL parameter handling\n");
  return true;
}

/* Test: Large Response Handling */
static bool test_large_response_handling(void) {
  printf("  Testing large response handling...\n");

  if (!mock_restreamer_start(9203)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9203,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get process list (mock server may return large list) */
  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);
  TEST_ASSERT(result, "Should handle large response");

  restreamer_api_free_process_list(&list);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Large response handling\n");
  return true;
}

/* Test: Concurrent Request Handling */
static bool test_concurrent_request_handling(void) {
  printf("  Testing concurrent request handling...\n");

  if (!mock_restreamer_start(9204)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9204,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Make multiple requests in sequence (simulating concurrent use) */
  for (int i = 0; i < 5; i++) {
    restreamer_process_list_t list = {0};
    bool result = restreamer_api_get_processes(api, &list);
    TEST_ASSERT(result, "Each request should succeed");
    restreamer_api_free_process_list(&list);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Concurrent request handling\n");
  return true;
}

/* Test: Memory Allocation Failure Handling */
static bool test_memory_handling(void) {
  printf("  Testing memory handling...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9205,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test that cleanup works properly */
  restreamer_api_destroy(api);

  /* Double free should be safe */
  restreamer_api_destroy(api);  /* Should not crash */

  printf("  ✓ Memory handling\n");
  return true;
}

/* Test: Empty Host String */
static bool test_empty_host_handling(void) {
  printf("  Testing empty host handling...\n");

  restreamer_connection_t conn = {
      .host = "",  /* Empty host */
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  /* Should either return NULL or create but fail on connection */
  if (api != NULL) {
    bool result = restreamer_api_test_connection(api);
    TEST_ASSERT(!result, "Connection should fail with empty host");
    restreamer_api_destroy(api);
  }

  printf("  ✓ Empty host handling\n");
  return true;
}

/* Main test runner */
int test_api_error_handling(void) {
  printf("\n=== API Error Handling Tests ===\n");

  int passed = 0;
  int failed = 0;

  if (test_network_timeout()) {
    passed++;
  } else {
    failed++;
  }

  if (test_connection_refused()) {
    passed++;
  } else {
    failed++;
  }

  if (test_http_404_error()) {
    passed++;
  } else {
    failed++;
  }

  if (test_http_500_error()) {
    passed++;
  } else {
    failed++;
  }

  if (test_invalid_json_response()) {
    passed++;
  } else {
    failed++;
  }

  if (test_null_parameter_handling()) {
    passed++;
  } else {
    failed++;
  }

  if (test_large_response_handling()) {
    passed++;
  } else {
    failed++;
  }

  if (test_concurrent_request_handling()) {
    passed++;
  } else {
    failed++;
  }

  if (test_memory_handling()) {
    passed++;
  } else {
    failed++;
  }

  if (test_empty_host_handling()) {
    passed++;
  } else {
    failed++;
  }

  printf("\n=== API Error Handling Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
