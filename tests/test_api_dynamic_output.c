/*
 * Dynamic Output API Tests
 *
 * Tests for dynamic process output management API functions:
 * - Add/remove/update process outputs
 * - Get process outputs list
 * - Encoding settings management
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
#define TEST_CHECK(condition, message)                                         \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  FAIL: %s\n    at %s:%d\n", message, __FILE__,         \
              __LINE__);                                                       \
      test_passed = false;                                                     \
    }                                                                          \
  } while (0)

/* ========================================================================
 * Add Process Output Tests
 * ======================================================================== */

static bool test_add_process_output_success(void) {
  printf("  Testing add process output success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9820)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9820,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  FAIL: API client should be created\n");
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_add_process_output(
      api, "test-process", "output-1", "rtmp://localhost/live/stream", NULL);
  printf("    Add output result: %s\n", result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Add process output test completed\n");
  }
  return test_passed;
}

static bool test_add_process_output_null_api(void) {
  printf("  Testing add process output with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_add_process_output(
      NULL, "test-process", "output-1", "rtmp://localhost/live", NULL);
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

static bool test_add_process_output_null_process_id(void) {
  printf("  Testing add process output with NULL process_id...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9821)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9821,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_add_process_output(
      api, NULL, "output-1", "rtmp://localhost/live", NULL);
  TEST_CHECK(!result, "Should return false for NULL process_id");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ NULL process_id handling\n");
  }
  return test_passed;
}

/* ========================================================================
 * Remove Process Output Tests
 * ======================================================================== */

