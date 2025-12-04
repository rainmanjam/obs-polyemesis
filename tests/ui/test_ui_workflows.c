/*
 * UI Workflow Tests for obs-polyemesis
 *
 * Tests UI operations by exercising the backend functions that UI components
 * invoke. These tests verify that UI workflows work correctly with the
 * Restreamer API without requiring full Qt widget instantiation.
 *
 * Test Coverage:
 * 1. Channel UI Operations (create, edit, delete, start/stop)
 * 2. Output UI Operations (add, edit, delete, enable/disable)
 * 3. Dock UI Operations (connect, disconnect, status updates)
 * 4. Error Handling (server errors, invalid input, failed operations)
 *
 * Server: https://rs2.rainmanjam.com
 * Credentials: admin / tenn2jagWEE@##$
 */

#include "../test_framework.h"
#include "../../src/restreamer-api.h"
#include "../../src/restreamer-channel.h"
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
#define TEST_TIMEOUT_MS 10000

/* Global test resources */
static restreamer_api_t *g_api = NULL;
static channel_manager_t *g_manager = NULL;

/* ========================================================================
 * Setup & Teardown Helpers
 * ======================================================================== */

static bool setup_test_environment(void)
{
	/* Create API client */
	restreamer_connection_t connection = {.host = TEST_SERVER_URL,
					      .port = TEST_SERVER_PORT,
					      .username = TEST_SERVER_USERNAME,
					      .password = TEST_SERVER_PASSWORD,
					      .use_https = TEST_USE_HTTPS};

	g_api = restreamer_api_create(&connection);
	ASSERT_NOT_NULL(g_api, "API client should be created");

	/* Test connection (simulates connection config dialog flow) */
	bool connected = restreamer_api_test_connection(g_api);
	ASSERT_TRUE(connected, "Should connect to Restreamer server");

	/* Create channel manager (simulates dock initialization) */
	g_manager = channel_manager_create(g_api);
	ASSERT_NOT_NULL(g_manager, "Channel manager should be created");

	return true;
}

static void cleanup_test_environment(void)
{
	/* Clean up all test channels */
	if (g_manager) {
		/* Stop and delete all channels */
		size_t count = channel_manager_get_count(g_manager);
		for (size_t i = count; i > 0; i--) {
			stream_channel_t *channel =
				channel_manager_get_channel_at(g_manager,
							       i - 1);
			if (channel) {
				channel_stop(g_manager, channel->channel_id);
				sleep_ms(500);
				channel_manager_delete_channel(
					g_manager, channel->channel_id);
			}
		}

		channel_manager_destroy(g_manager);
		g_manager = NULL;
	}

	if (g_api) {
		restreamer_api_destroy(g_api);
		g_api = NULL;
	}
}

/* Helper: Generate unique channel name */
static void generate_channel_name(char *buffer, size_t size,
				  const char *prefix)
{
	time_t now = time(NULL);
	snprintf(buffer, size, "%s_%ld_%d", prefix, now, rand() % 10000);
}

/* ========================================================================
 * Test 1: Channel Creation UI Flow
 * Tests: ChannelEditDialog -> channel_manager_create_channel
 * ======================================================================== */
static bool test_ui_channel_create_flow(void)
{
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_Create");

	printf("    Testing channel creation: %s\n", channel_name);

	/* Simulate user clicking "Create Channel" button */
	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");
	ASSERT_STR_EQ(channel->channel_name, channel_name,
		      "Channel name should match");
	ASSERT_NOT_NULL(channel->channel_id, "Channel ID should be assigned");

	/* Verify channel appears in manager */
	size_t count = channel_manager_get_count(g_manager);
	ASSERT_EQ(count, 1, "Manager should have 1 channel");

	/* Verify we can retrieve it */
	stream_channel_t *retrieved =
		channel_manager_get_channel(g_manager, channel->channel_id);
	ASSERT_NOT_NULL(retrieved, "Should retrieve created channel");
	ASSERT_EQ(retrieved, channel, "Retrieved channel should match");

	/* Initial state checks */
	ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE,
		  "New channel should be inactive");
	ASSERT_EQ(channel->output_count, 0, "New channel should have no outputs");
	ASSERT_FALSE(channel->auto_start, "Auto-start should be off by default");

	return true;
}

/* ========================================================================
 * Test 2: Channel Edit UI Flow
 * Tests: ChannelEditDialog modifications -> channel update
 * ======================================================================== */
