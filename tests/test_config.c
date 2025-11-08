/*
 * Configuration Tests
 *
 * Tests for configuration management
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "restreamer-config.h"

/* Test macros */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
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

/* Test: Global configuration initialization */
static bool test_global_config_init(void) {
  printf("  Testing global configuration initialization...\n");

  /* Initialize config */
  restreamer_config_init();

  /* Get global connection - should return non-NULL */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Global connection should be initialized");

  /* Default values should be set */
  TEST_ASSERT(conn->host != NULL, "Host should be initialized");
  TEST_ASSERT(conn->port > 0, "Port should be initialized");

  /* Clean up */
  restreamer_config_destroy();

  printf("  ✓ Global configuration initialization\n");
  return true;
}

/* Test: Set and get global connection */
static bool test_global_connection(void) {
  printf("  Testing global connection settings...\n");

  restreamer_config_init();

  /* Create a test connection */
  restreamer_connection_t test_conn = {
      .host = "192.168.1.100",
      .port = 8080,
      .username = "admin",
      .password = "secretpass",
      .use_https = true,
  };

  /* Set global connection */
  restreamer_config_set_global_connection(&test_conn);

  /* Get it back */
  restreamer_connection_t *conn = restreamer_config_get_global_connection();
  TEST_ASSERT(conn != NULL, "Should get global connection");
  TEST_ASSERT_STR_EQUAL("192.168.1.100", conn->host, "Host should match");
  TEST_ASSERT_EQUAL(8080, conn->port, "Port should match");
  TEST_ASSERT_STR_EQUAL("admin", conn->username, "Username should match");
  TEST_ASSERT_STR_EQUAL("secretpass", conn->password, "Password should match");
  TEST_ASSERT(conn->use_https, "HTTPS should be enabled");

  restreamer_config_destroy();

  printf("  ✓ Global connection settings\n");
  return true;
}

/* Test: Create API from global config */
static bool test_create_global_api(void) {
  printf("  Testing create API from global config...\n");

  restreamer_config_init();

  /* Set up a test connection */
  restreamer_connection_t test_conn = {
      .host = "localhost",
      .port = 8080,
      .username = "test",
      .password = "test",
      .use_https = false,
  };
  restreamer_config_set_global_connection(&test_conn);

  /* Create API from global config */
  restreamer_api_t *api = restreamer_config_create_global_api();
  TEST_ASSERT(api != NULL, "Should create API from global config");

  /* Clean up */
  restreamer_api_destroy(api);
  restreamer_config_destroy();

  printf("  ✓ Create API from global config\n");
  return true;
}

/* Run all configuration tests */
bool run_config_tests(void) {
  bool all_passed = true;

  all_passed &= test_global_config_init();
  all_passed &= test_global_connection();
  all_passed &= test_create_global_api();

  return all_passed;
}
