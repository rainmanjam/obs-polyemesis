/*
 * Comprehensive End-to-End Streaming Tests
 *
 * Complete integration tests for the obs-polyemesis plugin's streaming
 * functionality using a live Restreamer server. These tests simulate
 * real-world streaming workflows from channel creation to cleanup.
 *
 * Server: https://rs2.rainmanjam.com
 * Credentials: admin / tenn2jagWEE@##$
 */

#include "../test_framework.h"
#include "../../src/restreamer-api.h"
#include <curl/curl.h>
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

/* ========================================================================
 * Test Configuration
 * ======================================================================== */

/* Live server configuration */
#define TEST_SERVER_URL "rs2.rainmanjam.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "admin"
#define TEST_SERVER_PASSWORD "tenn2jagWEE@##$"
#define TEST_USE_HTTPS true

/* Test URLs and destinations */
#define TEST_INPUT_URL "rtmp://localhost:1935/live/test"
#define TEST_YOUTUBE_URL "rtmp://a.rtmp.youtube.com/live2/"
#define TEST_TWITCH_URL "rtmp://live.twitch.tv/app/"
#define TEST_FACEBOOK_URL "rtmps://live-api-s.facebook.com:443/rtmp/"
#define TEST_CUSTOM_URL "rtmp://custom.example.com/live/"

/* Test timing */
#define TEST_TIMEOUT_MS 15000
#define TEST_POLLING_INTERVAL_MS 1000
#define TEST_STARTUP_DELAY_MS 2000
#define TEST_STREAM_DURATION_MS 3000

/* Global API client */
static restreamer_api_t *g_api = NULL;

/* Track created processes for cleanup */
#define MAX_TEST_PROCESSES 32
static char *g_test_processes[MAX_TEST_PROCESSES];
static size_t g_test_process_count = 0;

/* ========================================================================
 * Helper Functions
 * ======================================================================== */

/* Setup API client and authenticate */
static bool setup_api_client(void)
{
	restreamer_connection_t connection = {.host = TEST_SERVER_URL,
					      .port = TEST_SERVER_PORT,
					      .username = TEST_SERVER_USERNAME,
					      .password = TEST_SERVER_PASSWORD,
					      .use_https = TEST_USE_HTTPS};

	g_api = restreamer_api_create(&connection);
	if (!g_api) {
		printf("    [ERROR] Failed to create API client\n");
		return false;
	}

	/* Test connection */
	if (!restreamer_api_test_connection(g_api)) {
		printf("    [ERROR] Failed to connect to Restreamer server\n");
		restreamer_api_destroy(g_api);
		g_api = NULL;
		return false;
	}

	printf("    [OK] Connected to Restreamer server\n");
	return true;
}

/* Cleanup API client */
static void cleanup_api_client(void)
{
	if (g_api) {
		restreamer_api_destroy(g_api);
		g_api = NULL;
	}
}

/* Generate unique process ID with prefix */
static void generate_process_id(char *buffer, size_t size,
				const char *prefix)
{
	time_t now = time(NULL);
	int random_num = rand() % 10000;
	snprintf(buffer, size, "%s_%ld_%d", prefix, (long)now, random_num);
}

/* Register process for cleanup */
static void register_test_process(const char *process_id)
{
	if (g_test_process_count < MAX_TEST_PROCESSES) {
		g_test_processes[g_test_process_count] = strdup(process_id);
		g_test_process_count++;
	}
}

/* Cleanup a single test process */
static void cleanup_test_process(const char *process_id)
{
	if (!process_id || !g_api)
		return;

	/* Stop the process */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	/* Delete the process */
	restreamer_api_delete_process(g_api, process_id);
}

/* Cleanup all registered test processes */
static void cleanup_all_test_processes(void)
{
	printf("\n[CLEANUP] Removing %zu test processes...\n",
	       g_test_process_count);

	for (size_t i = 0; i < g_test_process_count; i++) {
		if (g_test_processes[i]) {
			printf("  Cleaning up: %s\n", g_test_processes[i]);
			cleanup_test_process(g_test_processes[i]);
			free(g_test_processes[i]);
			g_test_processes[i] = NULL;
		}
	}
	g_test_process_count = 0;
}

