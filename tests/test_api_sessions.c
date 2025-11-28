/*
 * API Sessions Tests
 *
 * Tests for session-related API functions including:
 * - restreamer_api_get_sessions() - Get session list
 * - restreamer_api_free_session_list() - Free session list
 * - restreamer_api_get_process_logs() - Get process logs
 * - restreamer_api_free_log_list() - Free log list
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

/* Test: Get sessions list successfully */
static bool test_get_sessions_success(void) {
  printf("  Testing get sessions success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  /* Start mock server */
  if (!mock_restreamer_start(9780)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  /* Create API client */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9780,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get sessions */
  restreamer_session_list_t sessions = {0};
  bool result = restreamer_api_get_sessions(api, &sessions);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_sessions failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get sessions successfully");

  /* Verify session data if any sessions exist */
  if (sessions.count > 0) {
    TEST_ASSERT_NOT_NULL(sessions.sessions, "Sessions array should not be NULL");

    /* Check first session has required fields */
    if (sessions.sessions[0].session_id) {
      printf("    Found session: %s\n", sessions.sessions[0].session_id);
    }
  }

  /* Free session list */
  restreamer_api_free_session_list(&sessions);

  test_passed = true;
  printf("  ✓ Get sessions success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get sessions with NULL API */
static bool test_get_sessions_null_api(void) {
  printf("  Testing get sessions with NULL API...\n");

  restreamer_session_list_t sessions = {0};
  bool result = restreamer_api_get_sessions(NULL, &sessions);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get sessions NULL API handling\n");
  return true;
}

/* Test: Get sessions with NULL output */
static bool test_get_sessions_null_output(void) {
  printf("  Testing get sessions with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9781)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9781,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get sessions with NULL output */
  bool result = restreamer_api_get_sessions(api, NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get sessions NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Free session list with NULL (should be safe) */
static bool test_free_session_list_null(void) {
  printf("  Testing free session list with NULL...\n");

  /* Should not crash */
  restreamer_api_free_session_list(NULL);

  printf("  ✓ Free session list NULL safety\n");
  return true;
}

/* Test: Free session list with empty list */
static bool test_free_session_list_empty(void) {
  printf("  Testing free session list with empty list...\n");

  restreamer_session_list_t sessions = {0};
  sessions.sessions = NULL;
  sessions.count = 0;

  /* Should not crash */
  restreamer_api_free_session_list(&sessions);

  printf("  ✓ Free session list empty list safety\n");
  return true;
}

/* Test: Get process logs successfully */
static bool test_get_process_logs_success(void) {
  printf("  Testing get process logs success...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9782)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9782,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get process logs */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, "test-process-1", &logs);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_process_logs failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get process logs successfully");

  /* Verify log data if any logs exist */
  if (logs.count > 0) {
    TEST_ASSERT_NOT_NULL(logs.entries, "Log entries array should not be NULL");

    /* Check first log entry has required fields */
    if (logs.entries[0].message) {
      printf("    Found log entry: %s\n", logs.entries[0].message);
    }
  }

  /* Free log list */
  restreamer_api_free_log_list(&logs);

  test_passed = true;
  printf("  ✓ Get process logs success\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process logs with NULL API */
static bool test_get_process_logs_null_api(void) {
  printf("  Testing get process logs with NULL API...\n");

  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(NULL, "test-process-1", &logs);

  TEST_ASSERT(!result, "Should fail with NULL API");

  printf("  ✓ Get process logs NULL API handling\n");
  return true;
}

/* Test: Get process logs with NULL process ID */
static bool test_get_process_logs_null_process_id(void) {
  printf("  Testing get process logs with NULL process ID...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9783)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9783,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get logs with NULL process ID */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, NULL, &logs);
  TEST_ASSERT(!result, "Should fail with NULL process ID");

  test_passed = true;
  printf("  ✓ Get process logs NULL process ID handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process logs with empty process ID */
static bool test_get_process_logs_empty_process_id(void) {
  printf("  Testing get process logs with empty process ID...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9784)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9784,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Try to get logs with empty process ID */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, "", &logs);
  TEST_ASSERT(!result, "Should fail with empty process ID");

  test_passed = true;
  printf("  ✓ Get process logs empty process ID handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Get process logs with NULL output */
static bool test_get_process_logs_null_output(void) {
  printf("  Testing get process logs with NULL output...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9785)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9785,
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
  bool result = restreamer_api_get_process_logs(api, "test-process-1", NULL);
  TEST_ASSERT(!result, "Should fail with NULL output parameter");

  test_passed = true;
  printf("  ✓ Get process logs NULL output handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Free log list with NULL (should be safe) */
static bool test_free_log_list_null(void) {
  printf("  Testing free log list with NULL...\n");

  /* Should not crash */
  restreamer_api_free_log_list(NULL);

  printf("  ✓ Free log list NULL safety\n");
  return true;
}

/* Test: Free log list with empty list */
static bool test_free_log_list_empty(void) {
  printf("  Testing free log list with empty list...\n");

  restreamer_log_list_t logs = {0};
  logs.entries = NULL;
  logs.count = 0;

  /* Should not crash */
  restreamer_api_free_log_list(&logs);

  printf("  ✓ Free log list empty list safety\n");
  return true;
}

/* Test: Session list lifecycle */
static bool test_session_list_lifecycle(void) {
  printf("  Testing session list lifecycle...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9786)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9786,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get sessions */
  restreamer_session_list_t sessions = {0};
  bool result = restreamer_api_get_sessions(api, &sessions);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_sessions failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get sessions successfully");

  /* Verify session structure */
  if (sessions.count > 0) {
    TEST_ASSERT_NOT_NULL(sessions.sessions, "Sessions array should not be NULL");

    /* Verify session fields */
    for (size_t i = 0; i < sessions.count; i++) {
      restreamer_session_t *session = &sessions.sessions[i];

      /* Session ID might be present */
      if (session->session_id) {
        printf("    Session %zu: ID=%s\n", i, session->session_id);
      }

      /* Reference might be present */
      if (session->reference) {
        printf("    Session %zu: Reference=%s\n", i, session->reference);
      }

      /* Remote address might be present */
      if (session->remote_addr) {
        printf("    Session %zu: Remote=%s\n", i, session->remote_addr);
      }

      /* Bytes sent/received should be valid */
      printf("    Session %zu: TX=%llu RX=%llu\n", i,
             (unsigned long long)session->bytes_sent,
             (unsigned long long)session->bytes_received);
    }
  } else {
    printf("    No sessions found (count=0)\n");
  }

  /* Free session list */
  restreamer_api_free_session_list(&sessions);

  /* Verify cleanup - fields should be cleared */
  TEST_ASSERT(sessions.sessions == NULL || sessions.count == 0,
              "Session list should be cleared after free");

  test_passed = true;
  printf("  ✓ Session list lifecycle\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Log list lifecycle */
static bool test_log_list_lifecycle(void) {
  printf("  Testing log list lifecycle...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9787)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9787,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get process logs */
  restreamer_log_list_t logs = {0};
  bool result = restreamer_api_get_process_logs(api, "test-process-1", &logs);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  ✗ get_process_logs failed: %s\n",
            error ? error : "unknown error");
    goto cleanup;
  }

  TEST_ASSERT(result, "Should get process logs successfully");

  /* Verify log structure */
  if (logs.count > 0) {
    TEST_ASSERT_NOT_NULL(logs.entries, "Log entries array should not be NULL");

    /* Verify log entry fields */
    for (size_t i = 0; i < logs.count; i++) {
      restreamer_log_entry_t *entry = &logs.entries[i];

      /* Timestamp might be present */
      if (entry->timestamp) {
        printf("    Log %zu: Timestamp=%s\n", i, entry->timestamp);
      }

      /* Message might be present */
      if (entry->message) {
        printf("    Log %zu: Message=%s\n", i, entry->message);
      }

      /* Level might be present */
      if (entry->level) {
        printf("    Log %zu: Level=%s\n", i, entry->level);
      }
    }
  } else {
    printf("    No log entries found (count=0)\n");
  }

  /* Free log list */
  restreamer_api_free_log_list(&logs);

  /* Verify cleanup - fields should be cleared */
  TEST_ASSERT(logs.entries == NULL || logs.count == 0,
              "Log list should be cleared after free");

  test_passed = true;
  printf("  ✓ Log list lifecycle\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Multiple get operations */
static bool test_multiple_get_operations(void) {
  printf("  Testing multiple get operations...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9788)) {
    fprintf(stderr, "  ✗ Failed to start mock server\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9788,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Get sessions multiple times */
  for (int i = 0; i < 3; i++) {
    restreamer_session_list_t sessions = {0};
    bool result = restreamer_api_get_sessions(api, &sessions);
    if (!result) {
      const char *error = restreamer_api_get_error(api);
      fprintf(stderr, "  ✗ get_sessions iteration %d failed: %s\n", i,
              error ? error : "unknown error");
      goto cleanup;
    }
    TEST_ASSERT(result, "Should get sessions successfully in iteration");
    restreamer_api_free_session_list(&sessions);
  }

  /* Get logs multiple times */
  for (int i = 0; i < 3; i++) {
    restreamer_log_list_t logs = {0};
    bool result = restreamer_api_get_process_logs(api, "test-process-1", &logs);
    if (!result) {
      const char *error = restreamer_api_get_error(api);
      fprintf(stderr, "  ✗ get_process_logs iteration %d failed: %s\n", i,
              error ? error : "unknown error");
      goto cleanup;
    }
    TEST_ASSERT(result, "Should get logs successfully in iteration");
    restreamer_api_free_log_list(&logs);
  }

  test_passed = true;
  printf("  ✓ Multiple get operations\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Free operations idempotency */
static bool test_free_operations_idempotency(void) {
  printf("  Testing free operations idempotency...\n");

  /* Create and free session list multiple times */
  restreamer_session_list_t sessions = {0};
  restreamer_api_free_session_list(&sessions);
  restreamer_api_free_session_list(&sessions);
  restreamer_api_free_session_list(&sessions);

  /* Create and free log list multiple times */
  restreamer_log_list_t logs = {0};
  restreamer_api_free_log_list(&logs);
  restreamer_api_free_log_list(&logs);
  restreamer_api_free_log_list(&logs);

  printf("  ✓ Free operations idempotency\n");
  return true;
}

/* Run all API sessions tests */
int run_api_sessions_tests(void) {
  printf("\nRunning API Sessions Tests...\n");
  printf("========================================\n");

  int failed = 0;

  /* Session list tests */
  if (!test_get_sessions_success()) failed++;
  if (!test_get_sessions_null_api()) failed++;
  if (!test_get_sessions_null_output()) failed++;
  if (!test_free_session_list_null()) failed++;
  if (!test_free_session_list_empty()) failed++;

  /* Process logs tests */
  if (!test_get_process_logs_success()) failed++;
  if (!test_get_process_logs_null_api()) failed++;
  if (!test_get_process_logs_null_process_id()) failed++;
  if (!test_get_process_logs_empty_process_id()) failed++;
  if (!test_get_process_logs_null_output()) failed++;
  if (!test_free_log_list_null()) failed++;
  if (!test_free_log_list_empty()) failed++;

  /* Lifecycle tests */
  if (!test_session_list_lifecycle()) failed++;
  if (!test_log_list_lifecycle()) failed++;

  /* Integration tests */
  if (!test_multiple_get_operations()) failed++;
  if (!test_free_operations_idempotency()) failed++;

  printf("========================================\n");
  if (failed == 0) {
    printf("All API sessions tests passed!\n");
    return 0;
  } else {
    printf("%d test(s) failed\n", failed);
    return 1;
  }
}
