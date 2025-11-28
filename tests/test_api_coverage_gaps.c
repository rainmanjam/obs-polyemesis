/*
 * API Coverage Gaps Tests
 *
 * Tests focusing on improving code coverage for uncovered code paths in
 * restreamer-api.c. This file specifically targets:
 * - NULL parameter handling for various API functions
 * - Empty string parameter handling
 * - Edge cases in cleanup/free functions
 * - Error paths and boundary conditions
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

#define TEST_ASSERT_NULL(ptr, message)                                         \
  do {                                                                         \
    if ((ptr) != NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected NULL pointer\n    at %s:%d\n",       \
              message, __FILE__, __LINE__);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* ============================================================================
 * Skills API Additional Coverage
 * ========================================================================= */

/* Test: Free skills with NULL (should be safe) - not a function but test coverage */
static bool test_skills_api_edge_cases(void) {
  printf("  Testing skills API edge cases...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9950)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9950\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9950,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Test getting skills and freeing the result */
  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(api, &skills_json);

  if (result && skills_json) {
    /* Free the skills JSON string */
    free(skills_json);
    skills_json = NULL;
  }

  test_passed = true;
  printf("  ✓ Skills API edge cases\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* ============================================================================
 * Filesystem API Additional Coverage
 * ========================================================================= */

/* Test: list_files with empty storage string */
static bool test_list_files_empty_storage(void) {
  printf("  Testing list_files with empty storage...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9951)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9951\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9951,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Test with empty storage string (should fail or handle gracefully) */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "", NULL, &files);

  /* Empty storage may fail on server side, but should not crash */
  (void)result;

  /* Clean up in case it succeeded */
  restreamer_api_free_fs_list(&files);

  test_passed = true;
  printf("  ✓ List files empty storage handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: list_files with various glob patterns */
static bool test_list_files_glob_patterns(void) {
  printf("  Testing list_files with various glob patterns...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9952)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9952\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9952,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Test with empty glob pattern */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "disk", "", &files);
  (void)result;
  restreamer_api_free_fs_list(&files);

  /* Test with wildcard glob pattern */
  memset(&files, 0, sizeof(files));
  result = restreamer_api_list_files(api, "disk", "*", &files);
  (void)result;
  restreamer_api_free_fs_list(&files);

  /* Test with complex glob pattern */
  memset(&files, 0, sizeof(files));
  result = restreamer_api_list_files(api, "disk", "test[0-9].mp4", &files);
  (void)result;
  restreamer_api_free_fs_list(&files);

  test_passed = true;
  printf("  ✓ List files glob patterns handling\n");

cleanup:
  if (api) {
    restreamer_api_destroy(api);
  }
  mock_restreamer_stop();

  return test_passed;
}

/* Test: Free fs_list with partial data */
static bool test_free_fs_list_partial(void) {
  printf("  Testing free fs_list with partial data...\n");

  /* Create a partially filled fs_list */
  restreamer_fs_list_t files = {0};
  files.count = 2;
  files.entries = bzalloc(sizeof(restreamer_fs_entry_t) * 2);

  /* Only fill first entry */
  files.entries[0].name = bstrdup("test1.txt");
  files.entries[0].path = bstrdup("/path/to/test1.txt");
  /* Second entry has NULL fields */
  files.entries[1].name = NULL;
  files.entries[1].path = NULL;

  /* Should handle partial data safely */
  restreamer_api_free_fs_list(&files);

  /* Verify cleanup */
  TEST_ASSERT(files.entries == NULL, "Entries should be NULL after free");
  TEST_ASSERT(files.count == 0, "Count should be 0 after free");

  printf("  ✓ Free fs_list partial data handling\n");
  return true;
}

/* Test: Free fs_list multiple times (idempotency) */
static bool test_free_fs_list_idempotent(void) {
  printf("  Testing free fs_list idempotency...\n");

  restreamer_fs_list_t files = {0};

  /* Free multiple times should be safe */
  restreamer_api_free_fs_list(&files);
  restreamer_api_free_fs_list(&files);
  restreamer_api_free_fs_list(&files);

  printf("  ✓ Free fs_list idempotency\n");
  return true;
}

/* ============================================================================
 * Session API Additional Coverage
 * ========================================================================= */

/* Test: Free session_list with partial data */
static bool test_free_session_list_partial(void) {
  printf("  Testing free session_list with partial data...\n");

  /* Create a partially filled session_list */
  restreamer_session_list_t sessions = {0};
  sessions.count = 2;
  sessions.sessions = bzalloc(sizeof(restreamer_session_t) * 2);

  /* Only fill first session partially */
  sessions.sessions[0].session_id = bstrdup("session-1");
  sessions.sessions[0].reference = NULL;  /* NULL field */
  sessions.sessions[0].remote_addr = bstrdup("127.0.0.1");

  /* Second session completely NULL */
  sessions.sessions[1].session_id = NULL;
  sessions.sessions[1].reference = NULL;
  sessions.sessions[1].remote_addr = NULL;

  /* Should handle partial data safely */
  restreamer_api_free_session_list(&sessions);

  /* Verify cleanup */
  TEST_ASSERT(sessions.sessions == NULL, "Sessions should be NULL after free");
  TEST_ASSERT(sessions.count == 0, "Count should be 0 after free");

  printf("  ✓ Free session_list partial data handling\n");
  return true;
}

/* Test: Free session_list idempotency */
static bool test_free_session_list_idempotent(void) {
  printf("  Testing free session_list idempotency...\n");

  restreamer_session_list_t sessions = {0};

  /* Free multiple times should be safe */
  restreamer_api_free_session_list(&sessions);
  restreamer_api_free_session_list(&sessions);
  restreamer_api_free_session_list(&sessions);

  printf("  ✓ Free session_list idempotency\n");
  return true;
}

/* Test: Get sessions with connection issues */
static bool test_get_sessions_connection_error(void) {
  printf("  Testing get sessions with connection error...\n");

  /* Create API client pointing to non-existent server */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9999,  /* Port with no server */
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Try to get sessions - should fail gracefully */
  restreamer_session_list_t sessions = {0};
  bool result = restreamer_api_get_sessions(api, &sessions);

  TEST_ASSERT(!result, "Should fail when server is unreachable");

  /* Verify no partial data was allocated */
  TEST_ASSERT(sessions.sessions == NULL, "Sessions should be NULL on failure");
  TEST_ASSERT(sessions.count == 0, "Count should be 0 on failure");

  restreamer_api_destroy(api);

  printf("  ✓ Get sessions connection error handling\n");
  return true;
}

/* ============================================================================
 * Log List API Additional Coverage
 * ========================================================================= */

/* Test: Free log_list with partial data */
static bool test_free_log_list_partial(void) {
  printf("  Testing free log_list with partial data...\n");

  /* Create a partially filled log_list */
  restreamer_log_list_t logs = {0};
  logs.count = 3;
  logs.entries = bzalloc(sizeof(restreamer_log_entry_t) * 3);

  /* First entry fully filled */
  logs.entries[0].timestamp = bstrdup("2024-01-01T00:00:00Z");
  logs.entries[0].message = bstrdup("Test message 1");
  logs.entries[0].level = bstrdup("info");

  /* Second entry partially filled */
  logs.entries[1].timestamp = bstrdup("2024-01-01T00:00:01Z");
  logs.entries[1].message = NULL;  /* NULL message */
  logs.entries[1].level = bstrdup("warn");

  /* Third entry completely NULL */
  logs.entries[2].timestamp = NULL;
  logs.entries[2].message = NULL;
  logs.entries[2].level = NULL;

  /* Should handle partial data safely */
  restreamer_api_free_log_list(&logs);

  /* Verify cleanup */
  TEST_ASSERT(logs.entries == NULL, "Entries should be NULL after free");
  TEST_ASSERT(logs.count == 0, "Count should be 0 after free");

  printf("  ✓ Free log_list partial data handling\n");
  return true;
}

/* Test: Free log_list idempotency */
static bool test_free_log_list_idempotent(void) {
  printf("  Testing free log_list idempotency...\n");

  restreamer_log_list_t logs = {0};

  /* Free multiple times should be safe */
  restreamer_api_free_log_list(&logs);
  restreamer_api_free_log_list(&logs);
  restreamer_api_free_log_list(&logs);

  printf("  ✓ Free log_list idempotency\n");
  return true;
}

/* ============================================================================
 * Process API Additional Coverage
 * ========================================================================= */

/* Test: Free process with partial data */
static bool test_free_process_partial(void) {
  printf("  Testing free process with partial data...\n");

  restreamer_process_t process = {0};

  /* Partially fill process */
  process.id = bstrdup("process-1");
  process.reference = NULL;  /* NULL field */
  process.state = bstrdup("running");
  process.command = NULL;  /* NULL field */

  /* Should handle partial data safely */
  restreamer_api_free_process(&process);

  /* Verify cleanup */
  TEST_ASSERT(process.id == NULL, "ID should be NULL after free");
  TEST_ASSERT(process.reference == NULL, "Reference should be NULL after free");
  TEST_ASSERT(process.state == NULL, "State should be NULL after free");
  TEST_ASSERT(process.command == NULL, "Command should be NULL after free");

  printf("  ✓ Free process partial data handling\n");
  return true;
}

/* Test: Free process with NULL (should be safe) */
static bool test_free_process_null(void) {
  printf("  Testing free process with NULL...\n");

  /* Should not crash */
  restreamer_api_free_process(NULL);

  printf("  ✓ Free process NULL safety\n");
  return true;
}

/* Test: Free process multiple times */
static bool test_free_process_idempotent(void) {
  printf("  Testing free process idempotency...\n");

  restreamer_process_t process = {0};

  /* Free multiple times should be safe */
  restreamer_api_free_process(&process);
  restreamer_api_free_process(&process);
  restreamer_api_free_process(&process);

  printf("  ✓ Free process idempotency\n");
  return true;
}

/* ============================================================================
 * API Info Additional Coverage
 * ========================================================================= */

/* Test: Free info with partial data */
static bool test_free_info_partial(void) {
  printf("  Testing free info with partial data...\n");

  restreamer_api_info_t info = {0};

  /* Partially fill info */
  info.name = bstrdup("datarhei-core");
  info.version = NULL;  /* NULL field */
  info.build_date = bstrdup("2024-01-01");
  info.commit = NULL;  /* NULL field */

  /* Should handle partial data safely */
  restreamer_api_free_info(&info);

  /* Verify cleanup */
  TEST_ASSERT(info.name == NULL, "Name should be NULL after free");
  TEST_ASSERT(info.version == NULL, "Version should be NULL after free");
  TEST_ASSERT(info.build_date == NULL, "Build date should be NULL after free");
  TEST_ASSERT(info.commit == NULL, "Commit should be NULL after free");

  printf("  ✓ Free info partial data handling\n");
  return true;
}

/* Test: Free info idempotency */
static bool test_free_info_idempotent(void) {
  printf("  Testing free info idempotency...\n");

  restreamer_api_info_t info = {0};

  /* Free multiple times should be safe */
  restreamer_api_free_info(&info);
  restreamer_api_free_info(&info);
  restreamer_api_free_info(&info);

  printf("  ✓ Free info idempotency\n");
  return true;
}

/* ============================================================================
 * Error Handling Additional Coverage
 * ========================================================================= */

/* Test: Get error with NULL API */
static bool test_get_error_null_api(void) {
  printf("  Testing get error with NULL API...\n");

  const char *error = restreamer_api_get_error(NULL);

  TEST_ASSERT_NOT_NULL(error, "Should return error message for NULL API");
  TEST_ASSERT(strcmp(error, "Invalid API instance") == 0,
              "Should return 'Invalid API instance' message");

  printf("  ✓ Get error NULL API handling\n");
  return true;
}

/* Test: Get error after various failures */
static bool test_get_error_after_failures(void) {
  printf("  Testing get error after various failures...\n");

  /* Create API client pointing to non-existent server */
  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9999,  /* Port with no server */
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Trigger various failures and check error messages */

  /* Test connection failure */
  bool result = restreamer_api_test_connection(api);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    TEST_ASSERT_NOT_NULL(error, "Error message should be set after connection failure");
    printf("    Connection error: %s\n", error);
  }

  /* Test get_sessions failure */
  restreamer_session_list_t sessions = {0};
  result = restreamer_api_get_sessions(api, &sessions);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    TEST_ASSERT_NOT_NULL(error, "Error message should be set after get_sessions failure");
    printf("    Get sessions error: %s\n", error);
  }

  restreamer_api_destroy(api);

  printf("  ✓ Get error after failures\n");
  return true;
}

/* ============================================================================
 * Combined Lifecycle Tests
 * ========================================================================= */

/* Test: Multiple API operations in sequence */
static bool test_multiple_api_operations(void) {
  printf("  Testing multiple API operations in sequence...\n");

  restreamer_api_t *api = NULL;
  bool test_passed = false;

  if (!mock_restreamer_start(9953)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9953\n");
    return false;
  }

  sleep_ms(500);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9953,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  api = restreamer_api_create(&conn);
  if (!api) {
    fprintf(stderr, "  ✗ Failed to create API client\n");
    goto cleanup;
  }

  /* Perform multiple operations */

  /* 1. Get skills */
  char *skills_json = NULL;
  bool result = restreamer_api_get_skills(api, &skills_json);
  if (result && skills_json) {
    free(skills_json);
    skills_json = NULL;
  }

  /* 2. Get sessions */
  restreamer_session_list_t sessions = {0};
  result = restreamer_api_get_sessions(api, &sessions);
  if (result) {
    restreamer_api_free_session_list(&sessions);
  }

  /* 3. List filesystems */
  char *filesystems_json = NULL;
  result = restreamer_api_list_filesystems(api, &filesystems_json);
  if (result && filesystems_json) {
    free(filesystems_json);
    filesystems_json = NULL;
  }

  /* 4. List files */
  restreamer_fs_list_t files = {0};
  result = restreamer_api_list_files(api, "disk", NULL, &files);
  if (result) {
    restreamer_api_free_fs_list(&files);
  }

  test_passed = true;
  printf("  ✓ Multiple API operations\n");

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

/* Run all coverage gap tests */
int run_api_coverage_gaps_tests(void) {
  int failed = 0;

  printf("\n========================================\n");
  printf("API Coverage Gaps Tests\n");
  printf("========================================\n\n");

  /* Skills API tests */
  printf("Skills API Coverage:\n");
  if (!test_skills_api_edge_cases()) failed++;

  /* Filesystem API tests */
  printf("\nFilesystem API Coverage:\n");
  if (!test_list_files_empty_storage()) failed++;
  if (!test_list_files_glob_patterns()) failed++;
  if (!test_free_fs_list_partial()) failed++;
  if (!test_free_fs_list_idempotent()) failed++;

  /* Session API tests */
  printf("\nSession API Coverage:\n");
  if (!test_free_session_list_partial()) failed++;
  if (!test_free_session_list_idempotent()) failed++;
  if (!test_get_sessions_connection_error()) failed++;

  /* Log List API tests */
  printf("\nLog List API Coverage:\n");
  if (!test_free_log_list_partial()) failed++;
  if (!test_free_log_list_idempotent()) failed++;

  /* Process API tests */
  printf("\nProcess API Coverage:\n");
  if (!test_free_process_partial()) failed++;
  if (!test_free_process_null()) failed++;
  if (!test_free_process_idempotent()) failed++;

  /* API Info tests */
  printf("\nAPI Info Coverage:\n");
  if (!test_free_info_partial()) failed++;
  if (!test_free_info_idempotent()) failed++;

  /* Error handling tests */
  printf("\nError Handling Coverage:\n");
  if (!test_get_error_null_api()) failed++;
  if (!test_get_error_after_failures()) failed++;

  /* Combined lifecycle tests */
  printf("\nCombined Lifecycle Tests:\n");
  if (!test_multiple_api_operations()) failed++;

  if (failed == 0) {
    printf("\n✓ All coverage gap tests passed!\n");
  } else {
    printf("\n✗ %d test(s) failed\n", failed);
  }

  return failed;
}