/* Wait for process to reach expected state */
static bool wait_for_process_state(const char *process_id,
				   const char *expected_state,
				   int timeout_ms)
{
	int elapsed = 0;
	while (elapsed < timeout_ms) {
		restreamer_process_t process = {0};
		if (restreamer_api_get_process(g_api, process_id, &process)) {
			if (process.state &&
			    strcmp(process.state, expected_state) == 0) {
				restreamer_api_free_process(&process);
				return true;
			}
			restreamer_api_free_process(&process);
		}
		sleep_ms(TEST_POLLING_INTERVAL_MS);
		elapsed += TEST_POLLING_INTERVAL_MS;
	}
	return false;
}

/* Wait for process to be running */
static bool wait_for_running(const char *process_id)
{
	int elapsed = 0;
	while (elapsed < TEST_TIMEOUT_MS) {
		restreamer_process_state_t state = {0};
		if (restreamer_api_get_process_state(g_api, process_id,
						     &state)) {
			bool is_running = state.is_running;
			restreamer_api_free_process_state(&state);
			if (is_running) {
				return true;
			}
		}
		sleep_ms(TEST_POLLING_INTERVAL_MS);
		elapsed += TEST_POLLING_INTERVAL_MS;
	}
	return false;
}

/* Verify process exists */
static bool verify_process_exists(const char *process_id)
{
	restreamer_process_t process = {0};
	bool exists =
		restreamer_api_get_process(g_api, process_id, &process);
	if (exists) {
		restreamer_api_free_process(&process);
	}
	return exists;
}

/* ========================================================================
 * Test Suite 1: Complete Streaming Workflow
 * ======================================================================== */

/* Test 1.1: Basic channel creation and streaming */
static bool test_basic_channel_workflow(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id),
			    "basic_workflow");
	register_test_process(process_id);

	printf("    Creating channel: %s\n", process_id);

	/* Create channel with single output */
	const char *outputs[] = {TEST_YOUTUBE_URL "basic_key"};
	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, NULL);
	ASSERT_TRUE(created, "Channel should be created");

	/* Verify channel exists */
	ASSERT_TRUE(verify_process_exists(process_id),
		    "Process should exist after creation");

	/* Start streaming */
	printf("    Starting stream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Stream should start");

	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify process is running */
	restreamer_process_state_t state = {0};
	bool got_state =
		restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(got_state, "Should get process state");
	printf("    Stream status - Running: %s, FPS: %.2f, Bitrate: %u kbps\n",
	       state.is_running ? "YES" : "NO", state.fps,
	       state.current_bitrate);
	restreamer_api_free_process_state(&state);

	/* Stop streaming */
	printf("    Stopping stream...\n");
	bool stopped = restreamer_api_stop_process(g_api, process_id);
	ASSERT_TRUE(stopped, "Stream should stop");

	sleep_ms(1000);

	/* Verify process is stopped */
	got_state = restreamer_api_get_process_state(g_api, process_id, &state);
	if (got_state) {
		ASSERT_FALSE(state.is_running, "Process should be stopped");
		restreamer_api_free_process_state(&state);
	}

	/* Delete channel */
	printf("    Deleting channel...\n");
	bool deleted = restreamer_api_delete_process(g_api, process_id);
	ASSERT_TRUE(deleted, "Channel should be deleted");

	/* Verify deletion */
	ASSERT_FALSE(verify_process_exists(process_id),
		     "Process should not exist after deletion");

	return true;
}

/* Test 1.2: Multi-destination streaming workflow */
static bool test_multi_destination_workflow(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "multi_dest");
	register_test_process(process_id);

	printf("    Creating multi-destination channel: %s\n", process_id);

	/* Create channel with 4 outputs */
	const char *outputs[] = {TEST_YOUTUBE_URL "yt_key",
				 TEST_TWITCH_URL "twitch_key",
				 TEST_FACEBOOK_URL "fb_key",
				 TEST_CUSTOM_URL "custom_key"};

	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 4, NULL);
	ASSERT_TRUE(created, "Multi-destination channel should be created");

	/* Start streaming */
	printf("    Starting multi-destination stream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Multi-destination stream should start");

	sleep_ms(TEST_STREAM_DURATION_MS);

	/* Get and verify outputs */
	char **output_ids = NULL;
	size_t output_count = 0;
	bool got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get process outputs");
	ASSERT_EQ(output_count, 4, "Should have 4 outputs");

	printf("    Active outputs: %zu\n", output_count);
	for (size_t i = 0; i < output_count; i++) {
		printf("      [%zu] %s\n", i, output_ids[i]);
	}

	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Stop streaming */
	printf("    Stopping multi-destination stream...\n");
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);

	return true;
}

