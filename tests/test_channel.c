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

#include "restreamer-channel.h"
#include "restreamer-api.h"
#include "mock_restreamer.h"
#include <obs.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Test macros from test framework */
#define test_assert(condition, message)                                        \
  do {                                                                         \
    if (!(condition)) {                                                        \
      fprintf(stderr, "  ✗ FAIL: %s\n    at %s:%d\n", message, __FILE__,       \
              __LINE__);                                                       \
      return false;                                                            \
    }                                                                          \
  } while (0)

static void test_section_start(const char *name) { (void)name; }
static void test_section_end(const char *name) { (void)name; }
static void test_start(const char *name) { printf("  Testing %s...\n", name); }
static void test_end(void) {}
static void test_suite_start(const char *name) { printf("\n%s\n========================================\n", name); }
static void test_suite_end(const char *name, bool result) {
  if (result) printf("✓ %s: PASSED\n", name);
  else printf("✗ %s: FAILED\n", name);
}

/* Test Channel manager creation and destruction */
static bool test_channel_manager_lifecycle(void)
{
	test_section_start("Channel Manager Lifecycle");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	test_assert(api != NULL, "API creation should succeed");

	channel_manager_t *manager = channel_manager_create(api);
	test_assert(manager != NULL, "Manager creation should succeed");
	test_assert(manager->api == api, "Manager should reference API");
	test_assert(manager->channel_count == 0,
		    "New manager should have no channels");
	test_assert(manager->channels == NULL,
		    "New manager should have NULL channels array");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Manager Lifecycle");
	return true;
}

/* Test Channel creation and deletion */
static bool test_channel_creation(void)
{
	test_section_start("Channel Creation");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Create first channel */
	stream_channel_t *channel1 =
		channel_manager_create_channel(manager, "Test Channel 1");
	test_assert(channel1 != NULL, "Channel creation should succeed");
	test_assert(channel1->channel_name != NULL,
		    "Channel should have name");
	test_assert(strcmp(channel1->channel_name, "Test Channel 1") == 0,
		    "Channel name should match");
	test_assert(channel1->channel_id != NULL,
		    "Channel should have unique ID");
	test_assert(channel1->status == CHANNEL_STATUS_INACTIVE,
		    "New channel should be inactive");
	test_assert(channel1->output_count == 0,
		    "New channel should have no outputs");
	test_assert(manager->channel_count == 1,
		    "Manager should have 1 channel");

	/* Create second channel */
	stream_channel_t *channel2 =
		channel_manager_create_channel(manager, "Test Channel 2");
	test_assert(channel2 != NULL,
		    "Second channel creation should succeed");
	test_assert(manager->channel_count == 2,
		    "Manager should have 2 channels");
	test_assert(strcmp(channel1->channel_id, channel2->channel_id) != 0,
		    "Channel IDs should be unique");

	/* Get channel by index */
	stream_channel_t *retrieved =
		channel_manager_get_channel_at(manager, 0);
	test_assert(retrieved == channel1,
		    "Should retrieve first channel by index");

	retrieved = channel_manager_get_channel_at(manager, 1);
	test_assert(retrieved == channel2,
		    "Should retrieve second channel by index");

	/* Get channel by ID */
	retrieved =
		channel_manager_get_channel(manager, channel1->channel_id);
	test_assert(retrieved == channel1, "Should retrieve profile by ID");

	/* Get count */
	size_t count = channel_manager_get_count(manager);
	test_assert(count == 2, "Should return correct channel count");

	/* Save channel ID before deletion to avoid use-after-free */
	char *saved_channel_id = bstrdup(channel1->channel_id);

	/* Delete channel */
	bool deleted = channel_manager_delete_channel(manager,
						       saved_channel_id);
	test_assert(deleted, "Channel deletion should succeed");
	test_assert(manager->channel_count == 1,
		    "Manager should have 1 profile after deletion");

	retrieved =
		channel_manager_get_channel(manager, saved_channel_id);
	test_assert(retrieved == NULL,
		    "Deleted channel should not be retrievable");

	bfree(saved_channel_id);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Creation");
	return true;
}

