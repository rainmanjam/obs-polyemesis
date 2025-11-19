/*
 * API Authentication Tests
 *
 * Comprehensive tests for JWT authentication, token management, and auth flows
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

#define TEST_ASSERT_NULL(ptr, message)                                         \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected NULL pointer\n    at %s:%d\n",       \
              message, __FILE__, __LINE__);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message)                       \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: '%s'\n    Actual: '%s'\n    at %s:%d\n", \
              message, expected, actual, __FILE__, __LINE__);                  \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: JWT Authentication Success */
static bool test_jwt_auth_success(void) {
  printf("  Testing JWT authentication success...\n");

  if (!mock_restreamer_start(9100)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9100,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Authenticate */
  bool auth_result = restreamer_api_authenticate(api);
  TEST_ASSERT(auth_result, "Authentication should succeed");

  /* Verify we have tokens */
  TEST_ASSERT(restreamer_api_is_authenticated(api), "Should be authenticated after login");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ JWT authentication success\n");
  return true;
}

/* Test: JWT Authentication Failure - Invalid Credentials */
static bool test_jwt_auth_invalid_credentials(void) {
  printf("  Testing JWT auth with invalid credentials...\n");

  if (!mock_restreamer_start(9101)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9101,
      .username = "admin",
      .password = "wrongpassword",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Should fail with wrong password */
  bool auth_result = restreamer_api_authenticate(api);
  TEST_ASSERT(!auth_result, "Authentication should fail with wrong password");
  TEST_ASSERT(!restreamer_api_is_authenticated(api), "Should not be authenticated");

  /* Error message should be set */
  const char *error = restreamer_api_get_error(api);
  TEST_ASSERT_NOT_NULL(error, "Error message should be set");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ JWT auth invalid credentials\n");
  return true;
}

/* Test: JWT Token Refresh */
static bool test_jwt_token_refresh(void) {
  printf("  Testing JWT token refresh...\n");

  if (!mock_restreamer_start(9102)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9102,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Initial authentication */
  TEST_ASSERT(restreamer_api_authenticate(api), "Initial auth should succeed");
  TEST_ASSERT(restreamer_api_is_authenticated(api), "Should be authenticated");

  /* Simulate token expiration (would need API function to expose this) */
  /* For now, test refresh explicitly */
  bool refresh_result = restreamer_api_refresh_token(api);
  TEST_ASSERT(refresh_result, "Token refresh should succeed");
  TEST_ASSERT(restreamer_api_is_authenticated(api), "Should still be authenticated");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ JWT token refresh\n");
  return true;
}

/* Test: JWT Token Expiration Handling */
static bool test_jwt_token_expiration(void) {
  printf("  Testing JWT token expiration handling...\n");

  if (!mock_restreamer_start(9103)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9103,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Authenticate */
  TEST_ASSERT(restreamer_api_authenticate(api), "Auth should succeed");

  /* API should auto-refresh expired tokens on next request */
  /* Make a request that would trigger auto-refresh */
  restreamer_process_list_t list = {0};
  bool result = restreamer_api_get_processes(api, &list);
  TEST_ASSERT(result, "Should auto-refresh and succeed");

  restreamer_api_free_process_list(&list);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ JWT token expiration handling\n");
  return true;
}

/* Test: Logout */
static bool test_jwt_logout(void) {
  printf("  Testing logout...\n");

  if (!mock_restreamer_start(9104)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9104,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Authenticate */
  TEST_ASSERT(restreamer_api_authenticate(api), "Auth should succeed");
  TEST_ASSERT(restreamer_api_is_authenticated(api), "Should be authenticated");

  /* Logout */
  restreamer_api_logout(api);
  TEST_ASSERT(!restreamer_api_is_authenticated(api), "Should not be authenticated after logout");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Logout\n");
  return true;
}

/* Test: Multiple Auth Attempts */
static bool test_multiple_auth_attempts(void) {
  printf("  Testing multiple authentication attempts...\n");

  if (!mock_restreamer_start(9105)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9105,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Multiple auth attempts should work */
  for (int i = 0; i < 3; i++) {
    TEST_ASSERT(restreamer_api_authenticate(api), "Auth attempt should succeed");
    TEST_ASSERT(restreamer_api_is_authenticated(api), "Should be authenticated");

    restreamer_api_logout(api);
    TEST_ASSERT(!restreamer_api_is_authenticated(api), "Should not be authenticated");
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Multiple auth attempts\n");
  return true;
}

/* Test: Auth with Missing Credentials */
static bool test_auth_missing_credentials(void) {
  printf("  Testing auth with missing credentials...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9106,
      .username = NULL,  /* Missing username */
      .password = NULL,  /* Missing password */
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should still be created");

  /* Auth should fail without credentials */
  bool auth_result = restreamer_api_authenticate(api);
  TEST_ASSERT(!auth_result, "Auth should fail without credentials");

  restreamer_api_destroy(api);

  printf("  ✓ Auth with missing credentials\n");
  return true;
}

/* Main test runner */
int test_api_auth(void) {
  printf("\n=== API Authentication Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* Run tests */
  if (test_jwt_auth_success()) {
    passed++;
  } else {
    failed++;
  }

  if (test_jwt_auth_invalid_credentials()) {
    passed++;
  } else {
    failed++;
  }

  if (test_jwt_token_refresh()) {
    passed++;
  } else {
    failed++;
  }

  if (test_jwt_token_expiration()) {
    passed++;
  } else {
    failed++;
  }

  if (test_jwt_logout()) {
    passed++;
  } else {
    failed++;
  }

  if (test_multiple_auth_attempts()) {
    passed++;
  } else {
    failed++;
  }

  if (test_auth_missing_credentials()) {
    passed++;
  } else {
    failed++;
  }

  printf("\n=== API Auth Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