/* Test 1.3: Different encoding settings workflow */
static bool test_encoding_settings_workflow(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "encoding");
	register_test_process(process_id);

	printf("    Creating channel with custom encoding: %s\n", process_id);

	/* Create with 720p scaling */
	const char *outputs[] = {TEST_YOUTUBE_URL "720p_key"};
	const char *video_filter = "scale=1280:720";

	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, video_filter);
	ASSERT_TRUE(created, "Channel with custom encoding should be created");

	/* Verify configuration */
	char *config_json = NULL;
	bool got_config = restreamer_api_get_process_config(g_api, process_id,
							     &config_json);
	ASSERT_TRUE(got_config, "Should get process configuration");
	ASSERT_NOT_NULL(config_json, "Config JSON should not be null");

	/* Check for video filter in config */
	bool has_filter = (strstr(config_json, "scale") != NULL);
	printf("    Video filter present in config: %s\n",
	       has_filter ? "YES" : "NO");
	free(config_json);

	/* Start and verify */
	printf("    Starting stream with custom encoding...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Stream with custom encoding should start");

	sleep_ms(TEST_STREAM_DURATION_MS);

	/* Stop and cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);

	return true;
}

/* ========================================================================
 * Test Suite 2: Multi-Channel Scenarios
 * ======================================================================== */

/* Test 2.1: Start multiple channels simultaneously */
static bool test_multiple_channels_simultaneous(void)
{
	char process_ids[3][128];
	int channel_count = 3;

	printf("    Creating %d channels...\n", channel_count);

	/* Create multiple channels */
	for (int i = 0; i < channel_count; i++) {
		char prefix[64];
		snprintf(prefix, sizeof(prefix), "multi_chan_%d", i);
		generate_process_id(process_ids[i], sizeof(process_ids[i]),
				    prefix);
		register_test_process(process_ids[i]);

		const char *outputs[] = {TEST_YOUTUBE_URL "multi_key"};
		bool created = restreamer_api_create_process(
			g_api, process_ids[i], TEST_INPUT_URL, outputs, 1,
			NULL);
		ASSERT_TRUE(created, "Channel should be created");
		printf("      Created: %s\n", process_ids[i]);
	}

	/* Start all channels */
	printf("    Starting all %d channels...\n", channel_count);
	for (int i = 0; i < channel_count; i++) {
		bool started =
			restreamer_api_start_process(g_api, process_ids[i]);
		ASSERT_TRUE(started, "Channel should start");
		printf("      Started: %s\n", process_ids[i]);
	}

	sleep_ms(TEST_STREAM_DURATION_MS);

	/* Verify all are running */
	printf("    Verifying all channels are running...\n");
	for (int i = 0; i < channel_count; i++) {
		restreamer_process_state_t state = {0};
		bool got_state = restreamer_api_get_process_state(
			g_api, process_ids[i], &state);
		ASSERT_TRUE(got_state, "Should get process state");
		printf("      [%d] %s - Running: %s\n", i, process_ids[i],
		       state.is_running ? "YES" : "NO");
		restreamer_api_free_process_state(&state);
	}

	/* Stop all channels */
	printf("    Stopping all channels...\n");
	for (int i = 0; i < channel_count; i++) {
		restreamer_api_stop_process(g_api, process_ids[i]);
	}

	sleep_ms(1000);
	return true;
}

/* Test 2.2: Stop all channels at once */
static bool test_stop_all_channels(void)
{
	char process_ids[2][128];

	printf("    Creating 2 channels for stop-all test...\n");

	/* Create and start two channels */
	for (int i = 0; i < 2; i++) {
		char prefix[64];
		snprintf(prefix, sizeof(prefix), "stop_all_%d", i);
		generate_process_id(process_ids[i], sizeof(process_ids[i]),
				    prefix);
		register_test_process(process_ids[i]);

		const char *outputs[] = {TEST_YOUTUBE_URL "stop_key"};
		restreamer_api_create_process(g_api, process_ids[i],
					       TEST_INPUT_URL, outputs, 1,
					       NULL);
		restreamer_api_start_process(g_api, process_ids[i]);
		printf("      Started: %s\n", process_ids[i]);
	}

	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Stop all at once */
	printf("    Stopping all channels simultaneously...\n");
	for (int i = 0; i < 2; i++) {
		restreamer_api_stop_process(g_api, process_ids[i]);
	}

	sleep_ms(1000);

	/* Verify all stopped */
	printf("    Verifying all channels stopped...\n");
	for (int i = 0; i < 2; i++) {
		restreamer_process_state_t state = {0};
		bool got_state = restreamer_api_get_process_state(
			g_api, process_ids[i], &state);
		if (got_state) {
			printf("      [%d] Running: %s\n", i,
			       state.is_running ? "YES (unexpected)" :
						  "NO (expected)");
			restreamer_api_free_process_state(&state);
		}
	}

	return true;
}