/* Test Channel output management */
static bool test_channel_outputs(void)
{
	test_section_start("Channel Outputs");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Test Channel");

	/* Get default encoding settings */
	encoding_settings_t encoding = channel_get_default_encoding();
	test_assert(encoding.width == 0, "Default width should be 0");
	test_assert(encoding.height == 0, "Default height should be 0");
	test_assert(encoding.audio_track == 0,
		    "Default audio track should be 0 (use source settings)");

	/* Add output */
	bool added = channel_add_output(
		channel, SERVICE_TWITCH, "test_stream_key",
		ORIENTATION_HORIZONTAL, &encoding);
	test_assert(added, "Adding output should succeed");
	test_assert(channel->output_count == 1,
		    "Channel should have 1 output");
	test_assert(channel->outputs != NULL,
		    "Outputs array should be allocated");

	channel_output_t *dest = &channel->outputs[0];
	test_assert(dest->service == SERVICE_TWITCH,
		    "Output service should match");
	test_assert(dest->stream_key != NULL,
		    "Output should have stream key");
	test_assert(strcmp(dest->stream_key, "test_stream_key") == 0,
		    "Stream key should match");
	test_assert(dest->target_orientation == ORIENTATION_HORIZONTAL,
		    "Orientation should match");
	test_assert(dest->enabled == true,
		    "New output should be enabled");

	/* Add second output */
	added = channel_add_output(channel, SERVICE_YOUTUBE,
					"youtube_key", ORIENTATION_HORIZONTAL,
					&encoding);
	test_assert(added, "Adding second output should succeed");
	test_assert(channel->output_count == 2,
		    "Channel should have 2 outputs");

	/* Update encoding settings */
	encoding_settings_t new_encoding = {.width = 1920,
					     .height = 1080,
					     .bitrate = 6000,
					     .fps_num = 60,
					     .fps_den = 1,
					     .audio_bitrate = 128,
					     .audio_track = 1,
					     .max_bandwidth = 8000,
					     .low_latency = true};

	bool updated = channel_update_output_encoding(channel, 0,
							   &new_encoding);
	test_assert(updated, "Updating encoding should succeed");
	test_assert(channel->outputs[0].encoding.width == 1920,
		    "Width should be updated");
	test_assert(channel->outputs[0].encoding.bitrate == 6000,
		    "Bitrate should be updated");

	/* Enable/disable output */
	bool set_enabled = channel_set_output_enabled(channel, 0, false);
	test_assert(set_enabled, "Disabling output should succeed");
	test_assert(channel->outputs[0].enabled == false,
		    "Output should be disabled");

	set_enabled = channel_set_output_enabled(channel, 0, true);
	test_assert(set_enabled, "Enabling output should succeed");
	test_assert(channel->outputs[0].enabled == true,
		    "Output should be enabled");

	/* Remove output */
	bool removed = channel_remove_output(channel, 0);
	test_assert(removed, "Removing output should succeed");
	test_assert(channel->output_count == 1,
		    "Channel should have 1 output after removal");
	test_assert(channel->outputs[0].service == SERVICE_YOUTUBE,
		    "Remaining output should be YouTube");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Outputs");
	return true;
}

/* Test Channel ID generation */
static bool test_channel_id_generation(void)
{
	test_section_start("Channel ID Generation");

	/* Generate multiple IDs and ensure they're unique */
	char *id1 = channel_generate_id();
	char *id2 = channel_generate_id();
	char *id3 = channel_generate_id();

	test_assert(id1 != NULL, "ID generation should succeed");
	test_assert(id2 != NULL, "ID generation should succeed");
	test_assert(id3 != NULL, "ID generation should succeed");

	test_assert(strcmp(id1, id2) != 0, "IDs should be unique");
	test_assert(strcmp(id2, id3) != 0, "IDs should be unique");
	test_assert(strcmp(id1, id3) != 0, "IDs should be unique");

	test_assert(strlen(id1) > 0, "ID should not be empty");
	test_assert(strlen(id2) > 0, "ID should not be empty");
	test_assert(strlen(id3) > 0, "ID should not be empty");

	bfree(id1);
	bfree(id2);
	bfree(id3);

	test_section_end("Channel ID Generation");
	return true;
}