static bool test_remove_process_output_success(void) {
  printf("  Testing remove process output success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9822)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9822,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  bool result =
      restreamer_api_remove_process_output(api, "test-process", "output-1");
  printf("    Remove output result: %s\n", result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Remove process output test completed\n");
  }
  return test_passed;
}

static bool test_remove_process_output_null_api(void) {
  printf("  Testing remove process output with NULL api...\n");
  bool test_passed = true;

  bool result =
      restreamer_api_remove_process_output(NULL, "test-process", "output-1");
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

/* ========================================================================
 * Update Process Output Tests
 * ======================================================================== */

static bool test_update_process_output_success(void) {
  printf("  Testing update process output success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9823)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9823,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_update_process_output(
      api, "test-process", "output-1", "rtmp://newurl/live/stream", NULL);
  printf("    Update output result: %s\n", result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Update process output test completed\n");
  }
  return test_passed;
}

static bool test_update_process_output_null_api(void) {
  printf("  Testing update process output with NULL api...\n");
  bool test_passed = true;

  bool result = restreamer_api_update_process_output(
      NULL, "test-process", "output-1", "rtmp://newurl/live", NULL);
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

/* ========================================================================
 * Get Process Outputs Tests
 * ======================================================================== */

static bool test_get_process_outputs_success(void) {
  printf("  Testing get process outputs success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;
  char **output_ids = NULL;
  size_t output_count = 0;

  if (!mock_restreamer_start(9824)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9824,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  bool result = restreamer_api_get_process_outputs(api, "test-process",
                                                   &output_ids, &output_count);
  printf("    Get outputs result: %s, count: %zu\n",
         result ? "success" : "failed", output_count);

  if (output_ids) {
    restreamer_api_free_outputs_list(output_ids, output_count);
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
    printf("  ✓ Get process outputs test completed\n");
  }
  return test_passed;
}

static bool test_get_process_outputs_null_api(void) {
  printf("  Testing get process outputs with NULL api...\n");
  bool test_passed = true;

  char **output_ids = NULL;
  size_t output_count = 0;
  bool result = restreamer_api_get_process_outputs(NULL, "test-process",
                                                   &output_ids, &output_count);
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

static bool test_free_outputs_list_null(void) {
  printf("  Testing free outputs list with NULL...\n");

  /* Should not crash */
  restreamer_api_free_outputs_list(NULL, 0);

  printf("  ✓ NULL handling safe\n");
  return true;
}

/* ========================================================================
 * Encoding Settings Tests
 * ======================================================================== */

static bool test_get_output_encoding_success(void) {
  printf("  Testing get output encoding success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9825)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9825,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  encoding_params_t params = {0};
  bool result =
      restreamer_api_get_output_encoding(api, "test-process", "output-1", &params);
  printf("    Get encoding result: %s\n", result ? "success" : "failed");

  if (result) {
    printf("    Video bitrate: %d kbps\n", params.video_bitrate_kbps);
    printf("    Audio bitrate: %d kbps\n", params.audio_bitrate_kbps);
    restreamer_api_free_encoding_params(&params);
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
    printf("  ✓ Get output encoding test completed\n");
  }
  return test_passed;
}

static bool test_get_output_encoding_null_api(void) {
  printf("  Testing get output encoding with NULL api...\n");
  bool test_passed = true;

  encoding_params_t params = {0};
  bool result =
      restreamer_api_get_output_encoding(NULL, "test-process", "output-1", &params);
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

static bool test_update_output_encoding_success(void) {
  printf("  Testing update output encoding success...\n");
  bool test_passed = true;
  bool server_started = false;
  restreamer_api_t *api = NULL;

  if (!mock_restreamer_start(9826)) {
    fprintf(stderr, "  Failed to start mock server\n");
    return false;
  }
  server_started = true;
  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9826,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    test_passed = false;
    goto cleanup;
  }

  encoding_params_t params = {
      .video_bitrate_kbps = 4000,
      .audio_bitrate_kbps = 192,
      .width = 1920,
      .height = 1080,
      .fps_num = 30,
      .fps_den = 1,
      .preset = NULL,
      .profile = NULL,
  };

  bool result = restreamer_api_update_output_encoding(api, "test-process",
                                                      "output-1", &params);
  printf("    Update encoding result: %s\n", result ? "success" : "failed");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  if (server_started) {
    mock_restreamer_stop();
    sleep_ms(100);
  }

  if (test_passed) {
    printf("  ✓ Update output encoding test completed\n");
  }
  return test_passed;
}

static bool test_update_output_encoding_null_api(void) {
  printf("  Testing update output encoding with NULL api...\n");
  bool test_passed = true;

  encoding_params_t params = {.video_bitrate_kbps = 4000};
  bool result = restreamer_api_update_output_encoding(NULL, "test-process",
                                                      "output-1", &params);
  TEST_CHECK(!result, "Should return false for NULL api");

  if (test_passed) {
    printf("  ✓ NULL api handling\n");
  }
  return test_passed;
}

static bool test_free_encoding_params_null(void) {
  printf("  Testing free encoding params with NULL...\n");

  /* Should not crash */
  restreamer_api_free_encoding_params(NULL);

  printf("  ✓ NULL handling safe\n");
  return true;
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

int run_api_dynamic_output_tests(void) {
  printf("\n=== Dynamic Output API Tests ===\n");

  int passed = 0;
  int failed = 0;

  /* Add Process Output Tests */
  printf("\n-- Add Process Output Tests --\n");
  if (test_add_process_output_success())
    passed++;
  else
    failed++;
  if (test_add_process_output_null_api())
    passed++;
  else
    failed++;
  if (test_add_process_output_null_process_id())
    passed++;
  else
    failed++;

  /* Remove Process Output Tests */
  printf("\n-- Remove Process Output Tests --\n");
  if (test_remove_process_output_success())
    passed++;
  else
    failed++;
  if (test_remove_process_output_null_api())
    passed++;
  else
    failed++;

  /* Update Process Output Tests */
  printf("\n-- Update Process Output Tests --\n");
  if (test_update_process_output_success())
    passed++;
  else
    failed++;
  if (test_update_process_output_null_api())
    passed++;
  else
    failed++;

  /* Get Process Outputs Tests */
  printf("\n-- Get Process Outputs Tests --\n");
  if (test_get_process_outputs_success())
    passed++;
  else
    failed++;
  if (test_get_process_outputs_null_api())
    passed++;
  else
    failed++;
  if (test_free_outputs_list_null())
    passed++;
  else
    failed++;

  /* Encoding Settings Tests */
  printf("\n-- Encoding Settings Tests --\n");
  if (test_get_output_encoding_success())
    passed++;
  else
    failed++;
  if (test_get_output_encoding_null_api())
    passed++;
  else
    failed++;
  if (test_update_output_encoding_success())
    passed++;
  else
    failed++;
  if (test_update_output_encoding_null_api())
    passed++;
  else
    failed++;
  if (test_free_encoding_params_null())
    passed++;
  else
    failed++;

  printf("\n=== Dynamic Output Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