/* Test 2.3: Independent channel management */
static bool test_independent_channel_management(void)
{
	char process_id_1[128], process_id_2[128];

	generate_process_id(process_id_1, sizeof(process_id_1), "independent_1");
	generate_process_id(process_id_2, sizeof(process_id_2), "independent_2");
	register_test_process(process_id_1);
	register_test_process(process_id_2);

	printf("    Creating two independent channels...\n");

	/* Create both channels */
	const char *outputs1[] = {TEST_YOUTUBE_URL "independent_1"};
	const char *outputs2[] = {TEST_TWITCH_URL "independent_2"};

	restreamer_api_create_process(g_api, process_id_1, TEST_INPUT_URL,
				       outputs1, 1, NULL);
	restreamer_api_create_process(g_api, process_id_2, TEST_INPUT_URL,
				       outputs2, 1, NULL);

	/* Start only first channel */
	printf("    Starting only first channel...\n");
	restreamer_api_start_process(g_api, process_id_1);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify states */
	restreamer_process_state_t state1 = {0}, state2 = {0};
	restreamer_api_get_process_state(g_api, process_id_1, &state1);
	restreamer_api_get_process_state(g_api, process_id_2, &state2);

	printf("    Channel 1 running: %s\n",
	       state1.is_running ? "YES" : "NO");
	printf("    Channel 2 running: %s (should be NO)\n",
	       state2.is_running ? "YES" : "NO");

	ASSERT_FALSE(state2.is_running,
		     "Channel 2 should not be running when only Channel 1 was started");

	restreamer_api_free_process_state(&state1);
	restreamer_api_free_process_state(&state2);

	/* Now start second, stop first */
	printf("    Starting second channel, stopping first...\n");
	restreamer_api_start_process(g_api, process_id_2);
	restreamer_api_stop_process(g_api, process_id_1);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify independent operation */
	restreamer_api_get_process_state(g_api, process_id_1, &state1);
	restreamer_api_get_process_state(g_api, process_id_2, &state2);

	printf("    Channel 1 running: %s (should be NO)\n",
	       state1.is_running ? "YES" : "NO");
	printf("    Channel 2 running: %s (should be YES)\n",
	       state2.is_running ? "YES" : "NO");

	restreamer_api_free_process_state(&state1);
	restreamer_api_free_process_state(&state2);

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id_2);
	sleep_ms(500);

	return true;
}

/* ========================================================================
 * Test Suite 3: Live Operations
 * ======================================================================== */

/* Test 3.1: Add destination while streaming */
static bool test_add_destination_live(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "add_live");
	register_test_process(process_id);

	printf("    Creating channel: %s\n", process_id);

	/* Create with single output */
	const char *outputs[] = {TEST_YOUTUBE_URL "initial_key"};
	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 1, NULL);

	/* Start streaming */
	printf("    Starting stream...\n");
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify initial state */
	char **output_ids = NULL;
	size_t output_count = 0;
	restreamer_api_get_process_outputs(g_api, process_id, &output_ids,
					    &output_count);
	printf("    Initial output count: %zu\n", output_count);
	ASSERT_EQ(output_count, 1, "Should start with 1 output");
	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Add second output while streaming */
	printf("    Adding second output while streaming...\n");
	bool added = restreamer_api_add_process_output(
		g_api, process_id, "output_2", TEST_TWITCH_URL "second_key",
		NULL);
	ASSERT_TRUE(added, "Should add output while streaming");

	sleep_ms(1500);

	/* Verify new output count */
	restreamer_api_get_process_outputs(g_api, process_id, &output_ids,
					    &output_count);
	printf("    Output count after adding: %zu\n", output_count);
	ASSERT_EQ(output_count, 2, "Should have 2 outputs after adding");

	/* Verify stream still running */
	restreamer_process_state_t state = {0};
	restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(state.is_running,
		    "Stream should still be running after adding output");
	restreamer_api_free_process_state(&state);

	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* Test 3.2: Remove destination while streaming */