/* Test Channel settings persistence */
static bool test_channel_settings_persistence(void)
{
	test_section_start("Channel Settings Persistence");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Create channel with outputs */
	stream_channel_t *channel =
		channel_manager_create_channel(manager, "Persistent Channel");
	encoding_settings_t encoding = channel_get_default_encoding();

	channel_add_output(channel, SERVICE_TWITCH, "twitch_key",
				ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(channel, SERVICE_YOUTUBE, "youtube_key",
				ORIENTATION_HORIZONTAL, &encoding);

	channel->auto_start = true;
	channel->auto_reconnect = true;
	channel->reconnect_delay_sec = 10;

	/* Save to settings */
	obs_data_t *settings = obs_data_create();
	channel_manager_save_to_settings(manager, settings);

	/* Create new manager and load settings */
	channel_manager_t *manager2 = channel_manager_create(api);
	channel_manager_load_from_settings(manager2, settings);

	test_assert(manager2->channel_count == 1,
		    "Loaded manager should have 1 channel");

	stream_channel_t *loaded = channel_manager_get_channel_at(manager2, 0);
	test_assert(loaded != NULL, "Should load profile");
	test_assert(strcmp(loaded->channel_name, "Persistent Channel") == 0,
		    "Channel name should match");
	test_assert(loaded->output_count == 2,
		    "Should load all outputs");
	test_assert(loaded->auto_start == true,
		    "Auto-start should be preserved");
	test_assert(loaded->auto_reconnect == true,
		    "Auto-reconnect should be preserved");
	test_assert(loaded->reconnect_delay_sec == 10,
		    "Reconnect delay should be preserved");

	obs_data_release(settings);
	channel_manager_destroy(manager);
	channel_manager_destroy(manager2);
	restreamer_api_destroy(api);

	test_section_end("Channel Settings Persistence");
	return true;
}

/* Test Channel duplication */
static bool test_channel_duplication(void)
{
	test_section_start("Channel Duplication");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Create original profile */
	stream_channel_t *original =
		channel_manager_create_channel(manager, "Original Channel");
	encoding_settings_t encoding = channel_get_default_encoding();

	channel_add_output(original, SERVICE_TWITCH, "original_key",
				ORIENTATION_HORIZONTAL, &encoding);
	original->auto_start = true;
	original->source_width = 1920;
	original->source_height = 1080;

	/* Duplicate profile */
	stream_channel_t *duplicate =
		channel_duplicate(original, "Duplicated Channel");
	test_assert(duplicate != NULL, "Duplication should succeed");
	test_assert(strcmp(duplicate->channel_name, "Duplicated Channel") == 0,
		    "Duplicate should have new name");
	test_assert(strcmp(duplicate->channel_id, original->channel_id) != 0,
		    "Duplicate should have different ID");
	test_assert(duplicate->output_count == 1,
		    "Duplicate should have same number of outputs");
	test_assert(duplicate->auto_start == original->auto_start,
		    "Duplicate should have same settings");
	test_assert(duplicate->source_width == original->source_width,
		    "Duplicate should have same source dimensions");

	/* Cleanup - duplicate is not managed by profile_manager */
	bfree(duplicate->channel_name);
	bfree(duplicate->channel_id);
	for (size_t i = 0; i < duplicate->output_count; i++) {
		bfree(duplicate->outputs[i].stream_key);
		if (duplicate->outputs[i].rtmp_url)
			bfree(duplicate->outputs[i].rtmp_url);
		if (duplicate->outputs[i].service_name)
			bfree(duplicate->outputs[i].service_name);
	}
	bfree(duplicate->outputs);
	bfree(duplicate);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Duplication");
	return true;
}

/* Test edge cases */
static bool test_channel_edge_cases(void)
{
	test_section_start("Channel Edge Cases");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Test NULL channel name - should reject NULL */
	stream_channel_t *channel =
		channel_manager_create_channel(manager, NULL);
	test_assert(channel == NULL,
		    "Should reject NULL name (NULL is not allowed)");

	/* Test empty channel name */
	channel = channel_manager_create_channel(manager, "");
	test_assert(channel != NULL, "Should handle empty name");

	/* Test deletion of non-existent channel */
	bool deleted =
		channel_manager_delete_channel(manager, "nonexistent_id");
	test_assert(!deleted,
		    "Deleting non-existent channel should fail gracefully");

	/* Test get non-existent channel */
	stream_channel_t *retrieved =
		channel_manager_get_channel(manager, "nonexistent_id");
	test_assert(
		retrieved == NULL,
		"Getting non-existent channel should return NULL gracefully");

	/* Test invalid output operations */
	channel = channel_manager_get_channel_at(manager, 0);
	bool removed = channel_remove_output(channel, 999);
	test_assert(!removed,
		    "Removing invalid output should fail gracefully");

	encoding_settings_t encoding = channel_get_default_encoding();
	bool updated =
		channel_update_output_encoding(channel, 999, &encoding);
	test_assert(!updated,
		    "Updating invalid output should fail gracefully");

	bool set_enabled = channel_set_output_enabled(channel, 999, true);
	test_assert(!set_enabled, "Setting invalid output enabled should "
				   "fail gracefully");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Edge Cases");
	return true;
}

/* Test builtin templates */
static bool test_builtin_templates(void)
{
	test_section_start("Builtin Templates");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Manager should have built-in templates */
	test_assert(manager->template_count > 0, "Should have built-in templates");

	/* Get template by index */
	output_template_t *tmpl = channel_manager_get_template_at(manager, 0);
	test_assert(tmpl != NULL, "Should get template by index");
	test_assert(tmpl->template_name != NULL, "Template should have name");
	test_assert(tmpl->template_id != NULL, "Template should have ID");
	test_assert(tmpl->is_builtin == true, "Built-in template flag should be set");

	/* Get template by ID */
	output_template_t *tmpl2 = channel_manager_get_template(manager, tmpl->template_id);
	test_assert(tmpl2 == tmpl, "Should get same template by ID");

	/* Cannot delete built-in template */
	bool deleted = channel_manager_delete_template(manager, tmpl->template_id);
	test_assert(!deleted, "Should not delete built-in template");

	/* Invalid index should return NULL */
	tmpl = channel_manager_get_template_at(manager, 9999);
	test_assert(tmpl == NULL, "Invalid index should return NULL");

	/* Invalid ID should return NULL */
	tmpl = channel_manager_get_template(manager, "nonexistent");
	test_assert(tmpl == NULL, "Invalid ID should return NULL");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Builtin Templates");
	return true;
}

/* Test custom templates */
static bool test_custom_templates(void)
{
	test_section_start("Custom Templates");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	size_t initial_count = manager->template_count;

	/* Create custom template */
	encoding_settings_t enc = channel_get_default_encoding();
	enc.width = 1280;
	enc.height = 720;
	enc.bitrate = 4500;

	output_template_t *custom = channel_manager_create_template(
		manager, "Custom 720p", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom != NULL, "Should create custom template");
	test_assert(custom->is_builtin == false, "Custom template should not be built-in");
	test_assert(manager->template_count == initial_count + 1, "Template count should increase");

	/* Apply template to profile */
	stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");
	bool applied = channel_apply_template(channel, custom, "my_stream_key");
	test_assert(applied, "Should apply template to profile");
	test_assert(channel->output_count == 1, "Channel should have 1 output");
	test_assert(channel->outputs[0].encoding.width == 1280, "Encoding should match template");

	/* Delete custom template */
	char *custom_id = bstrdup(custom->template_id);
	bool deleted = channel_manager_delete_template(manager, custom_id);
	test_assert(deleted, "Should delete custom template");
	test_assert(manager->template_count == initial_count, "Template count should decrease");
	bfree(custom_id);

	/* Test NULL parameters */
	custom = channel_manager_create_template(NULL, "Test", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom == NULL, "NULL manager should fail");

	custom = channel_manager_create_template(manager, NULL, SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom == NULL, "NULL name should fail");

	custom = channel_manager_create_template(manager, "Test", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, NULL);
	test_assert(custom == NULL, "NULL encoding should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Custom Templates");
	return true;
}

/* Test template persistence */
static bool test_template_persistence(void)
{
	test_section_start("Template Persistence");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Create custom template */
	encoding_settings_t enc = channel_get_default_encoding();
	enc.width = 1920;
	enc.height = 1080;
	enc.bitrate = 6000;
	enc.audio_bitrate = 192;

	channel_manager_create_template(manager, "My Custom Template", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &enc);

	/* Save templates */
	obs_data_t *settings = obs_data_create();
	channel_manager_save_templates(manager, settings);

	/* Load into new manager */
	channel_manager_t *manager2 = channel_manager_create(api);
	size_t builtin_count = manager2->template_count;

	channel_manager_load_templates(manager2, settings);
	test_assert(manager2->template_count == builtin_count + 1, "Should load custom template");

	/* Find the loaded custom template (it's after builtin ones) */
	output_template_t *loaded = channel_manager_get_template_at(manager2, builtin_count);
	test_assert(loaded != NULL, "Should find loaded template");
	test_assert(strcmp(loaded->template_name, "My Custom Template") == 0, "Template name should match");
	test_assert(loaded->encoding.width == 1920, "Encoding width should match");
	test_assert(loaded->encoding.bitrate == 6000, "Encoding bitrate should match");
	test_assert(loaded->is_builtin == false, "Loaded template should not be builtin");

	obs_data_release(settings);
	channel_manager_destroy(manager);
	channel_manager_destroy(manager2);
	restreamer_api_destroy(api);

	test_section_end("Template Persistence");
	return true;
}

/* Test backup/failover configuration */
static bool test_backup_failover_config(void)
{
	test_section_start("Backup/Failover Configuration");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Failover Test");

	encoding_settings_t enc = channel_get_default_encoding();

	/* Add primary and backup outputs */
	channel_add_output(channel, SERVICE_TWITCH, "primary_key", ORIENTATION_HORIZONTAL, &enc);
	channel_add_output(channel, SERVICE_TWITCH, "backup_key", ORIENTATION_HORIZONTAL, &enc);

	/* Set backup relationship */
	bool set = channel_set_output_backup(channel, 0, 1);
	test_assert(set, "Should set backup relationship");
	test_assert(channel->outputs[0].backup_index == 1, "Primary should point to backup");
	test_assert(channel->outputs[1].is_backup == true, "Backup should be marked as backup");
	test_assert(channel->outputs[1].primary_index == 0, "Backup should point to primary");
	test_assert(channel->outputs[1].enabled == false, "Backup should start disabled");

	/* Cannot set output as its own backup */
	set = channel_set_output_backup(channel, 0, 0);
	test_assert(!set, "Should not set output as its own backup");

	/* Remove backup relationship */
	bool removed = channel_remove_output_backup(channel, 0);
	test_assert(removed, "Should remove backup relationship");
	test_assert(channel->outputs[0].backup_index == (size_t)-1, "Primary backup index should be cleared");
	test_assert(channel->outputs[1].is_backup == false, "Backup flag should be cleared");

	/* Remove non-existent backup should fail gracefully */
	removed = channel_remove_output_backup(channel, 0);
	test_assert(!removed, "Should fail to remove non-existent backup");

	/* Invalid indices should fail */
	set = channel_set_output_backup(channel, 999, 0);
	test_assert(!set, "Invalid primary index should fail");

	set = channel_set_output_backup(channel, 0, 999);
	test_assert(!set, "Invalid backup index should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Backup/Failover Configuration");
	return true;
}

/* Test bulk operations */
static bool test_bulk_operations(void)
{
	test_section_start("Bulk Operations");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Bulk Test");

	encoding_settings_t enc = channel_get_default_encoding();

	/* Add multiple outputs */
	channel_add_output(channel, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
	channel_add_output(channel, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
	channel_add_output(channel, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);
	channel_add_output(channel, SERVICE_CUSTOM, "key4", ORIENTATION_HORIZONTAL, &enc);

	/* Bulk enable/disable (profile not active, so no API call) */
	size_t indices[] = {0, 2};
	bool result = channel_bulk_enable_outputs(channel, NULL, indices, 2, false);
	test_assert(result, "Bulk disable should succeed");
	test_assert(channel->outputs[0].enabled == false, "First output should be disabled");
	test_assert(channel->outputs[1].enabled == true, "Second output should remain enabled");
	test_assert(channel->outputs[2].enabled == false, "Third output should be disabled");

	result = channel_bulk_enable_outputs(channel, NULL, indices, 2, true);
	test_assert(result, "Bulk enable should succeed");
	test_assert(channel->outputs[0].enabled == true, "First output should be enabled");
	test_assert(channel->outputs[2].enabled == true, "Third output should be enabled");

	/* Bulk update encoding */
	encoding_settings_t new_enc = channel_get_default_encoding();
	new_enc.width = 1280;
	new_enc.height = 720;
	new_enc.bitrate = 3000;

	result = channel_bulk_update_encoding(channel, NULL, indices, 2, &new_enc);
	test_assert(result, "Bulk encoding update should succeed");
	test_assert(channel->outputs[0].encoding.width == 1280, "First dest encoding should be updated");
	test_assert(channel->outputs[2].encoding.width == 1280, "Third dest encoding should be updated");
	test_assert(channel->outputs[1].encoding.width == 0, "Second dest encoding should be unchanged");

	/* Bulk delete (in descending order internally) */
	size_t delete_indices[] = {1, 3};
	result = channel_bulk_delete_outputs(channel, delete_indices, 2);
	test_assert(result, "Bulk delete should succeed");
	test_assert(channel->output_count == 2, "Should have 2 outputs remaining");

	/* NULL checks */
	result = channel_bulk_enable_outputs(NULL, NULL, indices, 2, true);
	test_assert(!result, "NULL channel should fail");

	result = channel_bulk_enable_outputs(channel, NULL, NULL, 2, true);
	test_assert(!result, "NULL indices should fail");

	result = channel_bulk_enable_outputs(channel, NULL, indices, 0, true);
	test_assert(!result, "Zero count should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Bulk Operations");
	return true;
}

/* Test health monitoring configuration */
static bool test_health_monitoring_config(void)
{
	test_section_start("Health Monitoring Configuration");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Health Test");

	encoding_settings_t enc = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);

	/* Initial state */
	test_assert(channel->health_monitoring_enabled == false, "Health monitoring should start disabled");

	/* Enable health monitoring */
	channel_set_health_monitoring(channel, true);
	test_assert(channel->health_monitoring_enabled == true, "Health monitoring should be enabled");
	test_assert(channel->health_check_interval_sec == 30, "Default interval should be 30 seconds");
	test_assert(channel->failure_threshold == 3, "Default failure threshold should be 3");
	test_assert(channel->max_reconnect_attempts == 5, "Default max reconnect should be 5");
	test_assert(channel->outputs[0].auto_reconnect_enabled == true, "Output auto-reconnect should be enabled");

	/* Disable health monitoring */
	channel_set_health_monitoring(channel, false);
	test_assert(channel->health_monitoring_enabled == false, "Health monitoring should be disabled");
	test_assert(channel->outputs[0].auto_reconnect_enabled == false, "Output auto-reconnect should be disabled");

	/* NULL channel should not crash */
	channel_set_health_monitoring(NULL, true);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Health Monitoring Configuration");
	return true;
}

/* Test preview mode (without actual streaming) */
static bool test_preview_mode_config(void)
{
	test_section_start("Preview Mode Configuration");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Preview Test");

	/* Initial state */
	test_assert(channel->preview_mode_enabled == false, "Preview mode should start disabled");
	test_assert(channel->preview_duration_sec == 0, "Preview duration should start at 0");

	/* Test preview timeout check with no preview */
	bool timeout = channel_check_preview_timeout(channel);
	test_assert(!timeout, "Should not timeout when preview not enabled");

	/* NULL channel should not crash */
	timeout = channel_check_preview_timeout(NULL);
	test_assert(!timeout, "NULL channel should return false");

	/* Test preview functions with NULL */
	bool result = channel_start_preview(NULL, "id", 60);
	test_assert(!result, "NULL manager should fail");

	result = channel_start_preview(manager, NULL, 60);
	test_assert(!result, "NULL channel_id should fail");

	result = channel_preview_to_live(NULL, "id");
	test_assert(!result, "NULL manager should fail preview_to_live");

	result = channel_cancel_preview(NULL, "id");
	test_assert(!result, "NULL manager should fail cancel_preview");

	/* Test with non-existent channel */
	result = channel_start_preview(manager, "nonexistent", 60);
	test_assert(!result, "Non-existent channel should fail");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Preview Mode Configuration");
	return true;
}

/* Test Channel start/stop without API (error paths) */
static bool test_channel_start_stop_errors(void)
{
	test_section_start("Channel Start/Stop Error Paths");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);

	/* Test with NULL manager */
	bool result = channel_start(NULL, "id");
	test_assert(!result, "NULL manager should fail start");

	result = channel_stop(NULL, "id");
	test_assert(!result, "NULL manager should fail stop");

	/* Test with NULL channel_id */
	channel_manager_t *manager = channel_manager_create(api);
	result = channel_start(manager, NULL);
	test_assert(!result, "NULL channel_id should fail start");

	result = channel_stop(manager, NULL);
	test_assert(!result, "NULL channel_id should fail stop");

	/* Test with non-existent channel */
	result = channel_start(manager, "nonexistent");
	test_assert(!result, "Non-existent channel should fail start");

	result = channel_stop(manager, "nonexistent");
	test_assert(!result, "Non-existent channel should fail stop");

	/* Test starting profile with no outputs */
	stream_channel_t *channel = channel_manager_create_channel(manager, "Empty Channel");
	result = channel_start(manager, channel->channel_id);
	test_assert(!result, "Channel with no enabled outputs should fail start");
	test_assert(channel->status == CHANNEL_STATUS_ERROR, "Channel should be in error state");
	test_assert(channel->last_error != NULL, "Channel should have error message");

	/* Test stopping already inactive channel */
	channel->status = CHANNEL_STATUS_INACTIVE;
	result = channel_stop(manager, channel->channel_id);
	test_assert(result, "Stopping inactive channel should succeed (no-op)");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Start/Stop Error Paths");
	return true;
}

/* Test manager-level operations */
static bool test_manager_operations(void)
{
	test_section_start("Manager Operations");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Test get_count with NULL */
	size_t count = channel_manager_get_count(NULL);
	test_assert(count == 0, "NULL manager should return 0 count");

	/* Test get_active_count */
	count = channel_manager_get_active_count(NULL);
	test_assert(count == 0, "NULL manager should return 0 active count");

	count = channel_manager_get_active_count(manager);
	test_assert(count == 0, "Empty manager should have 0 active channels");

	/* Test start_all and stop_all with NULL */
	bool result = channel_manager_start_all(NULL);
	test_assert(!result, "NULL manager should fail start_all");

	result = channel_manager_stop_all(NULL);
	test_assert(!result, "NULL manager should fail stop_all");

	/* Test with empty manager (should succeed, no-op) */
	result = channel_manager_stop_all(manager);
	test_assert(result, "Empty manager stop_all should succeed");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Manager Operations");
	return true;
}

/* Test single profile save/load */
static bool test_single_profile_persistence(void)
{
	test_section_start("Single Profile Persistence");

	/* Create a profile manually (not via manager) */
	obs_data_t *settings = obs_data_create();

	/* Set profile properties */
	obs_data_set_string(settings, "name", "Saved Channel");
	obs_data_set_string(settings, "id", "test_id_123");
	obs_data_set_int(settings, "source_orientation", ORIENTATION_HORIZONTAL);
	obs_data_set_bool(settings, "auto_detect_orientation", false);
	obs_data_set_int(settings, "source_width", 1920);
	obs_data_set_int(settings, "source_height", 1080);
	obs_data_set_string(settings, "input_url", "rtmp://custom/input");
	obs_data_set_bool(settings, "auto_start", true);
	obs_data_set_bool(settings, "auto_reconnect", true);
	obs_data_set_int(settings, "reconnect_delay_sec", 15);

	/* Add outputs array */
	obs_data_array_t *dests_array = obs_data_array_create();
	obs_data_t *dest = obs_data_create();
	obs_data_set_int(dest, "service", SERVICE_TWITCH);
	obs_data_set_string(dest, "stream_key", "my_key");
	obs_data_set_int(dest, "target_orientation", ORIENTATION_HORIZONTAL);
	obs_data_set_bool(dest, "enabled", true);
	obs_data_set_int(dest, "width", 1920);
	obs_data_set_int(dest, "height", 1080);
	obs_data_set_int(dest, "bitrate", 6000);
	obs_data_array_push_back(dests_array, dest);
	obs_data_release(dest);
	obs_data_set_array(settings, "outputs", dests_array);
	obs_data_array_release(dests_array);

	/* Load profile from settings */
	stream_channel_t *channel = channel_load_from_settings(settings);
	test_assert(channel != NULL, "Should load profile from settings");
	test_assert(strcmp(channel->channel_name, "Saved Channel") == 0, "Name should match");
	test_assert(strcmp(channel->channel_id, "test_id_123") == 0, "ID should match");
	test_assert(channel->source_orientation == ORIENTATION_HORIZONTAL, "Orientation should match");
	test_assert(strcmp(channel->input_url, "rtmp://custom/input") == 0, "Input URL should match");
	test_assert(channel->auto_start == true, "Auto start should match");
	test_assert(channel->reconnect_delay_sec == 15, "Reconnect delay should match");
	test_assert(channel->output_count == 1, "Should have 1 output");
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE, "Loaded channel should be inactive");

	/* Save profile back to settings */
	obs_data_t *save_settings = obs_data_create();
	channel_save_to_settings(channel, save_settings);

	/* Verify saved values */
	test_assert(strcmp(obs_data_get_string(save_settings, "name"), "Saved Channel") == 0, "Saved name should match");
	test_assert(strcmp(obs_data_get_string(save_settings, "id"), "test_id_123") == 0, "Saved ID should match");

	/* Test NULL handling */
	stream_channel_t *null_channel = channel_load_from_settings(NULL);
	test_assert(null_channel == NULL, "NULL settings should return NULL");

	channel_save_to_settings(NULL, save_settings);  /* Should not crash */
	channel_save_to_settings(channel, NULL);  /* Should not crash */

	/* Cleanup */
	obs_data_release(settings);
	obs_data_release(save_settings);

	/* Free profile manually since it wasn't added to a manager */
	bfree(channel->channel_name);
	bfree(channel->channel_id);
	bfree(channel->input_url);
	bfree(channel->last_error);
	bfree(channel->process_reference);
	for (size_t i = 0; i < channel->output_count; i++) {
		bfree(channel->outputs[i].service_name);
		bfree(channel->outputs[i].stream_key);
		bfree(channel->outputs[i].rtmp_url);
	}
	bfree(channel->outputs);
	bfree(channel);

	test_section_end("Single Profile Persistence");
	return true;
}

