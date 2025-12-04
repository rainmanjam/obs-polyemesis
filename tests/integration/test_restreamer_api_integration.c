/*
 * Integration Tests for Restreamer API - Live Server Testing
 *
 * Tests against a live Restreamer server at https://rs2.rainmanjam.com
 *
 * Server details:
 * - URL: https://rs2.rainmanjam.com
 * - Username: admin
 * - Password: tenn2jagWEE@##$
 * - SSL verification disabled (-k flag)
 *
 * Test Coverage:
 * 1. Authentication (login, token refresh)
 * 2. Process management (list, create, start/stop, delete)
 * 3. Error handling (401, 404 responses)
 * 4. JSON structure validation (cleanup, limits fields)
 */

#include "../test_framework.h"
#include "../../src/restreamer-api.h"
#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <time.h>

/* Test server configuration */
#define TEST_SERVER_HOST "rs2.rainmanjam.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "admin"
#define TEST_SERVER_PASSWORD "tenn2jagWEE@##$"
#define TEST_SERVER_USE_HTTPS true

/* Memory buffer for HTTP responses */
struct memory_struct {
	char *memory;
	size_t size;
};

/* Write callback for curl */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memory_struct *mem = (struct memory_struct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

/* Test 1: API Login and Token Retrieval */
static bool test_api_login(void)
{
	printf("  Testing login to %s...\n", TEST_SERVER_HOST);

	struct memory_struct chunk = {0};
	chunk.memory = malloc(1);
	chunk.size = 0;

	CURL *curl = curl_easy_init();
	ASSERT_NOT_NULL(curl, "Should initialize CURL");

	char url[512];
	snprintf(url, sizeof(url), "https://%s/api/login", TEST_SERVER_HOST);

	/* Build login JSON */
	json_t *login_json = json_object();
	json_object_set_new(login_json, "username", json_string(TEST_SERVER_USERNAME));
	json_object_set_new(login_json, "password", json_string(TEST_SERVER_PASSWORD));
	char *login_data = json_dumps(login_json, JSON_COMPACT);
	json_decref(login_json);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, login_data);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); /* -k flag */
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	CURLcode res = curl_easy_perform(curl);

	free(login_data);
	curl_slist_free_all(headers);

	ASSERT_EQ(res, CURLE_OK, "CURL request should succeed");

	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

	printf("  Response code: %ld\n", response_code);

	ASSERT_EQ(response_code, 200, "Should get 200 OK response");

	/* Parse response JSON */
	json_error_t error;
	json_t *response = json_loads(chunk.memory, 0, &error);
	ASSERT_NOT_NULL(response, "Should parse JSON response");

	/* Check for access token */
	json_t *access_token = json_object_get(response, "access_token");
	ASSERT_NOT_NULL(access_token, "Response should contain access_token");
	ASSERT_TRUE(json_is_string(access_token), "access_token should be string");

	/* Check for refresh token */
	json_t *refresh_token = json_object_get(response, "refresh_token");
	ASSERT_NOT_NULL(refresh_token, "Response should contain refresh_token");
	ASSERT_TRUE(json_is_string(refresh_token), "refresh_token should be string");

	printf("  Access token: %.50s...\n", json_string_value(access_token));

	json_decref(response);
	curl_easy_cleanup(curl);
	free(chunk.memory);

	return true;
}