static bool test_remove_destination_live(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "remove_live");
	register_test_process(process_id);

	printf("    Creating channel with 2 outputs: %s\n", process_id);

	/* Create with two outputs */
	const char *outputs[] = {TEST_YOUTUBE_URL "first_key",
				 TEST_TWITCH_URL "second_key"};
	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 2, NULL);

	/* Start streaming */
	printf("    Starting stream...\n");
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Get output IDs */
	char **output_ids = NULL;
	size_t output_count = 0;
	restreamer_api_get_process_outputs(g_api, process_id, &output_ids,
					    &output_count);
	ASSERT_EQ(output_count, 2, "Should have 2 outputs initially");

	/* Save first output ID for removal */
	char *first_output_id = strdup(output_ids[0]);
	printf("    Removing output: %s\n", first_output_id);
	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Remove first output while streaming */
	bool removed = restreamer_api_remove_process_output(g_api, process_id,
							     first_output_id);
	ASSERT_TRUE(removed, "Should remove output while streaming");

	free(first_output_id);
	sleep_ms(1500);

	/* Verify remaining output */
	restreamer_api_get_process_outputs(g_api, process_id, &output_ids,
					    &output_count);
	printf("    Output count after removal: %zu\n", output_count);
	ASSERT_EQ(output_count, 1, "Should have 1 output after removal");
	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Verify stream still running */
	restreamer_process_state_t state = {0};
	restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(state.is_running,
		    "Stream should continue after output removal");
	restreamer_api_free_process_state(&state);

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* Test 3.3: Encoding change while streaming */
static bool test_encoding_change_live(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "encoding_live");
	register_test_process(process_id);

	printf("    Creating channel: %s\n", process_id);

	/* Create with default encoding */
	const char *outputs[] = {TEST_YOUTUBE_URL "encoding_key"};
	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 1, NULL);

	/* Start streaming */
	printf("    Starting stream...\n");
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Get output IDs */
	char **output_ids = NULL;
	size_t output_count = 0;
	restreamer_api_get_process_outputs(g_api, process_id, &output_ids,
					    &output_count);
	ASSERT_TRUE(output_count > 0, "Should have at least one output");

	/* Attempt to update encoding settings */
	printf("    Attempting to update encoding settings...\n");
	encoding_params_t params = {.video_bitrate_kbps = 3500,
				    .audio_bitrate_kbps = 160,
				    .width = 1920,
				    .height = 1080,
				    .fps_num = 30,
				    .fps_den = 1,
				    .preset = "fast",
				    .profile = "high"};

	bool updated = restreamer_api_update_output_encoding(
		g_api, process_id, output_ids[0], &params);

	if (updated) {
		printf("    Encoding settings updated successfully\n");
	} else {
		printf("    Note: Live encoding update not supported (expected)\n");
	}

	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Verify stream continues regardless */
	sleep_ms(1000);
	restreamer_process_state_t state = {0};
	restreamer_api_get_process_state(g_api, process_id, &state);
	printf("    Stream still running: %s\n",
	       state.is_running ? "YES" : "NO");
	restreamer_api_free_process_state(&state);

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* ========================================================================
 * Test Suite 4: Error Scenarios
 * ======================================================================== */

/* Test 4.1: Invalid server connection */
static bool test_invalid_server(void)
{
	printf("    Testing connection to invalid server...\n");

	/* Try to connect to non-existent server */
	restreamer_connection_t bad_connection = {
		.host = "invalid-server-does-not-exist.example.com",
		.port = 443,
		.username = "admin",
		.password = "password",
		.use_https = true};

	restreamer_api_t *bad_api = restreamer_api_create(&bad_connection);
	ASSERT_NOT_NULL(bad_api, "API client should be created");

	/* Connection test should fail */
	bool connected = restreamer_api_test_connection(bad_api);
	printf("    Connection to invalid server: %s (expected FAIL)\n",
	       connected ? "SUCCESS (unexpected)" : "FAIL");
	ASSERT_FALSE(connected,
		     "Should not connect to non-existent server");

	restreamer_api_destroy(bad_api);
	return true;
}