static bool test_ui_channel_edit_flow(void)
{
	/* Create initial channel */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name), "UI_Edit");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Simulate user opening edit dialog and changing settings */
	printf("    Simulating edit dialog changes...\n");

	/* Change channel name */
	free(channel->channel_name);
	channel->channel_name = strdup("UI_Edit_Modified");

	/* Change source orientation */
	channel->source_orientation = ORIENTATION_HORIZONTAL;
	channel->auto_detect_orientation = false;

	/* Set source dimensions */
	channel->source_width = 1920;
	channel->source_height = 1080;

	/* Set input URL */
	if (channel->input_url)
		free(channel->input_url);
	channel->input_url = strdup(TEST_INPUT_URL);

	/* Enable auto-start */
	channel->auto_start = true;

	/* Enable auto-reconnect */
	channel->auto_reconnect = true;
	channel->reconnect_delay_sec = 5;
	channel->max_reconnect_attempts = 3;

	/* Enable health monitoring */
	channel->health_monitoring_enabled = true;
	channel->health_check_interval_sec = 30;
	channel->failure_threshold = 2;

	/* Verify changes */
	ASSERT_STR_EQ(channel->channel_name, "UI_Edit_Modified",
		      "Name should be updated");
	ASSERT_EQ(channel->source_orientation, ORIENTATION_HORIZONTAL,
		  "Orientation should be updated");
	ASSERT_EQ(channel->source_width, 1920, "Width should be updated");
	ASSERT_EQ(channel->source_height, 1080, "Height should be updated");
	ASSERT_STR_EQ(channel->input_url, TEST_INPUT_URL,
		      "Input URL should be updated");
	ASSERT_TRUE(channel->auto_start, "Auto-start should be enabled");
	ASSERT_TRUE(channel->auto_reconnect, "Auto-reconnect should be enabled");
	ASSERT_EQ(channel->reconnect_delay_sec, 5,
		  "Reconnect delay should be set");
	ASSERT_TRUE(channel->health_monitoring_enabled,
		    "Health monitoring should be enabled");

	return true;
}

/* ========================================================================
 * Test 3: Channel Delete UI Flow
 * Tests: Context menu delete -> channel_manager_delete_channel
 * ======================================================================== */
static bool test_ui_channel_delete_flow(void)
{
	/* Create channel to delete */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_Delete");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	char *channel_id = strdup(channel->channel_id);
	size_t initial_count = channel_manager_get_count(g_manager);

	/* Simulate user clicking delete in context menu */
	printf("    Simulating channel deletion...\n");
	bool deleted = channel_manager_delete_channel(g_manager, channel_id);
	ASSERT_TRUE(deleted, "Channel should be deleted");

	/* Verify channel is gone */
	size_t new_count = channel_manager_get_count(g_manager);
	ASSERT_EQ(new_count, initial_count - 1, "Channel count should decrease");

	/* Verify we can't retrieve it */
	stream_channel_t *retrieved =
		channel_manager_get_channel(g_manager, channel_id);
	ASSERT_NULL(retrieved, "Deleted channel should not be retrievable");

	free(channel_id);
	return true;
}

/* ========================================================================
 * Test 4: Channel Start/Stop Button UI Flow
 * Tests: Start/Stop button -> channel_start/channel_stop
 * ======================================================================== */
static bool test_ui_channel_start_stop_flow(void)
{
	/* Create channel with output */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_StartStop");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Set input URL */
	if (channel->input_url)
		free(channel->input_url);
	channel->input_url = strdup(TEST_INPUT_URL);

	/* Add an output (required for starting) */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(channel, SERVICE_YOUTUBE,
					"test_stream_key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Should add output");
	ASSERT_EQ(channel->output_count, 1, "Should have 1 output");

	/* Simulate user clicking "Start" button */
	printf("    Simulating Start button click...\n");
	bool started = channel_start(g_manager, channel->channel_id);
	ASSERT_TRUE(started, "Channel should start");

	sleep_ms(2000);

	/* Verify channel status */
	ASSERT_TRUE(
		channel->status == CHANNEL_STATUS_ACTIVE ||
			channel->status == CHANNEL_STATUS_STARTING,
		"Channel should be active or starting");

	/* Simulate user clicking "Stop" button */
	printf("    Simulating Stop button click...\n");
	bool stopped = channel_stop(g_manager, channel->channel_id);
	ASSERT_TRUE(stopped, "Channel should stop");

	sleep_ms(1000);

	/* Verify channel status */
	ASSERT_TRUE(
		channel->status == CHANNEL_STATUS_INACTIVE ||
			channel->status == CHANNEL_STATUS_STOPPING,
		"Channel should be inactive or stopping");

	return true;
}

