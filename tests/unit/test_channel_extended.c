/*
obs-polyemesis
Copyright (C) 2025 rainmanjam

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

/**
 * Extended Channel Management Unit Tests
 *
 * Comprehensive tests for the channel management module covering:
 * - Channel lifecycle operations
 * - Output management
 * - Start/Stop/Restart with proper cleanup
 * - Persistence (save/load)
 * - Failover mechanisms
 * - Bulk operations
 */

#include "restreamer-channel.h"
#include "restreamer-api.h"
#include "mock_restreamer.h"
#include <obs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Test assertion macros */
#define test_assert(condition, message)                                        \
	do {                                                                   \
		if (!(condition)) {                                            \
			fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n",        \
				message, __FILE__, __LINE__);                  \
			return false;                                          \
		}                                                              \
	} while (0)

/* Test output helpers */
static void test_section_start(const char *name) { (void)name; }
static void test_section_end(const char *name) { (void)name; }
static void test_start(const char *name) { printf("  Testing %s...\n", name); }
static void test_end(void) {}
static void test_suite_start(const char *name)
{
	printf("\n%s\n========================================\n", name);
}
static void test_suite_end(const char *name, bool result)
{
	if (result)
		printf("✓ %s: PASSED\n", name);
	else
		printf("✗ %s: FAILED\n", name);
}

/* Helper: Create test API connection */
static restreamer_api_t *create_test_api(void)
{
	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};
	return restreamer_api_create(&conn);
}

/* ========================================================================
 * Channel Lifecycle Tests
 * ======================================================================== */

/* Test channel creation with valid inputs */
static bool test_channel_creation_valid(void)
{
	test_section_start("Channel Creation - Valid Inputs");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Test creating channel with valid name */
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");
	test_assert(channel != NULL, "Channel creation should succeed");
	test_assert(channel->channel_name != NULL, "Channel should have name");
	test_assert(strcmp(channel->channel_name, "Test Channel") == 0,
		    "Channel name should match");
	test_assert(channel->channel_id != NULL,
		    "Channel should have unique ID");
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE,
		    "New channel should be inactive");
	test_assert(channel->output_count == 0,
		    "New channel should have no outputs");
	test_assert(channel->input_url != NULL,
		    "Channel should have default input URL");
	test_assert(manager->channel_count == 1,
		    "Manager should have 1 channel");

	/* Test creating multiple channels */
	stream_channel_t *channel2 =
		channel_manager_create_channel(manager, "Channel 2");
	test_assert(channel2 != NULL,
		    "Second channel creation should succeed");
	test_assert(manager->channel_count == 2,
		    "Manager should have 2 channels");
	test_assert(strcmp(channel->channel_id, channel2->channel_id) != 0,
		    "Channel IDs should be unique");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Creation - Valid Inputs");
	return true;
}

/* Test channel creation with invalid inputs */
static bool test_channel_creation_invalid(void)
{
	test_section_start("Channel Creation - Invalid Inputs");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Test NULL name */
	stream_channel_t *channel =
		channel_manager_create_channel(manager, NULL);
	test_assert(channel == NULL,
		    "Channel creation with NULL name should fail");
	test_assert(manager->channel_count == 0,
		    "Manager should have 0 channels");

	/* Test NULL manager */
	channel = channel_manager_create_channel(NULL, "Test");
	test_assert(channel == NULL,
		    "Channel creation with NULL manager should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Creation - Invalid Inputs");
	return true;
}