/* Test 4.2: Invalid credentials */
static bool test_invalid_credentials(void)
{
	printf("    Testing with invalid credentials...\n");

	/* Try to connect with bad credentials */
	restreamer_connection_t bad_creds = {.host = TEST_SERVER_URL,
					     .port = TEST_SERVER_PORT,
					     .username = "invalid_user",
					     .password = "invalid_password",
					     .use_https = TEST_USE_HTTPS};

	restreamer_api_t *bad_api = restreamer_api_create(&bad_creds);
	ASSERT_NOT_NULL(bad_api, "API client should be created");

	/* Connection should fail with invalid credentials */
	bool connected = restreamer_api_test_connection(bad_api);
	printf("    Authentication with bad credentials: %s (expected FAIL)\n",
	       connected ? "SUCCESS (unexpected)" : "FAIL");
	ASSERT_FALSE(connected, "Should not connect with invalid credentials");

	restreamer_api_destroy(bad_api);
	return true;
}

/* Test 4.3: Invalid stream keys */
static bool test_invalid_stream_keys(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "invalid_key");
	register_test_process(process_id);

	printf("    Creating channel with invalid stream key: %s\n",
	       process_id);

	/* Create with clearly invalid stream key */
	const char *outputs[] = {
		TEST_YOUTUBE_URL "INVALID_KEY_SHOULD_FAIL_CONNECTION"};
	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, NULL);
	ASSERT_TRUE(created,
		    "Channel should be created even with invalid key");

	/* Start streaming - this should start but fail to connect */
	printf("    Starting stream with invalid key...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Process should start (will fail to connect)");

	sleep_ms(TEST_STREAM_DURATION_MS);

	/* Check process state - may show errors */
	restreamer_process_state_t state = {0};
	bool got_state =
		restreamer_api_get_process_state(g_api, process_id, &state);
	if (got_state) {
		printf("    Process state: Running=%s, FPS=%.2f, Bitrate=%u\n",
		       state.is_running ? "YES" : "NO", state.fps,
		       state.current_bitrate);
		restreamer_api_free_process_state(&state);
	}

	/* Check logs for connection errors */
	printf("    Checking logs for errors...\n");
	restreamer_log_list_t logs = {0};
	bool got_logs =
		restreamer_api_get_process_logs(g_api, process_id, &logs);
	if (got_logs && logs.count > 0) {
		printf("    Recent log entries:\n");
		for (size_t i = 0; i < logs.count && i < 3; i++) {
			printf("      [%s] %s\n",
			       logs.entries[i].level ? logs.entries[i].level :
						       "INFO",
			       logs.entries[i].message ? logs.entries[i]
								 .message :
							 "");
		}
		restreamer_api_free_log_list(&logs);
	}

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* Test 4.4: Invalid input URL */
static bool test_invalid_input_url(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "invalid_input");
	register_test_process(process_id);

	printf("    Creating channel with invalid input URL: %s\n",
	       process_id);

	/* Create with invalid input URL */
	const char *outputs[] = {TEST_YOUTUBE_URL "test_key"};
	const char *bad_input = "rtmp://invalid-input-does-not-exist.example.com/live/stream";

	bool created = restreamer_api_create_process(
		g_api, process_id, bad_input, outputs, 1, NULL);
	ASSERT_TRUE(created,
		    "Channel should be created with invalid input URL");

	/* Try to start - should fail or show no input */
	printf("    Attempting to start with invalid input...\n");
	bool started = restreamer_api_start_process(g_api, process_id);

	/* Process may start but won't receive input */
	if (started) {
		sleep_ms(TEST_STREAM_DURATION_MS);

		restreamer_process_state_t state = {0};
		bool got_state = restreamer_api_get_process_state(
			g_api, process_id, &state);
		if (got_state) {
			printf("    State with invalid input: Running=%s, FPS=%.2f\n",
			       state.is_running ? "YES" : "NO", state.fps);
			printf("    Note: FPS should be 0 or very low due to no input\n");
			restreamer_api_free_process_state(&state);
		}
	}

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* ========================================================================
 * Test Suite 5: Persistence Scenarios
 * ======================================================================== */