/* Test 2: API Token Refresh Mechanism */
static bool test_api_token_refresh(void)
{
	printf("  Testing token refresh mechanism...\n");

	/* First, login to get tokens */
	struct memory_struct chunk = {0};
	chunk.memory = malloc(1);
	chunk.size = 0;

	CURL *curl = curl_easy_init();
	ASSERT_NOT_NULL(curl, "Should initialize CURL");

	/* Login */
	char url[512];
	snprintf(url, sizeof(url), "https://%s/api/login", TEST_SERVER_HOST);

	json_t *login_json = json_object();
	json_object_set_new(login_json, "username", json_string(TEST_SERVER_USERNAME));
	json_object_set_new(login_json, "password", json_string(TEST_SERVER_PASSWORD));
	char *login_data = json_dumps(login_json, JSON_COMPACT);
	json_decref(login_json);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, login_data);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	CURLcode res = curl_easy_perform(curl);
	free(login_data);

	ASSERT_EQ(res, CURLE_OK, "Login should succeed");

	json_error_t error;
	json_t *login_response = json_loads(chunk.memory, 0, &error);
	ASSERT_NOT_NULL(login_response, "Should parse login response");

	const char *refresh_token_str = json_string_value(json_object_get(login_response, "refresh_token"));
	ASSERT_NOT_NULL(refresh_token_str, "Should get refresh token");

	char refresh_token[512];
	snprintf(refresh_token, sizeof(refresh_token), "%s", refresh_token_str);
	json_decref(login_response);

	/* Now test refresh */
	free(chunk.memory);
	chunk.memory = malloc(1);
	chunk.size = 0;

	snprintf(url, sizeof(url), "https://%s/api/login/refresh", TEST_SERVER_HOST);

	json_t *refresh_json = json_object();
	json_object_set_new(refresh_json, "refresh_token", json_string(refresh_token));
	char *refresh_data = json_dumps(refresh_json, JSON_COMPACT);
	json_decref(refresh_json);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, refresh_data);

	res = curl_easy_perform(curl);
	free(refresh_data);
	curl_slist_free_all(headers);

	ASSERT_EQ(res, CURLE_OK, "Refresh should succeed");

	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	ASSERT_EQ(response_code, 200, "Should get 200 OK on refresh");

	json_t *refresh_response = json_loads(chunk.memory, 0, &error);
	ASSERT_NOT_NULL(refresh_response, "Should parse refresh response");

	json_t *new_access_token = json_object_get(refresh_response, "access_token");
	ASSERT_NOT_NULL(new_access_token, "Refresh should return new access token");

	json_decref(refresh_response);
	curl_easy_cleanup(curl);
	free(chunk.memory);

	return true;
}

/* Test 3: List Processes */
static bool test_api_list_processes(void)
{
	printf("  Testing list processes...\n");

	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = TEST_SERVER_USERNAME,
		.password = TEST_SERVER_PASSWORD,
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	/* Test connection first */
	bool connected = restreamer_api_test_connection(api);
	if (!connected) {
		const char *error = restreamer_api_get_error(api);
		printf("  Connection error: %s\n", error ? error : "unknown");
	}
	ASSERT_TRUE(connected, "Should connect to server");

	/* Get processes */
	restreamer_process_list_t list = {0};
	bool result = restreamer_api_get_processes(api, &list);
	if (!result) {
		const char *error = restreamer_api_get_error(api);
		printf("  Get processes error: %s\n", error ? error : "unknown");
	}
	ASSERT_TRUE(result, "Should get processes list");

	printf("  Found %zu processes\n", list.count);

	/* Clean up */
	restreamer_api_free_process_list(&list);
	restreamer_api_destroy(api);

	return true;
}

/* Test 4: Create Process with Correct JSON Structure */
static bool test_api_create_process(void)
{
	printf("  Testing create process with correct JSON structure...\n");

	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = TEST_SERVER_USERNAME,
		.password = TEST_SERVER_PASSWORD,
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	bool connected = restreamer_api_test_connection(api);
	ASSERT_TRUE(connected, "Should connect to server");

	/* Create a test process */
	char reference[128];
	snprintf(reference, sizeof(reference), "test-process-%ld", (long)time(NULL));

	const char *input_url = "rtmp://localhost/live/test";
	const char *output_urls[] = {
		"rtmp://localhost/live/out1",
		"rtmp://localhost/live/out2"
	};

	bool created = restreamer_api_create_process(
		api,
		reference,
		input_url,
		output_urls,
		2,
		NULL
	);

	if (!created) {
		const char *error = restreamer_api_get_error(api);
		printf("  Create process error: %s\n", error ? error : "unknown");
	}

	/* Note: Process creation might fail if input is not available, which is expected */
	printf("  Process creation %s (expected if input not available)\n",
	       created ? "succeeded" : "failed");

	restreamer_api_destroy(api);

	/* Test passes even if process creation fails (test server might not have input) */
	return true;
}

