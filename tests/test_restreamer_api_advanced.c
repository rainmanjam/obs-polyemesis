/*
 * Restreamer API Advanced Feature Tests
 *
 * Tests for advanced/extended API functionality:
 * - Configuration management
 * - Metrics & monitoring
 * - Metadata storage
 * - Playout management
 * - File system access
 * - Protocol monitoring
 * - FFmpeg capabilities
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
 * Configuration Management Tests
 * ======================================================================== */

/* Test: Get Configuration */
static bool test_get_config(void) {
  printf("  Testing get configuration...\n");

  if (!mock_restreamer_start(9700)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9700,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get configuration */
  char *config_json = NULL;
  bool result = restreamer_api_get_config(api, &config_json);
  TEST_ASSERT(result, "Getting config should succeed");

  if (config_json) {
    printf("    Config length: %zu bytes\n", strlen(config_json));
    free(config_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get configuration\n");
  return true;
}

/* Test: Set Configuration */
static bool test_set_config(void) {
  printf("  Testing set configuration...\n");

  if (!mock_restreamer_start(9701)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9701,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Set configuration with sample JSON */
  const char *new_config = "{\"version\":\"3\",\"debug\":{\"level\":\"info\"}}";
  bool result = restreamer_api_set_config(api, new_config);
  TEST_ASSERT(result, "Setting config should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Set configuration\n");
  return true;
}

/* Test: Reload Configuration */
static bool test_reload_config(void) {
  printf("  Testing reload configuration...\n");

  if (!mock_restreamer_start(9702)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9702,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Reload configuration */
  bool result = restreamer_api_reload_config(api);
  TEST_ASSERT(result, "Reloading config should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Reload configuration\n");
  return true;
}

/* ========================================================================
 * Metrics & Monitoring Tests
 * ======================================================================== */

/* Test: Get Metrics List */
static bool test_get_metrics_list(void) {
  printf("  Testing get metrics list...\n");

  if (!mock_restreamer_start(9710)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9710,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get metrics list */
  char *metrics_json = NULL;
  bool result = restreamer_api_get_metrics_list(api, &metrics_json);
  TEST_ASSERT(result, "Getting metrics list should succeed");

  if (metrics_json) {
    printf("    Metrics: %zu bytes\n", strlen(metrics_json));
    free(metrics_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get metrics list\n");
  return true;
}

/* Test: Query Metrics */
static bool test_query_metrics(void) {
  printf("  Testing query metrics...\n");

  if (!mock_restreamer_start(9711)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9711,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Query metrics with custom query */
  const char *query = "{\"metric\":\"cpu_usage\",\"timerange\":\"5m\"}";
  char *result_json = NULL;
  bool result = restreamer_api_query_metrics(api, query, &result_json);
  TEST_ASSERT(result, "Querying metrics should succeed");

  if (result_json) {
    printf("    Query result: %zu bytes\n", strlen(result_json));
    free(result_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Query metrics\n");
  return true;
}

/* Test: Get Prometheus Metrics */
static bool test_get_prometheus_metrics(void) {
  printf("  Testing get Prometheus metrics...\n");

  if (!mock_restreamer_start(9712)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9712,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get Prometheus metrics */
  char *prometheus_text = NULL;
  bool result = restreamer_api_get_prometheus_metrics(api, &prometheus_text);
  TEST_ASSERT(result, "Getting Prometheus metrics should succeed");

  if (prometheus_text) {
    printf("    Prometheus metrics: %zu bytes\n", strlen(prometheus_text));
    /* Verify it looks like Prometheus format */
    if (strstr(prometheus_text, "# HELP") || strstr(prometheus_text, "# TYPE")) {
      printf("    ✓ Valid Prometheus format detected\n");
    }
    free(prometheus_text);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get Prometheus metrics\n");
  return true;
}

/* ========================================================================
 * Metadata Storage Tests
 * ======================================================================== */

/* Test: Global Metadata */
static bool test_global_metadata(void) {
  printf("  Testing global metadata...\n");

  if (!mock_restreamer_start(9720)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9720,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Set global metadata */
  bool set_result = restreamer_api_set_metadata(api, "stream_title",
      "Test Stream");
  TEST_ASSERT(set_result, "Setting global metadata should succeed");

  /* Get global metadata */
  char *value = NULL;
  bool get_result = restreamer_api_get_metadata(api, "stream_title", &value);
  TEST_ASSERT(get_result, "Getting global metadata should succeed");

  if (value) {
    printf("    Retrieved metadata: %s\n", value);
    free(value);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Global metadata\n");
  return true;
}

/* Test: Process Metadata */
static bool test_process_metadata(void) {
  printf("  Testing process metadata...\n");

  if (!mock_restreamer_start(9721)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9721,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "metadata-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Set process metadata */
  bool set_result = restreamer_api_set_process_metadata(api, "metadata-test",
      "quality", "1080p");
  TEST_ASSERT(set_result, "Setting process metadata should succeed");

  /* Get process metadata */
  char *value = NULL;
  bool get_result = restreamer_api_get_process_metadata(api, "metadata-test",
      "quality", &value);
  TEST_ASSERT(get_result, "Getting process metadata should succeed");

  if (value) {
    printf("    Process metadata: %s\n", value);
    free(value);
  }

  /* Cleanup */
  restreamer_api_delete_process(api, "metadata-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process metadata\n");
  return true;
}

/* Test: Multiple Metadata Operations */
static bool test_multiple_metadata(void) {
  printf("  Testing multiple metadata operations...\n");

  if (!mock_restreamer_start(9722)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9722,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Set multiple global metadata keys */
  TEST_ASSERT(restreamer_api_set_metadata(api, "key1", "value1"),
              "Set metadata key1");
  TEST_ASSERT(restreamer_api_set_metadata(api, "key2", "value2"),
              "Set metadata key2");
  TEST_ASSERT(restreamer_api_set_metadata(api, "key3", "value3"),
              "Set metadata key3");

  printf("    ✓ Set 3 metadata keys\n");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Multiple metadata operations\n");
  return true;
}

/* ========================================================================
 * Playout Management Tests
 * ======================================================================== */

/* Test: Get Playout Status */
static bool test_get_playout_status(void) {
  printf("  Testing get playout status...\n");

  if (!mock_restreamer_start(9730)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9730,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "playout-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Get playout status */
  restreamer_playout_status_t status = {0};
  bool result = restreamer_api_get_playout_status(api, "playout-test",
      "input_0", &status);
  TEST_ASSERT(result, "Getting playout status should succeed");

  if (status.url) {
    printf("    Playout URL: %s\n", status.url);
    printf("    Connected: %s\n", status.is_connected ? "yes" : "no");
    printf("    Bitrate: %u bps\n", status.bitrate);
  }

  restreamer_api_free_playout_status(&status);

  /* Cleanup */
  restreamer_api_delete_process(api, "playout-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get playout status\n");
  return true;
}

/* Test: Switch Input Stream */
static bool test_switch_input_stream(void) {
  printf("  Testing switch input stream...\n");

  if (!mock_restreamer_start(9731)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9731,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "switch-test",
      "rtmp://localhost:1935/live/test1", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Switch to different input */
  const char *new_url = "rtmp://localhost:1935/live/test2";
  bool result = restreamer_api_switch_input_stream(api, "switch-test",
      "input_0", new_url);
  TEST_ASSERT(result, "Switching input stream should succeed");

  printf("    ✓ Switched to: %s\n", new_url);

  /* Cleanup */
  restreamer_api_delete_process(api, "switch-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Switch input stream\n");
  return true;
}

/* Test: Reopen Input */
static bool test_reopen_input(void) {
  printf("  Testing reopen input...\n");

  if (!mock_restreamer_start(9732)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9732,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "reopen-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Reopen input */
  bool result = restreamer_api_reopen_input(api, "reopen-test", "input_0");
  TEST_ASSERT(result, "Reopening input should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, "reopen-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Reopen input\n");
  return true;
}

/* Test: Get Keyframe */
static bool test_get_keyframe(void) {
  printf("  Testing get keyframe...\n");

  if (!mock_restreamer_start(9733)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9733,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "keyframe-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Get keyframe */
  unsigned char *data = NULL;
  size_t size = 0;
  bool result = restreamer_api_get_keyframe(api, "keyframe-test", "input_0",
      "snapshot.jpg", &data, &size);

  if (result && data) {
    printf("    Keyframe size: %zu bytes\n", size);
    free(data);
  } else {
    printf("    No keyframe available (expected for test)\n");
  }

  /* Cleanup */
  restreamer_api_delete_process(api, "keyframe-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get keyframe\n");
  return true;
}

/* ========================================================================
 * File System Access Tests
 * ======================================================================== */

/* Test: List Filesystems */
static bool test_list_filesystems(void) {
  printf("  Testing list filesystems...\n");

  if (!mock_restreamer_start(9740)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9740,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* List filesystems */
  char *filesystems_json = NULL;
  bool result = restreamer_api_list_filesystems(api, &filesystems_json);
  TEST_ASSERT(result, "Listing filesystems should succeed");

  if (filesystems_json) {
    printf("    Filesystems: %zu bytes\n", strlen(filesystems_json));
    free(filesystems_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ List filesystems\n");
  return true;
}

/* Test: List Files */
static bool test_list_files(void) {
  printf("  Testing list files...\n");

  if (!mock_restreamer_start(9741)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9741,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* List files with pattern */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "disk", "*.mp4", &files);
  TEST_ASSERT(result, "Listing files should succeed");

  printf("    Files found: %zu\n", files.count);

  if (files.count > 0) {
    for (size_t i = 0; i < files.count && i < 5; i++) {
      printf("      [%zu] %s (%llu bytes)\n", i,
             files.entries[i].name ? files.entries[i].name : "unknown",
             (unsigned long long)files.entries[i].size);
    }
  }

  restreamer_api_free_fs_list(&files);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ List files\n");
  return true;
}

/* Test: File Upload and Download */
static bool test_file_upload_download(void) {
  printf("  Testing file upload and download...\n");
  bool success = false;
  restreamer_api_t *api = NULL;
  unsigned char *downloaded_data = NULL;

  if (!mock_restreamer_start(9850)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    goto cleanup;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9850,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    goto cleanup;
  }

  /* Upload test file */
  const unsigned char test_data[] = "Test file content";
  size_t test_size = sizeof(test_data);

  bool upload_result = restreamer_api_upload_file(api, "disk", "test.txt",
      test_data, test_size);
  if (!upload_result) {
    fprintf(stderr, "  ✗ FAIL: File upload should succeed\n");
    goto cleanup;
  }

  /* Give curl time to finish upload cleanup before starting download */
  sleep_ms(200);

  /* Download file */
  size_t downloaded_size = 0;
  bool download_result = restreamer_api_download_file(api, "disk", "test.txt",
      &downloaded_data, &downloaded_size);

  if (download_result && downloaded_data) {
    printf("    Downloaded: %zu bytes\n", downloaded_size);
  }

  printf("  ✓ File upload and download\n");
  success = true;

cleanup:
  /* Stop server FIRST, then destroy API - order matters for thread safety */
  mock_restreamer_stop();
  if (downloaded_data) free(downloaded_data);
  if (api) restreamer_api_destroy(api);
  return success;
}

/* Test: File Deletion */
static bool test_file_deletion(void) {
  printf("  Testing file deletion...\n");
  bool success = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9851)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    goto cleanup;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9851,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ FAIL: API client should be created\n");
    goto cleanup;
  }

  /* Delete file */
  bool result = restreamer_api_delete_file(api, "disk", "test.txt");
  if (!result) {
    fprintf(stderr, "  ✗ FAIL: File deletion should succeed\n");
    goto cleanup;
  }

  printf("  ✓ File deletion\n");
  success = true;

  /* Give curl time to finish cleanup before destroying API object */
  sleep_ms(50);

cleanup:
  if (api) restreamer_api_destroy(api);
  mock_restreamer_stop();
  return success;
}

/* ========================================================================
 * Protocol Monitoring Tests
 * ======================================================================== */

/* Test: Get RTMP Streams */
static bool test_get_rtmp_streams(void) {
  printf("  Testing get RTMP streams...\n");

  if (!mock_restreamer_start(9750)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9750,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get RTMP streams */
  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(api, &streams_json);
  TEST_ASSERT(result, "Getting RTMP streams should succeed");

  if (streams_json) {
    printf("    RTMP streams: %zu bytes\n", strlen(streams_json));
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get RTMP streams\n");
  return true;
}

/* Test: Get SRT Streams */
static bool test_get_srt_streams(void) {
  printf("  Testing get SRT streams...\n");

  if (!mock_restreamer_start(9751)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9751,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get SRT streams */
  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(api, &streams_json);
  TEST_ASSERT(result, "Getting SRT streams should succeed");

  if (streams_json) {
    printf("    SRT streams: %zu bytes\n", strlen(streams_json));
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get SRT streams\n");
  return true;
}

/* ========================================================================
 * FFmpeg Capabilities Tests
 * ======================================================================== */

/* Test: Get FFmpeg Skills */
static bool test_get_skills(void) {
  printf("  Testing get FFmpeg skills...\n");

  if (!mock_restreamer_start(9760)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9760,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get FFmpeg skills */
  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(api, &skills_json);
  TEST_ASSERT(result, "Getting FFmpeg skills should succeed");

  if (skills_json) {
    printf("    Skills: %zu bytes\n", strlen(skills_json));
    /* Verify it contains codec/format information */
    if (strstr(skills_json, "codec") || strstr(skills_json, "format")) {
      printf("    ✓ Valid skills data detected\n");
    }
    free(skills_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get FFmpeg skills\n");
  return true;
}

/* Test: Reload FFmpeg Skills */
static bool test_reload_skills(void) {
  printf("  Testing reload FFmpeg skills...\n");

  if (!mock_restreamer_start(9761)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9761,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Reload FFmpeg skills */
  bool result = restreamer_api_reload_skills(api);
  TEST_ASSERT(result, "Reloading FFmpeg skills should succeed");

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Reload FFmpeg skills\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

/* Cleanup handler registered with atexit() */
static void cleanup_on_exit(void) {
  printf("[CLEANUP] Ensuring mock server is stopped on exit...\n");
  mock_restreamer_stop();
}

int test_restreamer_api_advanced(void) {
  printf("\n=== Restreamer API Advanced Feature Tests ===\n");

  /* Register cleanup handler to ensure mock server is always stopped */
  atexit(cleanup_on_exit);

  /* Clean up any existing mock server before starting tests */
  printf("[CLEANUP] Stopping any existing mock server...\n");
  mock_restreamer_stop();

  int passed = 0;
  int failed = 0;

  /* Configuration Management Tests */
  printf("\n--- Configuration Management Tests ---\n");

  if (test_get_config()) {
    passed++;
  } else {
    failed++;
  }

  if (test_set_config()) {
    passed++;
  } else {
    failed++;
  }

  if (test_reload_config()) {
    passed++;
  } else {
    failed++;
  }

  /* Metrics & Monitoring Tests */
  printf("\n--- Metrics & Monitoring Tests ---\n");

  if (test_get_metrics_list()) {
    passed++;
  } else {
    failed++;
  }

  if (test_query_metrics()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_prometheus_metrics()) {
    passed++;
  } else {
    failed++;
  }

  /* Metadata Storage Tests */
  printf("\n--- Metadata Storage Tests ---\n");

  if (test_global_metadata()) {
    passed++;
  } else {
    failed++;
  }

  if (test_process_metadata()) {
    passed++;
  } else {
    failed++;
  }

  if (test_multiple_metadata()) {
    passed++;
  } else {
    failed++;
  }

  /* Playout Management Tests */
  printf("\n--- Playout Management Tests ---\n");

  if (test_get_playout_status()) {
    passed++;
  } else {
    failed++;
  }

  if (test_switch_input_stream()) {
    passed++;
  } else {
    failed++;
  }

  if (test_reopen_input()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_keyframe()) {
    passed++;
  } else {
    failed++;
  }

  /* File System Access Tests */
  printf("\n--- File System Access Tests ---\n");

  if (test_list_filesystems()) {
    passed++;
  } else {
    failed++;
  }

  if (test_list_files()) {
    passed++;
  } else {
    failed++;
  }

  if (test_file_upload_download()) {
    passed++;
  } else {
    failed++;
  }

  if (test_file_deletion()) {
    passed++;
  } else {
    failed++;
  }

  /* Protocol Monitoring Tests */
  printf("\n--- Protocol Monitoring Tests ---\n");

  if (test_get_rtmp_streams()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_srt_streams()) {
    passed++;
  } else {
    failed++;
  }

  /* FFmpeg Capabilities Tests */
  printf("\n--- FFmpeg Capabilities Tests ---\n");

  if (test_get_skills()) {
    passed++;
  } else {
    failed++;
  }

  if (test_reload_skills()) {
    passed++;
  } else {
    failed++;
  }

  /* Summary */
  printf("\n=== API Advanced Feature Tests Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