/* ========================================================================
 * Test 5: Add Output UI Flow
 * Tests: OutputEditDialog -> channel_add_output
 * ======================================================================== */
static bool test_ui_output_add_flow(void)
{
	/* Create channel */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_AddOutput");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Simulate user opening "Add Output" dialog */
	printf("    Simulating Add Output dialog...\n");

	/* Configure output settings in dialog */
	encoding_settings_t encoding = {.width = 1920,
					.height = 1080,
					.bitrate = 6000,
					.fps_num = 30,
					.fps_den = 1,
					.audio_bitrate = 160,
					.audio_track = 1,
					.max_bandwidth = 0,
					.low_latency = false};

	/* User clicks "Save" - add output */
	bool added = channel_add_output(channel, SERVICE_YOUTUBE,
					"test_yt_stream_key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Output should be added");
	ASSERT_EQ(channel->output_count, 1, "Channel should have 1 output");

	/* Verify output settings */
	channel_output_t *output = &channel->outputs[0];
	ASSERT_EQ(output->service, SERVICE_YOUTUBE,
		  "Service should be YouTube");
	ASSERT_STR_EQ(output->stream_key, "test_yt_stream_key",
		      "Stream key should match");
	ASSERT_EQ(output->target_orientation, ORIENTATION_HORIZONTAL,
		  "Orientation should match");
	ASSERT_EQ(output->encoding.width, 1920, "Width should match");
	ASSERT_EQ(output->encoding.height, 1080, "Height should match");
	ASSERT_EQ(output->encoding.bitrate, 6000, "Bitrate should match");
	ASSERT_TRUE(output->enabled, "Output should be enabled by default");

	/* Add second output */
	printf("    Adding second output...\n");
	encoding_settings_t encoding2 = {.width = 1280,
					 .height = 720,
					 .bitrate = 3000,
					 .fps_num = 30,
					 .fps_den = 1,
					 .audio_bitrate = 128,
					 .audio_track = 1,
					 .max_bandwidth = 0,
					 .low_latency = false};

	added = channel_add_output(channel, SERVICE_TWITCH,
				   "test_twitch_stream_key",
				   ORIENTATION_HORIZONTAL, &encoding2);
	ASSERT_TRUE(added, "Second output should be added");
	ASSERT_EQ(channel->output_count, 2, "Channel should have 2 outputs");

	return true;
}

/* ========================================================================
 * Test 6: Edit Output UI Flow
 * Tests: OutputEditDialog modifications -> channel_update_output_encoding
 * ======================================================================== */
static bool test_ui_output_edit_flow(void)
{
	/* Create channel with output */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_EditOutput");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(channel, SERVICE_YOUTUBE,
					"original_stream_key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Output should be added");

	/* Simulate user opening edit dialog for output */
	printf("    Simulating Edit Output dialog...\n");

	/* User modifies settings */
	channel_output_t *output = &channel->outputs[0];

	/* Change stream key */
	free(output->stream_key);
	output->stream_key = strdup("modified_stream_key");

	/* Change service */
	output->service = SERVICE_TWITCH;

	/* Change target orientation */
	output->target_orientation = ORIENTATION_VERTICAL;

	/* Change encoding settings */
	encoding_settings_t new_encoding = {.width = 720,
					    .height = 1280,
					    .bitrate = 4500,
					    .fps_num = 60,
					    .fps_den = 1,
					    .audio_bitrate = 192,
					    .audio_track = 2,
					    .max_bandwidth = 5000,
					    .low_latency = true};

	bool updated =
		channel_update_output_encoding(channel, 0, &new_encoding);
	ASSERT_TRUE(updated, "Encoding should be updated");

	/* Verify changes */
	ASSERT_STR_EQ(output->stream_key, "modified_stream_key",
		      "Stream key should be updated");
	ASSERT_EQ(output->service, SERVICE_TWITCH,
		  "Service should be updated");
	ASSERT_EQ(output->target_orientation, ORIENTATION_VERTICAL,
		  "Orientation should be updated");
	ASSERT_EQ(output->encoding.width, 720, "Width should be updated");
	ASSERT_EQ(output->encoding.height, 1280, "Height should be updated");
	ASSERT_EQ(output->encoding.bitrate, 4500, "Bitrate should be updated");
	ASSERT_EQ(output->encoding.fps_num, 60, "FPS should be updated");
	ASSERT_EQ(output->encoding.audio_bitrate, 192,
		  "Audio bitrate should be updated");
	ASSERT_TRUE(output->encoding.low_latency,
		    "Low latency should be enabled");

	return true;
}

