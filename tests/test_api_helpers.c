/*
 * API Helper Function Tests
 *
 * Tests for internal helper functions in restreamer-api.c:
 * - secure_memzero() and secure_free() - security functions
 * - handle_login_failure() - login retry with exponential backoff
 * - is_login_throttled() - login throttling check
 * - write_callback() - CURL write callback
 * - parse_json_response() - JSON parsing helper
 * - json_get_string_dup() - JSON string extraction
 * - json_get_uint32() - JSON integer extraction
 * - json_get_string_as_uint32() - JSON string-to-integer parsing
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <jansson.h>
#include <util/bmem.h>
#include <util/dstr.h>

/* Test result tracking */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message)                                        \
  do {                                                                         \
    if (condition) {                                                           \
      tests_passed++;                                                          \
    } else {                                                                   \
      tests_failed++;                                                          \
      fprintf(stderr, "  FAIL: %s\n    at %s:%d\n", message, __FILE__,         \
              __LINE__);                                                       \
    }                                                                          \
  } while (0)

#define TEST_ASSERT_STR_EQ(actual, expected, message)                          \
  do {                                                                         \
    if ((actual) != NULL && (expected) != NULL &&                              \
        strcmp((actual), (expected)) == 0) {                                   \
      tests_passed++;                                                          \
    } else {                                                                   \
      tests_failed++;                                                          \
      fprintf(stderr, "  FAIL: %s\n    Expected: %s\n    Actual: %s\n",        \
              message, (expected) ? (expected) : "NULL",                       \
              (actual) ? (actual) : "NULL");                                   \
    }                                                                          \
  } while (0)

/* ========================================================================
 * Forward Declarations - Export internal functions for testing
 * ======================================================================== */

/* Opaque API type for testing */
typedef struct restreamer_api {
  char *access_token;
  char *refresh_token;
  time_t token_expires;
  time_t last_login_attempt;
  int login_backoff_ms;
  int login_retry_count;
  struct dstr last_error;
} restreamer_api_t;

/* Memory write callback structure */
struct memory_struct {
  char *memory;
  size_t size;
};

/* Export declarations to test internal functions */
extern void secure_memzero(void *ptr, size_t len);
extern void secure_free(char *ptr);
extern void handle_login_failure(restreamer_api_t *api, long http_code);
extern bool is_login_throttled(restreamer_api_t *api);
extern size_t write_callback(void *contents, size_t size, size_t nmemb,
                             void *userp);
extern json_t *parse_json_response(restreamer_api_t *api,
                                   struct memory_struct *response);
extern char *json_get_string_dup(const json_t *obj, const char *key);
extern uint32_t json_get_uint32(const json_t *obj, const char *key);
extern uint32_t json_get_string_as_uint32(const json_t *obj, const char *key);

/* ========================================================================
 * Test Helper Functions
 * ======================================================================== */

/* Helper to create a test API object */
static restreamer_api_t *create_test_api(void) {
  restreamer_api_t *api = bzalloc(sizeof(restreamer_api_t));
  if (!api) {
    return NULL;
  }
  dstr_init(&api->last_error);
  api->login_backoff_ms = 1000; /* Start with 1 second */
  api->login_retry_count = 0;
  api->last_login_attempt = 0;
  return api;
}

/* Helper to destroy test API object */
static void destroy_test_api(restreamer_api_t *api) {
  if (!api) {
    return;
  }
  if (api->access_token) {
    bfree(api->access_token);
  }
  if (api->refresh_token) {
    bfree(api->refresh_token);
  }
  dstr_free(&api->last_error);
  bfree(api);
}

/* ========================================================================
 * Security Function Tests - secure_memzero() and secure_free()
 * ======================================================================== */

static void test_secure_memzero_basic(void) {
  printf("  Testing secure_memzero basic operation...\n");

  char buffer[32];
  memset(buffer, 'A', sizeof(buffer));

  secure_memzero(buffer, sizeof(buffer));

  /* Verify all bytes are zeroed */
  bool all_zero = true;
  for (size_t i = 0; i < sizeof(buffer); i++) {
    if (buffer[i] != 0) {
      all_zero = false;
      break;
    }
  }

  TEST_ASSERT(all_zero, "secure_memzero should zero all bytes");
}

