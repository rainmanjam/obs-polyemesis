/*
 * API Filesystem and Connection Tests
 *
 * Tests for the Restreamer API filesystem and connection monitoring functionality
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

/* Test macros from test_main.c */
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

#define TEST_ASSERT_EQUAL(expected, actual, message)                           \
  do {                                                                         \
    if ((expected) != (actual)) {                                              \
      fprintf(stderr,                                                          \
              "  ✗ FAIL: %s\n    Expected: %d, Actual: %d\n    at %s:%d\n",    \
              message, (int)(expected), (int)(actual), __FILE__, __LINE__);    \
      return false;                                                            \
    }                                                                          \
  } while (0)

/* Test: List filesystems */
static bool test_list_filesystems(void) {
  printf("  Testing list filesystems...\n");

  /* Start mock server */
  if (!mock_restreamer_start(9890)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9890\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9890,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Connect first */
  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test list filesystems */
  char *filesystems_json = NULL;
  bool result = restreamer_api_list_filesystems(api, &filesystems_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  list_filesystems failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should list filesystems");
  TEST_ASSERT_NOT_NULL(filesystems_json, "Filesystems JSON should not be NULL");

  if (filesystems_json) {
    printf("  Filesystems response: %s\n", filesystems_json);
    free(filesystems_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();

  /* Give server time to fully shutdown */
  sleep_ms(1000);

  printf("  ✓ List filesystems\n");
  return true;
}

/* Test: List files */
static bool test_list_files(void) {
  printf("  Testing list files...\n");

  if (!mock_restreamer_start(9891)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9891\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9891,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test list files without glob pattern */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "disk", NULL, &files);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  list_files failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should list files");
  TEST_ASSERT(files.count > 0, "Should have at least one file");

  printf("  Found %zu files\n", files.count);
  if (files.count > 0) {
    TEST_ASSERT_NOT_NULL(files.entries[0].name, "First file should have name");
    printf("  First file: %s\n", files.entries[0].name);
  }

  restreamer_api_free_fs_list(&files);

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ List files\n");
  return true;
}

/* Test: List files with glob pattern */
static bool test_list_files_with_glob(void) {
  printf("  Testing list files with glob pattern...\n");

  if (!mock_restreamer_start(9892)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9892\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9892,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test list files with glob pattern */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "disk", "*.mp4", &files);
  TEST_ASSERT(result, "Should list files with glob pattern");

  printf("  Found %zu files matching *.mp4\n", files.count);

  restreamer_api_free_fs_list(&files);
  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ List files with glob pattern\n");
  return true;
}

/* Test: Download file */
static bool test_download_file(void) {
  printf("  Testing download file...\n");

  if (!mock_restreamer_start(9893)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9893\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9893,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test download file */
  unsigned char *data = NULL;
  size_t size = 0;
  bool result = restreamer_api_download_file(api, "disk", "test.txt", &data, &size);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  download_file failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should download file");
  TEST_ASSERT_NOT_NULL(data, "Downloaded data should not be NULL");
  TEST_ASSERT(size > 0, "Downloaded data size should be greater than 0");

  printf("  Downloaded %zu bytes\n", size);

  if (data) {
    free(data);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Download file\n");
  return true;
}

/* Test: Upload file */
static bool test_upload_file(void) {
  printf("  Testing upload file...\n");

  if (!mock_restreamer_start(9894)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9894\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9894,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test upload file */
  const unsigned char test_data[] = "Test file content for upload";
  size_t test_size = sizeof(test_data) - 1; /* Exclude null terminator */

  bool result = restreamer_api_upload_file(api, "disk", "uploaded.txt", test_data, test_size);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  upload_file failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should upload file");

  printf("  Uploaded %zu bytes\n", test_size);

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Upload file\n");
  return true;
}

/* Test: Delete file */
static bool test_delete_file(void) {
  printf("  Testing delete file...\n");

  if (!mock_restreamer_start(9895)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9895\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9895,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test delete file */
  bool result = restreamer_api_delete_file(api, "disk", "test.txt");
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  delete_file failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should delete file");

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Delete file\n");
  return true;
}

/* Test: Get RTMP connections */
static bool test_get_rtmp_connections(void) {
  printf("  Testing get RTMP connections...\n");

  if (!mock_restreamer_start(9896)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9896\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9896,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test get RTMP connections */
  char *streams_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(api, &streams_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_rtmp_streams failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should get RTMP connections");
  TEST_ASSERT_NOT_NULL(streams_json, "RTMP streams JSON should not be NULL");

  if (streams_json) {
    printf("  RTMP streams response: %s\n", streams_json);
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Get RTMP connections\n");
  return true;
}

/* Test: Get SRT connections */
static bool test_get_srt_connections(void) {
  printf("  Testing get SRT connections...\n");

  if (!mock_restreamer_start(9897)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9897\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9897,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test get SRT connections */
  char *streams_json = NULL;
  bool result = restreamer_api_get_srt_streams(api, &streams_json);
  if (!result) {
    const char *error = restreamer_api_get_error(api);
    fprintf(stderr, "  get_srt_streams failed: %s\n", error ? error : "unknown error");
  }
  TEST_ASSERT(result, "Should get SRT connections");
  TEST_ASSERT_NOT_NULL(streams_json, "SRT streams JSON should not be NULL");

  if (streams_json) {
    printf("  SRT streams response: %s\n", streams_json);
    free(streams_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Get SRT connections\n");
  return true;
}

/* Test: Filesystem API NULL parameter handling */
static bool test_filesystem_null_params(void) {
  printf("  Testing filesystem API NULL parameter handling...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test list_filesystems with NULL API */
  char *json = NULL;
  bool result = restreamer_api_list_filesystems(NULL, &json);
  TEST_ASSERT(!result, "list_filesystems should fail with NULL API");

  /* Test list_filesystems with NULL output */
  result = restreamer_api_list_filesystems(api, NULL);
  TEST_ASSERT(!result, "list_filesystems should fail with NULL output");

  /* Test list_files with NULL API */
  restreamer_fs_list_t files = {0};
  result = restreamer_api_list_files(NULL, "disk", NULL, &files);
  TEST_ASSERT(!result, "list_files should fail with NULL API");

  /* Test list_files with NULL storage */
  result = restreamer_api_list_files(api, NULL, NULL, &files);
  TEST_ASSERT(!result, "list_files should fail with NULL storage");

  /* Test list_files with NULL output */
  result = restreamer_api_list_files(api, "disk", NULL, NULL);
  TEST_ASSERT(!result, "list_files should fail with NULL output");

  /* Test download_file with NULL API */
  unsigned char *data = NULL;
  size_t size = 0;
  result = restreamer_api_download_file(NULL, "disk", "test.txt", &data, &size);
  TEST_ASSERT(!result, "download_file should fail with NULL API");

  /* Test download_file with NULL storage */
  result = restreamer_api_download_file(api, NULL, "test.txt", &data, &size);
  TEST_ASSERT(!result, "download_file should fail with NULL storage");

  /* Test download_file with NULL filepath */
  result = restreamer_api_download_file(api, "disk", NULL, &data, &size);
  TEST_ASSERT(!result, "download_file should fail with NULL filepath");

  /* Test download_file with NULL data pointer */
  result = restreamer_api_download_file(api, "disk", "test.txt", NULL, &size);
  TEST_ASSERT(!result, "download_file should fail with NULL data pointer");

  /* Test download_file with NULL size pointer */
  result = restreamer_api_download_file(api, "disk", "test.txt", &data, NULL);
  TEST_ASSERT(!result, "download_file should fail with NULL size pointer");

  /* Test upload_file with NULL API */
  const unsigned char test_data[] = "test";
  result = restreamer_api_upload_file(NULL, "disk", "test.txt", test_data, 4);
  TEST_ASSERT(!result, "upload_file should fail with NULL API");

  /* Test upload_file with NULL storage */
  result = restreamer_api_upload_file(api, NULL, "test.txt", test_data, 4);
  TEST_ASSERT(!result, "upload_file should fail with NULL storage");

  /* Test upload_file with NULL filepath */
  result = restreamer_api_upload_file(api, "disk", NULL, test_data, 4);
  TEST_ASSERT(!result, "upload_file should fail with NULL filepath");

  /* Test upload_file with NULL data */
  result = restreamer_api_upload_file(api, "disk", "test.txt", NULL, 4);
  TEST_ASSERT(!result, "upload_file should fail with NULL data");

  /* Test delete_file with NULL API */
  result = restreamer_api_delete_file(NULL, "disk", "test.txt");
  TEST_ASSERT(!result, "delete_file should fail with NULL API");

  /* Test delete_file with NULL storage */
  result = restreamer_api_delete_file(api, NULL, "test.txt");
  TEST_ASSERT(!result, "delete_file should fail with NULL storage");

  /* Test delete_file with NULL filepath */
  result = restreamer_api_delete_file(api, "disk", NULL);
  TEST_ASSERT(!result, "delete_file should fail with NULL filepath");

  /* Test free_fs_list with NULL */
  restreamer_api_free_fs_list(NULL); /* Should not crash */

  restreamer_api_destroy(api);

  printf("  ✓ Filesystem API NULL parameter handling\n");
  return true;
}

/* Test: Protocol monitoring API NULL parameter handling */
static bool test_protocol_null_params(void) {
  printf("  Testing protocol monitoring API NULL parameter handling...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test get_rtmp_streams with NULL API */
  char *json = NULL;
  bool result = restreamer_api_get_rtmp_streams(NULL, &json);
  TEST_ASSERT(!result, "get_rtmp_streams should fail with NULL API");

  /* Test get_rtmp_streams with NULL output */
  result = restreamer_api_get_rtmp_streams(api, NULL);
  TEST_ASSERT(!result, "get_rtmp_streams should fail with NULL output");

  /* Test get_srt_streams with NULL API */
  result = restreamer_api_get_srt_streams(NULL, &json);
  TEST_ASSERT(!result, "get_srt_streams should fail with NULL API");

  /* Test get_srt_streams with NULL output */
  result = restreamer_api_get_srt_streams(api, NULL);
  TEST_ASSERT(!result, "get_srt_streams should fail with NULL output");

  restreamer_api_destroy(api);

  printf("  ✓ Protocol monitoring API NULL parameter handling\n");
  return true;
}

/* Test: File operations with empty strings */
static bool test_filesystem_empty_strings(void) {
  printf("  Testing filesystem API with empty strings...\n");

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 8080,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  /* Test list_files with empty storage */
  restreamer_fs_list_t files = {0};
  bool result = restreamer_api_list_files(api, "", NULL, &files);
  /* Empty storage is technically valid, will just fail on server side */
  (void)result;

  /* Test download_file with empty strings */
  unsigned char *data = NULL;
  size_t size = 0;
  result = restreamer_api_download_file(api, "", "test.txt", &data, &size);
  (void)result;

  result = restreamer_api_download_file(api, "disk", "", &data, &size);
  (void)result;

  /* Test upload_file with empty strings */
  const unsigned char test_data[] = "test";
  result = restreamer_api_upload_file(api, "", "test.txt", test_data, 4);
  (void)result;

  result = restreamer_api_upload_file(api, "disk", "", test_data, 4);
  (void)result;

  /* Test delete_file with empty strings */
  result = restreamer_api_delete_file(api, "", "test.txt");
  (void)result;

  result = restreamer_api_delete_file(api, "disk", "");
  (void)result;

  restreamer_api_destroy(api);

  printf("  ✓ Filesystem API with empty strings\n");
  return true;
}

/* Test: Multiple sequential file operations */
static bool test_sequential_file_operations(void) {
  printf("  Testing sequential file operations...\n");

  if (!mock_restreamer_start(9898)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9898\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9898,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Perform multiple operations in sequence */

  /* List filesystems */
  char *filesystems_json = NULL;
  bool result = restreamer_api_list_filesystems(api, &filesystems_json);
  TEST_ASSERT(result, "Should list filesystems");
  if (filesystems_json) {
    free(filesystems_json);
  }

  /* List files */
  restreamer_fs_list_t files = {0};
  result = restreamer_api_list_files(api, "disk", "*.txt", &files);
  TEST_ASSERT(result, "Should list files");
  restreamer_api_free_fs_list(&files);

  /* Upload a file */
  const unsigned char upload_data[] = "Sequential test data";
  result = restreamer_api_upload_file(api, "disk", "seq_test.txt", upload_data, sizeof(upload_data) - 1);
  TEST_ASSERT(result, "Should upload file");

  /* Download the file */
  unsigned char *download_data = NULL;
  size_t download_size = 0;
  result = restreamer_api_download_file(api, "disk", "seq_test.txt", &download_data, &download_size);
  TEST_ASSERT(result, "Should download file");
  if (download_data) {
    free(download_data);
  }

  /* Delete the file */
  result = restreamer_api_delete_file(api, "disk", "seq_test.txt");
  TEST_ASSERT(result, "Should delete file");

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Sequential file operations\n");
  return true;
}

/* Test: Protocol monitoring operations */
static bool test_protocol_monitoring(void) {
  printf("  Testing protocol monitoring operations...\n");

  if (!mock_restreamer_start(9899)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9899\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9899,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Get both RTMP and SRT streams in sequence */
  char *rtmp_json = NULL;
  bool result = restreamer_api_get_rtmp_streams(api, &rtmp_json);
  TEST_ASSERT(result, "Should get RTMP streams");
  if (rtmp_json) {
    free(rtmp_json);
  }

  char *srt_json = NULL;
  result = restreamer_api_get_srt_streams(api, &srt_json);
  TEST_ASSERT(result, "Should get SRT streams");
  if (srt_json) {
    free(srt_json);
  }

  /* Get them again to test multiple calls */
  rtmp_json = NULL;
  result = restreamer_api_get_rtmp_streams(api, &rtmp_json);
  TEST_ASSERT(result, "Should get RTMP streams again");
  if (rtmp_json) {
    free(rtmp_json);
  }

  srt_json = NULL;
  result = restreamer_api_get_srt_streams(api, &srt_json);
  TEST_ASSERT(result, "Should get SRT streams again");
  if (srt_json) {
    free(srt_json);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Protocol monitoring operations\n");
  return true;
}

/* Test: Upload file with zero size */
static bool test_upload_zero_size(void) {
  printf("  Testing upload file with zero size...\n");

  if (!mock_restreamer_start(9900)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9900\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9900,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Upload empty file */
  const unsigned char empty_data[] = "";
  bool result = restreamer_api_upload_file(api, "disk", "empty.txt", empty_data, 0);
  /* This should succeed - empty files are valid */
  TEST_ASSERT(result, "Should upload empty file");

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Upload file with zero size\n");
  return true;
}

/* Test: Large file path handling */
static bool test_large_file_path(void) {
  printf("  Testing large file path handling...\n");

  if (!mock_restreamer_start(9901)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9901\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9901,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test with long file path */
  char long_path[512];
  memset(long_path, 'a', sizeof(long_path) - 1);
  long_path[sizeof(long_path) - 1] = '\0';

  unsigned char *data = NULL;
  size_t size = 0;
  bool result = restreamer_api_download_file(api, "disk", long_path, &data, &size);
  /* May succeed or fail depending on server, but should not crash */
  (void)result;
  if (data) {
    free(data);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Large file path handling\n");
  return true;
}

/* Test: Special characters in file paths */
static bool test_special_char_paths(void) {
  printf("  Testing special characters in file paths...\n");

  if (!mock_restreamer_start(9902)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9902\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9902,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test with special characters that need URL encoding */
  const char *special_paths[] = {
      "file with spaces.txt",
      "file&with&ampersands.txt",
      "file%with%percent.txt",
      "file+with+plus.txt",
      NULL
  };

  for (int i = 0; special_paths[i] != NULL; i++) {
    unsigned char *data = NULL;
    size_t size = 0;
    bool result = restreamer_api_download_file(api, "disk", special_paths[i], &data, &size);
    /* May succeed or fail, but should handle URL encoding properly */
    (void)result;
    if (data) {
      free(data);
    }
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Special characters in file paths\n");
  return true;
}

/* Test: Glob pattern URL encoding */
static bool test_glob_pattern_encoding(void) {
  printf("  Testing glob pattern URL encoding...\n");

  if (!mock_restreamer_start(9903)) {
    fprintf(stderr, "  ✗ Failed to start mock server on port 9903\n");
    return false;
  }

  sleep_ms(1000);

  restreamer_connection_t conn = {
      .host = "localhost",
      .port = 9903,
      .username = "admin",
      .password = "password",
      .use_https = false,
  };

  restreamer_api_t *api = restreamer_api_create(&conn);
  TEST_ASSERT_NOT_NULL(api, "API client should be created");

  bool connected = restreamer_api_test_connection(api);
  TEST_ASSERT(connected, "Should connect to mock server");

  /* Test various glob patterns that need URL encoding */
  const char *patterns[] = {
      "*.txt",
      "test*.mp4",
      "video[0-9].mkv",
      "*",
      NULL
  };

  for (int i = 0; patterns[i] != NULL; i++) {
    restreamer_fs_list_t files = {0};
    bool result = restreamer_api_list_files(api, "disk", patterns[i], &files);
    /* Should handle URL encoding of glob patterns */
    (void)result;
    restreamer_api_free_fs_list(&files);
  }

  restreamer_api_destroy(api);
  mock_restreamer_stop();
  sleep_ms(1000); /* Give server time to fully shutdown */

  printf("  ✓ Glob pattern URL encoding\n");
  return true;
}

/* Run all filesystem and connection tests */
bool run_api_filesystem_tests(void) {
  bool all_passed = true;

  /* Core filesystem operations */
  all_passed &= test_list_filesystems();
  all_passed &= test_list_files();
  all_passed &= test_list_files_with_glob();
  all_passed &= test_download_file();
  all_passed &= test_upload_file();
  all_passed &= test_delete_file();

  /* Protocol monitoring operations */
  all_passed &= test_get_rtmp_connections();
  all_passed &= test_get_srt_connections();

  /* Error handling and edge cases */
  all_passed &= test_filesystem_null_params();
  all_passed &= test_protocol_null_params();
  all_passed &= test_filesystem_empty_strings();

  /* Advanced operations */
  all_passed &= test_sequential_file_operations();
  all_passed &= test_protocol_monitoring();
  all_passed &= test_upload_zero_size();
  all_passed &= test_large_file_path();
  all_passed &= test_special_char_paths();
  all_passed &= test_glob_pattern_encoding();

  return all_passed;
}