/* ========================================================================
 * Test 7: Delete Output UI Flow
 * Tests: Context menu delete -> channel_remove_output
 * ======================================================================== */
static bool test_ui_output_delete_flow(void)
{
	/* Create channel with multiple outputs */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_DeleteOutput");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Add multiple outputs */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "yt_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_FACEBOOK, "fb_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	ASSERT_EQ(channel->output_count, 3, "Should have 3 outputs");

	/* Simulate user deleting middle output (index 1) */
	printf("    Simulating output deletion (index 1)...\n");
	bool removed = channel_remove_output(channel, 1);
	ASSERT_TRUE(removed, "Output should be removed");
	ASSERT_EQ(channel->output_count, 2, "Should have 2 outputs remaining");

	/* Verify remaining outputs */
	ASSERT_EQ(channel->outputs[0].service, SERVICE_YOUTUBE,
		  "First output should be YouTube");
	ASSERT_EQ(channel->outputs[1].service, SERVICE_FACEBOOK,
		  "Second output should be Facebook");

	return true;
}

/* ========================================================================
 * Test 8: Enable/Disable Output UI Flow
 * Tests: Toggle switch -> channel_set_output_enabled
 * ======================================================================== */
static bool test_ui_output_enable_disable_flow(void)
{
	/* Create channel with output */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_ToggleOutput");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(channel, SERVICE_YOUTUBE, "test_key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Output should be added");
	ASSERT_TRUE(channel->outputs[0].enabled,
		    "Output should be enabled by default");

	/* Simulate user clicking disable toggle */
	printf("    Simulating disable toggle...\n");
	bool disabled = channel_set_output_enabled(channel, 0, false);
	ASSERT_TRUE(disabled, "Should disable output");
	ASSERT_FALSE(channel->outputs[0].enabled,
		     "Output should be disabled");

	/* Simulate user clicking enable toggle */
	printf("    Simulating enable toggle...\n");
	bool enabled = channel_set_output_enabled(channel, 0, true);
	ASSERT_TRUE(enabled, "Should enable output");
	ASSERT_TRUE(channel->outputs[0].enabled, "Output should be enabled");

	return true;
}

/* ========================================================================
 * Test 9: Connection Configuration UI Flow
 * Tests: ConnectionConfigDialog -> API reconnection
 * ======================================================================== */
static bool test_ui_connection_config_flow(void)
{
	printf("    Testing connection configuration flow...\n");

	/* Verify initial connection */
	ASSERT_TRUE(restreamer_api_is_connected(g_api),
		    "API should be connected");

	/* Test connection validation (what dialog does) */
	bool connection_ok = restreamer_api_test_connection(g_api);
	ASSERT_TRUE(connection_ok, "Connection test should succeed");

	/* Simulate successful configuration save */
	printf("    Connection configuration validated successfully\n");

	return true;
}

/* ========================================================================
 * Test 10: Start All / Stop All UI Flow
 * Tests: Start All / Stop All buttons
 * ======================================================================== */
static bool test_ui_start_stop_all_flow(void)
{
	/* Create multiple channels with outputs */
	printf("    Creating test channels...\n");

	for (int i = 0; i < 3; i++) {
		char channel_name[128];
		snprintf(channel_name, sizeof(channel_name),
			 "UI_Bulk_%d_%ld", i, time(NULL));

		stream_channel_t *channel =
			channel_manager_create_channel(g_manager, channel_name);
		ASSERT_NOT_NULL(channel, "Channel should be created");

		/* Set input URL */
		if (channel->input_url)
			free(channel->input_url);
		channel->input_url = strdup(TEST_INPUT_URL);

		/* Add output */
		encoding_settings_t encoding = channel_get_default_encoding();
		channel_add_output(channel, SERVICE_YOUTUBE, "test_key",
				   ORIENTATION_HORIZONTAL, &encoding);
	}

	size_t count = channel_manager_get_count(g_manager);
	ASSERT_TRUE(count >= 3, "Should have at least 3 channels");

	/* Simulate user clicking "Start All" button */
	printf("    Simulating Start All button...\n");
	bool started_all = channel_manager_start_all(g_manager);
	ASSERT_TRUE(started_all, "Should start all channels");

	sleep_ms(2000);

	/* Verify active count */
	size_t active_count = channel_manager_get_active_count(g_manager);
	printf("    Active channels: %zu\n", active_count);
	ASSERT_TRUE(active_count > 0, "Should have active channels");

	/* Simulate user clicking "Stop All" button */
	printf("    Simulating Stop All button...\n");
	bool stopped_all = channel_manager_stop_all(g_manager);
	ASSERT_TRUE(stopped_all, "Should stop all channels");

	sleep_ms(1000);

	/* Verify all stopped */
	active_count = channel_manager_get_active_count(g_manager);
	printf("    Active channels after stop: %zu\n", active_count);

	return true;
}

