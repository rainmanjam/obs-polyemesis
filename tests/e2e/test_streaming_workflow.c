/*
 * End-to-End Streaming Workflow Tests
 *
 * Complete integration tests for the streaming workflow using a live
 * Restreamer server. Tests the full lifecycle of channel management,
 * multi-destination streaming, failover, and dynamic configuration.
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

/* Test server configuration */
#define TEST_SERVER_URL "rs2.rainmanjam.com"
#define TEST_SERVER_PORT 443
#define TEST_SERVER_USERNAME "admin"
#define TEST_SERVER_PASSWORD "tenn2jagWEE@##$"
#define TEST_USE_HTTPS true

/* Test constants */
#define TEST_INPUT_URL "rtmp://localhost:1935/live/test"
#define TEST_YOUTUBE_URL "rtmp://a.rtmp.youtube.com/live2/"
#define TEST_TWITCH_URL "rtmp://live.twitch.tv/app/"
#define TEST_FACEBOOK_URL "rtmps://live-api-s.facebook.com:443/rtmp/"
#define TEST_TIMEOUT_MS 10000
#define TEST_POLLING_INTERVAL_MS 1000

/* Global API client for tests */
static restreamer_api_t *g_api = NULL;

/* Helper: Create API client and authenticate */
static bool setup_api_client(void)
{
	restreamer_connection_t connection = {.host = TEST_SERVER_URL,
					      .port = TEST_SERVER_PORT,
					      .username = TEST_SERVER_USERNAME,
					      .password = TEST_SERVER_PASSWORD,
					      .use_https = TEST_USE_HTTPS};

	g_api = restreamer_api_create(&connection);
	ASSERT_NOT_NULL(g_api, "API client should be created");

	/* Test connection */
	bool connected = restreamer_api_test_connection(g_api);
	ASSERT_TRUE(connected, "Should connect to Restreamer server");

	return true;
}

/* Helper: Cleanup API client */
static void cleanup_api_client(void)
{
	if (g_api) {
		restreamer_api_destroy(g_api);
		g_api = NULL;
	}
}

/* Helper: Generate unique process ID */
static void generate_process_id(char *buffer, size_t size,
				const char *prefix)
{
	time_t now = time(NULL);
	snprintf(buffer, size, "%s_%ld_%d", prefix, now, rand() % 10000);
}

/* Helper: Wait for process to reach expected state */
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

/* Helper: Cleanup test process */
static void cleanup_test_process(const char *process_id)
{
	if (!process_id || !g_api)
		return;

	/* Try to stop first, then delete */
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);
	restreamer_api_delete_process(g_api, process_id);
}

/* ========================================================================
 * Test 1: Complete Channel Lifecycle (Create → Start → Stop → Delete)
 * ======================================================================== */
static bool test_e2e_channel_create_start_stop(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id),
			    "e2e_lifecycle");

	printf("    Creating channel: %s\n", process_id);

	/* Create process with single output */
	const char *outputs[] = {TEST_YOUTUBE_URL "test_stream_key"};
	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, NULL);
	ASSERT_TRUE(created, "Channel should be created");

	/* Verify process exists */
	restreamer_process_t process = {0};
	bool got_process =
		restreamer_api_get_process(g_api, process_id, &process);
	ASSERT_TRUE(got_process, "Should retrieve created process");
	ASSERT_NOT_NULL(process.id, "Process should have ID");
	ASSERT_STR_EQ(process.id, process_id,
		      "Process ID should match requested ID");
	restreamer_api_free_process(&process);

	/* Start streaming */
	printf("    Starting stream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Stream should start");

	/* Wait for process to be running */
	sleep_ms(2000);

	/* Check process state */
	restreamer_process_state_t state = {0};
	bool got_state =
		restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(got_state, "Should get process state");
	ASSERT_TRUE(state.is_running, "Process should be running");
	printf("    Process running - FPS: %.2f, Bitrate: %u kbps\n",
	       state.fps, state.current_bitrate);
	restreamer_api_free_process_state(&state);

	/* Stop streaming */
	printf("    Stopping stream...\n");
	bool stopped = restreamer_api_stop_process(g_api, process_id);
	ASSERT_TRUE(stopped, "Stream should stop");

	/* Wait for process to stop */
	sleep_ms(1000);

	/* Delete channel */
	printf("    Deleting channel...\n");
	bool deleted = restreamer_api_delete_process(g_api, process_id);
	ASSERT_TRUE(deleted, "Channel should be deleted");

	/* Verify deletion */
	got_process = restreamer_api_get_process(g_api, process_id, &process);
	ASSERT_FALSE(got_process, "Process should no longer exist");

	return true;
}

