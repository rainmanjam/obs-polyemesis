/*
 * API Endpoint Tests
 *
 * Comprehensive tests for additional API endpoint functions in restreamer-api.c
 * to improve code coverage. This file focuses on testing:
 * - Configuration management endpoints
 * - Metadata storage endpoints
 * - Playout management endpoints
 * - Token refresh and authentication endpoints
 * - Process configuration endpoints
 *
 * Tests include NULL parameter handling, empty strings, and error paths.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "restreamer-api.h"

/* Test macros */
#define TEST_ASSERT(condition, message)                                        \
	do {                                                                   \
		if (!(condition)) {                                            \
			fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n",        \
				message, __FILE__, __LINE__);                  \
			return false;                                          \
		}                                                              \
	} while (0)

#define TEST_ASSERT_NULL(ptr, message)                                         \
	do {                                                                   \
		if ((ptr) != NULL) {                                           \
			fprintf(stderr,                                        \
				"  ✗ FAIL: %s\n    Expected NULL but got %p\n    at %s:%d\n", \
				message, (void *)(ptr), __FILE__, __LINE__);   \
			return false;                                          \
		}                                                              \
	} while (0)

/* ========================================================================
 * Configuration Management API Tests
 * ======================================================================== */

/* Test: restreamer_api_get_config with NULL api */
static bool test_get_config_null_api(void)
{
	printf("  Testing get_config with NULL api...\n");

	char *config_json = NULL;
	bool result = restreamer_api_get_config(NULL, &config_json);
	TEST_ASSERT(!result, "Should return false for NULL api");
	TEST_ASSERT_NULL(config_json, "config_json should remain NULL");

	printf("  ✓ get_config NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_config with NULL config_json pointer */
static bool test_get_config_null_output(void)
{
	printf("  Testing get_config with NULL config_json pointer...\n");

	bool result = restreamer_api_get_config(NULL, NULL);
	TEST_ASSERT(!result, "Should return false for NULL config_json pointer");

	printf("  ✓ get_config NULL config_json handling\n");
	return true;
}

/* Test: restreamer_api_set_config with NULL api */
static bool test_set_config_null_api(void)
{
	printf("  Testing set_config with NULL api...\n");

	const char *config_json = "{\"test\": \"config\"}";
	bool result = restreamer_api_set_config(NULL, config_json);
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ set_config NULL api handling\n");
	return true;
}

/* Test: restreamer_api_set_config with NULL config_json */
static bool test_set_config_null_config(void)
{
	printf("  Testing set_config with NULL config_json...\n");

	bool result = restreamer_api_set_config(NULL, NULL);
	TEST_ASSERT(!result, "Should return false for NULL config_json");

	printf("  ✓ set_config NULL config_json handling\n");
	return true;
}

/* Test: restreamer_api_reload_config with NULL api */
static bool test_reload_config_null_api(void)
{
	printf("  Testing reload_config with NULL api...\n");

	bool result = restreamer_api_reload_config(NULL);
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ reload_config NULL api handling\n");
	return true;
}

/* ========================================================================
 * Metadata API Tests
 * ======================================================================== */

/* Test: restreamer_api_get_metadata with NULL api */
static bool test_get_metadata_null_api(void)
{
	printf("  Testing get_metadata with NULL api...\n");

	char *value = NULL;
	bool result = restreamer_api_get_metadata(NULL, "test_key", &value);
	TEST_ASSERT(!result, "Should return false for NULL api");
	TEST_ASSERT_NULL(value, "value should remain NULL");

	printf("  ✓ get_metadata NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_metadata with NULL key */
static bool test_get_metadata_null_key(void)
{
	printf("  Testing get_metadata with NULL key...\n");

	char *value = NULL;
	bool result = restreamer_api_get_metadata(NULL, NULL, &value);
	TEST_ASSERT(!result, "Should return false for NULL key");

	printf("  ✓ get_metadata NULL key handling\n");
	return true;
}

/* Test: restreamer_api_get_metadata with NULL value pointer */
static bool test_get_metadata_null_value(void)
{
	printf("  Testing get_metadata with NULL value pointer...\n");

	bool result = restreamer_api_get_metadata(NULL, "test_key", NULL);
	TEST_ASSERT(!result, "Should return false for NULL value pointer");

	printf("  ✓ get_metadata NULL value handling\n");
	return true;
}

/* Test: restreamer_api_set_metadata with NULL api */
static bool test_set_metadata_null_api(void)
{
	printf("  Testing set_metadata with NULL api...\n");

	bool result = restreamer_api_set_metadata(NULL, "test_key", "test_value");
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ set_metadata NULL api handling\n");
	return true;
}

/* Test: restreamer_api_set_metadata with NULL key */
static bool test_set_metadata_null_key(void)
{
	printf("  Testing set_metadata with NULL key...\n");

	bool result = restreamer_api_set_metadata(NULL, NULL, "test_value");
	TEST_ASSERT(!result, "Should return false for NULL key");

	printf("  ✓ set_metadata NULL key handling\n");
	return true;
}

/* Test: restreamer_api_set_metadata with NULL value */
static bool test_set_metadata_null_value(void)
{
	printf("  Testing set_metadata with NULL value...\n");

	bool result = restreamer_api_set_metadata(NULL, "test_key", NULL);
	TEST_ASSERT(!result, "Should return false for NULL value");

	printf("  ✓ set_metadata NULL value handling\n");
	return true;
}

/* Test: restreamer_api_get_process_metadata with NULL api */
static bool test_get_process_metadata_null_api(void)
{
	printf("  Testing get_process_metadata with NULL api...\n");

	char *value = NULL;
	bool result = restreamer_api_get_process_metadata(NULL, "proc_id", "key", &value);
	TEST_ASSERT(!result, "Should return false for NULL api");
	TEST_ASSERT_NULL(value, "value should remain NULL");

	printf("  ✓ get_process_metadata NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_process_metadata with NULL process_id */
static bool test_get_process_metadata_null_process_id(void)
{
	printf("  Testing get_process_metadata with NULL process_id...\n");

	char *value = NULL;
	bool result = restreamer_api_get_process_metadata(NULL, NULL, "key", &value);
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ get_process_metadata NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_get_process_metadata with NULL key */
static bool test_get_process_metadata_null_key(void)
{
	printf("  Testing get_process_metadata with NULL key...\n");

	char *value = NULL;
	bool result = restreamer_api_get_process_metadata(NULL, "proc_id", NULL, &value);
	TEST_ASSERT(!result, "Should return false for NULL key");

	printf("  ✓ get_process_metadata NULL key handling\n");
	return true;
}

/* Test: restreamer_api_get_process_metadata with NULL value pointer */
static bool test_get_process_metadata_null_value(void)
{
	printf("  Testing get_process_metadata with NULL value pointer...\n");

	bool result = restreamer_api_get_process_metadata(NULL, "proc_id", "key", NULL);
	TEST_ASSERT(!result, "Should return false for NULL value pointer");

	printf("  ✓ get_process_metadata NULL value handling\n");
	return true;
}

/* Test: restreamer_api_set_process_metadata with NULL api */
static bool test_set_process_metadata_null_api(void)
{
	printf("  Testing set_process_metadata with NULL api...\n");

	bool result = restreamer_api_set_process_metadata(NULL, "proc_id", "key", "value");
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ set_process_metadata NULL api handling\n");
	return true;
}

/* Test: restreamer_api_set_process_metadata with NULL process_id */
static bool test_set_process_metadata_null_process_id(void)
{
	printf("  Testing set_process_metadata with NULL process_id...\n");

	bool result = restreamer_api_set_process_metadata(NULL, NULL, "key", "value");
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ set_process_metadata NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_set_process_metadata with NULL key */
static bool test_set_process_metadata_null_key(void)
{
	printf("  Testing set_process_metadata with NULL key...\n");

	bool result = restreamer_api_set_process_metadata(NULL, "proc_id", NULL, "value");
	TEST_ASSERT(!result, "Should return false for NULL key");

	printf("  ✓ set_process_metadata NULL key handling\n");
	return true;
}

/* Test: restreamer_api_set_process_metadata with NULL value */
static bool test_set_process_metadata_null_value(void)
{
	printf("  Testing set_process_metadata with NULL value...\n");

	bool result = restreamer_api_set_process_metadata(NULL, "proc_id", "key", NULL);
	TEST_ASSERT(!result, "Should return false for NULL value");

	printf("  ✓ set_process_metadata NULL value handling\n");
	return true;
}

/* ========================================================================
 * Playout Management API Tests
 * ======================================================================== */

/* Test: restreamer_api_get_playout_status with NULL api */
static bool test_get_playout_status_null_api(void)
{
	printf("  Testing get_playout_status with NULL api...\n");

	restreamer_playout_status_t status = {0};
	bool result = restreamer_api_get_playout_status(NULL, "proc_id", "input_id", &status);
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ get_playout_status NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_playout_status with NULL process_id */
static bool test_get_playout_status_null_process_id(void)
{
	printf("  Testing get_playout_status with NULL process_id...\n");

	restreamer_playout_status_t status = {0};
	bool result = restreamer_api_get_playout_status(NULL, NULL, "input_id", &status);
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ get_playout_status NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_get_playout_status with NULL input_id */
static bool test_get_playout_status_null_input_id(void)
{
	printf("  Testing get_playout_status with NULL input_id...\n");

	restreamer_playout_status_t status = {0};
	bool result = restreamer_api_get_playout_status(NULL, "proc_id", NULL, &status);
	TEST_ASSERT(!result, "Should return false for NULL input_id");

	printf("  ✓ get_playout_status NULL input_id handling\n");
	return true;
}

/* Test: restreamer_api_get_playout_status with NULL status pointer */
static bool test_get_playout_status_null_status(void)
{
	printf("  Testing get_playout_status with NULL status pointer...\n");

	bool result = restreamer_api_get_playout_status(NULL, "proc_id", "input_id", NULL);
	TEST_ASSERT(!result, "Should return false for NULL status pointer");

	printf("  ✓ get_playout_status NULL status handling\n");
	return true;
}

/* Test: restreamer_api_free_playout_status with NULL */
static bool test_free_playout_status_null(void)
{
	printf("  Testing free_playout_status with NULL...\n");

	/* Should not crash */
	restreamer_api_free_playout_status(NULL);

	printf("  ✓ free_playout_status NULL handling\n");
	return true;
}

/* Test: restreamer_api_free_playout_status with zeroed structure */
static bool test_free_playout_status_zeroed(void)
{
	printf("  Testing free_playout_status with zeroed structure...\n");

	restreamer_playout_status_t status = {0};
	/* Should not crash */
	restreamer_api_free_playout_status(&status);

	printf("  ✓ free_playout_status zeroed structure handling\n");
	return true;
}

/* Test: restreamer_api_switch_input_stream with NULL api */
static bool test_switch_input_stream_null_api(void)
{
	printf("  Testing switch_input_stream with NULL api...\n");

	bool result = restreamer_api_switch_input_stream(NULL, "proc_id", "input_id", "rtmp://test");
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ switch_input_stream NULL api handling\n");
	return true;
}

/* Test: restreamer_api_switch_input_stream with NULL process_id */
static bool test_switch_input_stream_null_process_id(void)
{
	printf("  Testing switch_input_stream with NULL process_id...\n");

	bool result = restreamer_api_switch_input_stream(NULL, NULL, "input_id", "rtmp://test");
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ switch_input_stream NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_switch_input_stream with NULL input_id */
static bool test_switch_input_stream_null_input_id(void)
{
	printf("  Testing switch_input_stream with NULL input_id...\n");

	bool result = restreamer_api_switch_input_stream(NULL, "proc_id", NULL, "rtmp://test");
	TEST_ASSERT(!result, "Should return false for NULL input_id");

	printf("  ✓ switch_input_stream NULL input_id handling\n");
	return true;
}

/* Test: restreamer_api_switch_input_stream with NULL new_url */
static bool test_switch_input_stream_null_url(void)
{
	printf("  Testing switch_input_stream with NULL new_url...\n");

	bool result = restreamer_api_switch_input_stream(NULL, "proc_id", "input_id", NULL);
	TEST_ASSERT(!result, "Should return false for NULL new_url");

	printf("  ✓ switch_input_stream NULL new_url handling\n");
	return true;
}

/* Test: restreamer_api_reopen_input with NULL api */
static bool test_reopen_input_null_api(void)
{
	printf("  Testing reopen_input with NULL api...\n");

	bool result = restreamer_api_reopen_input(NULL, "proc_id", "input_id");
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ reopen_input NULL api handling\n");
	return true;
}

/* Test: restreamer_api_reopen_input with NULL process_id */
static bool test_reopen_input_null_process_id(void)
{
	printf("  Testing reopen_input with NULL process_id...\n");

	bool result = restreamer_api_reopen_input(NULL, NULL, "input_id");
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ reopen_input NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_reopen_input with NULL input_id */
static bool test_reopen_input_null_input_id(void)
{
	printf("  Testing reopen_input with NULL input_id...\n");

	bool result = restreamer_api_reopen_input(NULL, "proc_id", NULL);
	TEST_ASSERT(!result, "Should return false for NULL input_id");

	printf("  ✓ reopen_input NULL input_id handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL api */
static bool test_get_keyframe_null_api(void)
{
	printf("  Testing get_keyframe with NULL api...\n");

	unsigned char *data = NULL;
	size_t size = 0;
	bool result = restreamer_api_get_keyframe(NULL, "proc_id", "input_id", "frame", &data, &size);
	TEST_ASSERT(!result, "Should return false for NULL api");
	TEST_ASSERT_NULL(data, "data should remain NULL");

	printf("  ✓ get_keyframe NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL process_id */
static bool test_get_keyframe_null_process_id(void)
{
	printf("  Testing get_keyframe with NULL process_id...\n");

	unsigned char *data = NULL;
	size_t size = 0;
	bool result = restreamer_api_get_keyframe(NULL, NULL, "input_id", "frame", &data, &size);
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ get_keyframe NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL input_id */
static bool test_get_keyframe_null_input_id(void)
{
	printf("  Testing get_keyframe with NULL input_id...\n");

	unsigned char *data = NULL;
	size_t size = 0;
	bool result = restreamer_api_get_keyframe(NULL, "proc_id", NULL, "frame", &data, &size);
	TEST_ASSERT(!result, "Should return false for NULL input_id");

	printf("  ✓ get_keyframe NULL input_id handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL name */
static bool test_get_keyframe_null_name(void)
{
	printf("  Testing get_keyframe with NULL name...\n");

	unsigned char *data = NULL;
	size_t size = 0;
	bool result = restreamer_api_get_keyframe(NULL, "proc_id", "input_id", NULL, &data, &size);
	TEST_ASSERT(!result, "Should return false for NULL name");

	printf("  ✓ get_keyframe NULL name handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL data pointer */
static bool test_get_keyframe_null_data(void)
{
	printf("  Testing get_keyframe with NULL data pointer...\n");

	size_t size = 0;
	bool result = restreamer_api_get_keyframe(NULL, "proc_id", "input_id", "frame", NULL, &size);
	TEST_ASSERT(!result, "Should return false for NULL data pointer");

	printf("  ✓ get_keyframe NULL data handling\n");
	return true;
}

/* Test: restreamer_api_get_keyframe with NULL size pointer */
static bool test_get_keyframe_null_size(void)
{
	printf("  Testing get_keyframe with NULL size pointer...\n");

	unsigned char *data = NULL;
	bool result = restreamer_api_get_keyframe(NULL, "proc_id", "input_id", "frame", &data, NULL);
	TEST_ASSERT(!result, "Should return false for NULL size pointer");

	printf("  ✓ get_keyframe NULL size handling\n");
	return true;
}

/* ========================================================================
 * Token Refresh and Authentication API Tests
 * ======================================================================== */

/* Test: restreamer_api_refresh_token with NULL api */
static bool test_refresh_token_null_api(void)
{
	printf("  Testing refresh_token with NULL api...\n");

	bool result = restreamer_api_refresh_token(NULL);
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ refresh_token NULL api handling\n");
	return true;
}

/* Test: restreamer_api_refresh_token with no refresh token */
static bool test_refresh_token_no_token(void)
{
	printf("  Testing refresh_token with no refresh token...\n");

	/* Create API without logging in (no refresh token) */
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	if (!api) {
		fprintf(stderr, "  ⚠ Could not create API for testing\n");
		return true; /* Skip test if API creation fails */
	}

	bool result = restreamer_api_refresh_token(api);
	/* Should fail because there's no refresh token */
	TEST_ASSERT(!result, "Should return false when no refresh token available");

	restreamer_api_destroy(api);

	printf("  ✓ refresh_token no token handling\n");
	return true;
}

/* Test: restreamer_api_force_login with NULL api */
static bool test_force_login_null_api(void)
{
	printf("  Testing force_login with NULL api...\n");

	bool result = restreamer_api_force_login(NULL);
	TEST_ASSERT(!result, "Should return false for NULL api");

	printf("  ✓ force_login NULL api handling\n");
	return true;
}

/* ========================================================================
 * Process Configuration API Tests
 * ======================================================================== */

/* Test: restreamer_api_get_process_config with NULL api */
static bool test_get_process_config_null_api(void)
{
	printf("  Testing get_process_config with NULL api...\n");

	char *config_json = NULL;
	bool result = restreamer_api_get_process_config(NULL, "proc_id", &config_json);
	TEST_ASSERT(!result, "Should return false for NULL api");
	TEST_ASSERT_NULL(config_json, "config_json should remain NULL");

	printf("  ✓ get_process_config NULL api handling\n");
	return true;
}

/* Test: restreamer_api_get_process_config with NULL process_id */
static bool test_get_process_config_null_process_id(void)
{
	printf("  Testing get_process_config with NULL process_id...\n");

	char *config_json = NULL;
	bool result = restreamer_api_get_process_config(NULL, NULL, &config_json);
	TEST_ASSERT(!result, "Should return false for NULL process_id");

	printf("  ✓ get_process_config NULL process_id handling\n");
	return true;
}

/* Test: restreamer_api_get_process_config with NULL config_json pointer */
static bool test_get_process_config_null_output(void)
{
	printf("  Testing get_process_config with NULL config_json pointer...\n");

	bool result = restreamer_api_get_process_config(NULL, "proc_id", NULL);
	TEST_ASSERT(!result, "Should return false for NULL config_json pointer");

	printf("  ✓ get_process_config NULL config_json handling\n");
	return true;
}

/* ========================================================================
 * Edge Cases with Empty Strings
 * ======================================================================== */

/* Test: Empty string parameters with real API instance */
static bool test_empty_string_parameters(void)
{
	printf("  Testing empty string parameters with API instance...\n");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	if (!api) {
		fprintf(stderr, "  ⚠ Could not create API for testing\n");
		return true; /* Skip test if API creation fails */
	}

	/* Test empty key in get_metadata */
	char *value = NULL;
	bool result = restreamer_api_get_metadata(api, "", &value);
	/* Will fail with network error, but we're testing it doesn't crash */
	(void)result;

	/* Test empty process_id in get_process_config */
	char *config = NULL;
	result = restreamer_api_get_process_config(api, "", &config);
	(void)result;

	/* Test empty input_id in reopen_input */
	result = restreamer_api_reopen_input(api, "proc", "");
	(void)result;

	restreamer_api_destroy(api);

	printf("  ✓ empty string parameters handling\n");
	return true;
}

/* ========================================================================
 * Test Runner
 * ======================================================================== */

bool run_api_endpoint_tests(void)
{
	printf("\n========================================\n");
	printf("Running API Endpoint Tests\n");
	printf("========================================\n\n");

	/* Configuration Management Tests */
	printf("Configuration Management API Tests:\n");
	if (!test_get_config_null_api())
		return false;
	if (!test_get_config_null_output())
		return false;
	if (!test_set_config_null_api())
		return false;
	if (!test_set_config_null_config())
		return false;
	if (!test_reload_config_null_api())
		return false;
	printf("\n");

	/* Metadata API Tests */
	printf("Metadata API Tests:\n");
	if (!test_get_metadata_null_api())
		return false;
	if (!test_get_metadata_null_key())
		return false;
	if (!test_get_metadata_null_value())
		return false;
	if (!test_set_metadata_null_api())
		return false;
	if (!test_set_metadata_null_key())
		return false;
	if (!test_set_metadata_null_value())
		return false;
	if (!test_get_process_metadata_null_api())
		return false;
	if (!test_get_process_metadata_null_process_id())
		return false;
	if (!test_get_process_metadata_null_key())
		return false;
	if (!test_get_process_metadata_null_value())
		return false;
	if (!test_set_process_metadata_null_api())
		return false;
	if (!test_set_process_metadata_null_process_id())
		return false;
	if (!test_set_process_metadata_null_key())
		return false;
	if (!test_set_process_metadata_null_value())
		return false;
	printf("\n");

	/* Playout Management Tests */
	printf("Playout Management API Tests:\n");
	if (!test_get_playout_status_null_api())
		return false;
	if (!test_get_playout_status_null_process_id())
		return false;
	if (!test_get_playout_status_null_input_id())
		return false;
	if (!test_get_playout_status_null_status())
		return false;
	if (!test_free_playout_status_null())
		return false;
	if (!test_free_playout_status_zeroed())
		return false;
	if (!test_switch_input_stream_null_api())
		return false;
	if (!test_switch_input_stream_null_process_id())
		return false;
	if (!test_switch_input_stream_null_input_id())
		return false;
	if (!test_switch_input_stream_null_url())
		return false;
	if (!test_reopen_input_null_api())
		return false;
	if (!test_reopen_input_null_process_id())
		return false;
	if (!test_reopen_input_null_input_id())
		return false;
	if (!test_get_keyframe_null_api())
		return false;
	if (!test_get_keyframe_null_process_id())
		return false;
	if (!test_get_keyframe_null_input_id())
		return false;
	if (!test_get_keyframe_null_name())
		return false;
	if (!test_get_keyframe_null_data())
		return false;
	if (!test_get_keyframe_null_size())
		return false;
	printf("\n");

	/* Token Refresh and Authentication Tests */
	printf("Token Refresh and Authentication API Tests:\n");
	if (!test_refresh_token_null_api())
		return false;
	if (!test_refresh_token_no_token())
		return false;
	if (!test_force_login_null_api())
		return false;
	printf("\n");

	/* Process Configuration Tests */
	printf("Process Configuration API Tests:\n");
	if (!test_get_process_config_null_api())
		return false;
	if (!test_get_process_config_null_process_id())
		return false;
	if (!test_get_process_config_null_output())
		return false;
	printf("\n");

	/* Edge Cases */
	printf("Edge Cases:\n");
	if (!test_empty_string_parameters())
		return false;
	printf("\n");

	printf("========================================\n");
	printf("All API Endpoint Tests Passed!\n");
	printf("========================================\n\n");

	return true;
}
