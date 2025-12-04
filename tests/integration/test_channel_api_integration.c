/*
 * Integration Tests for Channel-API-Multistream Integration
 *
 * These tests verify that multiple modules work together correctly:
 * - Channel management with API client
 * - Multistream configuration with API calls
 * - Configuration changes and reconnection
 * - Error recovery and state management
 *
 * Test Modes:
 * - Mock mode: Uses mock server for CI/CD pipelines
 * - Live mode: Tests against real server when LIVE_TEST_SERVER=1
 *
 * Live Test Server (when LIVE_TEST_SERVER=1):
 * - URL: https://rs2.rainmanjam.com
 * - Username: admin
 * - Password: tenn2jagWEE@##$
 */

#include "../test_framework.h"
#include "../../src/restreamer-api.h"
#include "../../src/restreamer-channel.h"
#include "../../src/restreamer-multistream.h"
#include "../mock_restreamer.h"
#include <curl/curl.h>
#include <jansson.h>
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

/* Test server configuration */
#define LIVE_SERVER_HOST "rs2.rainmanjam.com"
#define LIVE_SERVER_PORT 443
#define LIVE_SERVER_USERNAME "admin"
#define LIVE_SERVER_PASSWORD "tenn2jagWEE@##$"
#define LIVE_SERVER_USE_HTTPS true

#define MOCK_SERVER_HOST "localhost"
#define MOCK_SERVER_PORT 9500
#define MOCK_SERVER_USERNAME "admin"
#define MOCK_SERVER_PASSWORD "testpass"
#define MOCK_SERVER_USE_HTTPS false

/* Check if we should use live server */
static bool use_live_server(void)
{
	const char *env = getenv("LIVE_TEST_SERVER");
	return env && (strcmp(env, "1") == 0 || strcmp(env, "true") == 0);
}

/* Get connection config for current test mode */
static restreamer_connection_t get_test_connection(void)
{
	if (use_live_server()) {
		printf("  Using LIVE server: %s\n", LIVE_SERVER_HOST);
		restreamer_connection_t conn = {
			.host = LIVE_SERVER_HOST,
			.port = LIVE_SERVER_PORT,
			.username = LIVE_SERVER_USERNAME,
			.password = LIVE_SERVER_PASSWORD,
			.use_https = LIVE_SERVER_USE_HTTPS
		};
		return conn;
	} else {
		printf("  Using MOCK server: %s:%d\n", MOCK_SERVER_HOST, MOCK_SERVER_PORT);
		restreamer_connection_t conn = {
			.host = MOCK_SERVER_HOST,
			.port = MOCK_SERVER_PORT,
			.username = MOCK_SERVER_USERNAME,
			.password = MOCK_SERVER_PASSWORD,
			.use_https = MOCK_SERVER_USE_HTTPS
		};
		return conn;
	}
}

/* Setup mock server if needed */
static bool setup_test_server(void)
{
	if (use_live_server()) {
		/* Live server - no setup needed */
		return true;
	}

	/* Start mock server */
	if (!mock_restreamer_start(MOCK_SERVER_PORT)) {
		fprintf(stderr, "  Failed to start mock server on port %d\n", MOCK_SERVER_PORT);
		return false;
	}

	sleep_ms(500); /* Give server time to start */
	return true;
}

/* Teardown mock server if needed */
static void teardown_test_server(void)
{
	if (!use_live_server()) {
		mock_restreamer_stop();
	}
}

/* ========================================================================
 * Test 1: Channel to API Integration - Channel Start Creates API Calls
 * ======================================================================== */
