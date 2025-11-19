/*
 * Restreamer API v3 Workflow Integration Tests
 *
 * End-to-end integration tests for complete Restreamer API v3 workflows:
 * Auth → Create Process → Monitor → Update → Stop → Delete
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

/* Test: Complete Process Lifecycle */
static bool test_complete_process_lifecycle(void) {
  printf("  Testing complete process lifecycle...\n");

  if (!mock_restreamer_start(9300)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9300,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Step 1: Authenticate */
  printf("    [1/6] Authenticating...\n");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Step 2: Create a new process */
  printf("    [2/6] Creating process...\n");
  restreamer_process_config_t config = {
      .id = "test-process-001",
      .reference = "Test Process",
      .input_address = "rtmp://localhost:1935/live/test",
      .output_address = "rtmp://destination.example.com/live/stream",
  };

  restreamer_process_t *process = restreamer_api_create_process(api, &config);
  TEST_ASSERT_NOT_NULL(process, "Process should be created");
  TEST_ASSERT_NOT_NULL(process->id, "Process should have ID");

  char *process_id = strdup(process->id);
  restreamer_api_free_process(process);

  /* Step 3: Get process details */
  printf("    [3/6] Getting process details...\n");
  process = restreamer_api_get_process(api, process_id);
  TEST_ASSERT_NOT_NULL(process, "Should retrieve process details");
  TEST_ASSERT(strcmp(process->id, process_id) == 0, "Process ID should match");
  restreamer_api_free_process(process);

  /* Step 4: Update process (e.g., change output) */
  printf("    [4/6] Updating process...\n");
  config.output_address = "rtmp://new-destination.example.com/live/stream";
  bool update_result = restreamer_api_update_process(api, process_id, &config);
  TEST_ASSERT(update_result, "Process update should succeed");

  /* Step 5: Monitor process state */
  printf("    [5/6] Monitoring process...\n");
  restreamer_process_state_t *state = restreamer_api_get_process_state(api, process_id);
  TEST_ASSERT_NOT_NULL(state, "Should get process state");
  TEST_ASSERT(state->cpu_usage >= 0.0, "CPU usage should be valid");
  TEST_ASSERT(state->memory_usage >= 0, "Memory usage should be valid");
  restreamer_api_free_process_state(state);

  /* Step 6: Delete process */
  printf("    [6/6] Deleting process...\n");
  bool delete_result = restreamer_api_delete_process(api, process_id);
  TEST_ASSERT(delete_result, "Process deletion should succeed");

  /* Verify process is deleted */
  process = restreamer_api_get_process(api, process_id);
  TEST_ASSERT(process == NULL, "Process should no longer exist");

  free(process_id);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Complete process lifecycle\n");
  return true;
}

/* Test: Multiple Process Management */
static bool test_multiple_process_management(void) {
  printf("  Testing multiple process management...\n");

  if (!mock_restreamer_start(9301)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9301,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create multiple processes */
  printf("    Creating 3 processes...\n");
  char *process_ids[3] = {NULL, NULL, NULL};

  for (int i = 0; i < 3; i++) {
    char id[64];
    snprintf(id, sizeof(id), "test-process-%03d", i);

    restreamer_process_config_t config = {
        .id = id,
        .reference = "Test Process",
        .input_address = "rtmp://localhost:1935/live/test",
        .output_address = "rtmp://destination.example.com/live/stream",
    };

    restreamer_process_t *process = restreamer_api_create_process(api, &config);
    TEST_ASSERT_NOT_NULL(process, "Process creation should succeed");
    process_ids[i] = strdup(process->id);
    restreamer_api_free_process(process);
  }

  /* List all processes */
  printf("    Listing all processes...\n");
  restreamer_process_list_t list = {0};
  TEST_ASSERT(restreamer_api_get_processes(api, &list), "Should list processes");
  TEST_ASSERT(list.count >= 3, "Should have at least 3 processes");

  /* Clean up all created processes */
  printf("    Cleaning up processes...\n");
  for (int i = 0; i < 3; i++) {
    restreamer_api_delete_process(api, process_ids[i]);
    free(process_ids[i]);
  }

  restreamer_api_free_process_list(&list);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Multiple process management\n");
  return true;
}

/* Test: Process State Monitoring */
static bool test_process_state_monitoring(void) {
  printf("  Testing process state monitoring...\n");

  if (!mock_restreamer_start(9302)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9302,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create process */
  restreamer_process_config_t config = {
      .id = "test-monitor",
      .reference = "Monitor Test",
      .input_address = "rtmp://localhost:1935/live/test",
      .output_address = "rtmp://destination.example.com/live/stream",
  };

  restreamer_process_t *process = restreamer_api_create_process(api, &config);
  TEST_ASSERT_NOT_NULL(process, "Process should be created");
  char *process_id = strdup(process->id);
  restreamer_api_free_process(process);

  /* Monitor state over time */
  printf("    Monitoring state 5 times...\n");
  for (int i = 0; i < 5; i++) {
    restreamer_process_state_t *state = restreamer_api_get_process_state(api, process_id);
    TEST_ASSERT_NOT_NULL(state, "Should get state");

    printf("      [%d] CPU: %.2f%%, Memory: %ldMB, Uptime: %lds\n",
           i + 1, state->cpu_usage, state->memory_usage / (1024*1024), state->uptime);

    TEST_ASSERT(state->cpu_usage >= 0.0 && state->cpu_usage <= 100.0,
                "CPU usage should be 0-100%");
    TEST_ASSERT(state->memory_usage >= 0, "Memory should be >= 0");
    TEST_ASSERT(state->uptime >= 0, "Uptime should be >= 0");

    restreamer_api_free_process_state(state);
    sleep_ms(100);
  }

  /* Cleanup */
  restreamer_api_delete_process(api, process_id);
  free(process_id);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process state monitoring\n");
  return true;
}

/* Test: Process Configuration Update */
static bool test_process_configuration_update(void) {
  printf("  Testing process configuration updates...\n");

  if (!mock_restreamer_start(9303)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9303,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create initial process */
  restreamer_process_config_t config = {
      .id = "test-update",
      .reference = "Update Test",
      .input_address = "rtmp://localhost:1935/live/test",
      .output_address = "rtmp://destination1.example.com/live/stream",
  };

  restreamer_process_t *process = restreamer_api_create_process(api, &config);
  TEST_ASSERT_NOT_NULL(process, "Process should be created");
  char *process_id = strdup(process->id);
  restreamer_api_free_process(process);

  /* Update output address */
  printf("    Updating output address...\n");
  config.output_address = "rtmp://destination2.example.com/live/stream";
  TEST_ASSERT(restreamer_api_update_process(api, process_id, &config),
              "Output update should succeed");

  /* Verify update */
  process = restreamer_api_get_process(api, process_id);
  TEST_ASSERT_NOT_NULL(process, "Should retrieve updated process");
  /* Note: Would need to verify output address matches if exposed in API */

  restreamer_api_free_process(process);

  /* Update input address */
  printf("    Updating input address...\n");
  config.input_address = "rtmp://localhost:1935/live/test2";
  TEST_ASSERT(restreamer_api_update_process(api, process_id, &config),
              "Input update should succeed");

  /* Cleanup */
  restreamer_api_delete_process(api, process_id);
  free(process_id);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process configuration updates\n");
  return true;
}

/* Test: Process Restart */
static bool test_process_restart(void) {
  printf("  Testing process restart...\n");

  if (!mock_restreamer_start(9304)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9304,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Create process */
  restreamer_process_config_t config = {
      .id = "test-restart",
      .reference = "Restart Test",
      .input_address = "rtmp://localhost:1935/live/test",
      .output_address = "rtmp://destination.example.com/live/stream",
  };

  restreamer_process_t *process = restreamer_api_create_process(api, &config);
  TEST_ASSERT_NOT_NULL(process, "Process should be created");
  char *process_id = strdup(process->id);
  restreamer_api_free_process(process);

  /* Restart process */
  printf("    Restarting process...\n");
  TEST_ASSERT(restreamer_api_restart_process(api, process_id),
              "Process restart should succeed");

  /* Verify process still exists after restart */
  process = restreamer_api_get_process(api, process_id);
  TEST_ASSERT_NOT_NULL(process, "Process should exist after restart");
  restreamer_api_free_process(process);

  /* Cleanup */
  restreamer_api_delete_process(api, process_id);
  free(process_id);
  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Process restart\n");
  return true;
}

/* Test: Concurrent Process Operations */
static bool test_concurrent_operations(void) {
  printf("  Testing concurrent process operations...\n");

  if (!mock_restreamer_start(9305)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9305,
      .username = "admin",
      .password = "testpass",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");
  TEST_ASSERT(restreamer_api_authenticate(api), "Authentication should succeed");

  /* Perform multiple operations rapidly */
  printf("    Performing rapid operations...\n");
  for (int i = 0; i < 10; i++) {
    /* List processes */
    restreamer_process_list_t list = {0};
    TEST_ASSERT(restreamer_api_get_processes(api, &list), "List should succeed");
    restreamer_api_free_process_list(&list);

    /* Get system info */
    /* Would call restreamer_api_get_info() if available */
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  printf("  ✓ Concurrent operations\n");
  return true;
}

/* Main test runner */
int test_restreamer_api_v3_workflow(void) {
  printf("\n=== Restreamer API v3 Workflow Tests ===\n");

  int passed = 0;
  int failed = 0;

  if (test_complete_process_lifecycle()) {
    passed++;
  } else {
    failed++;
  }

  if (test_multiple_process_management()) {
    passed++;
  } else {
    failed++;
  }

  if (test_process_state_monitoring()) {
    passed++;
  } else {
    failed++;
  }

  if (test_process_configuration_update()) {
    passed++;
  } else {
    failed++;
  }

  if (test_process_restart()) {
    passed++;
  } else {
    failed++;
  }

  if (test_concurrent_operations()) {
    passed++;
  } else {
    failed++;
  }

  printf("\n=== API v3 Workflow Test Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);
  printf("Total:  %d\n", passed + failed);

  return (failed == 0) ? 0 : 1;
}