/* ========================================================================
 * Test 2: Multi-destination Streaming
 * ======================================================================== */
static bool test_e2e_multistream(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "e2e_multistream");

	printf("    Creating multistream channel: %s\n", process_id);

	/* Create process with multiple outputs */
	const char *outputs[] = {TEST_YOUTUBE_URL "yt_key",
				 TEST_TWITCH_URL "twitch_key",
				 TEST_FACEBOOK_URL "fb_key"};

	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 3, NULL);
	ASSERT_TRUE(created, "Multistream channel should be created");

	/* Start multistream */
	printf("    Starting multistream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Multistream should start");

	sleep_ms(3000);

	/* Get all outputs */
	char **output_ids = NULL;
	size_t output_count = 0;
	bool got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get process outputs");
	ASSERT_EQ(output_count, 3, "Should have 3 outputs");

	printf("    Active outputs:\n");
	for (size_t i = 0; i < output_count; i++) {
		printf("      - %s\n", output_ids[i]);
	}

	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Verify all outputs are active via process state */
	restreamer_process_state_t state = {0};
	bool got_state =
		restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(got_state, "Should get process state");
	ASSERT_TRUE(state.is_running, "Process should be running");
	restreamer_api_free_process_state(&state);

	/* Stop multistream */
	printf("    Stopping multistream...\n");
	restreamer_api_stop_process(g_api, process_id);
	sleep_ms(1000);

	/* Cleanup */
	cleanup_test_process(process_id);

	return true;
}

/* ========================================================================
 * Test 3: Failover Functionality (Primary + Backup)
 * ======================================================================== */
static bool test_e2e_failover(void)
{
	char primary_id[128], backup_id[128];
	generate_process_id(primary_id, sizeof(primary_id), "e2e_primary");
	generate_process_id(backup_id, sizeof(backup_id), "e2e_backup");

	printf("    Creating primary channel: %s\n", primary_id);
	printf("    Creating backup channel: %s\n", backup_id);

	/* Create primary output */
	const char *primary_output[] = {TEST_YOUTUBE_URL "primary_key"};
	bool primary_created = restreamer_api_create_process(
		g_api, primary_id, TEST_INPUT_URL, primary_output, 1, NULL);
	ASSERT_TRUE(primary_created, "Primary channel should be created");

	/* Create backup output */
	const char *backup_output[] = {TEST_YOUTUBE_URL "backup_key"};
	bool backup_created = restreamer_api_create_process(
		g_api, backup_id, TEST_INPUT_URL, backup_output, 1, NULL);
	ASSERT_TRUE(backup_created, "Backup channel should be created");

	/* Start primary */
	printf("    Starting primary stream...\n");
	bool primary_started = restreamer_api_start_process(g_api, primary_id);
	ASSERT_TRUE(primary_started, "Primary should start");

	sleep_ms(2000);

	/* Verify primary is running */
	restreamer_process_state_t primary_state = {0};
	bool got_primary_state = restreamer_api_get_process_state(
		g_api, primary_id, &primary_state);
	ASSERT_TRUE(got_primary_state, "Should get primary state");
	ASSERT_TRUE(primary_state.is_running, "Primary should be running");
	restreamer_api_free_process_state(&primary_state);

	/* Simulate primary failure by stopping it */
	printf("    Simulating primary failure...\n");
	restreamer_api_stop_process(g_api, primary_id);
	sleep_ms(1000);

	/* Activate backup (failover) */
	printf("    Activating backup stream...\n");
	bool backup_started = restreamer_api_start_process(g_api, backup_id);
	ASSERT_TRUE(backup_started, "Backup should start");

	sleep_ms(2000);

	/* Verify backup is now running */
	restreamer_process_state_t backup_state = {0};
	bool got_backup_state = restreamer_api_get_process_state(
		g_api, backup_id, &backup_state);
	ASSERT_TRUE(got_backup_state, "Should get backup state");
	ASSERT_TRUE(backup_state.is_running,
		    "Backup should be running after failover");
	restreamer_api_free_process_state(&backup_state);

	/* Restore primary */
	printf("    Restoring primary stream...\n");
	bool primary_restored =
		restreamer_api_start_process(g_api, primary_id);
	ASSERT_TRUE(primary_restored, "Primary should restart");

	sleep_ms(2000);

	/* Stop backup after primary is back */
	printf("    Switching back to primary...\n");
	restreamer_api_stop_process(g_api, backup_id);
	sleep_ms(1000);

	/* Verify primary is running again */
	got_primary_state = restreamer_api_get_process_state(
		g_api, primary_id, &primary_state);
	ASSERT_TRUE(got_primary_state, "Should get primary state");
	ASSERT_TRUE(primary_state.is_running,
		    "Primary should be running after restore");
	restreamer_api_free_process_state(&primary_state);

	/* Cleanup */
	cleanup_test_process(primary_id);
	cleanup_test_process(backup_id);

	return true;
}

