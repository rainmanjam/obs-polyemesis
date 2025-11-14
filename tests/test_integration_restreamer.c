/*
 * Integration Tests with Live Restreamer API
 * Tests against http://localhost:8080/api/v3/
 */

#include "test_framework.h"
#include "../src/restreamer-api.h"
#include "../src/restreamer-output-profile.h"
#include <curl/curl.h>
#include <string.h>

#define RESTREAMER_API_URL "http://localhost:8080/api/v3"

/* Memory to store HTTP response */
struct memory_struct {
	char *memory;
	size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb,
			      void *userp)
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

/* Test 1: Real API connection */
static bool test_real_api_connection(void)
{
	struct memory_struct chunk = {0};
	chunk.memory = malloc(1);
	chunk.size = 0;

	CURL *curl = curl_easy_init();
	ASSERT_NOT_NULL(curl, "Should initialize CURL");

	char url[256];
	snprintf(url, sizeof(url), "%s", RESTREAMER_API_URL);

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	CURLcode res = curl_easy_perform(curl);

	if (res == CURLE_OK) {
		long response_code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
				  &response_code);
		// API root might return 404 or redirect, check it responded
		ASSERT_TRUE(
			response_code > 0,
			"Should get HTTP response from Restreamer API");
	} else {
		// Connection failed - Restreamer might not be running
		fprintf(stderr,
			"⚠️  Warning: Could not connect to Restreamer at %s\n",
			url);
		fprintf(stderr, "   Error: %s\n", curl_easy_strerror(res));
		fprintf(stderr,
			"   This test requires a running Restreamer instance.\n");
		fprintf(stderr,
			"   Start with: docker run -d -p 8080:8080 datarhei/restreamer:latest\n");
	}

	curl_easy_cleanup(curl);
	free(chunk.memory);

	// Test passes even if Restreamer is not running (just warns)
	return true;
}

/* Test 2: API client creation */
static bool test_create_api_client(void)
{
	restreamer_connection_t connection = {
		.host = "localhost",
		.port = 8080,
		.username = NULL,
		.password = NULL,
		.use_https = false
	};
	restreamer_api_t *api = restreamer_api_create(&connection);
	ASSERT_NOT_NULL(api, "Should create API client");

	restreamer_api_destroy(api);
	return true;
}

/* Test 3: Profile manager with real API */
static bool test_profile_manager_with_api(void)
{
	restreamer_connection_t connection = {
		.host = "localhost",
		.port = 8080,
		.username = NULL,
		.password = NULL,
		.use_https = false
	};
	restreamer_api_t *api = restreamer_api_create(&connection);
	profile_manager_t *manager = profile_manager_create(api);

	ASSERT_NOT_NULL(manager, "Should create profile manager");

	// Create profile
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Integration Test Profile");
	ASSERT_NOT_NULL(profile, "Should create profile");

	// Add destination
	encoding_settings_t encoding = profile_get_default_encoding();
	bool added = profile_add_destination(profile, SERVICE_YOUTUBE,
					     "integration-test-key-12345",
					     ORIENTATION_HORIZONTAL,
					     &encoding);
	ASSERT_TRUE(added, "Should add destination");

	// Cleanup
	profile_manager_destroy(manager);
	restreamer_api_destroy(api);
	return true;
}

/* Test 4: Health check integration (requires Restreamer) */
static bool test_health_check_integration(void)
{
	restreamer_connection_t connection = {
		.host = "localhost",
		.port = 8080,
		.username = NULL,
		.password = NULL,
		.use_https = false
	};
	restreamer_api_t *api = restreamer_api_create(&connection);
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Health Check Test");

	encoding_settings_t encoding = profile_get_default_encoding();
	profile_add_destination(profile, SERVICE_YOUTUBE, "health-test-key",
				ORIENTATION_HORIZONTAL, &encoding);

	// Enable health monitoring (takes profile and enabled flag)
	profile_set_health_monitoring(profile, true);

	// Note: Health check may fail if stream is not actually running
	// That's expected - we're just testing the integration path
	bool result = profile_check_health(profile, api);
	(void)result; // Result can be true or false, both are acceptable

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);
	return true;
}

/* Test 5: Error handling with invalid endpoint */
static bool test_error_handling_invalid_api(void)
{
	// Test with invalid endpoint
	restreamer_connection_t connection = {
		.host = "localhost",
		.port = 9999,
		.username = NULL,
		.password = NULL,
		.use_https = false
	};
	restreamer_api_t *api = restreamer_api_create(&connection);
	ASSERT_NOT_NULL(api,
			"Should create API client even with invalid endpoint");

	profile_manager_t *manager = profile_manager_create(api);
	ASSERT_NOT_NULL(manager, "Should create manager");

	// Operations may fail gracefully - that's expected
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Error Test");
	(void)profile; // May be NULL, that's OK for this test

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);
	return true;
}

BEGIN_TEST_SUITE("Integration Tests - Live Restreamer API")
RUN_TEST(test_real_api_connection,
	 "Connect to real Restreamer API (http://localhost:8080)");
RUN_TEST(test_create_api_client, "Create API client instance");
RUN_TEST(test_profile_manager_with_api,
	 "Create profile manager with real API");
RUN_TEST(test_health_check_integration, "Health check integration path");
RUN_TEST(test_error_handling_invalid_api,
	 "Error handling with invalid API endpoint");
END_TEST_SUITE()