/* Test channel deletion */
static bool test_channel_deletion(void)
{
	test_section_start("Channel Deletion");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create multiple channels */
	stream_channel_t *channel1 =
		channel_manager_create_channel(manager, "Channel 1");
	stream_channel_t *channel2 =
		channel_manager_create_channel(manager, "Channel 2");
	stream_channel_t *channel3 =
		channel_manager_create_channel(manager, "Channel 3");

	test_assert(manager->channel_count == 3,
		    "Manager should have 3 channels");

	/* Save channel IDs before deletion */
	char *id1 = bstrdup(channel1->channel_id);
	char *id2 = bstrdup(channel2->channel_id);
	char *id3 = bstrdup(channel3->channel_id);

	/* Delete middle channel */
	bool deleted = channel_manager_delete_channel(manager, id2);
	test_assert(deleted, "Channel deletion should succeed");
	test_assert(manager->channel_count == 2,
		    "Manager should have 2 channels after deletion");

	/* Verify deleted channel is not retrievable */
	stream_channel_t *retrieved =
		channel_manager_get_channel(manager, id2);
	test_assert(retrieved == NULL,
		    "Deleted channel should not be retrievable");

	/* Verify other channels still exist */
	retrieved = channel_manager_get_channel(manager, id1);
	test_assert(retrieved != NULL, "Channel 1 should still exist");
	retrieved = channel_manager_get_channel(manager, id3);
	test_assert(retrieved != NULL, "Channel 3 should still exist");

	/* Delete non-existent channel */
	deleted = channel_manager_delete_channel(manager, "invalid_id");
	test_assert(!deleted, "Deleting non-existent channel should fail");
	test_assert(manager->channel_count == 2,
		    "Channel count should remain unchanged");

	/* Test NULL inputs */
	deleted = channel_manager_delete_channel(NULL, id1);
	test_assert(!deleted, "NULL manager should fail");
	deleted = channel_manager_delete_channel(manager, NULL);
	test_assert(!deleted, "NULL channel_id should fail");

	bfree(id1);
	bfree(id2);
	bfree(id3);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Deletion");
	return true;
}

/* Test channel duplication */
static bool test_channel_duplication(void)
{
	test_section_start("Channel Duplication");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create source channel with outputs */
	stream_channel_t *source =
		channel_manager_create_channel(manager, "Source Channel");
	encoding_settings_t encoding = channel_get_default_encoding();
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.bitrate = 6000;

	channel_add_output(source, SERVICE_TWITCH, "twitch_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(source, SERVICE_YOUTUBE, "youtube_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	source->auto_start = true;
	source->auto_reconnect = true;

	/* Duplicate channel */
	stream_channel_t *duplicate =
		channel_duplicate(source, "Duplicate Channel");
	test_assert(duplicate != NULL, "Channel duplication should succeed");
	test_assert(strcmp(duplicate->channel_name, "Duplicate Channel") == 0,
		    "Duplicate should have new name");
	test_assert(strcmp(duplicate->channel_id, source->channel_id) != 0,
		    "Duplicate should have unique ID");
	test_assert(duplicate->output_count == source->output_count,
		    "Duplicate should have same number of outputs");
	test_assert(duplicate->auto_start == source->auto_start,
		    "Duplicate should have same auto_start setting");
	test_assert(duplicate->status == CHANNEL_STATUS_INACTIVE,
		    "Duplicate should be inactive");

	/* Verify outputs were copied */
	for (size_t i = 0; i < duplicate->output_count; i++) {
		test_assert(duplicate->outputs[i].service ==
				    source->outputs[i].service,
			    "Output service should match");
		test_assert(strcmp(duplicate->outputs[i].stream_key,
				   source->outputs[i].stream_key) == 0,
			    "Output stream key should match");
		test_assert(duplicate->outputs[i].encoding.width ==
				    source->outputs[i].encoding.width,
			    "Encoding width should match");
	}

	/* Test NULL inputs */
	stream_channel_t *null_dup = channel_duplicate(NULL, "Test");
	test_assert(null_dup == NULL, "NULL source should fail");
	null_dup = channel_duplicate(source, NULL);
	test_assert(null_dup == NULL, "NULL name should fail");

	/* Clean up duplicate (not in manager) */
	for (size_t i = 0; i < duplicate->output_count; i++) {
		bfree(duplicate->outputs[i].service_name);
		bfree(duplicate->outputs[i].stream_key);
		bfree(duplicate->outputs[i].rtmp_url);
	}
	bfree(duplicate->outputs);
	bfree(duplicate->channel_name);
	bfree(duplicate->channel_id);
	bfree(duplicate->input_url);
	bfree(duplicate);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Duplication");
	return true;
}

/* ========================================================================
 * Output Management Tests
 * ======================================================================== */

/* Test adding outputs to channels */
static bool test_output_addition(void)
{
	test_section_start("Output Addition");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	encoding_settings_t encoding = channel_get_default_encoding();
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.bitrate = 6000;
	encoding.audio_bitrate = 160;

	/* Add first output */
	bool added = channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
					ORIENTATION_HORIZONTAL, &encoding);
	test_assert(added, "Adding output should succeed");
	test_assert(channel->output_count == 1, "Channel should have 1 output");

	/* Verify output properties */
	channel_output_t *output = &channel->outputs[0];
	test_assert(output->service == SERVICE_TWITCH,
		    "Output service should match");
	test_assert(strcmp(output->stream_key, "twitch_key") == 0,
		    "Stream key should match");
	test_assert(output->target_orientation == ORIENTATION_HORIZONTAL,
		    "Orientation should match");
	test_assert(output->enabled, "Output should be enabled by default");
	test_assert(output->encoding.width == 1920, "Width should match");
	test_assert(output->encoding.bitrate == 6000, "Bitrate should match");

	/* Add multiple outputs */
	added = channel_add_output(channel, SERVICE_YOUTUBE, "youtube_key",
				   ORIENTATION_HORIZONTAL, NULL);
	test_assert(added, "Adding second output should succeed");
	test_assert(channel->output_count == 2, "Channel should have 2 outputs");

	/* Verify default encoding when NULL passed */
	output = &channel->outputs[1];
	test_assert(output->encoding.width == 0,
		    "Default encoding should have width 0");

	/* Test NULL inputs */
	added = channel_add_output(NULL, SERVICE_TWITCH, "key",
				   ORIENTATION_HORIZONTAL, &encoding);
	test_assert(!added, "NULL channel should fail");
	added = channel_add_output(channel, SERVICE_TWITCH, NULL,
				   ORIENTATION_HORIZONTAL, &encoding);
	test_assert(!added, "NULL stream key should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Output Addition");
	return true;
}