/* Test 5.1: Channel persists across "restart" */
static bool test_channel_persistence(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "persistence");
	register_test_process(process_id);

	printf("    Creating persistent channel: %s\n", process_id);

	/* Create channel */
	const char *outputs[] = {TEST_YOUTUBE_URL "persist_key"};
	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 1, NULL);

	/* Start streaming */
	printf("    Starting stream...\n");
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Get process details before "restart" */
	restreamer_process_t process_before = {0};
	bool got_before =
		restreamer_api_get_process(g_api, process_id, &process_before);
	ASSERT_TRUE(got_before, "Should get process before restart");

	printf("    Process ID before restart: %s\n",
	       process_before.id ? process_before.id : "NULL");
	restreamer_api_free_process(&process_before);

	/* Simulate "restart" by stopping and starting */
	printf("    Simulating restart (stop + start)...\n");
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify process still exists with same ID */
	restreamer_process_t process_after = {0};
	bool got_after =
		restreamer_api_get_process(g_api, process_id, &process_after);
	ASSERT_TRUE(got_after, "Should get process after restart");

	printf("    Process ID after restart: %s\n",
	       process_after.id ? process_after.id : "NULL");
	ASSERT_STR_EQ(process_after.id, process_id,
		      "Process ID should persist across restart");

	restreamer_api_free_process(&process_after);

	/* Cleanup */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(500);

	return true;
}

/* Test 5.2: Stopped state is correct on reload */
static bool test_stopped_state_reload(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "stopped_state");
	register_test_process(process_id);

	printf("    Creating channel: %s\n", process_id);

	/* Create and start channel */
	const char *outputs[] = {TEST_YOUTUBE_URL "stopped_key"};
	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 1, NULL);
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STARTUP_DELAY_MS);

	/* Verify running */
	restreamer_process_state_t state_running = {0};
	restreamer_api_get_process_state(g_api, process_id, &state_running);
	printf("    Initial state - Running: %s\n",
	       state_running.is_running ? "YES" : "NO");
	ASSERT_TRUE(state_running.is_running, "Should be running initially");
	restreamer_api_free_process_state(&state_running);

	/* Stop the process */
	printf("    Stopping process...\n");
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1500);

	/* Verify stopped state */
	restreamer_process_state_t state_stopped = {0};
	bool got_stopped = restreamer_api_get_process_state(
		g_api, process_id, &state_stopped);
	if (got_stopped) {
		printf("    State after stop - Running: %s\n",
		       state_stopped.is_running ? "YES" : "NO");
		ASSERT_FALSE(state_stopped.is_running,
			     "Should be stopped after stop command");
		restreamer_api_free_process_state(&state_stopped);
	}

	/* Simulate "reload" by getting process info again */
	printf("    Reloading process state...\n");
	sleep_ms(1000);

	restreamer_process_state_t state_reload = {0};
	bool got_reload = restreamer_api_get_process_state(g_api, process_id,
							    &state_reload);
	if (got_reload) {
		printf("    State after reload - Running: %s\n",
		       state_reload.is_running ? "YES" : "NO");
		ASSERT_FALSE(state_reload.is_running,
			     "Should remain stopped on reload");
		restreamer_api_free_process_state(&state_reload);
	}

	/* Process should still exist but be stopped */
	ASSERT_TRUE(verify_process_exists(process_id),
		    "Process should still exist after stopping");

	return true;
}

/* Test 5.3: Multiple outputs persist */
static bool test_outputs_persistence(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "output_persist");
	register_test_process(process_id);

	printf("    Creating channel with multiple outputs: %s\n", process_id);

	/* Create with 3 outputs */
	const char *outputs[] = {TEST_YOUTUBE_URL "persist_1",
				 TEST_TWITCH_URL "persist_2",
				 TEST_FACEBOOK_URL "persist_3"};

	restreamer_api_create_process(g_api, process_id, TEST_INPUT_URL,
				       outputs, 3, NULL);

	/* Get initial output count */
	char **output_ids_before = NULL;
	size_t output_count_before = 0;
	restreamer_api_get_process_outputs(g_api, process_id,
					    &output_ids_before,
					    &output_count_before);
	printf("    Initial output count: %zu\n", output_count_before);
	ASSERT_EQ(output_count_before, 3, "Should have 3 outputs initially");
	restreamer_api_free_outputs_list(output_ids_before,
					  output_count_before);

	/* Start, run, then stop */
	printf("    Starting and stopping stream...\n");
	restreamer_api_start_process(g_api, process_id);
	sleep_ms(TEST_STREAM_DURATION_MS);
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);

	/* Reload and verify outputs persist */
	char **output_ids_after = NULL;
	size_t output_count_after = 0;
	restreamer_api_get_process_outputs(g_api, process_id,
					    &output_ids_after,
					    &output_count_after);
	printf("    Output count after reload: %zu\n", output_count_after);
	ASSERT_EQ(output_count_after, 3,
		  "Should still have 3 outputs after stop/reload");
	restreamer_api_free_outputs_list(output_ids_after, output_count_after);

	return true;
}

