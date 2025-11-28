/*
 * API Skills and Extended Features Tests
 *
 * Tests for skills and other extended API functions including:
 * - restreamer_api_get_skills() - Get FFmpeg capabilities
 * - restreamer_api_reload_skills() - Reload skills
 * - restreamer_api_ping() - Server liveliness check
 * - restreamer_api_get_info() - API version info
 * - restreamer_api_get_logs() - Application logs
 * - restreamer_api_get_active_sessions() - Active sessions summary
 * - restreamer_api_get_process_config() - Process configuration
 * - File system operations (list, upload, download, delete)
 * - Protocol monitoring (RTMP, SRT)
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

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* ============================================================================
 * Skills API Tests
 * ========================================================================= */

/* Test: Get skills successfully */
static bool test_get_skills_success(void) {
  printf("  Testing get skills success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  /* Start mock server */
  if (!mock_restreamer_start(9870)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9870\n");
    return false;
  }

  sleep_ms(500);

  /* Create API client */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9870,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get skills */
  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(api, &skills_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_skills failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get skills successfully");
  TEST_ASSERT_NOT_NULL(skills_json, "Skills JSON should not be NULL");

  /* Verify we got valid JSON */
  if (skills_json) {
    printf("    Skills JSON: %s\n", skills_json);
    TEST_ASSERT(strlen(skills_json) > 0, "Skills JSON should not be empty");
    free(skills_json);
  }

  test_passed = true;
  printf("  ✓ Get skills success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get skills with NULL API */
static bool test_get_skills_null_api(void) {
  printf("  Testing get skills with NULL API...\n");

  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(NULL, &skills_json);

  TEST_ASSERT(!result, "Should fail with NULL API");
  TEST_ASSERT(skills_json == NULL, "Skills JSON should remain NULL");

  printf("  ✓ Get skills NULL API handling\n");
  return true;
}

/* Test: Get skills with NULL output */
static bool test_get_skills_null_output(void) {
  printf("  Testing get skills with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9871)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9871\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9871,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get skills with NULL output */
  bool result = restreamer_api_get_skills(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get skills NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Reload skills successfully */
static bool test_reload_skills_success(void) {
  printf("  Testing reload skills success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9872)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9872\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9872,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Reload skills */
  bool result = restreamer_api_reload_skills(api);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ reload_skills failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should reload skills successfully");

  test_passed = true;
  printf("  ✓ Reload skills success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Reload skills with NULL API */
static bool test_reload_skills_null_api(void) {
  printf("  Testing reload skills with NULL API...\n");

  bool result = restreamer_api_reload_skills(NULL);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Reload skills NULL API handling\n");
  return true;
}

/* ============================================================================
 * Server Info & Diagnostics Tests
 * ========================================================================= */

/* Test: Ping server successfully */
static bool test_ping_success(void) {
  printf("  Testing ping success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9873)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9873\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9873,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Ping server */
  bool result = restreamer_api_ping(api);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ ping failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should ping successfully");

  test_passed = true;
  printf("  ✓ Ping success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Ping with NULL API */
static bool test_ping_null_api(void) {
  printf("  Testing ping with NULL API...\n");

  bool result = restreamer_api_ping(NULL);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Ping NULL API handling\n");
  return true;
}

/* Test: Get API info successfully */
static bool test_get_info_success(void) {
  printf("  Testing get info success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9874)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9874\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9874,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get API info */
  restreamer_api_info_t info = {0};
  bool result = restreamer_api_get_info(api, &info);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_info failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get info successfully");

  /* Verify info fields (may be NULL depending on mock response) */
  if (info.name) {
    printf("    API name: %s\n", info.name);
  }
  if (info.version) {
    printf("    Version: %s\n", info.version);
  }

  /* Free info */
  restreamer_api_free_info(&info);

  test_passed = true;
  printf("  ✓ Get info success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get info with NULL API */
static bool test_get_info_null_api(void) {
  printf("  Testing get info with NULL API...\n");

  restreamer_api_info_t info = {0};
  bool result = restreamer_api_get_info(NULL, &info);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get info NULL API handling\n");
  return true;
}

/* Test: Get info with NULL output */
static bool test_get_info_null_output(void) {
  printf("  Testing get info with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9875)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9875\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9875,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get info with NULL output */
  bool result = restreamer_api_get_info(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get info NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Free API info with NULL (should be safe) */
static bool test_free_info_null(void) {
  printf("  Testing free info with NULL...\n");

  /* Should not crash */
  restreamer_api_free_info(NULL);

  printf("  ✓ Free info NULL safety\n");
  return true;
}

/* Test: Get logs successfully */
static bool test_get_logs_success(void) {
  printf("  Testing get logs success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9876)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9876\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9876,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get logs */
  char *logs_text = NULL;
  bool result = restreamer_api_get_logs(api, &logs_text);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_logs failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get logs successfully");

  if (logs_text) {
    printf("    Logs length: %zu bytes\n", strlen(logs_text));
    free(logs_text);
  }

  test_passed = true;
  printf("  ✓ Get logs success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get logs with NULL API */
static bool test_get_logs_null_api(void) {
  printf("  Testing get logs with NULL API...\n");

  char *logs_text = NULL;
  bool result = restreamer_api_get_logs(NULL, &logs_text);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get logs NULL API handling\n");
  return true;
}

/* Test: Get logs with NULL output */
static bool test_get_logs_null_output(void) {
  printf("  Testing get logs with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9877)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9877\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9877,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get logs with NULL output */
  bool result = restreamer_api_get_logs(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get logs NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get active sessions successfully */
static bool test_get_active_sessions_success(void) {
  printf("  Testing get active sessions success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9878)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9878\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9878,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get active sessions */
  restreamer_active_sessions_t sessions = {0};
  bool result = restreamer_api_get_active_sessions(api, &sessions);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_active_sessions failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get active sessions successfully");

  printf("    Session count: %zu\n", sessions.session_count);
  printf("    Total RX bytes: %llu\n", (unsigned long long)sessions.total_rx_bytes);
  printf("    Total TX bytes: %llu\n", (unsigned long long)sessions.total_tx_bytes);

  test_passed = true;
  printf("  ✓ Get active sessions success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get active sessions with NULL API */
static bool test_get_active_sessions_null_api(void) {
  printf("  Testing get active sessions with NULL API...\n");

  restreamer_active_sessions_t sessions = {0};
  bool result = restreamer_api_get_active_sessions(NULL, &sessions);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get active sessions NULL API handling\n");
  return true;
}

/* Test: Get active sessions with NULL output */
static bool test_get_active_sessions_null_output(void) {
  printf("  Testing get active sessions with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9879)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9879\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9879,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get active sessions with NULL output */
  bool result = restreamer_api_get_active_sessions(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get active sessions NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process config successfully */
static bool test_get_process_config_success(void) {
  printf("  Testing get process config success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9880)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9880\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9880,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get process config */
  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(api, "test-process-1", &config_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_process_config failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get process config successfully");

  if (config_json) {
    printf("    Config JSON length: %zu bytes\n", strlen(config_json));
    free(config_json);
  }

  test_passed = true;
  printf("  ✓ Get process config success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process config with NULL API */
static bool test_get_process_config_null_api(void) {
  printf("  Testing get process config with NULL API...\n");

  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(NULL, "test-process-1", &config_json);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get process config NULL API handling\n");
  return true;
}

/* Test: Get process config with NULL process ID */
static bool test_get_process_config_null_process_id(void) {
  printf("  Testing get process config with NULL process ID...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9881)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9881\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9881,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get process config with NULL process ID */
  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(api, NULL, &config_json);
  TEST_ASSERT(!result, "Should fail with NULL process ID");

  test_passed = true;
  printf("  ✓ Get process config NULL process ID handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process config with NULL output */
static bool test_get_process_config_null_output(void) {
  printf("  Testing get process config with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9882)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9882\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9882,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get process config with NULL output */
  bool result = restreamer_api_get_process_config(api, "test-process-1", NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get process config NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* ============================================================================
 * File System Operations Tests
 * ========================================================================= */

/* Test: List filesystems successfully */
static bool test_list_filesystems_success(void) {
  printf("  Testing list filesystems success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9883)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9883\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9883,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* List filesystems */
  char *filesystems_json = NULL;
  bool result = restreamer_api_list_filesystems(api, &filesystems_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ list_filesystems failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should list filesystems successfully");

  if (filesystems_json) {
    printf("    Filesystems JSON: %s\n", filesystems_json);
    free(filesystems_json);
  }

  test_passed = true;
  printf("  ✓ List filesystems success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: List filesystems with NULL API */
static bool test_list_filesystems_null_api(void) {
  printf("  Testing list filesystems with NULL API...\n");

  char *filesystems_json = NULL;
  bool result = restreamer_api_list_filesystems(NULL, &filesystems_json);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ List filesystems NULL API handling\n");
  return true;
}

/* Test: List filesystems with NULL output */
static bool test_list_filesystems_null_output(void) {
  printf("  Testing list filesystems with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9884)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9884\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9884,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to list filesystems with NULL output */
  bool result = restreamer_api_list_filesystems(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ List filesystems NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* ============================================================================
 * Protocol Monitoring Tests
 * ========================================================================= */

/* Test: Get RTMP streams successfully */
static bool test_get_rtmp_streams_success(void) {
  printf("  Testing get RTMP streams success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9885)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9885\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9885,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get RTMP streams */
  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(api, &streams_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_rtmp_streams failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get RTMP streams successfully");

  if (streams_json) {
    printf("    RTMP streams JSON: %s\n", streams_json);
    free(streams_json);
  }

  test_passed = true;
  printf("  ✓ Get RTMP streams success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get RTMP streams with NULL API */
static bool test_get_rtmp_streams_null_api(void) {
  printf("  Testing get RTMP streams with NULL API...\n");

  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(NULL, &streams_json);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get RTMP streams NULL API handling\n");
  return true;
}

/* Test: Get RTMP streams with NULL output */
static bool test_get_rtmp_streams_null_output(void) {
  printf("  Testing get RTMP streams with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9886)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9886\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9886,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get RTMP streams with NULL output */
  bool result = restreamer_api_get_rtmp_streams(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get RTMP streams NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get SRT streams successfully */
static bool test_get_srt_streams_success(void) {
  printf("  Testing get SRT streams success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9887)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9887\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9887,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get SRT streams */
  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(api, &streams_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_srt_streams failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get SRT streams successfully");

  if (streams_json) {
    printf("    SRT streams JSON: %s\n", streams_json);
    free(streams_json);
  }

  test_passed = true;
  printf("  ✓ Get SRT streams success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get SRT streams with NULL API */
static bool test_get_srt_streams_null_api(void) {
  printf("  Testing get SRT streams with NULL API...\n");

  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(NULL, &streams_json);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get SRT streams NULL API handling\n");
  return true;
}

/* Test: Get SRT streams with NULL output */
static bool test_get_srt_streams_null_output(void) {
  printf("  Testing get SRT streams with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9888)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9888\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9888,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get SRT streams with NULL output */
  bool result = restreamer_api_get_srt_streams(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get SRT streams NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* ============================================================================
 * Test Suite Runner
 * ========================================================================= */

/* Run all API skills and extended features tests */
int run_api_skills_tests(void) {
  int failed = 0;

  printf("\n========================================\n");
  printf("API Skills and Extended Features Tests\n");
  printf("========================================\n\n");

  /* Skills API tests */
  printf("Skills API Tests:\n");
  if (!test_get_skills_success()) failed++;
  if (!test_get_skills_null_api()) failed++;
  if (!test_get_skills_null_output()) failed++;
  if (!test_reload_skills_success()) failed++;
  if (!test_reload_skills_null_api()) failed++;

  /* Server info & diagnostics tests */
  printf("\nServer Info & Diagnostics Tests:\n");
  if (!test_ping_success()) failed++;
  if (!test_ping_null_api()) failed++;
  if (!test_get_info_success()) failed++;
  if (!test_get_info_null_api()) failed++;
  if (!test_get_info_null_output()) failed++;
  if (!test_free_info_null()) failed++;
  if (!test_get_logs_success()) failed++;
  if (!test_get_logs_null_api()) failed++;
  if (!test_get_logs_null_output()) failed++;
  if (!test_get_active_sessions_success()) failed++;
  if (!test_get_active_sessions_null_api()) failed++;
  if (!test_get_active_sessions_null_output()) failed++;
  if (!test_get_process_config_success()) failed++;
  if (!test_get_process_config_null_api()) failed++;
  if (!test_get_process_config_null_process_id()) failed++;
  if (!test_get_process_config_null_output()) failed++;

  /* File system operations tests */
  printf("\nFile System Operations Tests:\n");
  if (!test_list_filesystems_success()) failed++;
  if (!test_list_filesystems_null_api()) failed++;
  if (!test_list_filesystems_null_output()) failed++;

  /* Protocol monitoring tests */
  printf("\nProtocol Monitoring Tests:\n");
  if (!test_get_rtmp_streams_success()) failed++;
  if (!test_get_rtmp_streams_null_api()) failed++;
  if (!test_get_rtmp_streams_null_output()) failed++;
  if (!test_get_srt_streams_success()) failed++;
  if (!test_get_srt_streams_null_api()) failed++;
  if (!test_get_srt_streams_null_output()) failed++;

  if (failed == 0) {
    printf("\n✓ All API skills and extended features tests passed!\n");
  } else {
    printf("\n✗ %d test(s) failed\n", failed);
  }

  return failed;
}