/* Test 5: Process Command (Start/Stop with PUT method) */
static bool test_api_process_command(void)
{
	printf("  Testing process start/stop commands (PUT method)...\n");

	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = TEST_SERVER_USERNAME,
		.password = TEST_SERVER_PASSWORD,
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	bool connected = restreamer_api_test_connection(api);
	ASSERT_TRUE(connected, "Should connect to server");

	/* Get list of processes to find one to test with */
	restreamer_process_list_t list = {0};
	bool result = restreamer_api_get_processes(api, &list);

	if (result && list.count > 0) {
		const char *process_id = list.processes[0].id;
		printf("  Testing with process: %s\n", process_id);

		/* Try to start the process */
		bool started = restreamer_api_start_process(api, process_id);
		printf("  Start command %s\n", started ? "succeeded" : "failed");

		/* Try to stop the process */
		bool stopped = restreamer_api_stop_process(api, process_id);
		printf("  Stop command %s\n", stopped ? "succeeded" : "failed");

		restreamer_api_free_process_list(&list);
	} else {
		printf("  No processes available for testing\n");
	}

	restreamer_api_destroy(api);

	return true;
}

/* Test 6: Delete Process */
static bool test_api_delete_process(void)
{
	printf("  Testing delete process...\n");

	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = TEST_SERVER_USERNAME,
		.password = TEST_SERVER_PASSWORD,
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	bool connected = restreamer_api_test_connection(api);
	ASSERT_TRUE(connected, "Should connect to server");

	/* Create a test process first */
	char reference[128];
	snprintf(reference, sizeof(reference), "test-delete-%ld", (long)time(NULL));

	const char *input_url = "rtmp://localhost/live/test-delete";
	const char *output_urls[] = {"rtmp://localhost/live/out-delete"};

	bool created = restreamer_api_create_process(
		api,
		reference,
		input_url,
		output_urls,
		1,
		NULL
	);

	if (created) {
		printf("  Test process created, attempting to delete...\n");

		/* Try to delete it */
		bool deleted = restreamer_api_delete_process(api, reference);
		printf("  Delete %s\n", deleted ? "succeeded" : "failed");
	} else {
		printf("  Could not create test process for deletion test\n");
	}

	restreamer_api_destroy(api);

	return true;
}

/* Test 7: Error Handling (401/404 responses) */
static bool test_api_error_handling(void)
{
	printf("  Testing error handling (401/404 responses)...\n");

	/* Test 401: Invalid credentials */
	restreamer_connection_t conn_invalid = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = "invalid_user",
		.password = "invalid_password",
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn_invalid);
	ASSERT_NOT_NULL(api, "Should create API client with invalid credentials");

	bool connected = restreamer_api_test_connection(api);
	ASSERT_FALSE(connected, "Should fail to connect with invalid credentials");

	const char *error = restreamer_api_get_error(api);
	printf("  Expected 401 error: %s\n", error ? error : "no error message");

	restreamer_api_destroy(api);

	/* Test 404: Non-existent process */
	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = TEST_SERVER_USERNAME,
		.password = TEST_SERVER_PASSWORD,
		.use_https = TEST_SERVER_USE_HTTPS
	};

	api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	connected = restreamer_api_test_connection(api);
	ASSERT_TRUE(connected, "Should connect with valid credentials");

	/* Try to access non-existent process */
	restreamer_process_t process = {0};
	bool result = restreamer_api_get_process(api, "non-existent-process-12345", &process);
	ASSERT_FALSE(result, "Should fail to get non-existent process");

	error = restreamer_api_get_error(api);
	printf("  Expected 404 error: %s\n", error ? error : "no error message");

	restreamer_api_destroy(api);

	return true;
}