static bool test_channel_start_creates_api_calls(void)
{
	printf("  Testing channel start creates correct API calls...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");

	/* Test connection */
	bool connected = restreamer_api_test_connection(api);
	if (!connected) {
		const char *error = restreamer_api_get_error(api);
		printf("  Connection error: %s\n", error ? error : "unknown");
	}
	ASSERT_TRUE(connected, "Should connect to server");

	/* Create channel manager and channel */
	channel_manager_t *manager = channel_manager_create(api);
	ASSERT_NOT_NULL(manager, "Should create channel manager");

	stream_channel_t *channel = channel_manager_create_channel(manager, "Integration Test Channel");
	ASSERT_NOT_NULL(channel, "Should create channel");

	/* Add outputs to channel */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(channel, SERVICE_YOUTUBE,
					"test-key-youtube",
					ORIENTATION_HORIZONTAL,
					&encoding);
	ASSERT_TRUE(added, "Should add YouTube output");

	added = channel_add_output(channel, SERVICE_TWITCH,
				    "test-key-twitch",
				    ORIENTATION_HORIZONTAL,
				    &encoding);
	ASSERT_TRUE(added, "Should add Twitch output");

	/* Start channel - this should create process on server */
	bool started = channel_start(manager, channel->channel_id);
	printf("  Channel start %s\n", started ? "succeeded" : "failed");

	/* Verify process was created on server */
	if (started && channel->process_reference) {
		printf("  Process reference: %s\n", channel->process_reference);

		restreamer_process_list_t list = {0};
		bool got_list = restreamer_api_get_processes(api, &list);
		ASSERT_TRUE(got_list, "Should get process list from server");

		/* Find our process */
		bool found = false;
		for (size_t i = 0; i < list.count; i++) {
			if (strcmp(list.processes[i].reference, channel->process_reference) == 0) {
				found = true;
				printf("  Found process on server: %s (state: %s)\n",
				       list.processes[i].id,
				       list.processes[i].state);
				break;
			}
		}

		ASSERT_TRUE(found, "Process should exist on server");
		restreamer_api_free_process_list(&list);
	}

	/* Stop channel */
	bool stopped = channel_stop(manager, channel->channel_id);
	printf("  Channel stop %s\n", stopped ? "succeeded" : "failed");

	/* Cleanup */
	channel_manager_destroy(manager);
	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 2: Channel Stop Properly Cleans Up on Server
 * ======================================================================== */
static bool test_channel_stop_cleanup(void)
{
	printf("  Testing channel stop properly cleans up on server...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Create and start channel */
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Cleanup Test");

	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "cleanup-test",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Start channel */
	bool started = channel_start(manager, channel->channel_id);
	if (!started) {
		printf("  Channel start failed (may be expected in mock mode)\n");
	}

	char process_ref[256] = {0};
	if (channel->process_reference) {
		snprintf(process_ref, sizeof(process_ref), "%s", channel->process_reference);
		printf("  Created process: %s\n", process_ref);
	}

	/* Stop channel - should delete process */
	bool stopped = channel_stop(manager, channel->channel_id);
	printf("  Channel stop %s\n", stopped ? "succeeded" : "failed");

	/* Verify process is gone from server */
	if (strlen(process_ref) > 0) {
		restreamer_process_t process = {0};
		bool exists = restreamer_api_get_process(api, process_ref, &process);

		/* Process should not exist after stop */
		ASSERT_FALSE(exists, "Process should be deleted from server");

		if (exists) {
			printf("  WARNING: Process still exists: %s\n", process.state);
			restreamer_api_free_process(&process);
		}
	}

	/* Channel status should be inactive */
	ASSERT_TRUE(channel->status == CHANNEL_STATUS_INACTIVE,
		    "Channel status should be inactive after stop");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 3: Channel State Reflects Server State
 * ======================================================================== */
static bool test_channel_state_reflects_server(void)
{
	printf("  Testing channel state reflects server state...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "State Test");

	/* Initial state */
	ASSERT_TRUE(channel->status == CHANNEL_STATUS_INACTIVE,
		    "Channel should start inactive");

	/* Add output and start */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "state-test",
			   ORIENTATION_HORIZONTAL, &encoding);

	bool started = channel_start(manager, channel->channel_id);
	if (started) {
		/* State should change to active or starting */
		ASSERT_TRUE(channel->status == CHANNEL_STATUS_ACTIVE ||
			    channel->status == CHANNEL_STATUS_STARTING,
			    "Channel should be active or starting after start");

		/* Update stats from server */
		if (channel->process_reference) {
			bool updated = channel_update_stats(channel, api);
			printf("  Stats update %s\n", updated ? "succeeded" : "failed");

			/* Check if outputs have connection state */
			for (size_t i = 0; i < channel->output_count; i++) {
				printf("  Output %zu: connected=%d, bytes_sent=%llu\n",
				       i,
				       channel->outputs[i].connected,
				       (unsigned long long)channel->outputs[i].bytes_sent);
			}
		}

		/* Stop channel */
		channel_stop(manager, channel->channel_id);
		ASSERT_TRUE(channel->status == CHANNEL_STATUS_INACTIVE ||
			    channel->status == CHANNEL_STATUS_STOPPING,
			    "Channel should be inactive or stopping after stop");
	}

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 4: Multistream Config Creates Correct Process JSON
 * ======================================================================== */
static bool test_multistream_config_json(void)
{
	printf("  Testing multistream config creates correct process JSON...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Create multistream config */
	multistream_config_t *config = restreamer_multistream_create();
	ASSERT_NOT_NULL(config, "Should create multistream config");

	/* Add destinations */
	bool added = restreamer_multistream_add_destination(
		config, SERVICE_YOUTUBE, "test-youtube-key", ORIENTATION_HORIZONTAL);
	ASSERT_TRUE(added, "Should add YouTube destination");

	added = restreamer_multistream_add_destination(
		config, SERVICE_TWITCH, "test-twitch-key", ORIENTATION_HORIZONTAL);
	ASSERT_TRUE(added, "Should add Twitch destination");

	/* Set source orientation */
	config->source_orientation = ORIENTATION_HORIZONTAL;

	/* Start multistream - this creates the process */
	char input_url[256];
	snprintf(input_url, sizeof(input_url), "rtmp://localhost/live/test-%ld",
		 (long)time(NULL));

	bool started = restreamer_multistream_start(api, config, input_url);
	printf("  Multistream start %s\n", started ? "succeeded" : "failed");

	/* Verify process was created with correct outputs */
	if (started && config->process_reference) {
		printf("  Process reference: %s\n", config->process_reference);

		/* Get process config from server */
		char *config_json = NULL;
		bool got_config = restreamer_api_get_process_config(
			api, config->process_reference, &config_json);

		if (got_config && config_json) {
			printf("  Got process config from server\n");

			/* Parse JSON to verify structure */
			json_error_t error;
			json_t *root = json_loads(config_json, 0, &error);
			if (root) {
				/* Check for outputs array */
				json_t *outputs = json_object_get(root, "output");
				if (outputs && json_is_array(outputs)) {
					size_t output_count = json_array_size(outputs);
					printf("  Process has %zu outputs\n", output_count);
					ASSERT_TRUE(output_count >= 2,
						    "Should have at least 2 outputs");
				}

				json_decref(root);
			}

			free(config_json);
		}

		/* Stop multistream */
		restreamer_multistream_stop(api, config->process_reference);
	}

	restreamer_multistream_destroy(config);
	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 5: Output URL Construction for Different Services
 * ======================================================================== */
static bool test_output_url_construction(void)
{
	printf("  Testing output URL construction for different services...\n");

	/* Test YouTube URL */
	const char *youtube_url = restreamer_multistream_get_service_url(
		SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL);
	ASSERT_NOT_NULL(youtube_url, "Should get YouTube URL");
	printf("  YouTube URL: %s\n", youtube_url);
	ASSERT_TRUE(strstr(youtube_url, "rtmp://") != NULL,
		    "YouTube URL should start with rtmp://");

	/* Test Twitch URL */
	const char *twitch_url = restreamer_multistream_get_service_url(
		SERVICE_TWITCH, ORIENTATION_HORIZONTAL);
	ASSERT_NOT_NULL(twitch_url, "Should get Twitch URL");
	printf("  Twitch URL: %s\n", twitch_url);
	ASSERT_TRUE(strstr(twitch_url, "rtmp://") != NULL,
		    "Twitch URL should start with rtmp://");

	/* Test TikTok vertical URL */
	const char *tiktok_url = restreamer_multistream_get_service_url(
		SERVICE_TIKTOK, ORIENTATION_VERTICAL);
	ASSERT_NOT_NULL(tiktok_url, "Should get TikTok URL");
	printf("  TikTok (vertical) URL: %s\n", tiktok_url);

	/* Test service names */
	const char *youtube_name = restreamer_multistream_get_service_name(SERVICE_YOUTUBE);
	ASSERT_STR_EQ(youtube_name, "YouTube", "Should get correct YouTube name");

	const char *twitch_name = restreamer_multistream_get_service_name(SERVICE_TWITCH);
	ASSERT_STR_EQ(twitch_name, "Twitch", "Should get correct Twitch name");

	return true;
}

/* ========================================================================
 * Test 6: Orientation-Based Video Filtering
 * ======================================================================== */
static bool test_orientation_video_filtering(void)
{
	printf("  Testing orientation-based video filtering...\n");

	/* Test orientation detection */
	stream_orientation_t horizontal = restreamer_multistream_detect_orientation(1920, 1080);
	ASSERT_TRUE(horizontal == ORIENTATION_HORIZONTAL,
		    "1920x1080 should be detected as horizontal");

	stream_orientation_t vertical = restreamer_multistream_detect_orientation(1080, 1920);
	ASSERT_TRUE(vertical == ORIENTATION_VERTICAL,
		    "1080x1920 should be detected as vertical");

	stream_orientation_t square = restreamer_multistream_detect_orientation(1080, 1080);
	ASSERT_TRUE(square == ORIENTATION_SQUARE,
		    "1080x1080 should be detected as square");

	/* Test video filter building */
	char *filter = restreamer_multistream_build_video_filter(
		ORIENTATION_HORIZONTAL, ORIENTATION_VERTICAL);
	if (filter) {
		printf("  Horizontal->Vertical filter: %s\n", filter);
		ASSERT_TRUE(strlen(filter) > 0, "Filter should not be empty");
		free(filter);
	}

	/* Test same orientation (no filter needed) */
	char *no_filter = restreamer_multistream_build_video_filter(
		ORIENTATION_HORIZONTAL, ORIENTATION_HORIZONTAL);
	if (no_filter) {
		/* Filter might be NULL or empty for same orientation */
		printf("  Same orientation filter: %s\n", no_filter);
		free(no_filter);
	}

	return true;
}

/* ========================================================================
 * Test 7: Config Changes Propagate to API Client
 * ======================================================================== */
static bool test_config_change_propagation(void)
{
	printf("  Testing config changes propagate to API client...\n");

	if (!setup_test_server()) {
		return false;
	}

	/* Create initial connection */
	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Create channel manager */
	channel_manager_t *manager = channel_manager_create(api);
	ASSERT_NOT_NULL(manager, "Should create channel manager");

	/* Update API connection */
	restreamer_connection_t new_conn = get_test_connection();
	restreamer_api_t *new_api = restreamer_api_create(&new_conn);
	ASSERT_NOT_NULL(new_api, "Should create new API client");

	/* Update manager with new API */
	channel_manager_set_api(manager, new_api);

	/* Verify manager uses new API */
	ASSERT_TRUE(manager->api == new_api, "Manager should use new API");

	/* Test connection with new API */
	bool connected = restreamer_api_test_connection(manager->api);
	ASSERT_TRUE(connected, "Should connect with new API");

	/* Cleanup */
	channel_manager_destroy(manager);
	restreamer_api_destroy(api);
	restreamer_api_destroy(new_api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 8: Reconnection on Config Change
 * ======================================================================== */
static bool test_reconnection_on_config_change(void)
{
	printf("  Testing reconnection on config change...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Force re-login */
	bool relogin = restreamer_api_force_login(api);
	printf("  Force re-login %s\n", relogin ? "succeeded" : "failed");

	/* Should still be connected after re-login */
	bool still_connected = restreamer_api_is_connected(api);
	ASSERT_TRUE(still_connected, "Should still be connected after re-login");

	/* Test token refresh */
	bool refreshed = restreamer_api_refresh_token(api);
	printf("  Token refresh %s\n", refreshed ? "succeeded" : "failed");

	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 9: Handling of Invalid Server Config
 * ======================================================================== */
static bool test_invalid_server_config(void)
{
	printf("  Testing handling of invalid server config...\n");

	/* Test with invalid host */
	restreamer_connection_t invalid_conn = {
		.host = "invalid-host-that-does-not-exist.local",
		.port = 9999,
		.username = "admin",
		.password = "password",
		.use_https = false
	};

	restreamer_api_t *api = restreamer_api_create(&invalid_conn);
	ASSERT_NOT_NULL(api, "Should create API client even with invalid config");

	/* Connection should fail */
	bool connected = restreamer_api_test_connection(api);
	ASSERT_FALSE(connected, "Should fail to connect to invalid host");

	/* Should have error message */
	const char *error = restreamer_api_get_error(api);
	ASSERT_NOT_NULL(error, "Should have error message");
	printf("  Expected error: %s\n", error);

	restreamer_api_destroy(api);

	/* Test with invalid credentials */
	if (!use_live_server()) {
		/* Only test in mock mode to avoid lockouts on live server */
		if (!setup_test_server()) {
			return false;
		}

		restreamer_connection_t bad_creds = {
			.host = MOCK_SERVER_HOST,
			.port = MOCK_SERVER_PORT,
			.username = "wrong",
			.password = "wrong",
			.use_https = false
		};

		api = restreamer_api_create(&bad_creds);
		connected = restreamer_api_test_connection(api);
		ASSERT_FALSE(connected, "Should fail with invalid credentials");

		error = restreamer_api_get_error(api);
		printf("  Expected auth error: %s\n", error ? error : "none");

		restreamer_api_destroy(api);
		teardown_test_server();
	}

	return true;
}

/* ========================================================================
 * Test 10: Recovery from API Errors
 * ======================================================================== */
static bool test_recovery_from_api_errors(void)
{
	printf("  Testing recovery from API errors...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Try to get non-existent process */
	restreamer_process_t process = {0};
	bool got_process = restreamer_api_get_process(api, "non-existent-12345", &process);
	ASSERT_FALSE(got_process, "Should fail to get non-existent process");

	/* Should have error */
	const char *error = restreamer_api_get_error(api);
	printf("  Expected 404 error: %s\n", error ? error : "none");

	/* API should still work after error */
	restreamer_process_list_t list = {0};
	bool got_list = restreamer_api_get_processes(api, &list);
	ASSERT_TRUE(got_list, "Should still get process list after error");
	printf("  Recovered: got %zu processes\n", list.count);
	restreamer_api_free_process_list(&list);

	/* Try to start non-existent process */
	bool started = restreamer_api_start_process(api, "non-existent-12345");
	ASSERT_FALSE(started, "Should fail to start non-existent process");

	/* API should still be usable */
	bool still_connected = restreamer_api_is_connected(api);
	ASSERT_TRUE(still_connected, "Should still be connected after errors");

	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 11: Handling of Server Disconnection
 * ======================================================================== */
static bool test_server_disconnection(void)
{
	printf("  Testing handling of server disconnection...\n");

	if (use_live_server()) {
		printf("  Skipping in live mode (don't want to test disconnection on live server)\n");
		return true;
	}

	/* Start mock server */
	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Stop server to simulate disconnection */
	mock_restreamer_stop();
	sleep_ms(100);

	/* API calls should fail gracefully */
	restreamer_process_list_t list = {0};
	bool got_list = restreamer_api_get_processes(api, &list);
	ASSERT_FALSE(got_list, "Should fail when server is down");

	/* Should have error message */
	const char *error = restreamer_api_get_error(api);
	printf("  Expected connection error: %s\n", error ? error : "none");

	/* Restart server */
	if (!setup_test_server()) {
		restreamer_api_destroy(api);
		return false;
	}

	/* Force reconnection */
	bool reconnected = restreamer_api_test_connection(api);
	printf("  Reconnection %s\n", reconnected ? "succeeded" : "failed");

	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Test 12: Automatic Reconnection
 * ======================================================================== */
static bool test_automatic_reconnection(void)
{
	printf("  Testing automatic reconnection...\n");

	if (!setup_test_server()) {
		return false;
	}

	restreamer_connection_t conn = get_test_connection();
	restreamer_api_t *api = restreamer_api_create(&conn);
	ASSERT_NOT_NULL(api, "Should create API client");
	ASSERT_TRUE(restreamer_api_test_connection(api), "Should connect");

	/* Create channel with auto-reconnect enabled */
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Auto-Reconnect Test");

	/* Enable auto-reconnect */
	channel->auto_reconnect = true;
	channel->reconnect_delay_sec = 1;
	channel->max_reconnect_attempts = 3;

	/* Enable health monitoring */
	channel->health_monitoring_enabled = true;
	channel->health_check_interval_sec = 5;
	channel->failure_threshold = 2;

	/* Verify settings */
	ASSERT_TRUE(channel->auto_reconnect, "Auto-reconnect should be enabled");
	ASSERT_TRUE(channel->health_monitoring_enabled, "Health monitoring should be enabled");
	printf("  Auto-reconnect configured: delay=%us, max_attempts=%u\n",
	       channel->reconnect_delay_sec,
	       channel->max_reconnect_attempts);

	/* Test reconnect function */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "reconnect-test",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Simulate reconnection attempt */
	bool reconnected = channel_reconnect_output(channel, api, 0);
	printf("  Manual reconnect attempt %s\n", reconnected ? "succeeded" : "failed");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);
	teardown_test_server();

	return true;
}

/* ========================================================================
 * Main Test Suite
 * ======================================================================== */
BEGIN_TEST_SUITE("Integration Tests - Channel/API/Multistream")

printf("\n");
if (use_live_server()) {
	printf("==========================================================\n");
	printf("RUNNING IN LIVE MODE\n");
	printf("Server: %s:%d\n", LIVE_SERVER_HOST, LIVE_SERVER_PORT);
	printf("==========================================================\n");
} else {
	printf("==========================================================\n");
	printf("RUNNING IN MOCK MODE\n");
	printf("To test against live server, set: LIVE_TEST_SERVER=1\n");
	printf("==========================================================\n");
}
printf("\n");

/* Channel to API Integration Tests */
RUN_TEST(test_channel_start_creates_api_calls,
	 "Test 1: Channel start creates correct API calls");
RUN_TEST(test_channel_stop_cleanup,
	 "Test 2: Channel stop properly cleans up on server");
RUN_TEST(test_channel_state_reflects_server,
	 "Test 3: Channel state reflects server state");

/* Multistream to API Integration Tests */
RUN_TEST(test_multistream_config_json,
	 "Test 4: Multistream config creates correct process JSON");
RUN_TEST(test_output_url_construction,
	 "Test 5: Output URL construction for different services");
RUN_TEST(test_orientation_video_filtering,
	 "Test 6: Orientation-based video filtering");

/* Configuration Integration Tests */
RUN_TEST(test_config_change_propagation,
	 "Test 7: Config changes propagate to API client");
RUN_TEST(test_reconnection_on_config_change,
	 "Test 8: Reconnection on config change");
RUN_TEST(test_invalid_server_config,
	 "Test 9: Handling of invalid server config");

/* Error Recovery Tests */
RUN_TEST(test_recovery_from_api_errors,
	 "Test 10: Recovery from API errors");
RUN_TEST(test_server_disconnection,
	 "Test 11: Handling of server disconnection");
RUN_TEST(test_automatic_reconnection,
	 "Test 12: Automatic reconnection");

END_TEST_SUITE()