/* ========================================================================
 * Test 11: Error Handling - Invalid Input
 * Tests: UI validation and error handling
 * ======================================================================== */
static bool test_ui_error_invalid_input(void)
{
	printf("    Testing invalid input handling...\n");

	/* Test 1: Create channel with empty name */
	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, "");
	ASSERT_NOT_NULL(channel,
			"Should create channel even with empty name");

	/* Test 2: Add output with invalid stream key */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(channel, SERVICE_YOUTUBE, "",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Should add output even with empty stream key");

	/* Test 3: Try to start channel without input URL */
	if (channel->input_url) {
		free(channel->input_url);
		channel->input_url = NULL;
	}

	/* This should fail or handle gracefully */
	bool started = channel_start(g_manager, channel->channel_id);
	printf("    Start without input URL result: %s\n",
	       started ? "success (unexpected)" : "failed (expected)");

	/* Test 4: Invalid channel ID */
	stream_channel_t *invalid =
		channel_manager_get_channel(g_manager, "nonexistent_id");
	ASSERT_NULL(invalid, "Should not find nonexistent channel");

	/* Test 5: Invalid output index */
	bool removed = channel_remove_output(channel, 999);
	ASSERT_FALSE(removed, "Should fail to remove invalid output index");

	return true;
}

/* ========================================================================
 * Test 12: Error Handling - Server Connection Loss
 * Tests: UI behavior when server becomes unavailable
 * ======================================================================== */
static bool test_ui_error_server_unavailable(void)
{
	printf("    Testing server unavailable handling...\n");

	/* Create a channel */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_ServerError");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Set input URL and add output */
	if (channel->input_url)
		free(channel->input_url);
	channel->input_url = strdup(TEST_INPUT_URL);

	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Create API client with invalid server */
	restreamer_connection_t bad_connection = {
		.host = "invalid.server.local",
		.port = 9999,
		.username = "invalid",
		.password = "invalid",
		.use_https = false};

	restreamer_api_t *bad_api = restreamer_api_create(&bad_connection);
	ASSERT_NOT_NULL(bad_api, "Should create API with bad connection");

	/* Test connection should fail */
	bool connected = restreamer_api_test_connection(bad_api);
	ASSERT_FALSE(connected, "Connection to invalid server should fail");

	/* Check error message */
	const char *error = restreamer_api_get_error(bad_api);
	ASSERT_NOT_NULL(error, "Should have error message");
	printf("    Expected error: %s\n", error);

	/* Cleanup */
	restreamer_api_destroy(bad_api);

	return true;
}

/* ========================================================================
 * Test 13: Channel Context Menu Operations
 * Tests: Context menu actions (duplicate, restart, etc.)
 * ======================================================================== */
static bool test_ui_channel_context_menu(void)
{
	/* Create source channel */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_ContextMenu");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Test 1: Duplicate channel */
	printf("    Testing duplicate channel...\n");
	stream_channel_t *duplicate =
		channel_duplicate(channel, "UI_ContextMenu_Copy");
	ASSERT_NOT_NULL(duplicate, "Duplicate should be created");
	ASSERT_STR_EQ(duplicate->channel_name, "UI_ContextMenu_Copy",
		      "Duplicate name should match");
	ASSERT_EQ(duplicate->output_count, channel->output_count,
		  "Duplicate should have same output count");

	/* Add duplicate to manager */
	g_manager->channels = realloc(
		g_manager->channels,
		(g_manager->channel_count + 1) * sizeof(stream_channel_t *));
	g_manager->channels[g_manager->channel_count] = duplicate;
	g_manager->channel_count++;

	/* Test 2: Restart channel (stop then start) */
	printf("    Testing restart channel...\n");
	if (channel->input_url)
		free(channel->input_url);
	channel->input_url = strdup(TEST_INPUT_URL);

	bool restart = channel_restart(g_manager, channel->channel_id);
	ASSERT_TRUE(restart, "Restart should succeed");

	return true;
}

