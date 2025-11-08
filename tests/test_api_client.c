/*
 * API Client Tests
 *
 * Tests for the Restreamer API client functionality
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

/* Test macros from test_main.c */
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

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: API client creation */
static bool test_api_create(void) {
  printf("  Testing API client creation...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  restreamer_api_destroy(api);

  printf("  ✓ API client creation\n");
  return true;
}

/* Test: Connection testing */
static bool test_api_connection(void) {
  printf("  Testing API connection...\n");

  /* Start mock server */
  if (!mock_restreamer_start(9090)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  /* Give server time to start */
  sleep_ms(100); /* 100ms */

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9090,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test connection (this will make actual HTTP request to mock server) */
  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ API connection testing\n");
  return true;
}

/* Test: Get processes */
static bool test_api_get_processes(void) {
  printf("  Testing get processes...\n");

  /* Start mock server */
  if (!mock_restreamer_start(9091)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(100);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9091,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);
  TEST_ASSERT(result, "Should get processes from mock server");
  TEST_ASSERT(list.count > 0, "Should have at least one process");

  if (list.count > 0) {
    TEST_ASSERT_NOT_NULL(list.processes[0].id, "Process should have ID");
    TEST_ASSERT_NOT_NULL(list.processes[0].reference,
                         "Process should have reference");
  }

  restreamer_api_free_process_list(&list);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get processes\n");
  return true;
}

/* Test: Process control (start/stop) */
static bool test_api_process_control(void) {
  printf("  Testing process control...\n");

  /* Start mock server */
  if (!mock_restreamer_start(9092)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(100);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9092,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test start process */
  bool started = restreamer_api_start_process(api, "test-process-1");
  TEST_ASSERT(started, "Should start process");

  /* Test stop process */
  bool stopped = restreamer_api_stop_process(api, "test-process-1");
  TEST_ASSERT(stopped, "Should stop process");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process control\n");
  return true;
}

/* Test: Error handling */
static bool test_api_error_handling(void) {
  printf("  Testing error handling...\n");

  /* Test connection to non-existent server */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 65535, /* Unlikely to be in use */
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api,
                       "API client should be created even with invalid server");

  /* Connection should fail */
  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(!connected, "Should fail to connect to non-existent server");

  /* Error message should be set */
  const char *error = restreamer_api_get_error(api);
  TEST_ASSERT_NOT_NULL(error, "Should have error message");

  restreamer_api_destroy(api);

  printf("  ✓ Error handling\n");
  return true;
}

/* Run all API client tests */
bool run_api_client_tests(void) {
  bool all_passed = true;

  all_passed &= test_api_create();
  all_passed &= test_api_connection();
  all_passed &= test_api_get_processes();
  all_passed &= test_api_process_control();
  all_passed &= test_api_error_handling();

  return all_passed;
}