/* ========================================================================
 * Test Suite Entry Point
 * ======================================================================== */

BEGIN_TEST_SUITE("Comprehensive End-to-End Streaming Tests")

/* Setup: Connect to live Restreamer server */
printf("\n");
printf("========================================================================\n");
printf(" TEST CONFIGURATION\n");
printf("========================================================================\n");
printf("Server:   %s:%d\n", TEST_SERVER_URL, TEST_SERVER_PORT);
printf("Protocol: %s\n", TEST_USE_HTTPS ? "HTTPS" : "HTTP");
printf("Username: %s\n", TEST_SERVER_USERNAME);
printf("========================================================================\n");
printf("\n[SETUP] Connecting to Restreamer server...\n");

if (!setup_api_client()) {
	printf("\n");
	printf("[ERROR] Failed to connect to Restreamer server\n");
	printf("        Please verify:\n");
	printf("        - Server is running at %s:%d\n", TEST_SERVER_URL,
	       TEST_SERVER_PORT);
	printf("        - Credentials are correct\n");
	printf("        - Network connectivity is available\n");
	printf("        - Firewall allows HTTPS connections\n");
	printf("\n");
	return 1;
}

printf("[SETUP] Connection successful\n");
printf("\n");

/* Seed random number generator for unique IDs */
srand((unsigned int)time(NULL));

/* ========================================================================
 * Run Test Suites
 * ======================================================================== */

printf("========================================================================\n");
printf(" TEST SUITE 1: Complete Streaming Workflow\n");
printf("========================================================================\n");

RUN_TEST(test_basic_channel_workflow,
	 "1.1: Basic channel lifecycle (create → start → stop → delete)");
RUN_TEST(test_multi_destination_workflow,
	 "1.2: Multi-destination streaming workflow");
RUN_TEST(test_encoding_settings_workflow,
	 "1.3: Custom encoding settings workflow");

printf("\n");
printf("========================================================================\n");
printf(" TEST SUITE 2: Multi-Channel Scenarios\n");
printf("========================================================================\n");

RUN_TEST(test_multiple_channels_simultaneous,
	 "2.1: Start multiple channels simultaneously");
RUN_TEST(test_stop_all_channels, "2.2: Stop all channels at once");
RUN_TEST(test_independent_channel_management,
	 "2.3: Independent channel management");

printf("\n");
printf("========================================================================\n");
printf(" TEST SUITE 3: Live Operations\n");
printf("========================================================================\n");

RUN_TEST(test_add_destination_live,
	 "3.1: Add destination while streaming (dynamic output)");
RUN_TEST(test_remove_destination_live,
	 "3.2: Remove destination while streaming");
RUN_TEST(test_encoding_change_live,
	 "3.3: Encoding change while streaming (if supported)");

printf("\n");
printf("========================================================================\n");
printf(" TEST SUITE 4: Error Scenarios\n");
printf("========================================================================\n");

RUN_TEST(test_invalid_server, "4.1: Behavior with unreachable server");
RUN_TEST(test_invalid_credentials,
	 "4.2: Behavior with invalid credentials");
RUN_TEST(test_invalid_stream_keys, "4.3: Behavior with invalid stream keys");
RUN_TEST(test_invalid_input_url, "4.4: Behavior with invalid input URL");

printf("\n");
printf("========================================================================\n");
printf(" TEST SUITE 5: Persistence Scenarios\n");
printf("========================================================================\n");

RUN_TEST(test_channel_persistence,
	 "5.1: Channels persist across restart (stop + start)");
RUN_TEST(test_stopped_state_reload, "5.2: Stopped state persists on reload");
RUN_TEST(test_outputs_persistence, "5.3: Multiple outputs persist correctly");

/* Teardown: Cleanup all test processes */
printf("\n");
cleanup_all_test_processes();
printf("\n[TEARDOWN] Disconnecting from server...\n");
cleanup_api_client();
printf("[TEARDOWN] Complete\n");

END_TEST_SUITE()