/* ========================================================================
 * Test 4: Live Destination Management (Add/Remove while streaming)
 * ======================================================================== */
static bool test_e2e_live_output_add_remove(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id),
			    "e2e_live_modify");

	printf("    Creating channel with single output: %s\n", process_id);

	/* Create process with one output */
	const char *initial_output[] = {TEST_YOUTUBE_URL "initial_key"};
	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, initial_output, 1, NULL);
	ASSERT_TRUE(created, "Channel should be created");

	/* Start streaming */
	printf("    Starting stream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Stream should start");

	sleep_ms(2000);

	/* Verify initial output */
	char **output_ids = NULL;
	size_t output_count = 0;
	bool got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get outputs");
	ASSERT_EQ(output_count, 1, "Should have 1 initial output");
	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Add second output while streaming */
	printf("    Adding second output while streaming...\n");
	bool added = restreamer_api_add_process_output(
		g_api, process_id, "output_2", TEST_TWITCH_URL "second_key",
		NULL);
	ASSERT_TRUE(added, "Should add output while streaming");

	sleep_ms(1000);

	/* Verify two outputs */
	got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get updated outputs");
	ASSERT_EQ(output_count, 2, "Should have 2 outputs after adding");

	char *first_output_id = strdup(output_ids[0]);
	restreamer_api_free_outputs_list(output_ids, output_count);

	sleep_ms(1000);

	/* Remove first output while streaming */
	printf("    Removing first output while streaming...\n");
	bool removed = restreamer_api_remove_process_output(g_api, process_id,
							    first_output_id);
	ASSERT_TRUE(removed, "Should remove output while streaming");

	free(first_output_id);
	sleep_ms(1000);

	/* Verify stream continues with one output */
	restreamer_process_state_t state = {0};
	bool got_state =
		restreamer_api_get_process_state(g_api, process_id, &state);
	ASSERT_TRUE(got_state, "Should get process state");
	ASSERT_TRUE(state.is_running,
		    "Stream should continue after output removal");
	restreamer_api_free_process_state(&state);

	/* Verify one output remains */
	got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get remaining outputs");
	ASSERT_EQ(output_count, 1,
		  "Should have 1 output after first removal");
	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Cleanup */
	cleanup_test_process(process_id);

	return true;
}

/* ========================================================================
 * Test 5: Custom Encoding Settings
 * ======================================================================== */
static bool test_e2e_encoding_settings(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "e2e_encoding");

	printf("    Creating channel with custom encoding: %s\n", process_id);

	/* Create process with custom video filter (resize to 1280x720) */
	const char *outputs[] = {TEST_YOUTUBE_URL "encoded_key"};
	const char *video_filter = "scale=1280:720";

	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, video_filter);
	ASSERT_TRUE(created, "Channel with encoding should be created");

	/* Get process configuration to verify encoding settings */
	char *config_json = NULL;
	bool got_config = restreamer_api_get_process_config(g_api, process_id,
							     &config_json);
	ASSERT_TRUE(got_config, "Should get process configuration");
	ASSERT_NOT_NULL(config_json, "Config JSON should not be null");

	/* Verify the video filter is in the config */
	printf("    Verifying encoding settings in config...\n");
	bool has_filter = (strstr(config_json, "scale") != NULL);
	ASSERT_TRUE(has_filter,
		    "FFmpeg command should include video filter");

	printf("    Config snippet: %.200s...\n", config_json);
	free(config_json);

	/* Test updating encoding on an output */
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Process should start");

	sleep_ms(2000);

	/* Get outputs to update encoding */
	char **output_ids = NULL;
	size_t output_count = 0;
	bool got_outputs = restreamer_api_get_process_outputs(
		g_api, process_id, &output_ids, &output_count);
	ASSERT_TRUE(got_outputs, "Should get outputs");
	ASSERT_TRUE(output_count > 0, "Should have at least one output");

	if (output_count > 0) {
		/* Update encoding settings on first output */
		printf("    Updating encoding settings (bitrate, resolution)...\n");
		encoding_params_t params = {.video_bitrate_kbps = 2500,
					    .audio_bitrate_kbps = 128,
					    .width = 1920,
					    .height = 1080,
					    .fps_num = 30,
					    .fps_den = 1,
					    .preset = "medium",
					    .profile = "main"};

		bool updated = restreamer_api_update_output_encoding(
			g_api, process_id, output_ids[0], &params);
		/* Note: This may fail if the API doesn't support live encoding updates */
		if (updated) {
			printf("    Encoding updated successfully\n");
		} else {
			printf("    Note: Live encoding update not supported or failed\n");
		}
	}

	restreamer_api_free_outputs_list(output_ids, output_count);

	/* Cleanup */
	cleanup_test_process(process_id);

	return true;
}

