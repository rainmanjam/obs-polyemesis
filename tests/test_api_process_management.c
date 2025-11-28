/*
 * Process Management API Tests
 *
 * Comprehensive tests for process management API functions covering:
 * - restreamer_api_get_processes() - Get list of processes
 * - restreamer_api_get_process() - Get single process details
 * - restreamer_api_start_process() - Start a process
 * - restreamer_api_stop_process() - Stop a process
 * - restreamer_api_restart_process() - Restart a process
 * - restreamer_api_create_process() - Create a new process
 * - restreamer_api_delete_process() - Delete a process
 * - restreamer_api_free_process_list() - Free process list
 * - restreamer_api_free_process() - Free process
 *
 * Each test covers:
 * - Successful operation
 * - NULL parameter validation
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
 * restreamer_api_get_processes() Tests
 * ======================================================================== */

/* Test: Successfully get list of processes */
static bool test_get_processes_success(void)
{
  printf("  Testing get processes success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9760)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9760,
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

  /* Get processes */
  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);

  if (result) {
    printf("    Retrieved %zu processes\n", list.count);
    restreamer_api_free_process_list(&list);
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Get processes failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Get processes success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_get_processes_null_api(void)
{
  printf("  Testing get processes with NULL api...\n");
  bool test_passed = true;

  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(NULL, &list);

  TEST_CHECK(!result, "Should return false with NULL api");
  TEST_CHECK(list.count == 0, "List count should remain 0");
  TEST_CHECK(list.processes == NULL, "Processes pointer should remain NULL");

  if (test_passed) {
    printf("  ✓ Get processes NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL list parameter */
static bool test_get_processes_null_list(void)
{
  printf("  Testing get processes with NULL list...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9761)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9761,
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

  bool result = restreamer_api_get_processes(api, NULL);
  TEST_CHECK(!result, "Should return false with NULL list");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Get processes NULL list test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_get_process() Tests
 * ======================================================================== */

/* Test: Successfully get single process details */
static bool test_get_process_success(void)
{
  printf("  Testing get process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9762)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9762,
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

  /* Get process details */
  restreamer_process_t process = {0};
  bool result = restreamer_api_get_process(api, "test-process-id", &process);

  if (result) {
    printf("    Retrieved process: %s\n", process.id ? process.id : "unknown");
    restreamer_api_free_process(&process);
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Get process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Get process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_get_process_null_api(void)
{
  printf("  Testing get process with NULL api...\n");
  bool test_passed = true;

  restreamer_process_t process = {0};
  bool result = restreamer_api_get_process(NULL, "test-id", &process);

  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Get process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter */
static bool test_get_process_null_id(void)
{
  printf("  Testing get process with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9763)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9763,
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

  restreamer_process_t process = {0};
  bool result = restreamer_api_get_process(api, NULL, &process);
  TEST_CHECK(!result, "Should return false with NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Get process NULL id test passed\n");
  }
  return test_passed;
}

/* Test: NULL process parameter */
static bool test_get_process_null_process(void)
{
  printf("  Testing get process with NULL process...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9764)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9764,
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

  bool result = restreamer_api_get_process(api, "test-id", NULL);
  TEST_CHECK(!result, "Should return false with NULL process");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Get process NULL process test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_start_process() Tests
 * ======================================================================== */

/* Test: Successfully start a process */
static bool test_start_process_success(void)
{
  printf("  Testing start process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9765)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9765,
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

  /* Start process */
  bool result = restreamer_api_start_process(api, "test-process-id");

  if (result) {
    printf("    Process started successfully\n");
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Start process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Start process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_start_process_null_api(void)
{
  printf("  Testing start process with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_start_process(NULL, "test-id");
  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Start process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter */
static bool test_start_process_null_id(void)
{
  printf("  Testing start process with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9766)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9766,
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

  bool result = restreamer_api_start_process(api, NULL);
  TEST_CHECK(!result, "Should return false with NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Start process NULL id test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_stop_process() Tests
 * ======================================================================== */

/* Test: Successfully stop a process */
static bool test_stop_process_success(void)
{
  printf("  Testing stop process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9767)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9767,
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

  /* Stop process */
  bool result = restreamer_api_stop_process(api, "test-process-id");

  if (result) {
    printf("    Process stopped successfully\n");
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Stop process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Stop process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_stop_process_null_api(void)
{
  printf("  Testing stop process with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_stop_process(NULL, "test-id");
  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Stop process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter */
static bool test_stop_process_null_id(void)
{
  printf("  Testing stop process with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9768)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9768,
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

  bool result = restreamer_api_stop_process(api, NULL);
  TEST_CHECK(!result, "Should return false with NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Stop process NULL id test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_restart_process() Tests
 * ======================================================================== */

/* Test: Successfully restart a process */
static bool test_restart_process_success(void)
{
  printf("  Testing restart process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9769)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9769,
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

  /* Restart process */
  bool result = restreamer_api_restart_process(api, "test-process-id");

  if (result) {
    printf("    Process restarted successfully\n");
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Restart process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Restart process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_restart_process_null_api(void)
{
  printf("  Testing restart process with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_restart_process(NULL, "test-id");
  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Restart process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter */
static bool test_restart_process_null_id(void)
{
  printf("  Testing restart process with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9770)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9770,
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

  bool result = restreamer_api_restart_process(api, NULL);
  TEST_CHECK(!result, "Should return false with NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Restart process NULL id test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_create_process() Tests
 * ======================================================================== */

/* Test: Successfully create a new process */
static bool test_create_process_success(void)
{
  printf("  Testing create process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9771)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9771,
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

  /* Create process */
  const char *output_urls[] = {
      "rtmp://example.com/live/stream1",
      "rtmp://example.com/live/stream2"
  };
  bool result = restreamer_api_create_process(
      api,
      "test-reference",
      "rtmp://source.example.com/live/input",
      output_urls,
      2,
      NULL);

  if (result) {
    printf("    Process created successfully\n");
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Create process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Create process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_create_process_null_api(void)
{
  printf("  Testing create process with NULL api...\n");
  bool test_passed = true;

  const char *output_urls[] = {"rtmp://example.com/live/stream1"};
  bool result = restreamer_api_create_process(
      NULL,
      "test-ref",
      "rtmp://input.com/live",
      output_urls,
      1,
      NULL);

  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Create process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL reference parameter */
static bool test_create_process_null_reference(void)
{
  printf("  Testing create process with NULL reference...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9772)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9772,
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

  const char *output_urls[] = {"rtmp://example.com/live/stream1"};
  bool result = restreamer_api_create_process(
      api,
      NULL,
      "rtmp://input.com/live",
      output_urls,
      1,
      NULL);

  TEST_CHECK(!result, "Should return false with NULL reference");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Create process NULL reference test passed\n");
  }
  return test_passed;
}

/* Test: NULL input_url parameter */
static bool test_create_process_null_input_url(void)
{
  printf("  Testing create process with NULL input_url...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9773)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9773,
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

  const char *output_urls[] = {"rtmp://example.com/live/stream1"};
  bool result = restreamer_api_create_process(
      api,
      "test-ref",
      NULL,
      output_urls,
      1,
      NULL);

  TEST_CHECK(!result, "Should return false with NULL input_url");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Create process NULL input_url test passed\n");
  }
  return test_passed;
}

/* Test: NULL output_urls parameter */
static bool test_create_process_null_output_urls(void)
{
  printf("  Testing create process with NULL output_urls...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9774)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9774,
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

  bool result = restreamer_api_create_process(
      api,
      "test-ref",
      "rtmp://input.com/live",
      NULL,
      1,
      NULL);

  TEST_CHECK(!result, "Should return false with NULL output_urls");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Create process NULL output_urls test passed\n");
  }
  return test_passed;
}

/* Test: Zero output_count */
static bool test_create_process_zero_output_count(void)
{
  printf("  Testing create process with zero output_count...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9775)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9775,
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

  const char *output_urls[] = {"rtmp://example.com/live/stream1"};
  bool result = restreamer_api_create_process(
      api,
      "test-ref",
      "rtmp://input.com/live",
      output_urls,
      0,
      NULL);

  TEST_CHECK(!result, "Should return false with zero output_count");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Create process zero output_count test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_delete_process() Tests
 * ======================================================================== */

/* Test: Successfully delete a process */
static bool test_delete_process_success(void)
{
  printf("  Testing delete process success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9776)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9776,
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

  /* Delete process */
  bool result = restreamer_api_delete_process(api, "test-process-id");

  if (result) {
    printf("    Process deleted successfully\n");
  } else {
    const char *error = restreamer_api_get_error(api);
    printf("    Delete process failed: %s\n", error ? error : "unknown");
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
    printf("  ✓ Delete process success test completed\n");
  }
  return test_passed;
}

/* Test: NULL api parameter */
static bool test_delete_process_null_api(void)
{
  printf("  Testing delete process with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_delete_process(NULL, "test-id");
  TEST_CHECK(!result, "Should return false with NULL api");

  if (test_passed) {
    printf("  ✓ Delete process NULL api test passed\n");
  }
  return test_passed;
}

/* Test: NULL process_id parameter */
static bool test_delete_process_null_id(void)
{
  printf("  Testing delete process with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9777)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9777,
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

  bool result = restreamer_api_delete_process(api, NULL);
  TEST_CHECK(!result, "Should return false with NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Delete process NULL id test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_free_process_list() Tests
 * ======================================================================== */

/* Test: Free valid process list */
static bool test_free_process_list_valid(void)
{
  printf("  Testing free process list with valid data...\n");
  bool test_passed = true;

  /* Create a process list with allocated data */
  restreamer_process_list_t list = {0};
  list.count = 2;
  list.processes = (restreamer_process_t *)bmalloc(sizeof(restreamer_process_t) * 2);

  list.processes[0].id = bstrdup("process-1");
  list.processes[0].reference = bstrdup("ref-1");
  list.processes[0].state = bstrdup("running");
  list.processes[0].command = bstrdup("ffmpeg -i input");

  list.processes[1].id = bstrdup("process-2");
  list.processes[1].reference = bstrdup("ref-2");
  list.processes[1].state = bstrdup("stopped");
  list.processes[1].command = bstrdup("ffmpeg -i input2");

  /* Free the list - should not crash */
  restreamer_api_free_process_list(&list);

  TEST_CHECK(list.count == 0, "Count should be reset to 0");
  TEST_CHECK(list.processes == NULL, "Processes pointer should be NULL");

  if (test_passed) {
    printf("  ✓ Free process list valid test passed\n");
  }
  return test_passed;
}

/* Test: Free NULL process list */
static bool test_free_process_list_null(void)
{
  printf("  Testing free process list with NULL...\n");
  bool test_passed = true;

  /* Should not crash */
  restreamer_api_free_process_list(NULL);

  if (test_passed) {
    printf("  ✓ Free process list NULL test passed\n");
  }
  return test_passed;
}

/* Test: Free empty process list */
static bool test_free_process_list_empty(void)
{
  printf("  Testing free process list with empty list...\n");
  bool test_passed = true;

  restreamer_process_list_t list = {0};

  /* Should not crash */
  restreamer_api_free_process_list(&list);

  if (test_passed) {
    printf("  ✓ Free process list empty test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * restreamer_api_free_process() Tests
 * ======================================================================== */

/* Test: Free valid process */
static bool test_free_process_valid(void)
{
  printf("  Testing free process with valid data...\n");
  bool test_passed = true;

  /* Create a process with allocated data */
  restreamer_process_t process = {0};
  process.id = bstrdup("process-1");
  process.reference = bstrdup("ref-1");
  process.state = bstrdup("running");
  process.command = bstrdup("ffmpeg -i input");

  /* Free the process - should not crash */
  restreamer_api_free_process(&process);

  TEST_CHECK(process.id == NULL, "ID should be NULL");
  TEST_CHECK(process.reference == NULL, "Reference should be NULL");
  TEST_CHECK(process.state == NULL, "State should be NULL");
  TEST_CHECK(process.command == NULL, "Command should be NULL");

  if (test_passed) {
    printf("  ✓ Free process valid test passed\n");
  }
  return test_passed;
}

/* Test: Free NULL process */
static bool test_free_process_null(void)
{
  printf("  Testing free process with NULL...\n");
  bool test_passed = true;

  /* Should not crash */
  restreamer_api_free_process(NULL);

  if (test_passed) {
    printf("  ✓ Free process NULL test passed\n");
  }
  return test_passed;
}

/* Test: Free empty process */
static bool test_free_process_empty(void)
{
  printf("  Testing free process with empty process...\n");
  bool test_passed = true;

  restreamer_process_t process = {0};

  /* Should not crash */
  restreamer_api_free_process(&process);

  if (test_passed) {
    printf("  ✓ Free process empty test passed\n");
  }
  return test_passed;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int run_api_process_management_tests(void)
{
  printf("\n=== Process Management API Tests ===\n\n");

  int passed = 0;
  int failed = 0;

  /* restreamer_api_get_processes() tests */
  printf("--- restreamer_api_get_processes() ---\n");
  if (test_get_processes_success()) passed++; else failed++;
  if (test_get_processes_null_api()) passed++; else failed++;
  if (test_get_processes_null_list()) passed++; else failed++;

  /* restreamer_api_get_process() tests */
  printf("\n--- restreamer_api_get_process() ---\n");
  if (test_get_process_success()) passed++; else failed++;
  if (test_get_process_null_api()) passed++; else failed++;
  if (test_get_process_null_id()) passed++; else failed++;
  if (test_get_process_null_process()) passed++; else failed++;

  /* restreamer_api_start_process() tests */
  printf("\n--- restreamer_api_start_process() ---\n");
  if (test_start_process_success()) passed++; else failed++;
  if (test_start_process_null_api()) passed++; else failed++;
  if (test_start_process_null_id()) passed++; else failed++;

  /* restreamer_api_stop_process() tests */
  printf("\n--- restreamer_api_stop_process() ---\n");
  if (test_stop_process_success()) passed++; else failed++;
  if (test_stop_process_null_api()) passed++; else failed++;
  if (test_stop_process_null_id()) passed++; else failed++;

  /* restreamer_api_restart_process() tests */
  printf("\n--- restreamer_api_restart_process() ---\n");
  if (test_restart_process_success()) passed++; else failed++;
  if (test_restart_process_null_api()) passed++; else failed++;
  if (test_restart_process_null_id()) passed++; else failed++;

  /* restreamer_api_create_process() tests */
  printf("\n--- restreamer_api_create_process() ---\n");
  if (test_create_process_success()) passed++; else failed++;
  if (test_create_process_null_api()) passed++; else failed++;
  if (test_create_process_null_reference()) passed++; else failed++;
  if (test_create_process_null_input_url()) passed++; else failed++;
  if (test_create_process_null_output_urls()) passed++; else failed++;
  if (test_create_process_zero_output_count()) passed++; else failed++;

  /* restreamer_api_delete_process() tests */
  printf("\n--- restreamer_api_delete_process() ---\n");
  if (test_delete_process_success()) passed++; else failed++;
  if (test_delete_process_null_api()) passed++; else failed++;
  if (test_delete_process_null_id()) passed++; else failed++;

  /* restreamer_api_free_process_list() tests */
  printf("\n--- restreamer_api_free_process_list() ---\n");
  if (test_free_process_list_valid()) passed++; else failed++;
  if (test_free_process_list_null()) passed++; else failed++;
  if (test_free_process_list_empty()) passed++; else failed++;

  /* restreamer_api_free_process() tests */
  printf("\n--- restreamer_api_free_process() ---\n");
  if (test_free_process_valid()) passed++; else failed++;
  if (test_free_process_null()) passed++; else failed++;
  if (test_free_process_empty()) passed++; else failed++;

  printf("\n=== Process Management Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
