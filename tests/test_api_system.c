/*
 * API System & Configuration Tests
 *
 * Tests for Restreamer API system information, diagnostics, and configuration
 * management functions.
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

#define TEST_ASSERT_NULL(ptr, message)                                         \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected NULL but got %p\n    at %s:%d\n",    \
              message, (void *)(ptr), __FILE__, __LINE__);                     \
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

#define TEST_ASSERT_STR_CONTAINS(haystack, needle, message)                    \
  do {                                                                         \
    if ((haystack) == NULL || strstr((haystack), (needle)) == NULL) {          \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected to find \"%s\" in \"%s\"\n    at "   \
              "%s:%d\n",                                                       \
              message, (needle), (haystack) ? (haystack) : "(null)",           \
              __FILE__, __LINE__);                                             \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: API ping endpoint */
static bool test_api_ping(void) {
  printf("  Testing API ping endpoint...\n");

  if (!mock_restreamer_start(9850)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9850,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test ping */
  bool ping_result = restreamer_api_ping(api);
  if (!ping_result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ping failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(ping_result, "Ping should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ API ping endpoint\n");
  return true;
}

/* Test: API get_info endpoint */
static bool test_api_get_info(void) {
  printf("  Testing API get_info endpoint...\n");

  if (!mock_restreamer_start(9851)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9851,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test get_info */
  restreamer_api_info_t info = {0};
  bool result = restreamer_api_get_info(api, &info);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_info failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "get_info should succeed");

  /* Verify info structure is populated */
  TEST_ASSERT_NOT_NULL(info.name, "API name should be populated");
  TEST_ASSERT_NOT_NULL(info.version, "API version should be populated");

  printf("    API Name: %s\n", info.name);
  printf("    API Version: %s\n", info.version);

  /* Free info structure */
  restreamer_api_free_info(&info);

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ API get_info endpoint\n");
  return true;
}

/* Test: API get_logs endpoint */
static bool test_api_get_logs(void) {
  printf("  Testing API get_logs endpoint...\n");

  if (!mock_restreamer_start(9852)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9852,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test get_logs */
  char *logs_text = NULL;
  bool result = restreamer_api_get_logs(api, &logs_text);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_logs failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "get_logs should succeed");
  TEST_ASSERT_NOT_NULL(logs_text, "Logs text should be returned");

  printf("    Logs received: %zu bytes\n", strlen(logs_text));

  /* Free logs */
  if (logs_text) {
    free(logs_text);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ API get_logs endpoint\n");
  return true;
}

/* Test: API get_active_sessions endpoint */
static bool test_api_get_active_sessions(void) {
  printf("  Testing API get_active_sessions endpoint...\n");

  if (!mock_restreamer_start(9853)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9853,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test get_active_sessions */
  restreamer_active_sessions_t sessions = {0};
  bool result = restreamer_api_get_active_sessions(api, &sessions);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_active_sessions failed: %s\n",
            error ? error : "unknown error");
  }
  TEST_ASSERT(result, "get_active_sessions should succeed");

  printf("    Session count: %zu\n", sessions.session_count);
  printf("    Total RX bytes: %llu\n", (unsigned long long)sessions.total_rx_bytes);
  printf("    Total TX bytes: %llu\n", (unsigned long long)sessions.total_tx_bytes);

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ API get_active_sessions endpoint\n");
  return true;
}

/* Test: Configuration get/set/reload operations */
static bool test_api_config_management(void) {
  printf("  Testing configuration management...\n");

  if (!mock_restreamer_start(9854)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9854,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test 1: Get config */
  char *config_json = NULL;
  bool got_config = restreamer_api_get_config(api, &config_json);
  if (!got_config) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_config failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(got_config, "Should get configuration");
  TEST_ASSERT_NOT_NULL(config_json, "Config JSON should not be NULL");

  printf("    Retrieved config: %.50s...\n", config_json);

  if (config_json) {
    free(config_json);
  }

  /* Test 2: Set config */
  const char *new_config = "{\"setting\": \"new_value\", \"enabled\": true}";
  bool set_config = restreamer_api_set_config(api, new_config);
  if (!set_config) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  set_config failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(set_config, "Should set configuration");

  /* Test 3: Reload config */
  bool reloaded = restreamer_api_reload_config(api);
  if (!reloaded) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  reload_config failed: %s\n",
            error ? error : "unknown error");
  }
  TEST_ASSERT(reloaded, "Should reload configuration");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Configuration management\n");
  return true;
}

/* Test: Config operations with NULL parameters */
static bool test_api_config_null_params(void) {
  printf("  Testing config operations with NULL parameters...\n");

  if (!mock_restreamer_start(9855)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9855,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test get_config with NULL output */
  bool result = restreamer_api_get_config(api, NULL);
  TEST_ASSERT(!result, "get_config should fail with NULL output");

  /* Test set_config with NULL config */
  result = restreamer_api_set_config(api, NULL);
  TEST_ASSERT(!result, "set_config should fail with NULL config");

  /* Test with NULL API */
  result = restreamer_api_get_config(NULL, NULL);
  TEST_ASSERT(!result, "get_config should fail with NULL API");

  result = restreamer_api_set_config(NULL, "{}");
  TEST_ASSERT(!result, "set_config should fail with NULL API");

  result = restreamer_api_reload_config(NULL);
  TEST_ASSERT(!result, "reload_config should fail with NULL API");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Config NULL parameter handling\n");
  return true;
}

/* Test: Config operations with empty/invalid data */
static bool test_api_config_invalid_data(void) {
  printf("  Testing config operations with invalid data...\n");

  if (!mock_restreamer_start(9856)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9856,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test set_config with empty string */
  bool result = restreamer_api_set_config(api, "");
  /* Empty string may or may not be accepted depending on implementation */
  /* Just verify it doesn't crash */

  /* Test set_config with malformed JSON (implementation may still accept it) */
  result = restreamer_api_set_config(api, "{invalid json}");
  /* Just verify it doesn't crash */

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Config invalid data handling\n");
  return true;
}

/* Test: System diagnostic operations */
static bool test_api_diagnostics(void) {
  printf("  Testing system diagnostics...\n");

  if (!mock_restreamer_start(9857)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9857,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test ping for health check */
  bool ping_ok = restreamer_api_ping(api);
  TEST_ASSERT(ping_ok, "Ping should succeed for health check");

  /* Test get_info for version info */
  restreamer_api_info_t info = {0};
  bool info_ok = restreamer_api_get_info(api, &info);
  TEST_ASSERT(info_ok, "Should get API info");

  if (info_ok) {
    TEST_ASSERT_NOT_NULL(info.name, "Info should have name");
    TEST_ASSERT_NOT_NULL(info.version, "Info should have version");
    restreamer_api_free_info(&info);
  }

  /* Test get_logs for troubleshooting */
  char *logs = NULL;
  bool logs_ok = restreamer_api_get_logs(api, &logs);
  TEST_ASSERT(logs_ok, "Should get logs");

  if (logs) {
    TEST_ASSERT(strlen(logs) > 0, "Logs should not be empty");
    free(logs);
  }

  /* Test active sessions for monitoring */
  restreamer_active_sessions_t sessions = {0};
  bool sessions_ok = restreamer_api_get_active_sessions(api, &sessions);
  TEST_ASSERT(sessions_ok, "Should get active sessions");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ System diagnostics\n");
  return true;
}

/* Test: Ping with NULL API */
static bool test_api_ping_null(void) {
  printf("  Testing ping with NULL API...\n");

  bool result = restreamer_api_ping(NULL);
  TEST_ASSERT(!result, "ping should fail with NULL API");

  printf("  ✓ Ping NULL handling\n");
  return true;
}

/* Test: get_info with NULL parameters */
static bool test_api_get_info_null(void) {
  printf("  Testing get_info with NULL parameters...\n");

  if (!mock_restreamer_start(9858)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9858,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test with NULL info output */
  bool result = restreamer_api_get_info(api, NULL);
  TEST_ASSERT(!result, "get_info should fail with NULL output");

  /* Test with NULL API */
  restreamer_api_info_t info = {0};
  result = restreamer_api_get_info(NULL, &info);
  TEST_ASSERT(!result, "get_info should fail with NULL API");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ get_info NULL handling\n");
  return true;
}

/* Test: get_logs with NULL parameters */
static bool test_api_get_logs_null(void) {
  printf("  Testing get_logs with NULL parameters...\n");

  if (!mock_restreamer_start(9859)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9859,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test with NULL logs output */
  bool result = restreamer_api_get_logs(api, NULL);
  TEST_ASSERT(!result, "get_logs should fail with NULL output");

  /* Test with NULL API */
  char *logs = NULL;
  result = restreamer_api_get_logs(NULL, &logs);
  TEST_ASSERT(!result, "get_logs should fail with NULL API");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ get_logs NULL handling\n");
  return true;
}

/* Test: get_active_sessions with NULL parameters */
static bool test_api_get_active_sessions_null(void) {
  printf("  Testing get_active_sessions with NULL parameters...\n");

  if (!mock_restreamer_start(9860)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9860,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test with NULL sessions output */
  bool result = restreamer_api_get_active_sessions(api, NULL);
  TEST_ASSERT(!result, "get_active_sessions should fail with NULL output");

  /* Test with NULL API */
  restreamer_active_sessions_t sessions = {0};
  result = restreamer_api_get_active_sessions(NULL, &sessions);
  TEST_ASSERT(!result, "get_active_sessions should fail with NULL API");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ get_active_sessions NULL handling\n");
  return true;
}

/* Test: Multiple rapid config changes */
static bool test_api_config_rapid_changes(void) {
  printf("  Testing rapid config changes...\n");

  if (!mock_restreamer_start(9861)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9861,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Perform multiple config operations rapidly */
  for (int i = 0; i < 5; i++) {
    char config[128];
    snprintf(config, sizeof(config), "{\"iteration\": %d, \"enabled\": true}", i);

    bool set_ok = restreamer_api_set_config(api, config);
    TEST_ASSERT(set_ok, "Config set should succeed");

    char *retrieved_config = NULL;
    bool get_ok = restreamer_api_get_config(api, &retrieved_config);
    TEST_ASSERT(get_ok, "Config get should succeed");

    if (retrieved_config) {
      free(retrieved_config);
    }

    bool reload_ok = restreamer_api_reload_config(api);
    TEST_ASSERT(reload_ok, "Config reload should succeed");
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Rapid config changes\n");
  return true;
}

/* Test: Info structure free with NULL */
static bool test_api_free_info_null(void) {
  printf("  Testing free_info with NULL...\n");

  /* Should not crash */
  restreamer_api_free_info(NULL);

  printf("  ✓ free_info NULL handling\n");
  return true;
}

/* Test: Connection state during diagnostics */
static bool test_api_diagnostics_connection_state(void) {
  printf("  Testing diagnostics with various connection states...\n");

  if (!mock_restreamer_start(9862)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9862,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test diagnostics before connection */
  bool ping1 = restreamer_api_ping(api);
  /* May succeed or fail depending on implementation */
  (void)ping1;

  /* Connect */
  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to server");

  /* Test diagnostics after connection */
  bool ping2 = restreamer_api_ping(api);
  TEST_ASSERT(ping2, "Ping should succeed after connection");

  restreamer_api_info_t info = {0};
  bool info_ok = restreamer_api_get_info(api, &info);
  TEST_ASSERT(info_ok, "get_info should succeed after connection");

  if (info_ok) {
    restreamer_api_free_info(&info);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Diagnostics connection state\n");
  return true;
}

/* Run all API system tests */
bool run_api_system_tests(void) {
  bool all_passed = true;

  printf("\nAPI System & Configuration Tests\n");
  printf("========================================\n");

  all_passed &= test_api_ping();
  all_passed &= test_api_get_info();
  all_passed &= test_api_get_logs();
  all_passed &= test_api_get_active_sessions();
  all_passed &= test_api_config_management();
  all_passed &= test_api_config_null_params();
  all_passed &= test_api_config_invalid_data();
  all_passed &= test_api_diagnostics();
  all_passed &= test_api_ping_null();
  all_passed &= test_api_get_info_null();
  all_passed &= test_api_get_logs_null();
  all_passed &= test_api_get_active_sessions_null();
  all_passed &= test_api_config_rapid_changes();
  all_passed &= test_api_free_info_null();
  all_passed &= test_api_diagnostics_connection_state();

  return all_passed;
}