/* Test Channel restart function */
static bool test_channel_restart(void)
{
	test_section_start("Channel Restart");

	/* Test NULL handling */
	bool result = channel_restart(NULL, "id");
	test_assert(!result, "NULL manager should fail restart");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	result = channel_restart(manager, NULL);
	test_assert(!result, "NULL channel_id should fail restart");

	result = channel_restart(manager, "nonexistent");
	test_assert(!result, "Non-existent channel should fail restart");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel Restart");
	return true;
}

/* Test error message handling and state transitions */
static bool test_error_state_handling(void)
{
	test_section_start("Error State Handling");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Create a profile with no outputs to trigger error state */
	stream_channel_t *channel = channel_manager_create_channel(manager, "Error Test");
	test_assert(channel != NULL, "Channel creation should succeed");
	test_assert(channel->last_error == NULL, "New channel should have no error");

	/* Try to start profile with no outputs - this should set last_error */
	bool result = channel_start(manager, channel->channel_id);
	test_assert(!result, "Starting channel with no outputs should fail");
	test_assert(channel->status == CHANNEL_STATUS_ERROR, "Channel should be in error state");
	test_assert(channel->last_error != NULL, "Channel should have error message set");

	/* Verify error message content */
	test_assert(strstr(channel->last_error, "No enabled outputs") != NULL,
		    "Error message should mention no enabled outputs");

	/* Add a output and manually set last_error to test clearing behavior */
	encoding_settings_t enc = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "test_key", ORIENTATION_HORIZONTAL, &enc);

	/* Manually set an error to verify it gets cleared on successful operations */
	bfree(channel->last_error);
	channel->last_error = bstrdup("Previous error message");
	channel->status = CHANNEL_STATUS_INACTIVE;

	test_assert(channel->last_error != NULL, "Error should be set before operation");
	test_assert(strcmp(channel->last_error, "Previous error message") == 0,
		    "Error message should match what we set");

	/* Test that stopping an inactive channel succeeds but doesn't modify state  */
	/* Note: Current implementation returns early for inactive channels and doesn't clear errors */
	/* This is expected behavior - inactive channels don't go through full stop flow */
	result = channel_stop(manager, channel->channel_id);
	test_assert(result, "Stopping inactive channel should succeed");
	/* Error is not cleared in early return path for inactive channels */
	test_assert(channel->last_error != NULL, "Error remains after stopping already-inactive channel");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Error State Handling");
	return true;
}