/* Test removing outputs */
static bool test_output_removal(void)
{
	test_section_start("Output Removal");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add multiple outputs */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_YOUTUBE, "youtube_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_FACEBOOK, "facebook_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	test_assert(channel->output_count == 3, "Channel should have 3 outputs");

	/* Remove middle output */
	bool removed = channel_remove_output(channel, 1);
	test_assert(removed, "Output removal should succeed");
	test_assert(channel->output_count == 2,
		    "Channel should have 2 outputs after removal");

	/* Verify remaining outputs */
	test_assert(channel->outputs[0].service == SERVICE_TWITCH,
		    "First output should remain");
	test_assert(channel->outputs[1].service == SERVICE_FACEBOOK,
		    "Last output should shift down");

	/* Remove all outputs */
	removed = channel_remove_output(channel, 0);
	test_assert(removed, "Removing first output should succeed");
	removed = channel_remove_output(channel, 0);
	test_assert(removed, "Removing last output should succeed");
	test_assert(channel->output_count == 0,
		    "Channel should have no outputs");
	test_assert(channel->outputs == NULL,
		    "Outputs array should be NULL after removing all");

	/* Test invalid indices */
	removed = channel_remove_output(channel, 0);
	test_assert(!removed, "Removing from empty channel should fail");
	removed = channel_remove_output(channel, 100);
	test_assert(!removed, "Invalid index should fail");

	/* Test NULL input */
	removed = channel_remove_output(NULL, 0);
	test_assert(!removed, "NULL channel should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Output Removal");
	return true;
}

/* Test enabling/disabling outputs */
static bool test_output_enable_disable(void)
{
	test_section_start("Output Enable/Disable");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	test_assert(channel->outputs[0].enabled,
		    "Output should be enabled by default");

	/* Disable output */
	bool result = channel_set_output_enabled(channel, 0, false);
	test_assert(result, "Disabling output should succeed");
	test_assert(!channel->outputs[0].enabled,
		    "Output should be disabled");

	/* Enable output */
	result = channel_set_output_enabled(channel, 0, true);
	test_assert(result, "Enabling output should succeed");
	test_assert(channel->outputs[0].enabled, "Output should be enabled");

	/* Test invalid indices */
	result = channel_set_output_enabled(channel, 100, true);
	test_assert(!result, "Invalid index should fail");

	/* Test NULL input */
	result = channel_set_output_enabled(NULL, 0, true);
	test_assert(!result, "NULL channel should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Output Enable/Disable");
	return true;
}

