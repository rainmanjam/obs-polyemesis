/*
 * Multi-Destination Streaming Integration Tests
 *
 * Tests for streaming to multiple platforms simultaneously
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

/* Test: Stream to Multiple Destinations */
static bool test_stream_multiple_destinations(void) {
  printf("  Testing streaming to multiple destinations...\n");

  if (!mock_restreamer_start(9400)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9400,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create process with multiple outputs */
  printf("    Creating multi-destination process...\n");

  const char *destinations[] = {
      "rtmp://live.twitch.tv/app/streamkey1",
      "rtmp://a.rtmp.youtube.com/live2/streamkey2",
      "rtmp://live-api-s.facebook.com:443/rtmp/streamkey3",
  };

  restreamer_multistream_config_t multistream_config = {
      .process_id = "multistream-test",
      .input_address = "rtmp://localhost:1935/live/input",
      .destination_count = 3,
      .destinations = (char **)destinations,
  };

  bool result = restreamer_api_create_multistream(api, &multistream_config);
  TEST_ASSERT(result, "Multistream creation should succeed");

  /* Verify all destinations are active */
  printf("    Verifying all destinations...\n");
  restreamer_process_t *process = restreamer_api_get_process(api, "multistream-test");
  TEST_ASSERT_NOT_NULL(process, "Should retrieve multistream process");

  /* Check state for each destination */
  for (int i = 0; i < 3; i++) {
    restreamer_destination_state_t *dest_state =
        restreamer_api_get_destination_state(api, "multistream-test", i);
    if (dest_state) {
      printf("      Destination %d: %s (bitrate: %.2f Mbps)\n",
             i + 1, dest_state->url, dest_state->bitrate / 1000000.0);
      TEST_ASSERT(dest_state->bitrate >= 0, "Bitrate should be valid");
      restreamer_api_free_destination_state(dest_state);
    }
  }

  restreamer_api_free_process(process);
  restreamer_api_delete_process(api, "multistream-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Multiple destinations\n");
  return true;
}

/* Test: Add/Remove Destinations Dynamically */
static bool test_dynamic_destination_management(void) {
  printf("  Testing dynamic destination management...\n");

  if (!mock_restreamer_start(9401)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9401,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Start with one destination */
  const char *initial_dest[] = {"rtmp://initial.example.com/live/stream"};

  restreamer_multistream_config_t config = {
      .process_id = "dynamic-test",
      .input_address = "rtmp://localhost:1935/live/input",
      .destination_count = 1,
      .destinations = (char **)initial_dest,
  };

  TEST_ASSERT(restreamer_api_create_multistream(api, &config),
              "Initial multistream should be created");

  /* Add a new destination */
  printf("    Adding second destination...\n");
  restreamer_destination_config_t new_dest = {
      .url = "rtmp://second.example.com/live/stream",
      .stream_key = "key123",
      .enabled = true,
  };

  TEST_ASSERT(restreamer_api_add_destination(api, "dynamic-test", &new_dest),
              "Adding destination should succeed");

  /* Add a third destination */
  printf("    Adding third destination...\n");
  new_dest.url = "rtmp://third.example.com/live/stream";
  TEST_ASSERT(restreamer_api_add_destination(api, "dynamic-test", &new_dest),
              "Adding third destination should succeed");

  /* Remove second destination */
  printf("    Removing second destination...\n");
  TEST_ASSERT(restreamer_api_remove_destination(api, "dynamic-test", 1),
              "Removing destination should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, "dynamic-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Dynamic destination management\n");
  return true;
}

/* Test: Per-Destination Settings */
static bool test_per_destination_settings(void) {
  printf("  Testing per-destination settings...\n");

  if (!mock_restreamer_start(9402)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9402,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Configure destinations with different settings */
  restreamer_destination_config_t destinations[] = {
      {
          .url = "rtmp://low-quality.example.com/live/stream",
          .stream_key = "lowq",
          .bitrate = 2500000,  /* 2.5 Mbps */
          .resolution = "1280x720",
          .enabled = true,
      },
      {
          .url = "rtmp://high-quality.example.com/live/stream",
          .stream_key = "highq",
          .bitrate = 6000000,  /* 6 Mbps */
          .resolution = "1920x1080",
          .enabled = true,
      },
  };

  /* Create multistream with custom settings */
  for (int i = 0; i < 2; i++) {
    char process_id[64];
    snprintf(process_id, sizeof(process_id), "custom-dest-%d", i);

    TEST_ASSERT(restreamer_api_add_destination(api, process_id, &destinations[i]),
                "Custom destination should be added");
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Per-destination settings\n");
  return true;
}

/* Test: Destination Failure Handling */
static bool test_destination_failure_handling(void) {
  printf("  Testing destination failure handling...\n");

  if (!mock_restreamer_start(9403)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9403,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create multistream with one good and one bad destination */
  const char *destinations[] = {
      "rtmp://good.example.com/live/stream",
      "rtmp://invalid-host-that-does-not-exist.example.com/live/stream",
      "rtmp://another-good.example.com/live/stream",
  };

  restreamer_multistream_config_t config = {
      .process_id = "failure-test",
      .input_address = "rtmp://localhost:1935/live/input",
      .destination_count = 3,
      .destinations = (char **)destinations,
  };

  TEST_ASSERT(restreamer_api_create_multistream(api, &config),
              "Multistream should be created even with bad destination");

  /* Check individual destination states */
  printf("    Checking destination states...\n");
  for (int i = 0; i < 3; i++) {
    restreamer_destination_state_t *state =
        restreamer_api_get_destination_state(api, "failure-test", i);
    if (state) {
      printf("      Destination %d: %s (%s)\n",
             i + 1, state->url, state->is_connected ? "Connected" : "Disconnected");
      /* Bad destination should show as disconnected */
      if (i == 1) {
        TEST_ASSERT(!state->is_connected, "Bad destination should be disconnected");
      }
      restreamer_api_free_destination_state(state);
    }
  }

  /* Cleanup */
  restreamer_api_delete_process(api, "failure-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Destination failure handling\n");
  return true;
}

/* Test: Bandwidth Distribution */
static bool test_bandwidth_distribution(void) {
  printf("  Testing bandwidth distribution across destinations...\n");

  if (!mock_restreamer_start(9404)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9404,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create multistream */
  const char *destinations[] = {
      "rtmp://dest1.example.com/live/stream",
      "rtmp://dest2.example.com/live/stream",
      "rtmp://dest3.example.com/live/stream",
  };

  restreamer_multistream_config_t config = {
      .process_id = "bandwidth-test",
      .input_address = "rtmp://localhost:1935/live/input",
      .destination_count = 3,
      .destinations = (char **)destinations,
  };

  TEST_ASSERT(restreamer_api_create_multistream(api, &config),
              "Multistream should be created");

  /* Monitor total bandwidth usage */
  printf("    Monitoring bandwidth...\n");
  for (int i = 0; i < 3; i++) {
    restreamer_process_state_t *state =
        restreamer_api_get_process_state(api, "bandwidth-test");
    if (state) {
      printf("      Iteration %d: Total bandwidth: %.2f Mbps\n",
             i + 1, state->total_bitrate / 1000000.0);
      TEST_ASSERT(state->total_bitrate >= 0, "Total bitrate should be valid");
      restreamer_api_free_process_state(state);
    }
    sleep_ms(100);
  }

  /* Cleanup */
  restreamer_api_delete_process(api, "bandwidth-test");
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Bandwidth distribution\n");
  return true;
}

/* Main test runner */
int test_multistream_integration(void) {
  printf("\n=== Multi-Destination Streaming Integration Tests ===\n");

  int passed = 0;
  int failed = 0;

  if (test_stream_multiple_destinations()) {
    passed++;
  } else {
    failed++;
  }

  if (test_dynamic_destination_management()) {
    passed++;
  } else {
    failed++;
  }

  if (test_per_destination_settings()) {
    passed++;
  } else {
    failed++;
  }

  if (test_destination_failure_handling()) {
    passed++;
  } else {
    failed++;
  }

  if (test_bandwidth_distribution()) {
    passed++;
  } else {
    failed++;
  }

  printf("\n=== Multistream Integration Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