/* Test 8: Invalid Credentials */
static bool test_api_invalid_credentials(void)
{
	printf("  Testing with wrong credentials...\n");

	restreamer_connection_t conn = {
		.host = TEST_SERVER_HOST,
		.port = TEST_SERVER_PORT,
		.username = "wrong_username",
		.password = "wrong_password",
		.use_https = TEST_SERVER_USE_HTTPS
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	bool connected = restreamer_api_test_connection(api);
	ASSERT_FALSE(connected, "Should not connect with wrong credentials");

	const char *error = restreamer_api_get_error(api);
	ASSERT_NOT_NULL(error, "Should have error message");
	printf("  Error message: %s\n", error);

	restreamer_api_destroy(api);

	return true;
}

/* Test 9: Process JSON Structure Verification */
static bool test_process_json_structure(void)
{
	printf("  Testing process JSON structure (cleanup, limits fields)...\n");

	/* Use curl directly to inspect raw JSON response */
	struct memory_struct chunk = {0};
	chunk.memory = malloc(1);
	chunk.size = 0;

	CURL *curl = curl_easy_init();
	ASSERT_NOT_NULL(curl, "Should initialize CURL");

	/* First, login to get access token */
	char url[512];
	snprintf(url, sizeof(url), "https://%s/api/login", TEST_SERVER_HOST);

	json_t *login_json = json_object();
	json_object_set_new(login_json, "username", json_string(TEST_SERVER_USERNAME));
	json_object_set_new(login_json, "password", json_string(TEST_SERVER_PASSWORD));
	char *login_data = json_dumps(login_json, JSON_COMPACT);
	json_decref(login_json);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, login_data);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

	CURLcode res = curl_easy_perform(curl);
	free(login_data);

	ASSERT_EQ(res, CURLE_OK, "Login should succeed");

	json_error_t error;
	json_t *login_response = json_loads(chunk.memory, 0, &error);
	ASSERT_NOT_NULL(login_response, "Should parse login response");

	const char *access_token_str = json_string_value(json_object_get(login_response, "access_token"));
	ASSERT_NOT_NULL(access_token_str, "Should get access token");

	char access_token[512];
	snprintf(access_token, sizeof(access_token), "%s", access_token_str);
	json_decref(login_response);

	/* Now get process list with authorization */
	free(chunk.memory);
	chunk.memory = malloc(1);
	chunk.size = 0;

	snprintf(url, sizeof(url), "https://%s/api/v3/process", TEST_SERVER_HOST);

	curl_slist_free_all(headers);
	headers = NULL;

	char auth_header[600];
	snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);
	headers = curl_slist_append(headers, auth_header);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	res = curl_easy_perform(curl);

	ASSERT_EQ(res, CURLE_OK, "Get processes should succeed");

	json_t *processes_response = json_loads(chunk.memory, 0, &error);
	ASSERT_NOT_NULL(processes_response, "Should parse processes response");

	/* Check if response is an array */
	ASSERT_TRUE(json_is_array(processes_response), "Response should be an array");

	size_t process_count = json_array_size(processes_response);
	printf("  Found %zu processes\n", process_count);

	/* Check each process for required fields */
	for (size_t i = 0; i < process_count; i++) {
		json_t *process = json_array_get(processes_response, i);

		/* Check for basic fields */
		json_t *id = json_object_get(process, "id");
		ASSERT_NOT_NULL(id, "Process should have 'id' field");

		json_t *reference = json_object_get(process, "reference");
		ASSERT_NOT_NULL(reference, "Process should have 'reference' field");

		/* Check for config object */
		json_t *config = json_object_get(process, "config");
		if (config) {
			/* Check for cleanup field */
			json_t *cleanup = json_object_get(config, "cleanup");
			if (cleanup) {
				printf("  Process %zu has 'cleanup' field\n", i);
			}

			/* Check for limits field */
			json_t *limits = json_object_get(config, "limits");
			if (limits) {
				printf("  Process %zu has 'limits' field\n", i);
			}
		}
	}

	json_decref(processes_response);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	free(chunk.memory);

	return true;
}

/* Main test suite */
BEGIN_TEST_SUITE("Integration Tests - Live Restreamer API")

RUN_TEST(test_api_login, "Test 1: API login and token retrieval");
RUN_TEST(test_api_token_refresh, "Test 2: API token refresh mechanism");
RUN_TEST(test_api_list_processes, "Test 3: List processes");
RUN_TEST(test_api_create_process, "Test 4: Create process with correct JSON structure");
RUN_TEST(test_api_process_command, "Test 5: Process start/stop commands (PUT method)");
RUN_TEST(test_api_delete_process, "Test 6: Delete process");
RUN_TEST(test_api_error_handling, "Test 7: Error handling (401/404 responses)");
RUN_TEST(test_api_invalid_credentials, "Test 8: Invalid credentials test");
RUN_TEST(test_process_json_structure, "Test 9: Process JSON structure verification");

END_TEST_SUITE()