static void test_secure_memzero_partial(void) {
  printf("  Testing secure_memzero partial clear...\n");

  char buffer[32];
  memset(buffer, 'B', sizeof(buffer));

  /* Clear only first 16 bytes */
  secure_memzero(buffer, 16);

  /* Verify first 16 bytes are zero */
  bool first_half_zero = true;
  for (size_t i = 0; i < 16; i++) {
    if (buffer[i] != 0) {
      first_half_zero = false;
      break;
    }
  }

  /* Verify last 16 bytes are unchanged */
  bool second_half_unchanged = true;
  for (size_t i = 16; i < 32; i++) {
    if (buffer[i] != 'B') {
      second_half_unchanged = false;
      break;
    }
  }

  TEST_ASSERT(first_half_zero, "secure_memzero should zero first half");
  TEST_ASSERT(second_half_unchanged, "secure_memzero should not touch second half");
}

static void test_secure_memzero_zero_length(void) {
  printf("  Testing secure_memzero with zero length...\n");

  char buffer[8];
  memset(buffer, 'C', sizeof(buffer));

  secure_memzero(buffer, 0);

  /* Verify buffer is unchanged */
  bool unchanged = true;
  for (size_t i = 0; i < sizeof(buffer); i++) {
    if (buffer[i] != 'C') {
      unchanged = false;
      break;
    }
  }

  TEST_ASSERT(unchanged, "secure_memzero with zero length should not change buffer");
}

static void test_secure_free_basic(void) {
  printf("  Testing secure_free basic operation...\n");

  char *str = bstrdup("sensitive_data");
  secure_free(str);

  /* Can't verify memory is zeroed after free, but function should not crash */
  TEST_ASSERT(true, "secure_free should not crash on valid string");
}

static void test_secure_free_null(void) {
  printf("  Testing secure_free with NULL...\n");

  secure_free(NULL);

  TEST_ASSERT(true, "secure_free should handle NULL safely");
}

static void test_secure_free_empty_string(void) {
  printf("  Testing secure_free with empty string...\n");

  char *str = bstrdup("");
  secure_free(str);

  TEST_ASSERT(true, "secure_free should handle empty string safely");
}

/* ========================================================================
 * Login Failure Handler Tests - handle_login_failure()
 * ======================================================================== */