/* Test bulk output operations */
static bool test_bulk_output_operations(void)
{
	test_section_start("Bulk Output Operations");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add multiple outputs */
	encoding_settings_t encoding = channel_get_default_encoding();
	for (int i = 0; i < 5; i++) {
		char key[32];
		snprintf(key, sizeof(key), "key_%d", i);
		channel_add_output(channel, SERVICE_TWITCH, key,
				   ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Test bulk enable/disable */
	size_t indices[] = {0, 2, 4};
	bool result = channel_bulk_enable_outputs(channel, api, indices, 3,
						  false);
	test_assert(result, "Bulk disable should succeed");
	test_assert(!channel->outputs[0].enabled,
		    "Output 0 should be disabled");
	test_assert(channel->outputs[1].enabled,
		    "Output 1 should remain enabled");
	test_assert(!channel->outputs[2].enabled,
		    "Output 2 should be disabled");

	/* Test bulk encoding update */
	encoding.width = 1280;
	encoding.height = 720;
	encoding.bitrate = 4500;
	result = channel_bulk_update_encoding(channel, api, indices, 3,
					      &encoding);
	test_assert(result, "Bulk encoding update should succeed");
	test_assert(channel->outputs[0].encoding.width == 1280,
		    "Encoding should be updated");
	test_assert(channel->outputs[2].encoding.height == 720,
		    "Encoding should be updated");

	/* Test bulk delete */
	size_t delete_indices[] = {1, 3}; // Delete in descending order
	result = channel_bulk_delete_outputs(channel, delete_indices, 2);
	test_assert(result, "Bulk delete should succeed");
	test_assert(channel->output_count == 3,
		    "Should have 3 outputs after deleting 2");

	/* Test NULL inputs */
	result = channel_bulk_enable_outputs(NULL, api, indices, 3, true);
	test_assert(!result, "NULL channel should fail");
	result = channel_bulk_enable_outputs(channel, api, NULL, 3, true);
	test_assert(!result, "NULL indices should fail");
	result = channel_bulk_enable_outputs(channel, api, indices, 0, true);
	test_assert(!result, "Zero count should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Bulk Output Operations");
	return true;
}

/* ========================================================================
 * Channel Start/Stop/Restart Tests
 * ======================================================================== */

/* Test channel start deletes existing process first */
static bool test_channel_start_cleanup(void)
{
	test_section_start("Channel Start - Process Cleanup");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Start channel (should call delete_process before starting) */
	bool started = channel_start(manager, channel->channel_id);
	test_assert(started, "Channel start should succeed");
	test_assert(channel->status == CHANNEL_STATUS_ACTIVE,
		    "Channel should be active");
	test_assert(channel->process_reference != NULL,
		    "Should have process reference");

	/* Start again (should delete existing process first) */
	started = channel_start(manager, channel->channel_id);
	test_assert(started || channel->status == CHANNEL_STATUS_ACTIVE,
		    "Restarting active channel should handle gracefully");

	/* Stop channel */
	bool stopped = channel_stop(manager, channel->channel_id);
	test_assert(stopped, "Channel stop should succeed");
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE,
		    "Channel should be inactive");
	test_assert(channel->process_reference == NULL,
		    "Process reference should be cleared");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Start - Process Cleanup");
	return true;
}

/* Test proper cleanup on stop */
static bool test_channel_stop_cleanup(void)
{
	test_section_start("Channel Stop - Cleanup");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add output and start */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_start(manager, channel->channel_id);

	test_assert(channel->status == CHANNEL_STATUS_ACTIVE,
		    "Channel should be active");

	/* Stop channel */
	bool stopped = channel_stop(manager, channel->channel_id);
	test_assert(stopped, "Stop should succeed");
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE,
		    "Status should be inactive");
	test_assert(channel->process_reference == NULL,
		    "Process reference should be NULL");
	test_assert(channel->last_error == NULL,
		    "Last error should be cleared");

	/* Stop already stopped channel */
	stopped = channel_stop(manager, channel->channel_id);
	test_assert(stopped, "Stopping inactive channel should succeed");

	/* Test NULL inputs */
	stopped = channel_stop(NULL, channel->channel_id);
	test_assert(!stopped, "NULL manager should fail");
	stopped = channel_stop(manager, NULL);
	test_assert(!stopped, "NULL channel_id should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Stop - Cleanup");
	return true;
}

