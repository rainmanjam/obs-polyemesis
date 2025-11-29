/*
 * API Parse Helper Functions Tests
 *
 * Comprehensive tests for the JSON parsing helper functions in restreamer-api.c
 * to improve test coverage.
 *
 * This file tests the following static helper functions (exposed via TESTING_MODE):
 * - parse_log_entry_fields() - lines 597-617
 * - parse_session_fields() - lines 620-650
 * - parse_fs_entry_fields() - lines 653-683
 *
 * Note: parse_process_fields() is already tested in other test suites.
 * TESTING_MODE is already defined in CMakeLists.txt target_compile_definitions
 */

#include "restreamer-api.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <util/bmem.h>

/* External declarations for static functions exposed via TESTING_MODE */
extern void parse_log_entry_fields(const json_t *json_obj,
                                   restreamer_log_entry_t *entry);
extern void parse_session_fields(const json_t *json_obj,
                                 restreamer_session_t *session);
extern void parse_fs_entry_fields(const json_t *json_obj,
                                  restreamer_fs_entry_t *entry);

/* Test macros */
#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
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

#define TEST_ASSERT_NOT_NULL(ptr, message)                                     \
  do {                                                                         \
    if ((ptr) == NULL) {                                                       \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected non-NULL pointer\n    at %s:%d\n",   \
              message, __FILE__, __LINE__);                                    \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message)                       \
  do {                                                                         \
    if (strcmp((expected), (actual)) != 0) {                                   \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: \"%s\", Actual: \"%s\"\n    at "    \
              "%s:%d\n",                                                       \
              message, (expected), (actual), __FILE__, __LINE__);              \
      return false;                                                            \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %lld, Actual: %lld\n    at %s:%d\n", \
              message, (long long)(expected), (long long)(actual), __FILE__, __LINE__); \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* ========================================================================
 * parse_log_entry_fields() Tests
 * ======================================================================== */

/* Test: Parse log entry with all fields complete */
static bool test_parse_log_entry_fields_complete(void) {
  printf("  Testing parse_log_entry_fields with complete data...\n");

  /* Create JSON object with all fields */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "timestamp", json_string("2024-01-15T10:30:00Z"));
  json_object_set_new(json_obj, "message", json_string("Stream started successfully"));
  json_object_set_new(json_obj, "level", json_string("info"));

  /* Initialize log entry */
  restreamer_log_entry_t entry = {0};

  /* Parse the JSON */
  parse_log_entry_fields(json_obj, &entry);

  /* Verify all fields were parsed correctly */
  TEST_ASSERT_NOT_NULL(entry.timestamp, "timestamp should not be NULL");
  TEST_ASSERT_STR_EQUAL("2024-01-15T10:30:00Z", entry.timestamp, "timestamp mismatch");

  TEST_ASSERT_NOT_NULL(entry.message, "message should not be NULL");
  TEST_ASSERT_STR_EQUAL("Stream started successfully", entry.message, "message mismatch");

  TEST_ASSERT_NOT_NULL(entry.level, "level should not be NULL");
  TEST_ASSERT_STR_EQUAL("info", entry.level, "level mismatch");

  /* Cleanup */
  restreamer_api_free_log_list(&(restreamer_log_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_log_entry_fields complete data\n");
  return true;
}

/* Test: Parse log entry with some fields missing */
static bool test_parse_log_entry_fields_partial(void) {
  printf("  Testing parse_log_entry_fields with partial data...\n");

  /* Create JSON object with only timestamp and message */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "timestamp", json_string("2024-01-15T10:30:00Z"));
  json_object_set_new(json_obj, "message", json_string("Partial log entry"));
  /* level is missing */

  /* Initialize log entry */
  restreamer_log_entry_t entry = {0};

  /* Parse the JSON */
  parse_log_entry_fields(json_obj, &entry);

  /* Verify parsed fields */
  TEST_ASSERT_NOT_NULL(entry.timestamp, "timestamp should not be NULL");
  TEST_ASSERT_STR_EQUAL("2024-01-15T10:30:00Z", entry.timestamp, "timestamp mismatch");

  TEST_ASSERT_NOT_NULL(entry.message, "message should not be NULL");
  TEST_ASSERT_STR_EQUAL("Partial log entry", entry.message, "message mismatch");

  /* level should be NULL since it wasn't in JSON */
  TEST_ASSERT_NULL(entry.level, "level should be NULL when not present");

  /* Cleanup */
  restreamer_api_free_log_list(&(restreamer_log_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_log_entry_fields partial data\n");
  return true;
}

/* Test: Parse log entry with NULL JSON input */
static bool test_parse_log_entry_fields_null_input(void) {
  printf("  Testing parse_log_entry_fields with NULL input...\n");

  restreamer_log_entry_t entry = {0};

  /* Parse with NULL JSON - should return without crashing */
  parse_log_entry_fields(NULL, &entry);

  /* Verify entry is still empty */
  TEST_ASSERT_NULL(entry.timestamp, "timestamp should remain NULL");
  TEST_ASSERT_NULL(entry.message, "message should remain NULL");
  TEST_ASSERT_NULL(entry.level, "level should remain NULL");

  printf("  ✓ parse_log_entry_fields NULL input handling\n");
  return true;
}

/* Test: Parse log entry with NULL entry pointer */
static bool test_parse_log_entry_fields_null_entry(void) {
  printf("  Testing parse_log_entry_fields with NULL entry pointer...\n");

  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "timestamp", json_string("2024-01-15T10:30:00Z"));

  /* Parse with NULL entry - should return without crashing */
  parse_log_entry_fields(json_obj, NULL);

  json_decref(json_obj);

  printf("  ✓ parse_log_entry_fields NULL entry handling\n");
  return true;
}

/* Test: Parse log entry with wrong field types */
static bool test_parse_log_entry_fields_wrong_types(void) {
  printf("  Testing parse_log_entry_fields with wrong field types...\n");

  /* Create JSON with non-string values */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "timestamp", json_integer(12345)); // Wrong type
  json_object_set_new(json_obj, "message", json_string("Valid message"));
  json_object_set_new(json_obj, "level", json_boolean(true)); // Wrong type

  restreamer_log_entry_t entry = {0};
  parse_log_entry_fields(json_obj, &entry);

  /* Only message should be parsed (correct type) */
  TEST_ASSERT_NULL(entry.timestamp, "timestamp should be NULL (wrong type)");
  TEST_ASSERT_NOT_NULL(entry.message, "message should be parsed");
  TEST_ASSERT_NULL(entry.level, "level should be NULL (wrong type)");

  /* Cleanup */
  restreamer_api_free_log_list(&(restreamer_log_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_log_entry_fields wrong types handling\n");
  return true;
}

/* ========================================================================
 * parse_session_fields() Tests
 * ======================================================================== */

/* Test: Parse session with all fields complete */
static bool test_parse_session_fields_complete(void) {
  printf("  Testing parse_session_fields with complete data...\n");

  /* Create JSON object with all fields */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "id", json_string("session-abc123"));
  json_object_set_new(json_obj, "reference", json_string("stream-main"));
  json_object_set_new(json_obj, "bytes_sent", json_integer(1024000));
  json_object_set_new(json_obj, "bytes_received", json_integer(2048000));
  json_object_set_new(json_obj, "remote_addr", json_string("192.168.1.100"));

  /* Initialize session */
  restreamer_session_t session = {0};

  /* Parse the JSON */
  parse_session_fields(json_obj, &session);

  /* Verify all fields were parsed correctly */
  TEST_ASSERT_NOT_NULL(session.session_id, "session_id should not be NULL");
  TEST_ASSERT_STR_EQUAL("session-abc123", session.session_id, "session_id mismatch");

  TEST_ASSERT_NOT_NULL(session.reference, "reference should not be NULL");
  TEST_ASSERT_STR_EQUAL("stream-main", session.reference, "reference mismatch");

  TEST_ASSERT_EQUAL(1024000, session.bytes_sent, "bytes_sent mismatch");
  TEST_ASSERT_EQUAL(2048000, session.bytes_received, "bytes_received mismatch");

  TEST_ASSERT_NOT_NULL(session.remote_addr, "remote_addr should not be NULL");
  TEST_ASSERT_STR_EQUAL("192.168.1.100", session.remote_addr, "remote_addr mismatch");

  /* Cleanup */
  restreamer_api_free_session_list(&(restreamer_session_list_t){.sessions = &session, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_session_fields complete data\n");
  return true;
}

/* Test: Parse session with some fields missing */
static bool test_parse_session_fields_partial(void) {
  printf("  Testing parse_session_fields with partial data...\n");

  /* Create JSON object with only id and bytes_sent */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "id", json_string("session-xyz789"));
  json_object_set_new(json_obj, "bytes_sent", json_integer(512000));
  /* reference, bytes_received, and remote_addr are missing */

  /* Initialize session */
  restreamer_session_t session = {0};

  /* Parse the JSON */
  parse_session_fields(json_obj, &session);

  /* Verify parsed fields */
  TEST_ASSERT_NOT_NULL(session.session_id, "session_id should not be NULL");
  TEST_ASSERT_STR_EQUAL("session-xyz789", session.session_id, "session_id mismatch");

  TEST_ASSERT_EQUAL(512000, session.bytes_sent, "bytes_sent mismatch");

  /* Missing fields should be NULL/0 */
  TEST_ASSERT_NULL(session.reference, "reference should be NULL when not present");
  TEST_ASSERT_EQUAL(0, session.bytes_received, "bytes_received should be 0 when not present");
  TEST_ASSERT_NULL(session.remote_addr, "remote_addr should be NULL when not present");

  /* Cleanup */
  restreamer_api_free_session_list(&(restreamer_session_list_t){.sessions = &session, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_session_fields partial data\n");
  return true;
}

/* Test: Parse session with NULL JSON input */
static bool test_parse_session_fields_null_input(void) {
  printf("  Testing parse_session_fields with NULL input...\n");

  restreamer_session_t session = {0};

  /* Parse with NULL JSON - should return without crashing */
  parse_session_fields(NULL, &session);

  /* Verify session is still empty */
  TEST_ASSERT_NULL(session.session_id, "session_id should remain NULL");
  TEST_ASSERT_NULL(session.reference, "reference should remain NULL");
  TEST_ASSERT_EQUAL(0, session.bytes_sent, "bytes_sent should remain 0");
  TEST_ASSERT_EQUAL(0, session.bytes_received, "bytes_received should remain 0");
  TEST_ASSERT_NULL(session.remote_addr, "remote_addr should remain NULL");

  printf("  ✓ parse_session_fields NULL input handling\n");
  return true;
}

/* Test: Parse session with NULL session pointer */
static bool test_parse_session_fields_null_session(void) {
  printf("  Testing parse_session_fields with NULL session pointer...\n");

  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "id", json_string("session-test"));

  /* Parse with NULL session - should return without crashing */
  parse_session_fields(json_obj, NULL);

  json_decref(json_obj);

  printf("  ✓ parse_session_fields NULL session handling\n");
  return true;
}

/* Test: Parse session with wrong field types */
static bool test_parse_session_fields_wrong_types(void) {
  printf("  Testing parse_session_fields with wrong field types...\n");

  /* Create JSON with mixed correct and wrong types */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "id", json_string("session-valid"));
  json_object_set_new(json_obj, "reference", json_integer(12345)); // Wrong type
  json_object_set_new(json_obj, "bytes_sent", json_string("not-a-number")); // Wrong type
  json_object_set_new(json_obj, "bytes_received", json_integer(1024));
  json_object_set_new(json_obj, "remote_addr", json_array()); // Wrong type

  restreamer_session_t session = {0};
  parse_session_fields(json_obj, &session);

  /* Only correctly typed fields should be parsed */
  TEST_ASSERT_NOT_NULL(session.session_id, "session_id should be parsed");
  TEST_ASSERT_NULL(session.reference, "reference should be NULL (wrong type)");
  TEST_ASSERT_EQUAL(0, session.bytes_sent, "bytes_sent should be 0 (wrong type)");
  TEST_ASSERT_EQUAL(1024, session.bytes_received, "bytes_received should be parsed");
  TEST_ASSERT_NULL(session.remote_addr, "remote_addr should be NULL (wrong type)");

  /* Cleanup */
  restreamer_api_free_session_list(&(restreamer_session_list_t){.sessions = &session, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_session_fields wrong types handling\n");
  return true;
}

/* ========================================================================
 * parse_fs_entry_fields() Tests
 * ======================================================================== */

/* Test: Parse file entry with all fields */
static bool test_parse_fs_entry_fields_file(void) {
  printf("  Testing parse_fs_entry_fields with file entry...\n");

  /* Create JSON object for a file */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("video.mp4"));
  json_object_set_new(json_obj, "path", json_string("/media/videos/video.mp4"));
  json_object_set_new(json_obj, "size", json_integer(10485760)); // 10MB
  json_object_set_new(json_obj, "modified", json_integer(1705318800)); // Unix timestamp
  json_object_set_new(json_obj, "is_directory", json_false());

  /* Initialize fs_entry */
  restreamer_fs_entry_t entry = {0};

  /* Parse the JSON */
  parse_fs_entry_fields(json_obj, &entry);

  /* Verify all fields were parsed correctly */
  TEST_ASSERT_NOT_NULL(entry.name, "name should not be NULL");
  TEST_ASSERT_STR_EQUAL("video.mp4", entry.name, "name mismatch");

  TEST_ASSERT_NOT_NULL(entry.path, "path should not be NULL");
  TEST_ASSERT_STR_EQUAL("/media/videos/video.mp4", entry.path, "path mismatch");

  TEST_ASSERT_EQUAL(10485760, entry.size, "size mismatch");
  TEST_ASSERT_EQUAL(1705318800, entry.modified, "modified timestamp mismatch");
  TEST_ASSERT(entry.is_directory == false, "is_directory should be false for file");

  /* Cleanup */
  restreamer_api_free_fs_list(&(restreamer_fs_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields file entry\n");
  return true;
}

/* Test: Parse directory entry with all fields */
static bool test_parse_fs_entry_fields_directory(void) {
  printf("  Testing parse_fs_entry_fields with directory entry...\n");

  /* Create JSON object for a directory */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("recordings"));
  json_object_set_new(json_obj, "path", json_string("/media/recordings"));
  json_object_set_new(json_obj, "size", json_integer(0)); // Directories typically size 0
  json_object_set_new(json_obj, "modified", json_integer(1705318900));
  json_object_set_new(json_obj, "is_directory", json_true());

  /* Initialize fs_entry */
  restreamer_fs_entry_t entry = {0};

  /* Parse the JSON */
  parse_fs_entry_fields(json_obj, &entry);

  /* Verify all fields were parsed correctly */
  TEST_ASSERT_NOT_NULL(entry.name, "name should not be NULL");
  TEST_ASSERT_STR_EQUAL("recordings", entry.name, "name mismatch");

  TEST_ASSERT_NOT_NULL(entry.path, "path should not be NULL");
  TEST_ASSERT_STR_EQUAL("/media/recordings", entry.path, "path mismatch");

  TEST_ASSERT_EQUAL(0, entry.size, "size should be 0 for directory");
  TEST_ASSERT_EQUAL(1705318900, entry.modified, "modified timestamp mismatch");
  TEST_ASSERT(entry.is_directory == true, "is_directory should be true for directory");

  /* Cleanup */
  restreamer_api_free_fs_list(&(restreamer_fs_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields directory entry\n");
  return true;
}

/* Test: Parse fs_entry with some fields missing */
static bool test_parse_fs_entry_fields_partial(void) {
  printf("  Testing parse_fs_entry_fields with partial data...\n");

  /* Create JSON object with only name and path */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("partial.txt"));
  json_object_set_new(json_obj, "path", json_string("/tmp/partial.txt"));
  /* size, modified, and is_directory are missing */

  /* Initialize fs_entry */
  restreamer_fs_entry_t entry = {0};

  /* Parse the JSON */
  parse_fs_entry_fields(json_obj, &entry);

  /* Verify parsed fields */
  TEST_ASSERT_NOT_NULL(entry.name, "name should not be NULL");
  TEST_ASSERT_STR_EQUAL("partial.txt", entry.name, "name mismatch");

  TEST_ASSERT_NOT_NULL(entry.path, "path should not be NULL");
  TEST_ASSERT_STR_EQUAL("/tmp/partial.txt", entry.path, "path mismatch");

  /* Missing numeric/boolean fields should be 0/false */
  TEST_ASSERT_EQUAL(0, entry.size, "size should be 0 when not present");
  TEST_ASSERT_EQUAL(0, entry.modified, "modified should be 0 when not present");
  TEST_ASSERT(entry.is_directory == false, "is_directory should be false when not present");

  /* Cleanup */
  restreamer_api_free_fs_list(&(restreamer_fs_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields partial data\n");
  return true;
}

/* Test: Parse fs_entry with NULL JSON input */
static bool test_parse_fs_entry_fields_null_input(void) {
  printf("  Testing parse_fs_entry_fields with NULL input...\n");

  restreamer_fs_entry_t entry = {0};

  /* Parse with NULL JSON - should return without crashing */
  parse_fs_entry_fields(NULL, &entry);

  /* Verify entry is still empty */
  TEST_ASSERT_NULL(entry.name, "name should remain NULL");
  TEST_ASSERT_NULL(entry.path, "path should remain NULL");
  TEST_ASSERT_EQUAL(0, entry.size, "size should remain 0");
  TEST_ASSERT_EQUAL(0, entry.modified, "modified should remain 0");
  TEST_ASSERT(entry.is_directory == false, "is_directory should remain false");

  printf("  ✓ parse_fs_entry_fields NULL input handling\n");
  return true;
}

/* Test: Parse fs_entry with NULL entry pointer */
static bool test_parse_fs_entry_fields_null_entry(void) {
  printf("  Testing parse_fs_entry_fields with NULL entry pointer...\n");

  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("test.txt"));

  /* Parse with NULL entry - should return without crashing */
  parse_fs_entry_fields(json_obj, NULL);

  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields NULL entry handling\n");
  return true;
}

/* Test: Parse fs_entry with wrong field types */
static bool test_parse_fs_entry_fields_wrong_types(void) {
  printf("  Testing parse_fs_entry_fields with wrong field types...\n");

  /* Create JSON with mixed correct and wrong types */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("valid-name.txt"));
  json_object_set_new(json_obj, "path", json_integer(12345)); // Wrong type
  json_object_set_new(json_obj, "size", json_string("not-a-number")); // Wrong type
  json_object_set_new(json_obj, "modified", json_integer(1705318800));
  json_object_set_new(json_obj, "is_directory", json_string("true")); // Wrong type

  restreamer_fs_entry_t entry = {0};
  parse_fs_entry_fields(json_obj, &entry);

  /* Only correctly typed fields should be parsed */
  TEST_ASSERT_NOT_NULL(entry.name, "name should be parsed");
  TEST_ASSERT_STR_EQUAL("valid-name.txt", entry.name, "name should match");
  TEST_ASSERT_NULL(entry.path, "path should be NULL (wrong type)");
  TEST_ASSERT_EQUAL(0, entry.size, "size should be 0 (wrong type)");
  TEST_ASSERT_EQUAL(1705318800, entry.modified, "modified should be parsed");
  TEST_ASSERT(entry.is_directory == false, "is_directory should be false (wrong type)");

  /* Cleanup */
  restreamer_api_free_fs_list(&(restreamer_fs_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields wrong types handling\n");
  return true;
}

/* Test: Parse fs_entry with large file size */
static bool test_parse_fs_entry_fields_large_size(void) {
  printf("  Testing parse_fs_entry_fields with large file size...\n");

  /* Create JSON object with very large file size (> 4GB) */
  json_t *json_obj = json_object();
  json_object_set_new(json_obj, "name", json_string("large-file.mkv"));
  json_object_set_new(json_obj, "path", json_string("/media/large-file.mkv"));
  json_object_set_new(json_obj, "size", json_integer(5368709120LL)); // 5GB
  json_object_set_new(json_obj, "modified", json_integer(1705318800));
  json_object_set_new(json_obj, "is_directory", json_false());

  restreamer_fs_entry_t entry = {0};
  parse_fs_entry_fields(json_obj, &entry);

  /* Verify large size is handled correctly */
  TEST_ASSERT_NOT_NULL(entry.name, "name should be parsed");
  TEST_ASSERT_EQUAL(5368709120LL, entry.size, "large size should be parsed correctly");

  /* Cleanup */
  restreamer_api_free_fs_list(&(restreamer_fs_list_t){.entries = &entry, .count = 1});
  json_decref(json_obj);

  printf("  ✓ parse_fs_entry_fields large file size\n");
  return true;
}

/* ========================================================================
 * Test Suite Runner
 * ======================================================================== */

bool run_api_parse_helper_tests(void) {
  printf("\n========================================\n");
  printf("API Parse Helper Functions Tests\n");
  printf("========================================\n");

  bool all_passed = true;

  /* parse_log_entry_fields tests */
  printf("\nparse_log_entry_fields() tests:\n");
  all_passed &= test_parse_log_entry_fields_complete();
  all_passed &= test_parse_log_entry_fields_partial();
  all_passed &= test_parse_log_entry_fields_null_input();
  all_passed &= test_parse_log_entry_fields_null_entry();
  all_passed &= test_parse_log_entry_fields_wrong_types();

  /* parse_session_fields tests */
  printf("\nparse_session_fields() tests:\n");
  all_passed &= test_parse_session_fields_complete();
  all_passed &= test_parse_session_fields_partial();
  all_passed &= test_parse_session_fields_null_input();
  all_passed &= test_parse_session_fields_null_session();
  all_passed &= test_parse_session_fields_wrong_types();

  /* parse_fs_entry_fields tests */
  printf("\nparse_fs_entry_fields() tests:\n");
  all_passed &= test_parse_fs_entry_fields_file();
  all_passed &= test_parse_fs_entry_fields_directory();
  all_passed &= test_parse_fs_entry_fields_partial();
  all_passed &= test_parse_fs_entry_fields_null_input();
  all_passed &= test_parse_fs_entry_fields_null_entry();
  all_passed &= test_parse_fs_entry_fields_wrong_types();
  all_passed &= test_parse_fs_entry_fields_large_size();

  if (all_passed) {
    printf("\n✓ All API parse helper tests passed\n");
  } else {
    printf("\n✗ Some API parse helper tests failed\n");
  }

  return all_passed;
}
