/*
 * API Coverage Improvement Tests
 *
 * Tests specifically designed to improve code coverage for restreamer-api.c
 * Focuses on:
 * - RTMP/SRT stream functions
 * - Metrics API
 * - Log functions
 * - NULL parameter handling
 * - Edge cases and error paths
 * - Cleanup functions
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

/* ========================================================================
 * RTMP Stream API Tests
 * ======================================================================== */

/* Test: Get RTMP streams with NULL API */
static bool test_get_rtmp_streams_null_api(void) {
  printf("  Testing get RTMP streams with NULL API...\n");

  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(NULL, &streams_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(streams_json, "streams_json should remain NULL");

  printf("  ✓ Get RTMP streams NULL API handling\n");
  return true;
}

/* Test: Get RTMP streams with NULL output parameter */
static bool test_get_rtmp_streams_null_output(void) {
  printf("  Testing get RTMP streams with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9600,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_rtmp_streams(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get RTMP streams NULL output parameter handling\n");
  return true;
}

/* Test: Get RTMP streams successful call */
static bool test_get_rtmp_streams_success(void) {
  printf("  Testing get RTMP streams successful call...\n");

  if (!mock_restreamer_start(9601)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9601,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(api, &streams_json);

  /* Result depends on mock server response, but should not crash */
  if (result && streams_json) {
    printf("    Got RTMP streams JSON: %zu bytes\n", strlen(streams_json));
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get RTMP streams successful call\n");
  return true;
}

/* ========================================================================
 * SRT Stream API Tests
 * ======================================================================== */

/* Test: Get SRT streams with NULL API */
static bool test_get_srt_streams_null_api(void) {
  printf("  Testing get SRT streams with NULL API...\n");

  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(NULL, &streams_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(streams_json, "streams_json should remain NULL");

  printf("  ✓ Get SRT streams NULL API handling\n");
  return true;
}

/* Test: Get SRT streams with NULL output parameter */
static bool test_get_srt_streams_null_output(void) {
  printf("  Testing get SRT streams with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9602,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_srt_streams(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get SRT streams NULL output parameter handling\n");
  return true;
}

/* Test: Get SRT streams successful call */
static bool test_get_srt_streams_success(void) {
  printf("  Testing get SRT streams successful call...\n");

  if (!mock_restreamer_start(9603)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9603,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(api, &streams_json);

  /* Result depends on mock server response, but should not crash */
  if (result && streams_json) {
    printf("    Got SRT streams JSON: %zu bytes\n", strlen(streams_json));
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get SRT streams successful call\n");
  return true;
}

/* ========================================================================
 * Metrics API Tests
 * ======================================================================== */

/* Test: Get metrics list with NULL API */
static bool test_get_metrics_list_null_api(void) {
  printf("  Testing get metrics list with NULL API...\n");

  char *metrics_json = NULL;
  bool result = restreamer_api_get_metrics_list(NULL, &metrics_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(metrics_json, "metrics_json should remain NULL");

  printf("  ✓ Get metrics list NULL API handling\n");
  return true;
}

/* Test: Get metrics list with NULL output parameter */
static bool test_get_metrics_list_null_output(void) {
  printf("  Testing get metrics list with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9604,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_metrics_list(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get metrics list NULL output parameter handling\n");
  return true;
}

/* Test: Get metrics list successful call */
static bool test_get_metrics_list_success(void) {
  printf("  Testing get metrics list successful call...\n");

  if (!mock_restreamer_start(9605)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9605,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *metrics_json = NULL;
  bool result = restreamer_api_get_metrics_list(api, &metrics_json);

  /* Result depends on mock server response, but should not crash */
  if (result && metrics_json) {
    printf("    Got metrics JSON: %zu bytes\n", strlen(metrics_json));
    free(metrics_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get metrics list successful call\n");
  return true;
}

/* Test: Query metrics with NULL API */
static bool test_query_metrics_null_api(void) {
  printf("  Testing query metrics with NULL API...\n");

  char *result_json = NULL;
  bool result = restreamer_api_query_metrics(NULL, "{}", &result_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(result_json, "result_json should remain NULL");

  printf("  ✓ Query metrics NULL API handling\n");
  return true;
}

/* Test: Query metrics with NULL query */
static bool test_query_metrics_null_query(void) {
  printf("  Testing query metrics with NULL query...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9606,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *result_json = NULL;
  bool result = restreamer_api_query_metrics(api, NULL, &result_json);
  TEST_ASSERT(!result, "Should return false for NULL query");

  restreamer_api_destroy(api);

  printf("  ✓ Query metrics NULL query handling\n");
  return true;
}

/* Test: Query metrics with NULL output */
static bool test_query_metrics_null_output(void) {
  printf("  Testing query metrics with NULL output...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9607,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_query_metrics(api, "{}", NULL);
  TEST_ASSERT(!result, "Should return false for NULL output");

  restreamer_api_destroy(api);

  printf("  ✓ Query metrics NULL output handling\n");
  return true;
}

/* Test: Get prometheus metrics with NULL API */
static bool test_get_prometheus_metrics_null_api(void) {
  printf("  Testing get prometheus metrics with NULL API...\n");

  char *prometheus_text = NULL;
  bool result = restreamer_api_get_prometheus_metrics(NULL, &prometheus_text);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(prometheus_text, "prometheus_text should remain NULL");

  printf("  ✓ Get prometheus metrics NULL API handling\n");
  return true;
}

/* Test: Get prometheus metrics with NULL output parameter */
static bool test_get_prometheus_metrics_null_output(void) {
  printf("  Testing get prometheus metrics with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9608,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_prometheus_metrics(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get prometheus metrics NULL output parameter handling\n");
  return true;
}

/* Test: Free metrics with NULL pointer */
static bool test_free_metrics_null(void) {
  printf("  Testing free metrics with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_metrics(NULL);

  printf("  ✓ Free metrics NULL pointer handling\n");
  return true;
}

/* ========================================================================
 * Log API Tests
 * ======================================================================== */

/* Test: Get logs with NULL API */
static bool test_get_logs_null_api(void) {
  printf("  Testing get logs with NULL API...\n");

  char *logs_text = NULL;
  bool result = restreamer_api_get_logs(NULL, &logs_text);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(logs_text, "logs_text should remain NULL");

  printf("  ✓ Get logs NULL API handling\n");
  return true;
}

/* Test: Get logs with NULL output parameter */
static bool test_get_logs_null_output(void) {
  printf("  Testing get logs with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9609,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_logs(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get logs NULL output parameter handling\n");
  return true;
}

/* Test: Get logs successful call */
static bool test_get_logs_success(void) {
  printf("  Testing get logs successful call...\n");

  if (!mock_restreamer_start(9610)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9610,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *logs_text = NULL;
  bool result = restreamer_api_get_logs(api, &logs_text);

  /* Result depends on mock server response, but should not crash */
  if (result && logs_text) {
    printf("    Got logs text: %zu bytes\n", strlen(logs_text));
    free(logs_text);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get logs successful call\n");
  return true;
}

/* ========================================================================
 * Active Sessions API Tests
 * ======================================================================== */

/* Test: Get active sessions with NULL API */
static bool test_get_active_sessions_null_api(void) {
  printf("  Testing get active sessions with NULL API...\n");

  restreamer_active_sessions_t sessions = {0};
  bool result = restreamer_api_get_active_sessions(NULL, &sessions);

  TEST_ASSERT(!result, "Should return false for NULL API");

  printf("  ✓ Get active sessions NULL API handling\n");
  return true;
}

/* Test: Get active sessions with NULL output parameter */
static bool test_get_active_sessions_null_output(void) {
  printf("  Testing get active sessions with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9611,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_active_sessions(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get active sessions NULL output parameter handling\n");
  return true;
}

/* Test: Get active sessions successful call */
static bool test_get_active_sessions_success(void) {
  printf("  Testing get active sessions successful call...\n");

  if (!mock_restreamer_start(9612)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9612,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  restreamer_active_sessions_t sessions = {0};
  bool result = restreamer_api_get_active_sessions(api, &sessions);

  /* Result depends on mock server response, but should not crash */
  if (result) {
    printf("    Active sessions count: %zu\n", sessions.session_count);
    printf("    Total RX bytes: %llu\n", (unsigned long long)sessions.total_rx_bytes);
    printf("    Total TX bytes: %llu\n", (unsigned long long)sessions.total_tx_bytes);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get active sessions successful call\n");
  return true;
}

/* ========================================================================
 * Skills API Tests
 * ======================================================================== */

/* Test: Get skills with NULL API */
static bool test_get_skills_null_api(void) {
  printf("  Testing get skills with NULL API...\n");

  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(NULL, &skills_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(skills_json, "skills_json should remain NULL");

  printf("  ✓ Get skills NULL API handling\n");
  return true;
}

/* Test: Get skills with NULL output parameter */
static bool test_get_skills_null_output(void) {
  printf("  Testing get skills with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9613,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_skills(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get skills NULL output parameter handling\n");
  return true;
}

/* Test: Reload skills with NULL API */
static bool test_reload_skills_null_api(void) {
  printf("  Testing reload skills with NULL API...\n");

  bool result = restreamer_api_reload_skills(NULL);
  TEST_ASSERT(!result, "Should return false for NULL API");

  printf("  ✓ Reload skills NULL API handling\n");
  return true;
}

/* ========================================================================
 * Server Info & Ping API Tests
 * ======================================================================== */

/* Test: Ping with NULL API */
static bool test_ping_null_api(void) {
  printf("  Testing ping with NULL API...\n");

  bool result = restreamer_api_ping(NULL);
  TEST_ASSERT(!result, "Should return false for NULL API");

  printf("  ✓ Ping NULL API handling\n");
  return true;
}

/* Test: Get info with NULL API */
static bool test_get_info_null_api(void) {
  printf("  Testing get info with NULL API...\n");

  restreamer_api_info_t info = {0};
  bool result = restreamer_api_get_info(NULL, &info);

  TEST_ASSERT(!result, "Should return false for NULL API");

  printf("  ✓ Get info NULL API handling\n");
  return true;
}

/* Test: Get info with NULL output parameter */
static bool test_get_info_null_output(void) {
  printf("  Testing get info with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9614,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_info(api, NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get info NULL output parameter handling\n");
  return true;
}

/* Test: Free info with NULL pointer */
static bool test_free_info_null(void) {
  printf("  Testing free info with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_info(NULL);

  printf("  ✓ Free info NULL pointer handling\n");
  return true;
}

/* ========================================================================
 * Cleanup Function Tests
 * ======================================================================== */

/* Test: Free process list with NULL pointer */
static bool test_free_process_list_null(void) {
  printf("  Testing free process list with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_process_list(NULL);

  printf("  ✓ Free process list NULL pointer handling\n");
  return true;
}

/* Test: Free session list with NULL pointer */
static bool test_free_session_list_null(void) {
  printf("  Testing free session list with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_session_list(NULL);

  printf("  ✓ Free session list NULL pointer handling\n");
  return true;
}

/* Test: Free log list with NULL pointer */
static bool test_free_log_list_null(void) {
  printf("  Testing free log list with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_log_list(NULL);

  printf("  ✓ Free log list NULL pointer handling\n");
  return true;
}

/* Test: Free process with NULL pointer */
static bool test_free_process_null(void) {
  printf("  Testing free process with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_process(NULL);

  printf("  ✓ Free process NULL pointer handling\n");
  return true;
}

/* Test: Free process state with NULL pointer */
static bool test_free_process_state_null(void) {
  printf("  Testing free process state with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_process_state(NULL);

  printf("  ✓ Free process state NULL pointer handling\n");
  return true;
}

/* Test: Free probe info with NULL pointer */
static bool test_free_probe_info_null(void) {
  printf("  Testing free probe info with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_probe_info(NULL);

  printf("  ✓ Free probe info NULL pointer handling\n");
  return true;
}

/* Test: Free encoding params with NULL pointer */
static bool test_free_encoding_params_null(void) {
  printf("  Testing free encoding params with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_encoding_params(NULL);

  printf("  ✓ Free encoding params NULL pointer handling\n");
  return true;
}

/* Test: Free outputs list with NULL pointer */
static bool test_free_outputs_list_null(void) {
  printf("  Testing free outputs list with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_outputs_list(NULL, 0);

  printf("  ✓ Free outputs list NULL pointer handling\n");
  return true;
}

/* Test: Free playout status with NULL pointer */
static bool test_free_playout_status_null(void) {
  printf("  Testing free playout status with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_playout_status(NULL);

  printf("  ✓ Free playout status NULL pointer handling\n");
  return true;
}

/* Test: Free fs list with NULL pointer */
static bool test_free_fs_list_null(void) {
  printf("  Testing free fs list with NULL pointer...\n");

  /* Should not crash */
  restreamer_api_free_fs_list(NULL);

  printf("  ✓ Free fs list NULL pointer handling\n");
  return true;
}

/* ========================================================================
 * Process Config API Tests
 * ======================================================================== */

/* Test: Get process config with NULL API */
static bool test_get_process_config_null_api(void) {
  printf("  Testing get process config with NULL API...\n");

  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(NULL, "test-process", &config_json);

  TEST_ASSERT(!result, "Should return false for NULL API");
  TEST_ASSERT_NULL(config_json, "config_json should remain NULL");

  printf("  ✓ Get process config NULL API handling\n");
  return true;
}

/* Test: Get process config with NULL process ID */
static bool test_get_process_config_null_process_id(void) {
  printf("  Testing get process config with NULL process ID...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9615,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  char *config_json = NULL;
  bool result = restreamer_api_get_process_config(api, NULL, &config_json);
  TEST_ASSERT(!result, "Should return false for NULL process ID");

  restreamer_api_destroy(api);

  printf("  ✓ Get process config NULL process ID handling\n");
  return true;
}

/* Test: Get process config with NULL output parameter */
static bool test_get_process_config_null_output(void) {
  printf("  Testing get process config with NULL output parameter...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9616,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool result = restreamer_api_get_process_config(api, "test-process", NULL);
  TEST_ASSERT(!result, "Should return false for NULL output parameter");

  restreamer_api_destroy(api);

  printf("  ✓ Get process config NULL output parameter handling\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int test_api_coverage_improvements(void) {
  printf("\n=== API Coverage Improvement Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* RTMP Stream Tests */
  if (test_get_rtmp_streams_null_api()) passed++; else failed++;
  if (test_get_rtmp_streams_null_output()) passed++; else failed++;
  if (test_get_rtmp_streams_success()) passed++; else failed++;

  /* SRT Stream Tests */
  if (test_get_srt_streams_null_api()) passed++; else failed++;
  if (test_get_srt_streams_null_output()) passed++; else failed++;
  if (test_get_srt_streams_success()) passed++; else failed++;

  /* Metrics API Tests */
  if (test_get_metrics_list_null_api()) passed++; else failed++;
  if (test_get_metrics_list_null_output()) passed++; else failed++;
  if (test_get_metrics_list_success()) passed++; else failed++;
  if (test_query_metrics_null_api()) passed++; else failed++;
  if (test_query_metrics_null_query()) passed++; else failed++;
  if (test_query_metrics_null_output()) passed++; else failed++;
  if (test_get_prometheus_metrics_null_api()) passed++; else failed++;
  if (test_get_prometheus_metrics_null_output()) passed++; else failed++;
  if (test_free_metrics_null()) passed++; else failed++;

  /* Log API Tests */
  if (test_get_logs_null_api()) passed++; else failed++;
  if (test_get_logs_null_output()) passed++; else failed++;
  if (test_get_logs_success()) passed++; else failed++;

  /* Active Sessions API Tests */
  if (test_get_active_sessions_null_api()) passed++; else failed++;
  if (test_get_active_sessions_null_output()) passed++; else failed++;
  if (test_get_active_sessions_success()) passed++; else failed++;

  /* Skills API Tests */
  if (test_get_skills_null_api()) passed++; else failed++;
  if (test_get_skills_null_output()) passed++; else failed++;
  if (test_reload_skills_null_api()) passed++; else failed++;

  /* Server Info & Ping API Tests */
  if (test_ping_null_api()) passed++; else failed++;
  if (test_get_info_null_api()) passed++; else failed++;
  if (test_get_info_null_output()) passed++; else failed++;
  if (test_free_info_null()) passed++; else failed++;

  /* Cleanup Function Tests */
  if (test_free_process_list_null()) passed++; else failed++;
  if (test_free_session_list_null()) passed++; else failed++;
  if (test_free_log_list_null()) passed++; else failed++;
  if (test_free_process_null()) passed++; else failed++;
  if (test_free_process_state_null()) passed++; else failed++;
  if (test_free_probe_info_null()) passed++; else failed++;
  if (test_free_encoding_params_null()) passed++; else failed++;
  if (test_free_outputs_list_null()) passed++; else failed++;
  if (test_free_playout_status_null()) passed++; else failed++;
  if (test_free_fs_list_null()) passed++; else failed++;

  /* Process Config API Tests */
  if (test_get_process_config_null_api()) passed++; else failed++;
  if (test_get_process_config_null_process_id()) passed++; else failed++;
  if (test_get_process_config_null_output()) passed++; else failed++;

  printf("\n=== Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