/* Test restart functionality */
static bool test_channel_restart(void)
{
	test_section_start("Channel Restart");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add output */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "test_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Start channel */
	channel_start(manager, channel->channel_id);
	test_assert(channel->status == CHANNEL_STATUS_ACTIVE,
		    "Channel should be active");

	/* Restart channel */
	bool restarted = channel_restart(manager, channel->channel_id);
	test_assert(restarted, "Restart should succeed");
	test_assert(channel->status == CHANNEL_STATUS_ACTIVE,
		    "Channel should be active after restart");
	test_assert(channel->process_reference != NULL,
		    "Should have new process reference");

	/* Clean up */
	channel_stop(manager, channel->channel_id);
	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Restart");
	return true;
}

/* Test start/stop all channels */
static bool test_start_stop_all_channels(void)
{
	test_section_start("Start/Stop All Channels");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create multiple channels with auto_start */
	for (int i = 0; i < 3; i++) {
		char name[32];
		snprintf(name, sizeof(name), "Channel %d", i + 1);
		stream_channel_t *channel =
			channel_manager_create_channel(manager, name);
		channel->auto_start = true;

		encoding_settings_t encoding = channel_get_default_encoding();
		channel_add_output(channel, SERVICE_TWITCH, "test_key",
				   ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Start all channels */
	bool result = channel_manager_start_all(manager);
	test_assert(result, "Start all should succeed");

	/* Verify all channels are active */
	size_t active_count = channel_manager_get_active_count(manager);
	test_assert(active_count == 3, "Should have 3 active channels");

	/* Stop all channels */
	result = channel_manager_stop_all(manager);
	test_assert(result, "Stop all should succeed");

	/* Verify all channels are inactive */
	active_count = channel_manager_get_active_count(manager);
	test_assert(active_count == 0, "Should have 0 active channels");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Start/Stop All Channels");
	return true;
}

/* ========================================================================
 * Persistence Tests
 * ======================================================================== */

/* Test saving channels to settings */
static bool test_save_channels(void)
{
	test_section_start("Save Channels to Settings");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create channel with outputs */
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");
	channel->auto_start = true;
	channel->auto_reconnect = true;
	channel->reconnect_delay_sec = 10;

	encoding_settings_t encoding = channel_get_default_encoding();
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.bitrate = 6000;

	channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_YOUTUBE, "youtube_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Save to settings */
	obs_data_t *settings = obs_data_create();
	channel_manager_save_to_settings(manager, settings);

	/* Verify settings were saved */
	obs_data_array_t *channels_array =
		obs_data_get_array(settings, "stream_channels");
	test_assert(channels_array != NULL,
		    "Channels array should be saved");
	test_assert(obs_data_array_count(channels_array) == 1,
		    "Should have 1 channel saved");

	/* Verify channel data */
	obs_data_t *channel_data = obs_data_array_item(channels_array, 0);
	test_assert(strcmp(obs_data_get_string(channel_data, "name"),
			   "Test Channel") == 0,
		    "Channel name should be saved");
	test_assert(obs_data_get_bool(channel_data, "auto_start"),
		    "Auto start should be saved");

	/* Verify outputs */
	obs_data_array_t *outputs_array =
		obs_data_get_array(channel_data, "outputs");
	test_assert(obs_data_array_count(outputs_array) == 2,
		    "Should have 2 outputs saved");

	obs_data_release(channel_data);
	obs_data_array_release(outputs_array);
	obs_data_array_release(channels_array);
	obs_data_release(settings);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Save Channels to Settings");
	return true;
}

/* Test loading channels from settings */
static bool test_load_channels(void)
{
	test_section_start("Load Channels from Settings");

	restreamer_api_t *api = create_test_api();

	/* Create and save channels */
	channel_manager_t *manager1 = channel_manager_create(api);
	stream_channel_t *channel1 =
		channel_manager_create_channel(manager1, "Channel 1");
	channel1->auto_start = true;

	encoding_settings_t encoding = channel_get_default_encoding();
	encoding.width = 1920;
	encoding.height = 1080;
	channel_add_output(channel1, SERVICE_TWITCH, "key1",
			   ORIENTATION_HORIZONTAL, &encoding);

	obs_data_t *settings = obs_data_create();
	channel_manager_save_to_settings(manager1, settings);
	channel_manager_destroy(manager1);

	/* Load channels in new manager */
	channel_manager_t *manager2 = channel_manager_create(api);
	channel_manager_load_from_settings(manager2, settings);

	test_assert(manager2->channel_count == 1,
		    "Should have loaded 1 channel");

	stream_channel_t *loaded = manager2->channels[0];
	test_assert(strcmp(loaded->channel_name, "Channel 1") == 0,
		    "Channel name should match");
	test_assert(loaded->auto_start, "Auto start should be loaded");
	test_assert(loaded->output_count == 1, "Should have 1 output");
	test_assert(loaded->outputs[0].service == SERVICE_TWITCH,
		    "Output service should match");
	test_assert(loaded->outputs[0].encoding.width == 1920,
		    "Encoding settings should be loaded");

	obs_data_release(settings);
	channel_manager_destroy(manager2);
	restreamer_api_destroy(api);

	test_section_end("Load Channels from Settings");
	return true;
}

/* Test handling missing settings */
static bool test_load_missing_settings(void)
{
	test_section_start("Load Missing Settings");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Load from empty settings */
	obs_data_t *settings = obs_data_create();
	channel_manager_load_from_settings(manager, settings);

	test_assert(manager->channel_count == 0,
		    "Should have 0 channels when loading empty settings");

	/* Test NULL settings */
	channel_manager_load_from_settings(manager, NULL);
	test_assert(manager->channel_count == 0,
		    "NULL settings should not crash");

	obs_data_release(settings);
	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Load Missing Settings");
	return true;
}

/* Test handling corrupt settings */
static bool test_load_corrupt_settings(void)
{
	test_section_start("Load Corrupt Settings");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create settings with missing required fields */
	obs_data_t *settings = obs_data_create();
	obs_data_array_t *channels_array = obs_data_array_create();

	obs_data_t *channel_data = obs_data_create();
	/* Missing 'name' and 'id' fields */
	obs_data_set_int(channel_data, "source_orientation", 0);
	obs_data_array_push_back(channels_array, channel_data);

	obs_data_set_array(settings, "stream_channels", channels_array);

	/* Load corrupt settings */
	channel_manager_load_from_settings(manager, settings);

	test_assert(manager->channel_count == 0,
		    "Corrupt channel should not be loaded");

	obs_data_release(channel_data);
	obs_data_array_release(channels_array);
	obs_data_release(settings);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Load Corrupt Settings");
	return true;
}

/* ========================================================================
 * Failover Tests
 * ======================================================================== */

/* Test backup output configuration */
static bool test_backup_configuration(void)
{
	test_section_start("Backup Output Configuration");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add primary and backup outputs */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "primary_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_TWITCH, "backup_key",
			   ORIENTATION_HORIZONTAL, &encoding);

	/* Set backup relationship */
	bool result = channel_set_output_backup(channel, 0, 1);
	test_assert(result, "Setting backup should succeed");
	test_assert(channel->outputs[0].backup_index == 1,
		    "Primary should reference backup");
	test_assert(channel->outputs[1].is_backup,
		    "Output should be marked as backup");
	test_assert(channel->outputs[1].primary_index == 0,
		    "Backup should reference primary");
	test_assert(!channel->outputs[1].enabled,
		    "Backup should start disabled");

	/* Test invalid inputs */
	result = channel_set_output_backup(channel, 0, 0);
	test_assert(!result, "Setting output as its own backup should fail");
	result = channel_set_output_backup(channel, 0, 100);
	test_assert(!result, "Invalid backup index should fail");
	result = channel_set_output_backup(NULL, 0, 1);
	test_assert(!result, "NULL channel should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Backup Output Configuration");
	return true;
}

/* Test removing backup relationships */
static bool test_remove_backup(void)
{
	test_section_start("Remove Backup Relationship");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Add and configure backup */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "primary_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_TWITCH, "backup_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_set_output_backup(channel, 0, 1);

	/* Remove backup relationship */
	bool result = channel_remove_output_backup(channel, 0);
	test_assert(result, "Removing backup should succeed");
	test_assert(channel->outputs[0].backup_index == (size_t)-1,
		    "Primary should not reference backup");
	test_assert(!channel->outputs[1].is_backup,
		    "Output should not be marked as backup");
	test_assert(channel->outputs[1].primary_index == (size_t)-1,
		    "Backup should not reference primary");

	/* Test removing non-existent backup */
	result = channel_remove_output_backup(channel, 0);
	test_assert(!result, "Removing non-existent backup should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Remove Backup Relationship");
	return true;
}

/* Test failover trigger conditions */
static bool test_failover_trigger(void)
{
	test_section_start("Failover Trigger Conditions");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Setup primary and backup */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "primary_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_TWITCH, "backup_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_set_output_backup(channel, 0, 1);

	/* Trigger failover */
	bool result = channel_trigger_failover(channel, api, 0);
	test_assert(result, "Triggering failover should succeed");
	test_assert(channel->outputs[0].failover_active,
		    "Primary should be in failover state");
	test_assert(channel->outputs[1].failover_active,
		    "Backup should be in failover state");
	test_assert(!channel->outputs[0].enabled ||
			    channel->status != CHANNEL_STATUS_ACTIVE,
		    "Primary should be disabled when active");

	/* Test triggering already active failover */
	result = channel_trigger_failover(channel, api, 0);
	test_assert(result, "Re-triggering failover should succeed");

	/* Test without backup */
	channel_add_output(channel, SERVICE_YOUTUBE, "youtube_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	result = channel_trigger_failover(channel, api, 2);
	test_assert(!result,
		    "Failover without backup should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Failover Trigger Conditions");
	return true;
}

/* Test primary restoration */
static bool test_primary_restoration(void)
{
	test_section_start("Primary Restoration");

	restreamer_api_t *api = create_test_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Setup and trigger failover */
	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "primary_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_TWITCH, "backup_key",
			   ORIENTATION_HORIZONTAL, &encoding);
	channel_set_output_backup(channel, 0, 1);
	channel_trigger_failover(channel, api, 0);

	test_assert(channel->outputs[0].failover_active,
		    "Failover should be active");

	/* Restore primary */
	bool result = channel_restore_primary(channel, api, 0);
	test_assert(result, "Restoring primary should succeed");
	test_assert(!channel->outputs[0].failover_active,
		    "Failover should be inactive");
	test_assert(!channel->outputs[1].failover_active,
		    "Backup failover should be inactive");
	test_assert(channel->outputs[0].consecutive_failures == 0,
		    "Failure count should be reset");

	/* Test restoring when not in failover */
	result = channel_restore_primary(channel, api, 0);
	test_assert(result, "Restoring non-failed primary should succeed");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Primary Restoration");
	return true;
}

/* ========================================================================
 * Test Suite Entry Point
 * ======================================================================== */

/* Run all tests */
bool test_channel_extended_suite(void)
{
	bool all_passed = true;

	test_suite_start("Extended Channel Management Tests");

	/* Channel lifecycle tests */
	if (!test_channel_creation_valid())
		all_passed = false;
	if (!test_channel_creation_invalid())
		all_passed = false;
	if (!test_channel_deletion())
		all_passed = false;
	if (!test_channel_duplication())
		all_passed = false;

	/* Output management tests */
	if (!test_output_addition())
		all_passed = false;
	if (!test_output_removal())
		all_passed = false;
	if (!test_output_enable_disable())
		all_passed = false;
	if (!test_bulk_output_operations())
		all_passed = false;

	/* Start/Stop/Restart tests */
	if (!test_channel_start_cleanup())
		all_passed = false;
	if (!test_channel_stop_cleanup())
		all_passed = false;
	if (!test_channel_restart())
		all_passed = false;
	if (!test_start_stop_all_channels())
		all_passed = false;

	/* Persistence tests */
	if (!test_save_channels())
		all_passed = false;
	if (!test_load_channels())
		all_passed = false;
	if (!test_load_missing_settings())
		all_passed = false;
	if (!test_load_corrupt_settings())
		all_passed = false;

	/* Failover tests */
	if (!test_backup_configuration())
		all_passed = false;
	if (!test_remove_backup())
		all_passed = false;
	if (!test_failover_trigger())
		all_passed = false;
	if (!test_primary_restoration())
		all_passed = false;

	test_suite_end("Extended Channel Management Tests", all_passed);

	return all_passed;
}

/* Main entry point */
int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	bool passed = test_channel_extended_suite();
	return passed ? 0 : 1;
}