/* Test preview mode error clearing */
static bool test_preview_error_clearing(void)
{
	test_section_start("Preview Mode Error Clearing");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "Preview Error Test");

	/* Add a output */
	encoding_settings_t enc = channel_get_default_encoding();
	channel_add_output(channel, SERVICE_TWITCH, "test_key", ORIENTATION_HORIZONTAL, &enc);

	/* Set profile to preview status and manually set an error */
	channel->status = CHANNEL_STATUS_PREVIEW;
	channel->preview_mode_enabled = true;
	bfree(channel->last_error);
	channel->last_error = bstrdup("Preview error message");

	test_assert(channel->last_error != NULL, "Error should be set before preview_to_live");

	/* Convert preview to live - this should clear the error */
	bool result = channel_preview_to_live(manager, channel->channel_id);
	test_assert(result, "Preview to live should succeed");
	test_assert(channel->status == CHANNEL_STATUS_ACTIVE, "Channel should be active");
	test_assert(channel->last_error == NULL, "Error should be cleared on successful preview to live");
	test_assert(channel->preview_mode_enabled == false, "Preview mode should be disabled");

	/* Clean up by stopping the profile */
	channel_stop(manager, channel->channel_id);

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Preview Mode Error Clearing");
	return true;
}

/* Test Channel state validation */
static bool test_channel_state_validation(void)
{
	test_section_start("Channel State Validation");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *channel = channel_manager_create_channel(manager, "State Test");

	/* Test initial state */
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE, "New channel should be inactive");
	test_assert(channel->last_error == NULL, "New channel should have no error");

	/* Test invalid state transition for preview_to_live */
	channel->status = CHANNEL_STATUS_INACTIVE;
	bool result = channel_preview_to_live(manager, channel->channel_id);
	test_assert(!result, "preview_to_live should fail when not in preview mode");

	/* Test invalid state transition for cancel_preview */
	result = channel_cancel_preview(manager, channel->channel_id);
	test_assert(!result, "cancel_preview should fail when not in preview mode");

	/* Test that we can query profile status */
	test_assert(channel->status == CHANNEL_STATUS_INACTIVE, "Channel should still be inactive");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Channel State Validation");
	return true;
}

