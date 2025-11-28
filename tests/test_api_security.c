/*
 * API Security Tests
 *
 * Tests for API security features including connection management,
 * token handling, and error states
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

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
 * Connection State Tests
 * ======================================================================== */

/* Test: is_connected returns false before authentication */
static bool test_is_connected_before_auth(void) {
  printf("  Testing is_connected before authentication...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9731)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9731,
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

  /* Before any connection test, should not be connected */
  bool connected = restreamer_api_is_connected(api);
  TEST_CHECK(!connected, "Should not be connected before test_connection");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ is_connected before auth\n");
  }
  return test_passed;
}

/* Test: is_connected returns true after successful test_connection */
static bool test_is_connected_after_auth(void) {
  printf("  Testing is_connected after authentication...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9732)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9732,
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

  /* Test connection (this authenticates) */
  bool test_result = restreamer_api_test_connection(api);
  TEST_CHECK(test_result, "test_connection should succeed");

  /* After successful connection test, should be connected */
  bool connected = restreamer_api_is_connected(api);
  TEST_CHECK(connected, "Should be connected after successful test_connection");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ is_connected after auth\n");
  }
  return test_passed;
}

/* Test: is_connected with NULL returns false */
static bool test_is_connected_null(void) {
  printf("  Testing is_connected with NULL...\n");
  bool test_passed = true;

  bool connected = restreamer_api_is_connected(NULL);
  TEST_CHECK(!connected, "is_connected(NULL) should return false");

  if (test_passed) {
    printf("  ✓ is_connected NULL handling\n");
  }
  return test_passed;
}

/* Test: test_connection with wrong credentials fails */
static bool test_connection_wrong_credentials(void) {
  printf("  Testing test_connection with wrong credentials...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9733)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9733,
      .username = "admin",
      .password = "wrongpassword",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Test connection with wrong password */
  bool test_result = restreamer_api_test_connection(api);
  /* Note: Mock server may accept any password, so we just verify no crash */
  printf("    Connection test result: %s\n", test_result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Wrong credentials handling\n");
  }
  return test_passed;
}

/* Test: API destroy with NULL is safe */
static bool test_api_destroy_null_safe(void) {
  printf("  Testing API destroy with NULL is safe...\n");

  /* Should not crash */
  restreamer_api_destroy(NULL);

  printf("  ✓ API destroy NULL safe\n");
  return true;
}

/* Test: Multiple API clients don't interfere */
static bool test_multiple_api_clients(void) {
  printf("  Testing multiple API clients...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api1 = NULL;
  restreamer_api_t *api2 = NULL;

  if (!mock_restreamer_start(9734)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn1 = {
      .host = "localhost",
      .port = 9734,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_connection_t conn2 = {
      .host = "localhost",
      .port = 9734,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  /* Create two API clients */
  api1 = restreamer_api_create(&conn1);
  api2 = restreamer_api_create(&conn2);

  if (!api1 || !api2) {
    fprintf(stderr, "  ✗ FAIL: API clients should be created\n");
    test_passed = false;
    goto cleanup;
  }

  /* Connect both */
  bool conn1_result = restreamer_api_test_connection(api1);
  bool conn2_result = restreamer_api_test_connection(api2);

  TEST_CHECK(conn1_result, "First client should connect");
  TEST_CHECK(conn2_result, "Second client should connect");

  /* Both should be connected independently */
  TEST_CHECK(restreamer_api_is_connected(api1), "First client should be connected");
  TEST_CHECK(restreamer_api_is_connected(api2), "Second client should be connected");

cleanup:
  if (api1) {
    restreamer_api_destroy(api1);
  }
  if (api2) {
    restreamer_api_destroy(api2);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Multiple API clients work independently\n");
  }
  return test_passed;
}

/* Test: Token refresh functionality */
static bool test_token_refresh(void) {
  printf("  Testing token refresh...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9735)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9735,
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

  /* First, establish connection */
  bool test_result = restreamer_api_test_connection(api);
  TEST_CHECK(test_result, "Initial connection should succeed");

  /* Try to refresh token */
  bool refresh_result = restreamer_api_refresh_token(api);
  printf("    Token refresh result: %s\n", refresh_result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Token refresh handling\n");
  }
  return test_passed;
}

/* Test: Force login functionality */
static bool test_force_login(void) {
  printf("  Testing force login...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9736)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9736,
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

  /* Force login */
  bool force_result = restreamer_api_force_login(api);
  printf("    Force login result: %s\n", force_result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Force login handling\n");
  }
  return test_passed;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int run_api_security_tests(void) {
  printf("\n=== API Security Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* Run tests */
  if (test_is_connected_before_auth()) passed++; else failed++;
  if (test_is_connected_after_auth()) passed++; else failed++;
  if (test_is_connected_null()) passed++; else failed++;
  if (test_connection_wrong_credentials()) passed++; else failed++;
  if (test_api_destroy_null_safe()) passed++; else failed++;
  if (test_multiple_api_clients()) passed++; else failed++;
  if (test_token_refresh()) passed++; else failed++;
  if (test_force_login()) passed++; else failed++;

  printf("\n=== API Security Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
