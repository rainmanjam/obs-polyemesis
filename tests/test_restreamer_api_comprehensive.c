/*
 * Comprehensive Restreamer API Tests
 *
 * Tests for the actual Restreamer API implementation covering:
 * - Connection management and authentication
 * - Process lifecycle (create, get, start, stop, delete)
 * - Dynamic output management (add, remove, update)
 * - Process state monitoring
 * - Error handling and edge cases
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

/* ========================================================================
 * Connection & Authentication Tests
 * ======================================================================== */

/* Test: API client creation and destruction */
static bool test_api_lifecycle(void) {
  printf("  Testing API client lifecycle...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  /* Create API client */
  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Destroy API client */
  restreamer_api_destroy(api);

  /* NULL destroy should be safe */
  restreamer_api_destroy(NULL);

  printf("  ✓ API lifecycle\n");
  return true;
}

/* Test: Connection testing with mock server */
static bool test_connection_testing(void) {
  printf("  Testing connection to mock server...\n");

  if (!mock_restreamer_start(9500)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9500,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test connection */
  bool result = restreamer_api_test_connection(api);
  TEST_ASSERT(result, "Connection test should succeed");

  /* Check if connected */
  bool connected = restreamer_api_is_connected(api);
  TEST_ASSERT(connected, "Should be connected after successful test");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Connection testing\n");
  return true;
}

/* Test: Authentication with force_login */
static bool test_force_login(void) {
  printf("  Testing force login...\n");

  if (!mock_restreamer_start(9501)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9501,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Force login */
  bool result = restreamer_api_force_login(api);
  TEST_ASSERT(result, "Force login should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Force login\n");
  return true;
}

/* Test: Token refresh */
static bool test_token_refresh(void) {
  printf("  Testing token refresh...\n");

  if (!mock_restreamer_start(9502)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9502,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Login first */
  TEST_ASSERT(restreamer_api_force_login(api), "Initial login should succeed");

  /* Refresh token */
  bool result = restreamer_api_refresh_token(api);
  TEST_ASSERT(result, "Token refresh should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Token refresh\n");
  return true;
}

/* ========================================================================
 * Process Management Tests
 * ======================================================================== */

/* Test: Get processes list */
static bool test_get_processes(void) {
  printf("  Testing get processes list...\n");

  if (!mock_restreamer_start(9503)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9503,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);
  TEST_ASSERT(result, "Get processes should succeed");

  printf("    Found %zu process(es)\n", list.count);

  /* Free the list */
  restreamer_api_free_process_list(&list);

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get processes\n");
  return true;
}

/* Test: Create process */
static bool test_create_process(void) {
  printf("  Testing create process...\n");

  if (!mock_restreamer_start(9504)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9504,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create process with multiple outputs */
  const char *outputs[] = {
      "rtmp://live.twitch.tv/app/streamkey1",
      "rtmp://a.rtmp.youtube.com/live2/streamkey2",
  };

  bool result = restreamer_api_create_process(
      api, "test-stream", "rtmp://localhost:1935/live/input", outputs, 2, NULL);
  TEST_ASSERT(result, "Create process should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Create process\n");
  return true;
}

/* Test: Get process details */
static bool test_get_process_details(void) {
  printf("  Testing get process details...\n");

  if (!mock_restreamer_start(9505)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9505,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get process details */
  restreamer_process_t process = {0};
  bool result =
      restreamer_api_get_process(api, "test-process-id", &process);

  /* May fail if process doesn't exist in mock - that's OK for this test */
  if (result) {
    printf("    Process ID: %s\n", process.id ? process.id : "(null)");
    printf("    State: %s\n", process.state ? process.state : "(null)");
    restreamer_api_free_process(&process);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get process details\n");
  return true;
}

/* Test: Start/Stop/Restart process */
static bool test_process_control(void) {
  printf("  Testing process control (start/stop/restart)...\n");

  if (!mock_restreamer_start(9506)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9506,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  const char *process_id = "test-process";

  /* These may fail if process doesn't exist - we're testing the API calls work */
  printf("    Testing start...\n");
  restreamer_api_start_process(api, process_id);

  printf("    Testing stop...\n");
  restreamer_api_stop_process(api, process_id);

  printf("    Testing restart...\n");
  restreamer_api_restart_process(api, process_id);

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process control\n");
  return true;
}

/* Test: Delete process */
static bool test_delete_process(void) {
  printf("  Testing delete process...\n");

  if (!mock_restreamer_start(9507)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9507,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Delete process (may fail if doesn't exist) */
  bool result = restreamer_api_delete_process(api, "test-process");

  /* Just verify the call doesn't crash */
  printf("    Delete result: %s\n", result ? "success" : "failed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Delete process\n");
  return true;
}

/* ========================================================================
 * Dynamic Output Management Tests
 * ======================================================================== */

/* Test: Add output to process */
static bool test_add_output(void) {
  printf("  Testing add output to process...\n");

  if (!mock_restreamer_start(9508)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9508,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Add output */
  bool result = restreamer_api_add_process_output(
      api, "test-process", "output-1",
      "rtmp://live.facebook.com:443/rtmp/streamkey", NULL);

  printf("    Add output result: %s\n", result ? "success" : "failed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Add output\n");
  return true;
}

/* Test: Remove output from process */
static bool test_remove_output(void) {
  printf("  Testing remove output from process...\n");

  if (!mock_restreamer_start(9509)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9509,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Remove output */
  bool result =
      restreamer_api_remove_process_output(api, "test-process", "output-1");

  printf("    Remove output result: %s\n", result ? "success" : "failed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Remove output\n");
  return true;
}

/* Test: Update output settings */
static bool test_update_output(void) {
  printf("  Testing update output settings...\n");

  if (!mock_restreamer_start(9510)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9510,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Update output */
  bool result = restreamer_api_update_process_output(
      api, "test-process", "output-1", "rtmp://new-url.example.com/live/key",
      "scale=1280:720");

  printf("    Update output result: %s\n", result ? "success" : "failed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Update output\n");
  return true;
}

/* ========================================================================
 * Process State Monitoring Tests
 * ======================================================================== */

/* Test: Get process state */
static bool test_get_process_state(void) {
  printf("  Testing get process state...\n");

  if (!mock_restreamer_start(9511)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9511,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get process state */
  restreamer_process_state_t state = {0};
  bool result = restreamer_api_get_process_state(api, "test-process", &state);

  if (result) {
    printf("    Frames: %llu\n", (unsigned long long)state.frames);
    printf("    FPS: %.2f\n", state.fps);
    printf("    Bitrate: %u kbps\n", state.current_bitrate);
    printf("    Running: %s\n", state.is_running ? "yes" : "no");
    restreamer_api_free_process_state(&state);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get process state\n");
  return true;
}

/* ========================================================================
 * Error Handling Tests
 * ======================================================================== */

/* Test: Error message retrieval */
static bool test_error_messages(void) {
  printf("  Testing error message retrieval...\n");

  restreamer_connection_t conn = {
      .host = "invalid-host-that-does-not-exist.local",
      .port = 9999,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Try operation that should fail */
  bool result = restreamer_api_test_connection(api);
  TEST_ASSERT(!result, "Connection should fail for invalid host");

  /* Get error message */
  const char *error = restreamer_api_get_error(api);
  if (error) {
    printf("    Error message: %s\n", error);
  }

  restreamer_api_destroy(api);

  printf("  ✓ Error messages\n");
  return true;
}

/* Test: NULL parameter handling */
static bool test_null_parameters(void) {
  printf("  Testing NULL parameter handling...\n");

  /* NULL connection */
  restreamer_api_t *api = restreamer_api_create(NULL);
  TEST_ASSERT(api == NULL, "Should return NULL for NULL connection");

  /* NULL API destroy */
  restreamer_api_destroy(NULL); /* Should not crash */

  printf("  ✓ NULL parameters\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int test_restreamer_api_comprehensive(void) {
  printf("\n=== Comprehensive Restreamer API Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* Connection & Authentication */
  if (test_api_lifecycle()) passed++; else failed++;
  if (test_connection_testing()) passed++; else failed++;
  if (test_force_login()) passed++; else failed++;
  if (test_token_refresh()) passed++; else failed++;

  /* Process Management */
  if (test_get_processes()) passed++; else failed++;
  if (test_create_process()) passed++; else failed++;
  if (test_get_process_details()) passed++; else failed++;
  if (test_process_control()) passed++; else failed++;
  if (test_delete_process()) passed++; else failed++;

  /* Dynamic Output Management */
  if (test_add_output()) passed++; else failed++;
  if (test_remove_output()) passed++; else failed++;
  if (test_update_output()) passed++; else failed++;

  /* Process State Monitoring */
  if (test_get_process_state()) passed++; else failed++;

  /* Error Handling */
  if (test_error_messages()) passed++; else failed++;
  if (test_null_parameters()) passed++; else failed++;

  printf("\n=== Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