/* Test NULL safety in various operations */
static bool test_null_safety(void)
{
	test_section_start("NULL Safety");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	channel_manager_t *manager = channel_manager_create(api);

	/* Test NULL channel in various functions */
	bool result = channel_add_output(NULL, SERVICE_TWITCH, "key", ORIENTATION_HORIZONTAL, NULL);
	test_assert(!result, "add_output should fail with NULL channel");

	result = channel_remove_output(NULL, 0);
	test_assert(!result, "remove_output should fail with NULL channel");

	result = channel_update_output_encoding(NULL, 0, NULL);
	test_assert(!result, "update_output_encoding should fail with NULL channel");

	result = channel_set_output_enabled(NULL, 0, true);
	test_assert(!result, "set_output_enabled should fail with NULL channel");

	/* Test NULL stream key */
	stream_channel_t *channel = channel_manager_create_channel(manager, "NULL Test");
	encoding_settings_t enc = channel_get_default_encoding();
	result = channel_add_output(channel, SERVICE_TWITCH, NULL, ORIENTATION_HORIZONTAL, &enc);
	test_assert(!result, "add_output should fail with NULL stream_key");

	/* Test channel_duplicate with NULL */
	stream_channel_t *dup = channel_duplicate(NULL, "Duplicate");
	test_assert(dup == NULL, "channel_duplicate should return NULL for NULL source");

	dup = channel_duplicate(channel, NULL);
	test_assert(dup == NULL, "channel_duplicate should return NULL for NULL name");

	/* Test channel_update_stats with NULL */
	result = channel_update_stats(NULL, api);
	test_assert(!result, "channel_update_stats should fail with NULL channel");

	result = channel_update_stats(channel, NULL);
	test_assert(!result, "channel_update_stats should fail with NULL api");

	/* Test channel_check_health with NULL */
	result = channel_check_health(NULL, api);
	test_assert(!result, "channel_check_health should fail with NULL channel");

	result = channel_check_health(channel, NULL);
	test_assert(!result, "channel_check_health should fail with NULL api");

	channel_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("NULL Safety");
	return true;
}