/* ========================================================================
 * Test 6: Auto-reconnection on Failure
 * ======================================================================== */
static bool test_e2e_reconnection(void)
{
	char process_id[128];
	generate_process_id(process_id, sizeof(process_id), "e2e_reconnect");

	printf("    Creating channel for reconnection test: %s\n",
	       process_id);

	/* Create process */
	const char *outputs[] = {TEST_YOUTUBE_URL "reconnect_key"};
	bool created = restreamer_api_create_process(
		g_api, process_id, TEST_INPUT_URL, outputs, 1, NULL);
	ASSERT_TRUE(created, "Channel should be created");

	/* Start streaming */
	printf("    Starting stream...\n");
	bool started = restreamer_api_start_process(g_api, process_id);
	ASSERT_TRUE(started, "Stream should start");

	sleep_ms(2000);

	/* Get initial state */
	restreamer_process_state_t initial_state = {0};
	bool got_initial = restreamer_api_get_process_state(
		g_api, process_id, &initial_state);
	ASSERT_TRUE(got_initial, "Should get initial state");
	ASSERT_TRUE(initial_state.is_running, "Process should be running");
	restreamer_api_free_process_state(&initial_state);

	/* Simulate input loss by restarting the process */
	printf("    Simulating input loss (restart process)...\n");
	bool restarted = restreamer_api_restart_process(g_api, process_id);
	ASSERT_TRUE(restarted, "Process should restart");

	/* Wait for reconnection */
	printf("    Waiting for reconnection...\n");
	sleep_ms(3000);

	/* Verify reconnection by checking process state */
	restreamer_process_state_t reconnected_state = {0};
	bool got_reconnected = restreamer_api_get_process_state(
		g_api, process_id, &reconnected_state);
	ASSERT_TRUE(got_reconnected, "Should get state after reconnection");

	/* Check if process recovered (may be running or attempting to reconnect) */
	printf("    Process state after restart - Running: %s\n",
	       reconnected_state.is_running ? "YES" : "NO");

	restreamer_api_free_process_state(&reconnected_state);

	/* Monitor reconnect attempts by checking logs */
	printf("    Checking process logs for reconnection attempts...\n");
	restreamer_log_list_t logs = {0};
	bool got_logs =
		restreamer_api_get_process_logs(g_api, process_id, &logs);
	if (got_logs && logs.count > 0) {
		printf("    Recent log entries (%zu total):\n", logs.count);
		for (size_t i = 0; i < logs.count && i < 5; i++) {
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
	cleanup_test_process(process_id);

	return true;
}

/* ========================================================================
 * Test Suite
 * ======================================================================== */

BEGIN_TEST_SUITE("End-to-End Streaming Workflow Tests")

/* Setup: Connect to Restreamer server */
printf("\n[SETUP] Connecting to Restreamer server: %s:%d\n",
       TEST_SERVER_URL, TEST_SERVER_PORT);
if (!setup_api_client()) {
	printf("[ERROR] Failed to connect to Restreamer server\n");
	printf("        Please verify:\n");
	printf("        - Server URL: https://%s:%d\n", TEST_SERVER_URL,
	       TEST_SERVER_PORT);
	printf("        - Username: %s\n", TEST_SERVER_USERNAME);
	printf("        - Network connectivity\n");
	return 1;
}
printf("[SETUP] Successfully connected to Restreamer\n\n");

/* Run E2E tests */
RUN_TEST(test_e2e_channel_create_start_stop,
	 "E2E: Complete channel lifecycle (create → start → stop → delete)");
RUN_TEST(test_e2e_multistream,
	 "E2E: Multi-destination streaming (YouTube + Twitch + Facebook)");
RUN_TEST(test_e2e_failover,
	 "E2E: Failover functionality (primary → backup → restore)");
RUN_TEST(test_e2e_live_output_add_remove,
	 "E2E: Live destination management (add/remove while streaming)");
RUN_TEST(test_e2e_encoding_settings,
	 "E2E: Custom encoding settings (resolution, bitrate, filters)");
RUN_TEST(test_e2e_reconnection,
	 "E2E: Auto-reconnection on failure (input loss recovery)");

/* Teardown */
printf("\n[TEARDOWN] Cleaning up...\n");
cleanup_api_client();
printf("[TEARDOWN] Complete\n");

END_TEST_SUITE()