static void test_handle_login_failure_first_attempt(void) {
  printf("  Testing handle_login_failure first attempt...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  time_t before = time(NULL);
  handle_login_failure(api, 401);
  time_t after = time(NULL);

  TEST_ASSERT(api->login_retry_count == 1, "Retry count should be 1 after first failure");
  TEST_ASSERT(api->login_backoff_ms == 2000, "Backoff should double to 2000ms");
  TEST_ASSERT(api->last_login_attempt >= before && api->last_login_attempt <= after,
              "Last login attempt timestamp should be set");

  destroy_test_api(api);
}

static void test_handle_login_failure_exponential_backoff(void) {
  printf("  Testing handle_login_failure exponential backoff...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  /* First failure: 1000ms -> 2000ms */
  handle_login_failure(api, 401);
  TEST_ASSERT(api->login_backoff_ms == 2000, "First backoff should be 2000ms");

  /* Second failure: 2000ms -> 4000ms */
  handle_login_failure(api, 401);
  TEST_ASSERT(api->login_backoff_ms == 4000, "Second backoff should be 4000ms");

  /* Third failure: 4000ms -> 8000ms */
  handle_login_failure(api, 401);
  TEST_ASSERT(api->login_backoff_ms == 8000, "Third backoff should be 8000ms");
  TEST_ASSERT(api->login_retry_count == 3, "Retry count should be 3");

  destroy_test_api(api);
}

static void test_handle_login_failure_http_codes(void) {
  printf("  Testing handle_login_failure with various HTTP codes...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  /* Test with HTTP 401 */
  handle_login_failure(api, 401);
  TEST_ASSERT(api->login_retry_count == 1, "Should handle HTTP 401");

  /* Test with HTTP 500 */
  handle_login_failure(api, 500);
  TEST_ASSERT(api->login_retry_count == 2, "Should handle HTTP 500");

  /* Test with 0 (network error) */
  api->login_retry_count = 0;
  handle_login_failure(api, 0);
  TEST_ASSERT(api->login_retry_count == 1, "Should handle network error (0)");

  destroy_test_api(api);
}

static void test_handle_login_failure_max_retries(void) {
  printf("  Testing handle_login_failure at max retries...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  /* Simulate reaching max retries (3) */
  api->login_retry_count = 2;
  api->login_backoff_ms = 4000;

  handle_login_failure(api, 401);

  TEST_ASSERT(api->login_retry_count == 3, "Should reach max retry count");
  /* At max retries, backoff still doubles but we don't retry anymore */
  TEST_ASSERT(api->login_backoff_ms == 8000, "Backoff should still double");

  destroy_test_api(api);
}

/* ========================================================================
 * Login Throttle Tests - is_login_throttled()
 * ======================================================================== */

static void test_is_login_throttled_no_previous_attempt(void) {
  printf("  Testing is_login_throttled with no previous attempt...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  bool throttled = is_login_throttled(api);

  TEST_ASSERT(!throttled, "Should not be throttled with no previous attempt");

  destroy_test_api(api);
}

static void test_is_login_throttled_within_backoff(void) {
  printf("  Testing is_login_throttled within backoff period...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  api->login_retry_count = 1;
  api->login_backoff_ms = 5000; /* 5 seconds */
  api->last_login_attempt = time(NULL); /* Just now */

  bool throttled = is_login_throttled(api);

  TEST_ASSERT(throttled, "Should be throttled within backoff period");
  TEST_ASSERT(api->last_error.len > 0, "Error message should be set");

  destroy_test_api(api);
}

static void test_is_login_throttled_after_backoff(void) {
  printf("  Testing is_login_throttled after backoff period...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  api->login_retry_count = 1;
  api->login_backoff_ms = 1000; /* 1 second */
  api->last_login_attempt = time(NULL) - 2; /* 2 seconds ago */

  bool throttled = is_login_throttled(api);

  TEST_ASSERT(!throttled, "Should not be throttled after backoff period");

  destroy_test_api(api);
}

static void test_is_login_throttled_edge_cases(void) {
  printf("  Testing is_login_throttled edge cases...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  /* Test with retry count 0 */
  api->login_retry_count = 0;
  api->last_login_attempt = time(NULL);
  TEST_ASSERT(!is_login_throttled(api), "Should not throttle with retry count 0");

  /* Test with last_login_attempt = 0 */
  api->login_retry_count = 1;
  api->last_login_attempt = 0;
  TEST_ASSERT(!is_login_throttled(api), "Should not throttle with last_login_attempt = 0");

  destroy_test_api(api);
}

/* ========================================================================
 * CURL Write Callback Tests - write_callback()
 * ======================================================================== */

static void test_write_callback_basic(void) {
  printf("  Testing write_callback basic operation...\n");

  struct memory_struct mem = {NULL, 0};
  const char *data = "Hello, World!";
  size_t data_len = strlen(data);

  size_t written = write_callback((void *)data, 1, data_len, &mem);

  TEST_ASSERT(written == data_len, "Should return number of bytes written");
  TEST_ASSERT(mem.size == data_len, "Memory size should match data length");
  TEST_ASSERT(mem.memory != NULL, "Memory should be allocated");
  TEST_ASSERT(strncmp(mem.memory, data, data_len) == 0, "Data should match");
  TEST_ASSERT(mem.memory[mem.size] == 0, "Should be null-terminated");

  free(mem.memory);
}

static void test_write_callback_multiple_calls(void) {
  printf("  Testing write_callback with multiple calls...\n");

  struct memory_struct mem = {NULL, 0};
  const char *data1 = "Hello, ";
  const char *data2 = "World!";

  size_t written1 = write_callback((void *)data1, 1, strlen(data1), &mem);
  size_t written2 = write_callback((void *)data2, 1, strlen(data2), &mem);

  TEST_ASSERT(written1 == strlen(data1), "First write should succeed");
  TEST_ASSERT(written2 == strlen(data2), "Second write should succeed");
  TEST_ASSERT(mem.size == strlen(data1) + strlen(data2), "Total size should be sum");
  TEST_ASSERT(strncmp(mem.memory, "Hello, World!", mem.size) == 0,
              "Combined data should match");

  free(mem.memory);
}

static void test_write_callback_size_nmemb(void) {
  printf("  Testing write_callback size * nmemb calculation...\n");

  struct memory_struct mem = {NULL, 0};
  const char *data = "ABCD";

  /* Write 4 bytes with size=2, nmemb=2 */
  size_t written = write_callback((void *)data, 2, 2, &mem);

  TEST_ASSERT(written == 4, "Should return size * nmemb");
  TEST_ASSERT(mem.size == 4, "Memory size should be 4");
  TEST_ASSERT(strncmp(mem.memory, "ABCD", 4) == 0, "Data should match");

  free(mem.memory);
}

static void test_write_callback_empty_data(void) {
  printf("  Testing write_callback with empty data...\n");

  struct memory_struct mem = {NULL, 0};
  const char *data = "";

  size_t written = write_callback((void *)data, 1, 0, &mem);

  TEST_ASSERT(written == 0, "Should return 0 for empty data");
  TEST_ASSERT(mem.memory != NULL, "Memory should still be allocated");
  TEST_ASSERT(mem.size == 0, "Size should be 0");

  free(mem.memory);
}

static void test_write_callback_zero_size(void) {
  printf("  Testing write_callback with zero size...\n");

  struct memory_struct mem = {NULL, 0};
  const char *data = "test";

  size_t written = write_callback((void *)data, 0, 10, &mem);

  TEST_ASSERT(written == 0, "Should return 0 when size is 0");
  TEST_ASSERT(mem.memory != NULL, "Memory should still be allocated");
  TEST_ASSERT(mem.size == 0, "Size should be 0");

  free(mem.memory);
}

/* ========================================================================
 * JSON Response Parser Tests - parse_json_response()
 * ======================================================================== */

static void test_parse_json_response_valid(void) {
  printf("  Testing parse_json_response with valid JSON...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  struct memory_struct response;
  response.memory = malloc(100);
  strcpy(response.memory, "{\"key\": \"value\", \"number\": 42}");
  response.size = strlen(response.memory);

  json_t *json = parse_json_response(api, &response);

  TEST_ASSERT(json != NULL, "Should parse valid JSON");
  TEST_ASSERT(json_is_object(json), "Should return JSON object");
  TEST_ASSERT(response.memory == NULL, "Should free response memory");
  TEST_ASSERT(response.size == 0, "Should reset response size");

  if (json) {
    json_decref(json);
  }
  destroy_test_api(api);
}

static void test_parse_json_response_invalid(void) {
  printf("  Testing parse_json_response with invalid JSON...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  struct memory_struct response;
  response.memory = malloc(100);
  strcpy(response.memory, "{invalid json}");
  response.size = strlen(response.memory);

  json_t *json = parse_json_response(api, &response);

  TEST_ASSERT(json == NULL, "Should return NULL for invalid JSON");
  TEST_ASSERT(api->last_error.len > 0, "Should set error message");
  TEST_ASSERT(response.memory == NULL, "Should free response memory even on error");

  destroy_test_api(api);
}

static void test_parse_json_response_null_api(void) {
  printf("  Testing parse_json_response with NULL api...\n");

  struct memory_struct response;
  response.memory = malloc(100);
  strcpy(response.memory, "{\"test\": true}");
  response.size = strlen(response.memory);

  json_t *json = parse_json_response(NULL, &response);

  TEST_ASSERT(json == NULL, "Should return NULL for NULL api");

  /* Clean up - parse_json_response doesn't free on NULL api */
  free(response.memory);
}

static void test_parse_json_response_null_response(void) {
  printf("  Testing parse_json_response with NULL response...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  json_t *json = parse_json_response(api, NULL);

  TEST_ASSERT(json == NULL, "Should return NULL for NULL response");

  destroy_test_api(api);
}

static void test_parse_json_response_null_memory(void) {
  printf("  Testing parse_json_response with NULL memory...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  struct memory_struct response = {NULL, 0};

  json_t *json = parse_json_response(api, &response);

  TEST_ASSERT(json == NULL, "Should return NULL for NULL memory");

  destroy_test_api(api);
}

static void test_parse_json_response_empty_string(void) {
  printf("  Testing parse_json_response with empty string...\n");

  restreamer_api_t *api = create_test_api();
  TEST_ASSERT(api != NULL, "Failed to create test API");

  struct memory_struct response;
  response.memory = malloc(1);
  response.memory[0] = '\0';
  response.size = 0;

  json_t *json = parse_json_response(api, &response);

  TEST_ASSERT(json == NULL, "Should return NULL for empty string");
  TEST_ASSERT(api->last_error.len > 0, "Should set error message");

  destroy_test_api(api);
}

/* ========================================================================
 * JSON Helper Tests - json_get_string_dup()
 * ======================================================================== */

static void test_json_get_string_dup_valid(void) {
  printf("  Testing json_get_string_dup with valid string...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "name", json_string("test_value"));

  char *value = json_get_string_dup(obj, "name");

  TEST_ASSERT(value != NULL, "Should return non-NULL for valid string");
  TEST_ASSERT_STR_EQ(value, "test_value", "Should return correct string value");

  bfree(value);
  json_decref(obj);
}

static void test_json_get_string_dup_missing_key(void) {
  printf("  Testing json_get_string_dup with missing key...\n");

  json_t *obj = json_object();

  char *value = json_get_string_dup(obj, "nonexistent");

  TEST_ASSERT(value == NULL, "Should return NULL for missing key");

  json_decref(obj);
}

static void test_json_get_string_dup_wrong_type(void) {
  printf("  Testing json_get_string_dup with wrong type...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "number", json_integer(42));

  char *value = json_get_string_dup(obj, "number");

  TEST_ASSERT(value == NULL, "Should return NULL for non-string type");

  json_decref(obj);
}

static void test_json_get_string_dup_null_object(void) {
  printf("  Testing json_get_string_dup with NULL object...\n");

  char *value = json_get_string_dup(NULL, "key");

  TEST_ASSERT(value == NULL, "Should return NULL for NULL object");
}

static void test_json_get_string_dup_empty_string(void) {
  printf("  Testing json_get_string_dup with empty string...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "empty", json_string(""));

  char *value = json_get_string_dup(obj, "empty");

  TEST_ASSERT(value != NULL, "Should return non-NULL for empty string");
  TEST_ASSERT_STR_EQ(value, "", "Should return empty string");

  bfree(value);
  json_decref(obj);
}

/* ========================================================================
 * JSON Helper Tests - json_get_uint32()
 * ======================================================================== */

static void test_json_get_uint32_valid(void) {
  printf("  Testing json_get_uint32 with valid integer...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_integer(42));

  uint32_t value = json_get_uint32(obj, "count");

  TEST_ASSERT(value == 42, "Should return correct integer value");

  json_decref(obj);
}

static void test_json_get_uint32_zero(void) {
  printf("  Testing json_get_uint32 with zero...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_integer(0));

  uint32_t value = json_get_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should return 0 for zero value");

  json_decref(obj);
}

static void test_json_get_uint32_large_value(void) {
  printf("  Testing json_get_uint32 with large value...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_integer(0xFFFFFFFF));

  uint32_t value = json_get_uint32(obj, "count");

  TEST_ASSERT(value == 0xFFFFFFFF, "Should handle max uint32 value");

  json_decref(obj);
}

static void test_json_get_uint32_missing_key(void) {
  printf("  Testing json_get_uint32 with missing key...\n");

  json_t *obj = json_object();

  uint32_t value = json_get_uint32(obj, "nonexistent");

  TEST_ASSERT(value == 0, "Should return 0 for missing key");

  json_decref(obj);
}

static void test_json_get_uint32_wrong_type(void) {
  printf("  Testing json_get_uint32 with wrong type...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "text", json_string("42"));

  uint32_t value = json_get_uint32(obj, "text");

  TEST_ASSERT(value == 0, "Should return 0 for non-integer type");

  json_decref(obj);
}

static void test_json_get_uint32_null_object(void) {
  printf("  Testing json_get_uint32 with NULL object...\n");

  uint32_t value = json_get_uint32(NULL, "key");

  TEST_ASSERT(value == 0, "Should return 0 for NULL object");
}

/* ========================================================================
 * JSON Helper Tests - json_get_string_as_uint32()
 * ======================================================================== */

static void test_json_get_string_as_uint32_valid(void) {
  printf("  Testing json_get_string_as_uint32 with valid string...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("42"));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 42, "Should parse valid numeric string");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_zero(void) {
  printf("  Testing json_get_string_as_uint32 with zero...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("0"));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should parse zero string");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_large_value(void) {
  printf("  Testing json_get_string_as_uint32 with large value...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("4294967295")); /* Max uint32 */

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 4294967295U, "Should parse large numeric string");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_invalid_string(void) {
  printf("  Testing json_get_string_as_uint32 with invalid string...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("not_a_number"));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should return 0 for non-numeric string");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_negative(void) {
  printf("  Testing json_get_string_as_uint32 with negative number...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("-42"));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should return 0 for negative number");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_empty_string(void) {
  printf("  Testing json_get_string_as_uint32 with empty string...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string(""));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should return 0 for empty string");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_missing_key(void) {
  printf("  Testing json_get_string_as_uint32 with missing key...\n");

  json_t *obj = json_object();

  uint32_t value = json_get_string_as_uint32(obj, "nonexistent");

  TEST_ASSERT(value == 0, "Should return 0 for missing key");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_wrong_type(void) {
  printf("  Testing json_get_string_as_uint32 with wrong type...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_integer(42));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 0, "Should return 0 for non-string type");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_null_object(void) {
  printf("  Testing json_get_string_as_uint32 with NULL object...\n");

  uint32_t value = json_get_string_as_uint32(NULL, "key");

  TEST_ASSERT(value == 0, "Should return 0 for NULL object");
}

static void test_json_get_string_as_uint32_whitespace(void) {
  printf("  Testing json_get_string_as_uint32 with whitespace...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("  42  "));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  TEST_ASSERT(value == 42, "Should handle leading whitespace");

  json_decref(obj);
}

static void test_json_get_string_as_uint32_partial_number(void) {
  printf("  Testing json_get_string_as_uint32 with partial number...\n");

  json_t *obj = json_object();
  json_object_set_new(obj, "count", json_string("42abc"));

  uint32_t value = json_get_string_as_uint32(obj, "count");

  /* strtol parses valid prefix, so "42abc" should give 42 */
  TEST_ASSERT(value == 42, "Should parse valid numeric prefix");

  json_decref(obj);
}

/* ========================================================================
 * Main Test Runner
 * ======================================================================== */

bool run_api_helper_tests(void) {
  printf("\nAPI Helper Function Tests\n");
  printf("========================================\n");

  tests_passed = 0;
  tests_failed = 0;

  /* Security function tests */
  printf("\nSecurity Functions:\n");
  test_secure_memzero_basic();
  test_secure_memzero_partial();
  test_secure_memzero_zero_length();
  test_secure_free_basic();
  test_secure_free_null();
  test_secure_free_empty_string();

  /* Login failure handler tests */
  printf("\nLogin Failure Handler:\n");
  test_handle_login_failure_first_attempt();
  test_handle_login_failure_exponential_backoff();
  test_handle_login_failure_http_codes();
  test_handle_login_failure_max_retries();

  /* Login throttle tests */
  printf("\nLogin Throttle:\n");
  test_is_login_throttled_no_previous_attempt();
  test_is_login_throttled_within_backoff();
  test_is_login_throttled_after_backoff();
  test_is_login_throttled_edge_cases();

  /* CURL write callback tests */
  printf("\nCURL Write Callback:\n");
  test_write_callback_basic();
  test_write_callback_multiple_calls();
  test_write_callback_size_nmemb();
  test_write_callback_empty_data();
  test_write_callback_zero_size();

  /* JSON response parser tests */
  printf("\nJSON Response Parser:\n");
  test_parse_json_response_valid();
  test_parse_json_response_invalid();
  test_parse_json_response_null_api();
  test_parse_json_response_null_response();
  test_parse_json_response_null_memory();
  test_parse_json_response_empty_string();

  /* JSON string helper tests */
  printf("\nJSON String Helper (json_get_string_dup):\n");
  test_json_get_string_dup_valid();
  test_json_get_string_dup_missing_key();
  test_json_get_string_dup_wrong_type();
  test_json_get_string_dup_null_object();
  test_json_get_string_dup_empty_string();

  /* JSON uint32 helper tests */
  printf("\nJSON Integer Helper (json_get_uint32):\n");
  test_json_get_uint32_valid();
  test_json_get_uint32_zero();
  test_json_get_uint32_large_value();
  test_json_get_uint32_missing_key();
  test_json_get_uint32_wrong_type();
  test_json_get_uint32_null_object();

  /* JSON string-to-uint32 helper tests */
  printf("\nJSON String-to-Integer Helper (json_get_string_as_uint32):\n");
  test_json_get_string_as_uint32_valid();
  test_json_get_string_as_uint32_zero();
  test_json_get_string_as_uint32_large_value();
  test_json_get_string_as_uint32_invalid_string();
  test_json_get_string_as_uint32_negative();
  test_json_get_string_as_uint32_empty_string();
  test_json_get_string_as_uint32_missing_key();
  test_json_get_string_as_uint32_wrong_type();
  test_json_get_string_as_uint32_null_object();
  test_json_get_string_as_uint32_whitespace();
  test_json_get_string_as_uint32_partial_number();

  /* Print summary */
  printf("\n========================================\n");
  printf("Test Results:\n");
  printf("  Passed: %d\n", tests_passed);
  printf("  Failed: %d\n", tests_failed);
  printf("========================================\n");

  return (tests_failed == 0);
}