/* Test suite runner */
bool run_stream_channel_tests(void)
{
	test_suite_start("Stream Channel Tests");

	bool result = true;

	test_start("Channel manager lifecycle");
	result &= test_channel_manager_lifecycle();
	test_end();

	test_start("Channel creation and deletion");
	result &= test_channel_creation();
	test_end();

	test_start("Channel output management");
	result &= test_channel_outputs();
	test_end();

	test_start("Channel ID generation");
	result &= test_channel_id_generation();
	test_end();

	test_start("Channel settings persistence");
	result &= test_channel_settings_persistence();
	test_end();

	test_start("Channel duplication");
	result &= test_channel_duplication();
	test_end();

	test_start("Channel edge cases");
	result &= test_channel_edge_cases();
	test_end();

	test_start("Builtin templates");
	result &= test_builtin_templates();
	test_end();

	test_start("Custom templates");
	result &= test_custom_templates();
	test_end();

	test_start("Template persistence");
	result &= test_template_persistence();
	test_end();

	test_start("Backup/failover configuration");
	result &= test_backup_failover_config();
	test_end();

	test_start("Bulk operations");
	result &= test_bulk_operations();
	test_end();

	test_start("Health monitoring configuration");
	result &= test_health_monitoring_config();
	test_end();

	test_start("Preview mode configuration");
	result &= test_preview_mode_config();
	test_end();

	test_start("Channel start/stop error paths");
	result &= test_channel_start_stop_errors();
	test_end();

	test_start("Manager operations");
	result &= test_manager_operations();
	test_end();

	test_start("Single profile persistence");
	result &= test_single_profile_persistence();
	test_end();

	test_start("Channel restart");
	result &= test_channel_restart();
	test_end();

	test_start("Error state handling");
	result &= test_error_state_handling();
	test_end();

	test_start("Preview mode error clearing");
	result &= test_preview_error_clearing();
	test_end();

	test_start("Channel state validation");
	result &= test_channel_state_validation();
	test_end();

	test_start("NULL safety");
	result &= test_null_safety();
	test_end();

	test_suite_end("Stream Channel Tests", result);
	return result;
}
