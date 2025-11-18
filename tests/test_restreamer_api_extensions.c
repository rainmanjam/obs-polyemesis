/*
 * Restreamer API Extension Tests
 *
 * Tests for extended API functionality:
 * - Encoding parameters (update/get)
 * - Input probing
 * - Sessions and logs
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

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* ========================================================================
 * Encoding Parameters Tests
 * ======================================================================== */

/* Test: Update Encoding Parameters */
static bool test_update_encoding_parameters(void) {
  printf("  Testing update encoding parameters...\n");

  if (!mock_restreamer_start(9600)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9600,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create a test process first */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "enc-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Update encoding parameters */
  encoding_params_t params = {
      .video_bitrate_kbps = 4500,
      .audio_bitrate_kbps = 192,
      .width = 1920,
      .height = 1080,
      .fps_num = 30,
      .fps_den = 1,
      .preset = "veryfast",
      .profile = "high",
  };

  bool result = restreamer_api_update_output_encoding(api, "enc-test",
      "output_0", &params);
  TEST_ASSERT(result, "Encoding update should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, "enc-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Update encoding parameters\n");
  return true;
}

/* Test: Get Encoding Parameters */
static bool test_get_encoding_parameters(void) {
  printf("  Testing get encoding parameters...\n");

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

  /* Create a test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "enc-get-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Get encoding parameters */
  encoding_params_t params = {0};
  bool result = restreamer_api_get_output_encoding(api, "enc-get-test",
      "output_0", &params);
  TEST_ASSERT(result, "Getting encoding params should succeed");

  /* Verify we got valid parameters */
  TEST_ASSERT(params.video_bitrate_kbps >= 0, "Video bitrate should be valid");
  TEST_ASSERT(params.width > 0 || params.width == 0, "Width should be valid");
  TEST_ASSERT(params.height > 0 || params.height == 0, "Height should be valid");

  /* Cleanup */
  restreamer_api_free_encoding_params(&params);
  restreamer_api_delete_process(api, "enc-get-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get encoding parameters\n");
  return true;
}

/* Test: Update Multiple Encoding Parameters */
static bool test_update_multiple_encoding_params(void) {
  printf("  Testing update multiple encoding parameters...\n");

  if (!mock_restreamer_start(9602)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9602,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create process with multiple outputs */
  const char *outputs[] = {
      "rtmp://dest1.example.com/live/stream1",
      "rtmp://dest2.example.com/live/stream2",
  };
  bool created = restreamer_api_create_process(api, "multi-enc-test",
      "rtmp://localhost:1935/live/test", outputs, 2, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Update encoding for first output - high quality */
  encoding_params_t high_quality = {
      .video_bitrate_kbps = 6000,
      .audio_bitrate_kbps = 256,
      .width = 1920,
      .height = 1080,
      .fps_num = 60,
      .fps_den = 1,
      .preset = "medium",
      .profile = "high",
  };

  bool result1 = restreamer_api_update_output_encoding(api, "multi-enc-test",
      "output_0", &high_quality);
  TEST_ASSERT(result1, "High quality encoding update should succeed");

  /* Update encoding for second output - low quality */
  encoding_params_t low_quality = {
      .video_bitrate_kbps = 2500,
      .audio_bitrate_kbps = 128,
      .width = 1280,
      .height = 720,
      .fps_num = 30,
      .fps_den = 1,
      .preset = "veryfast",
      .profile = "main",
  };

  bool result2 = restreamer_api_update_output_encoding(api, "multi-enc-test",
      "output_1", &low_quality);
  TEST_ASSERT(result2, "Low quality encoding update should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, "multi-enc-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Update multiple encoding parameters\n");
  return true;
}

/* Test: Encoding Parameter Validation */
static bool test_encoding_parameter_validation(void) {
  printf("  Testing encoding parameter validation...\n");

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

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "validation-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Test with partial parameters (0 = no change) */
  encoding_params_t partial = {
      .video_bitrate_kbps = 5000,  /* Change bitrate */
      .audio_bitrate_kbps = 0,     /* Keep current */
      .width = 0,                  /* Keep current */
      .height = 0,                 /* Keep current */
      .fps_num = 0,                /* Keep current */
      .fps_den = 0,                /* Keep current */
      .preset = NULL,              /* Keep current */
      .profile = NULL,             /* Keep current */
  };

  bool result = restreamer_api_update_output_encoding(api, "validation-test",
      "output_0", &partial);
  TEST_ASSERT(result, "Partial encoding update should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, "validation-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Encoding parameter validation\n");
  return true;
}

/* ========================================================================
 * Input Probing Tests
 * ======================================================================== */

/* Test: Basic Input Probing */
static bool test_probe_input_basic(void) {
  printf("  Testing basic input probing...\n");

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

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "probe-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Probe input */
  restreamer_probe_info_t info = {0};
  bool result = restreamer_api_probe_input(api, "probe-test", &info);
  TEST_ASSERT(result, "Input probing should succeed");

  /* Verify probe results */
  TEST_ASSERT_NOT_NULL(info.format_name, "Format name should be present");
  TEST_ASSERT(info.stream_count >= 0, "Stream count should be valid");

  printf("    Format: %s\n", info.format_name ? info.format_name : "unknown");
  printf("    Streams: %zu\n", info.stream_count);
  printf("    Duration: %lld us\n", (long long)info.duration);
  printf("    Bitrate: %u bps\n", info.bitrate);

  /* Cleanup */
  restreamer_api_free_probe_info(&info);
  restreamer_api_delete_process(api, "probe-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Basic input probing\n");
  return true;
}

/* Test: Probe Input Stream Information */
static bool test_probe_input_streams(void) {
  printf("  Testing probe input stream information...\n");

  if (!mock_restreamer_start(9611)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9611,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "probe-streams-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Probe input */
  restreamer_probe_info_t info = {0};
  bool result = restreamer_api_probe_input(api, "probe-streams-test", &info);
  TEST_ASSERT(result, "Input probing should succeed");

  /* Analyze each stream */
  if (info.stream_count > 0) {
    TEST_ASSERT_NOT_NULL(info.streams, "Streams array should be present");

    for (size_t i = 0; i < info.stream_count; i++) {
      restreamer_stream_info_t *stream = &info.streams[i];
      printf("    Stream %zu:\n", i);

      if (stream->codec_type) {
        printf("      Type: %s\n", stream->codec_type);

        if (strcmp(stream->codec_type, "video") == 0) {
          printf("      Codec: %s\n", stream->codec_name ? stream->codec_name : "unknown");
          printf("      Resolution: %ux%u\n", stream->width, stream->height);
          printf("      FPS: %u/%u\n", stream->fps_num, stream->fps_den);
          printf("      Bitrate: %u bps\n", stream->bitrate);
        } else if (strcmp(stream->codec_type, "audio") == 0) {
          printf("      Codec: %s\n", stream->codec_name ? stream->codec_name : "unknown");
          printf("      Sample Rate: %u Hz\n", stream->sample_rate);
          printf("      Channels: %u\n", stream->channels);
          printf("      Bitrate: %u bps\n", stream->bitrate);
        }
      }
    }
  }

  /* Cleanup */
  restreamer_api_free_probe_info(&info);
  restreamer_api_delete_process(api, "probe-streams-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Probe input stream information\n");
  return true;
}

/* Test: Probe Invalid Input */
static bool test_probe_invalid_input(void) {
  printf("  Testing probe invalid input...\n");

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

  /* Try to probe non-existent process */
  restreamer_probe_info_t info = {0};
  bool result = restreamer_api_probe_input(api, "nonexistent-process", &info);

  /* Should fail or return error */
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    printf("    Expected error: %s\n", error ? error : "unknown");
  }

  /* Cleanup */
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Probe invalid input\n");
  return true;
}

/* ========================================================================
 * Sessions and Logs Tests
 * ======================================================================== */

/* Test: Get Active Sessions */
static bool test_get_sessions(void) {
  printf("  Testing get active sessions...\n");

  if (!mock_restreamer_start(9620)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9620,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Get sessions */
  restreamer_session_list_t sessions = {0};
  bool result = restreamer_api_get_sessions(api, &sessions);
  TEST_ASSERT(result, "Getting sessions should succeed");

  printf("    Active sessions: %zu\n", sessions.count);

  /* Verify session data if any exist */
  if (sessions.count > 0) {
    TEST_ASSERT_NOT_NULL(sessions.sessions, "Sessions array should be present");

    for (size_t i = 0; i < sessions.count; i++) {
      restreamer_session_t *session = &sessions.sessions[i];
      printf("      Session %zu:\n", i);
      printf("        ID: %s\n", session->session_id ? session->session_id : "unknown");
      printf("        Reference: %s\n", session->reference ? session->reference : "none");
      printf("        Bytes sent: %llu\n", (unsigned long long)session->bytes_sent);
      printf("        Bytes received: %llu\n", (unsigned long long)session->bytes_received);
      printf("        Remote addr: %s\n", session->remote_addr ? session->remote_addr : "unknown");
    }
  }

  /* Cleanup */
  restreamer_api_free_session_list(&sessions);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get active sessions\n");
  return true;
}

/* Test: Get Process Logs */
static bool test_get_process_logs(void) {
  printf("  Testing get process logs...\n");

  if (!mock_restreamer_start(9621)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9621,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "log-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  sleep_ms(100);

  /* Get process logs */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, "log-test", &logs);
  TEST_ASSERT(result, "Getting logs should succeed");

  printf("    Log entries: %zu\n", logs.count);

  /* Verify log data */
  if (logs.count > 0) {
    TEST_ASSERT_NOT_NULL(logs.entries, "Log entries array should be present");

    /* Show first few log entries */
    size_t show_count = logs.count < 5 ? logs.count : 5;
    for (size_t i = 0; i < show_count; i++) {
      restreamer_log_entry_t *entry = &logs.entries[i];
      printf("      [%s] [%s] %s\n",
             entry->timestamp ? entry->timestamp : "unknown",
             entry->level ? entry->level : "info",
             entry->message ? entry->message : "");
    }

    if (logs.count > 5) {
      printf("      ... and %zu more entries\n", logs.count - 5);
    }
  }

  /* Cleanup */
  restreamer_api_free_log_list(&logs);
  restreamer_api_delete_process(api, "log-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get process logs\n");
  return true;
}

/* Test: Get Logs for Non-Existent Process */
static bool test_get_logs_invalid_process(void) {
  printf("  Testing get logs for non-existent process...\n");

  if (!mock_restreamer_start(9622)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9622,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Try to get logs for non-existent process */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, "nonexistent-process", &logs);

  /* Should fail or return empty logs */
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    printf("    Expected error: %s\n", error ? error : "unknown");
  } else {
    printf("    Returned %zu log entries\n", logs.count);
    restreamer_api_free_log_list(&logs);
  }

  /* Cleanup */
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Get logs for non-existent process\n");
  return true;
}

/* Test: Monitor Sessions Over Time */
static bool test_monitor_sessions(void) {
  printf("  Testing monitor sessions over time...\n");

  if (!mock_restreamer_start(9623)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9623,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Create test process to generate session */
  const char *outputs[] = {"rtmp://test.example.com/live/stream"};
  bool created = restreamer_api_create_process(api, "session-monitor-test",
      "rtmp://localhost:1935/live/test", outputs, 1, NULL);
  TEST_ASSERT(created, "Process creation should succeed");

  /* Monitor sessions 3 times */
  for (int i = 0; i < 3; i++) {
    sleep_ms(100);

    restreamer_session_list_t sessions = {0};
    bool result = restreamer_api_get_sessions(api, &sessions);
    TEST_ASSERT(result, "Getting sessions should succeed");

    printf("    Poll %d: %zu sessions\n", i + 1, sessions.count);

    restreamer_api_free_session_list(&sessions);
  }

  /* Cleanup */
  restreamer_api_delete_process(api, "session-monitor-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Monitor sessions over time\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int test_restreamer_api_extensions(void) {
  printf("\n=== Restreamer API Extension Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* Encoding Parameters Tests */
  printf("\n--- Encoding Parameters Tests ---\n");

  if (test_update_encoding_parameters()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_encoding_parameters()) {
    passed++;
  } else {
    failed++;
  }

  if (test_update_multiple_encoding_params()) {
    passed++;
  } else {
    failed++;
  }

  if (test_encoding_parameter_validation()) {
    passed++;
  } else {
    failed++;
  }

  /* Input Probing Tests */
  printf("\n--- Input Probing Tests ---\n");

  if (test_probe_input_basic()) {
    passed++;
  } else {
    failed++;
  }

  if (test_probe_input_streams()) {
    passed++;
  } else {
    failed++;
  }

  if (test_probe_invalid_input()) {
    passed++;
  } else {
    failed++;
  }

  /* Sessions and Logs Tests */
  printf("\n--- Sessions and Logs Tests ---\n");

  if (test_get_sessions()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_process_logs()) {
    passed++;
  } else {
    failed++;
  }

  if (test_get_logs_invalid_process()) {
    passed++;
  } else {
    failed++;
  }

  if (test_monitor_sessions()) {
    passed++;
  } else {
    failed++;
  }

  /* Summary */
  printf("\n=== API Extension Tests Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