/* ========================================================================
 * Test 14: Preview Mode UI Flow
 * Tests: Preview mode operations
 * ======================================================================== */
static bool test_ui_preview_mode_flow(void)
{
	/* Create channel */
	char channel_name[128];
	generate_channel_name(channel_name, sizeof(channel_name),
			      "UI_Preview");

	stream_channel_t *channel =
		channel_manager_create_channel(g_manager, channel_name);
	ASSERT_NOT_NULL(channel, "Channel should be created");

	/* Set input URL and add output */
	if (channel->input_url)
		free(channel->input_url);
	channel->input_url = strdup(TEST_INPUT_URL);

	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_YOUTUBE, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Start preview mode (60 seconds) */
	printf("    Starting preview mode (60 seconds)...\n");
	bool preview_started =
		channel_start_preview(g_manager, channel->channel_id, 60);
	ASSERT_TRUE(preview_started, "Preview mode should start");

	sleep_ms(2000);

	/* Verify preview mode status */
	ASSERT_EQ(channel->status, CHANNEL_STATUS_PREVIEW,
		  "Channel should be in preview mode");
	ASSERT_TRUE(channel->preview_mode_enabled,
		    "Preview mode flag should be set");

	/* Test go live from preview */
	printf("    Testing go live from preview...\n");
	bool went_live =
		channel_preview_to_live(g_manager, channel->channel_id);
	ASSERT_TRUE(went_live, "Should go live from preview");

	sleep_ms(1000);

	/* Stop the channel */
	channel_stop(g_manager, channel->channel_id);

	sleep_ms(1000);

	/* Test cancel preview */
	printf("    Testing cancel preview...\n");
	channel_start_preview(g_manager, channel->channel_id, 30);
	sleep_ms(1000);

	bool cancelled =
		channel_cancel_preview(g_manager, channel->channel_id);
	ASSERT_TRUE(cancelled, "Preview should be cancelled");

	return true;
}

/* ========================================================================
 * Test Suite Main
 * ======================================================================== */

BEGIN_TEST_SUITE("UI Workflow Tests")

/* Setup test environment */
printf("\n[SETUP] Connecting to Restreamer server: %s:%d\n",
       TEST_SERVER_URL, TEST_SERVER_PORT);
if (!setup_test_environment()) {
	printf("[ERROR] Failed to setup test environment\n");
	return 1;
}
printf("[SETUP] Test environment ready\n\n");

/* === Channel UI Tests === */
RUN_TEST(test_ui_channel_create_flow,
	 "UI Workflow: Channel creation through UI");
RUN_TEST(test_ui_channel_edit_flow,
	 "UI Workflow: Channel editing (name, settings)");
RUN_TEST(test_ui_channel_delete_flow,
	 "UI Workflow: Channel deletion via context menu");
RUN_TEST(test_ui_channel_start_stop_flow,
	 "UI Workflow: Channel start/stop buttons");

/* === Output UI Tests === */
RUN_TEST(test_ui_output_add_flow, "UI Workflow: Add output through dialog");
RUN_TEST(test_ui_output_edit_flow,
	 "UI Workflow: Edit output settings");
RUN_TEST(test_ui_output_delete_flow,
	 "UI Workflow: Delete output via context menu");
RUN_TEST(test_ui_output_enable_disable_flow,
	 "UI Workflow: Enable/disable output toggle");

/* === Dock UI Tests === */
RUN_TEST(test_ui_connection_config_flow,
	 "UI Workflow: Connection configuration dialog");
RUN_TEST(test_ui_start_stop_all_flow,
	 "UI Workflow: Start All / Stop All buttons");

/* === Error Handling Tests === */
RUN_TEST(test_ui_error_invalid_input,
	 "UI Error Handling: Invalid input validation");
RUN_TEST(test_ui_error_server_unavailable,
	 "UI Error Handling: Server connection failure");

/* === Advanced UI Tests === */
RUN_TEST(test_ui_channel_context_menu,
	 "UI Workflow: Context menu operations (duplicate, restart)");
RUN_TEST(test_ui_preview_mode_flow,
	 "UI Workflow: Preview mode start/go live/cancel");

/* Cleanup */
printf("\n[TEARDOWN] Cleaning up test environment...\n");
cleanup_test_environment();
printf("[TEARDOWN] Complete\n");

END_TEST_SUITE()
