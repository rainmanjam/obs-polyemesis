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

#include "restreamer-output-profile.h"
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

/* Test profile manager creation and destruction */
static bool test_profile_manager_lifecycle(void)
{
	test_section_start("Profile Manager Lifecycle");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	test_assert(api != NULL, "API creation should succeed");

	profile_manager_t *manager = profile_manager_create(api);
	test_assert(manager != NULL, "Manager creation should succeed");
	test_assert(manager->api == api, "Manager should reference API");
	test_assert(manager->profile_count == 0,
		    "New manager should have no profiles");
	test_assert(manager->profiles == NULL,
		    "New manager should have NULL profiles array");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Manager Lifecycle");
	return true;
}

/* Test profile creation and deletion */
static bool test_profile_creation(void)
{
	test_section_start("Profile Creation");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);

	/* Create first profile */
	output_profile_t *profile1 =
		profile_manager_create_profile(manager, "Test Profile 1");
	test_assert(profile1 != NULL, "Profile creation should succeed");
	test_assert(profile1->profile_name != NULL,
		    "Profile should have name");
	test_assert(strcmp(profile1->profile_name, "Test Profile 1") == 0,
		    "Profile name should match");
	test_assert(profile1->profile_id != NULL,
		    "Profile should have unique ID");
	test_assert(profile1->status == PROFILE_STATUS_INACTIVE,
		    "New profile should be inactive");
	test_assert(profile1->destination_count == 0,
		    "New profile should have no destinations");
	test_assert(manager->profile_count == 1,
		    "Manager should have 1 profile");

	/* Create second profile */
	output_profile_t *profile2 =
		profile_manager_create_profile(manager, "Test Profile 2");
	test_assert(profile2 != NULL,
		    "Second profile creation should succeed");
	test_assert(manager->profile_count == 2,
		    "Manager should have 2 profiles");
	test_assert(strcmp(profile1->profile_id, profile2->profile_id) != 0,
		    "Profile IDs should be unique");

	/* Get profile by index */
	output_profile_t *retrieved =
		profile_manager_get_profile_at(manager, 0);
	test_assert(retrieved == profile1,
		    "Should retrieve first profile by index");

	retrieved = profile_manager_get_profile_at(manager, 1);
	test_assert(retrieved == profile2,
		    "Should retrieve second profile by index");

	/* Get profile by ID */
	retrieved =
		profile_manager_get_profile(manager, profile1->profile_id);
	test_assert(retrieved == profile1, "Should retrieve profile by ID");

	/* Get count */
	size_t count = profile_manager_get_count(manager);
	test_assert(count == 2, "Should return correct profile count");

	/* Save profile ID before deletion to avoid use-after-free */
	char *saved_profile_id = bstrdup(profile1->profile_id);

	/* Delete profile */
	bool deleted = profile_manager_delete_profile(manager,
						       saved_profile_id);
	test_assert(deleted, "Profile deletion should succeed");
	test_assert(manager->profile_count == 1,
		    "Manager should have 1 profile after deletion");

	retrieved =
		profile_manager_get_profile(manager, saved_profile_id);
	test_assert(retrieved == NULL,
		    "Deleted profile should not be retrievable");

	bfree(saved_profile_id);

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Creation");
	return true;
}

/* Test profile destination management */
static bool test_profile_destinations(void)
{
	test_section_start("Profile Destinations");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Test Profile");

	/* Get default encoding settings */
	encoding_settings_t encoding = profile_get_default_encoding();
	test_assert(encoding.width == 0, "Default width should be 0");
	test_assert(encoding.height == 0, "Default height should be 0");
	test_assert(encoding.audio_track == 0,
		    "Default audio track should be 0 (use source settings)");

	/* Add destination */
	bool added = profile_add_destination(
		profile, SERVICE_TWITCH, "test_stream_key",
		ORIENTATION_HORIZONTAL, &encoding);
	test_assert(added, "Adding destination should succeed");
	test_assert(profile->destination_count == 1,
		    "Profile should have 1 destination");
	test_assert(profile->destinations != NULL,
		    "Destinations array should be allocated");

	profile_destination_t *dest = &profile->destinations[0];
	test_assert(dest->service == SERVICE_TWITCH,
		    "Destination service should match");
	test_assert(dest->stream_key != NULL,
		    "Destination should have stream key");
	test_assert(strcmp(dest->stream_key, "test_stream_key") == 0,
		    "Stream key should match");
	test_assert(dest->target_orientation == ORIENTATION_HORIZONTAL,
		    "Orientation should match");
	test_assert(dest->enabled == true,
		    "New destination should be enabled");

	/* Add second destination */
	added = profile_add_destination(profile, SERVICE_YOUTUBE,
					"youtube_key", ORIENTATION_HORIZONTAL,
					&encoding);
	test_assert(added, "Adding second destination should succeed");
	test_assert(profile->destination_count == 2,
		    "Profile should have 2 destinations");

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

	bool updated = profile_update_destination_encoding(profile, 0,
							   &new_encoding);
	test_assert(updated, "Updating encoding should succeed");
	test_assert(profile->destinations[0].encoding.width == 1920,
		    "Width should be updated");
	test_assert(profile->destinations[0].encoding.bitrate == 6000,
		    "Bitrate should be updated");

	/* Enable/disable destination */
	bool set_enabled = profile_set_destination_enabled(profile, 0, false);
	test_assert(set_enabled, "Disabling destination should succeed");
	test_assert(profile->destinations[0].enabled == false,
		    "Destination should be disabled");

	set_enabled = profile_set_destination_enabled(profile, 0, true);
	test_assert(set_enabled, "Enabling destination should succeed");
	test_assert(profile->destinations[0].enabled == true,
		    "Destination should be enabled");

	/* Remove destination */
	bool removed = profile_remove_destination(profile, 0);
	test_assert(removed, "Removing destination should succeed");
	test_assert(profile->destination_count == 1,
		    "Profile should have 1 destination after removal");
	test_assert(profile->destinations[0].service == SERVICE_YOUTUBE,
		    "Remaining destination should be YouTube");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Destinations");
	return true;
}

/* Test profile ID generation */
static bool test_profile_id_generation(void)
{
	test_section_start("Profile ID Generation");

	/* Generate multiple IDs and ensure they're unique */
	char *id1 = profile_generate_id();
	char *id2 = profile_generate_id();
	char *id3 = profile_generate_id();

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

	test_section_end("Profile ID Generation");
	return true;
}

/* Test profile settings persistence */
static bool test_profile_settings_persistence(void)
{
	test_section_start("Profile Settings Persistence");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);

	/* Create profile with destinations */
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Persistent Profile");
	encoding_settings_t encoding = profile_get_default_encoding();

	profile_add_destination(profile, SERVICE_TWITCH, "twitch_key",
				ORIENTATION_HORIZONTAL, &encoding);
	profile_add_destination(profile, SERVICE_YOUTUBE, "youtube_key",
				ORIENTATION_HORIZONTAL, &encoding);

	profile->auto_start = true;
	profile->auto_reconnect = true;
	profile->reconnect_delay_sec = 10;

	/* Save to settings */
	obs_data_t *settings = obs_data_create();
	profile_manager_save_to_settings(manager, settings);

	/* Create new manager and load settings */
	profile_manager_t *manager2 = profile_manager_create(api);
	profile_manager_load_from_settings(manager2, settings);

	test_assert(manager2->profile_count == 1,
		    "Loaded manager should have 1 profile");

	output_profile_t *loaded = profile_manager_get_profile_at(manager2, 0);
	test_assert(loaded != NULL, "Should load profile");
	test_assert(strcmp(loaded->profile_name, "Persistent Profile") == 0,
		    "Profile name should match");
	test_assert(loaded->destination_count == 2,
		    "Should load all destinations");
	test_assert(loaded->auto_start == true,
		    "Auto-start should be preserved");
	test_assert(loaded->auto_reconnect == true,
		    "Auto-reconnect should be preserved");
	test_assert(loaded->reconnect_delay_sec == 10,
		    "Reconnect delay should be preserved");

	obs_data_release(settings);
	profile_manager_destroy(manager);
	profile_manager_destroy(manager2);
	restreamer_api_destroy(api);

	test_section_end("Profile Settings Persistence");
	return true;
}

/* Test profile duplication */
static bool test_profile_duplication(void)
{
	test_section_start("Profile Duplication");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);

	/* Create original profile */
	output_profile_t *original =
		profile_manager_create_profile(manager, "Original Profile");
	encoding_settings_t encoding = profile_get_default_encoding();

	profile_add_destination(original, SERVICE_TWITCH, "original_key",
				ORIENTATION_HORIZONTAL, &encoding);
	original->auto_start = true;
	original->source_width = 1920;
	original->source_height = 1080;

	/* Duplicate profile */
	output_profile_t *duplicate =
		profile_duplicate(original, "Duplicated Profile");
	test_assert(duplicate != NULL, "Duplication should succeed");
	test_assert(strcmp(duplicate->profile_name, "Duplicated Profile") == 0,
		    "Duplicate should have new name");
	test_assert(strcmp(duplicate->profile_id, original->profile_id) != 0,
		    "Duplicate should have different ID");
	test_assert(duplicate->destination_count == 1,
		    "Duplicate should have same number of destinations");
	test_assert(duplicate->auto_start == original->auto_start,
		    "Duplicate should have same settings");
	test_assert(duplicate->source_width == original->source_width,
		    "Duplicate should have same source dimensions");

	/* Cleanup - duplicate is not managed by profile_manager */
	bfree(duplicate->profile_name);
	bfree(duplicate->profile_id);
	for (size_t i = 0; i < duplicate->destination_count; i++) {
		bfree(duplicate->destinations[i].stream_key);
		if (duplicate->destinations[i].rtmp_url)
			bfree(duplicate->destinations[i].rtmp_url);
		if (duplicate->destinations[i].service_name)
			bfree(duplicate->destinations[i].service_name);
	}
	bfree(duplicate->destinations);
	bfree(duplicate);

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Duplication");
	return true;
}

/* Test edge cases */
static bool test_profile_edge_cases(void)
{
	test_section_start("Profile Edge Cases");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);

	/* Test NULL profile name - should reject NULL */
	output_profile_t *profile =
		profile_manager_create_profile(manager, NULL);
	test_assert(profile == NULL,
		    "Should reject NULL name (NULL is not allowed)");

	/* Test empty profile name */
	profile = profile_manager_create_profile(manager, "");
	test_assert(profile != NULL, "Should handle empty name");

	/* Test deletion of non-existent profile */
	bool deleted =
		profile_manager_delete_profile(manager, "nonexistent_id");
	test_assert(!deleted,
		    "Deleting non-existent profile should fail gracefully");

	/* Test get non-existent profile */
	output_profile_t *retrieved =
		profile_manager_get_profile(manager, "nonexistent_id");
	test_assert(
		retrieved == NULL,
		"Getting non-existent profile should return NULL gracefully");

	/* Test invalid destination operations */
	profile = profile_manager_get_profile_at(manager, 0);
	bool removed = profile_remove_destination(profile, 999);
	test_assert(!removed,
		    "Removing invalid destination should fail gracefully");

	encoding_settings_t encoding = profile_get_default_encoding();
	bool updated =
		profile_update_destination_encoding(profile, 999, &encoding);
	test_assert(!updated,
		    "Updating invalid destination should fail gracefully");

	bool set_enabled = profile_set_destination_enabled(profile, 999, true);
	test_assert(!set_enabled, "Setting invalid destination enabled should "
				   "fail gracefully");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Edge Cases");
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
	profile_manager_t *manager = profile_manager_create(api);

	/* Manager should have built-in templates */
	test_assert(manager->template_count > 0, "Should have built-in templates");

	/* Get template by index */
	destination_template_t *tmpl = profile_manager_get_template_at(manager, 0);
	test_assert(tmpl != NULL, "Should get template by index");
	test_assert(tmpl->template_name != NULL, "Template should have name");
	test_assert(tmpl->template_id != NULL, "Template should have ID");
	test_assert(tmpl->is_builtin == true, "Built-in template flag should be set");

	/* Get template by ID */
	destination_template_t *tmpl2 = profile_manager_get_template(manager, tmpl->template_id);
	test_assert(tmpl2 == tmpl, "Should get same template by ID");

	/* Cannot delete built-in template */
	bool deleted = profile_manager_delete_template(manager, tmpl->template_id);
	test_assert(!deleted, "Should not delete built-in template");

	/* Invalid index should return NULL */
	tmpl = profile_manager_get_template_at(manager, 9999);
	test_assert(tmpl == NULL, "Invalid index should return NULL");

	/* Invalid ID should return NULL */
	tmpl = profile_manager_get_template(manager, "nonexistent");
	test_assert(tmpl == NULL, "Invalid ID should return NULL");

	profile_manager_destroy(manager);
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
	profile_manager_t *manager = profile_manager_create(api);

	size_t initial_count = manager->template_count;

	/* Create custom template */
	encoding_settings_t enc = profile_get_default_encoding();
	enc.width = 1280;
	enc.height = 720;
	enc.bitrate = 4500;

	destination_template_t *custom = profile_manager_create_template(
		manager, "Custom 720p", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom != NULL, "Should create custom template");
	test_assert(custom->is_builtin == false, "Custom template should not be built-in");
	test_assert(manager->template_count == initial_count + 1, "Template count should increase");

	/* Apply template to profile */
	output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");
	bool applied = profile_apply_template(profile, custom, "my_stream_key");
	test_assert(applied, "Should apply template to profile");
	test_assert(profile->destination_count == 1, "Profile should have 1 destination");
	test_assert(profile->destinations[0].encoding.width == 1280, "Encoding should match template");

	/* Delete custom template */
	char *custom_id = bstrdup(custom->template_id);
	bool deleted = profile_manager_delete_template(manager, custom_id);
	test_assert(deleted, "Should delete custom template");
	test_assert(manager->template_count == initial_count, "Template count should decrease");
	bfree(custom_id);

	/* Test NULL parameters */
	custom = profile_manager_create_template(NULL, "Test", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom == NULL, "NULL manager should fail");

	custom = profile_manager_create_template(manager, NULL, SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, &enc);
	test_assert(custom == NULL, "NULL name should fail");

	custom = profile_manager_create_template(manager, "Test", SERVICE_CUSTOM, ORIENTATION_HORIZONTAL, NULL);
	test_assert(custom == NULL, "NULL encoding should fail");

	profile_manager_destroy(manager);
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
	profile_manager_t *manager = profile_manager_create(api);

	/* Create custom template */
	encoding_settings_t enc = profile_get_default_encoding();
	enc.width = 1920;
	enc.height = 1080;
	enc.bitrate = 6000;
	enc.audio_bitrate = 192;

	profile_manager_create_template(manager, "My Custom Template", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &enc);

	/* Save templates */
	obs_data_t *settings = obs_data_create();
	profile_manager_save_templates(manager, settings);

	/* Load into new manager */
	profile_manager_t *manager2 = profile_manager_create(api);
	size_t builtin_count = manager2->template_count;

	profile_manager_load_templates(manager2, settings);
	test_assert(manager2->template_count == builtin_count + 1, "Should load custom template");

	/* Find the loaded custom template (it's after builtin ones) */
	destination_template_t *loaded = profile_manager_get_template_at(manager2, builtin_count);
	test_assert(loaded != NULL, "Should find loaded template");
	test_assert(strcmp(loaded->template_name, "My Custom Template") == 0, "Template name should match");
	test_assert(loaded->encoding.width == 1920, "Encoding width should match");
	test_assert(loaded->encoding.bitrate == 6000, "Encoding bitrate should match");
	test_assert(loaded->is_builtin == false, "Loaded template should not be builtin");

	obs_data_release(settings);
	profile_manager_destroy(manager);
	profile_manager_destroy(manager2);
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
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

	encoding_settings_t enc = profile_get_default_encoding();

	/* Add primary and backup destinations */
	profile_add_destination(profile, SERVICE_TWITCH, "primary_key", ORIENTATION_HORIZONTAL, &enc);
	profile_add_destination(profile, SERVICE_TWITCH, "backup_key", ORIENTATION_HORIZONTAL, &enc);

	/* Set backup relationship */
	bool set = profile_set_destination_backup(profile, 0, 1);
	test_assert(set, "Should set backup relationship");
	test_assert(profile->destinations[0].backup_index == 1, "Primary should point to backup");
	test_assert(profile->destinations[1].is_backup == true, "Backup should be marked as backup");
	test_assert(profile->destinations[1].primary_index == 0, "Backup should point to primary");
	test_assert(profile->destinations[1].enabled == false, "Backup should start disabled");

	/* Cannot set destination as its own backup */
	set = profile_set_destination_backup(profile, 0, 0);
	test_assert(!set, "Should not set destination as its own backup");

	/* Remove backup relationship */
	bool removed = profile_remove_destination_backup(profile, 0);
	test_assert(removed, "Should remove backup relationship");
	test_assert(profile->destinations[0].backup_index == (size_t)-1, "Primary backup index should be cleared");
	test_assert(profile->destinations[1].is_backup == false, "Backup flag should be cleared");

	/* Remove non-existent backup should fail gracefully */
	removed = profile_remove_destination_backup(profile, 0);
	test_assert(!removed, "Should fail to remove non-existent backup");

	/* Invalid indices should fail */
	set = profile_set_destination_backup(profile, 999, 0);
	test_assert(!set, "Invalid primary index should fail");

	set = profile_set_destination_backup(profile, 0, 999);
	test_assert(!set, "Invalid backup index should fail");

	profile_manager_destroy(manager);
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
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

	encoding_settings_t enc = profile_get_default_encoding();

	/* Add multiple destinations */
	profile_add_destination(profile, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);
	profile_add_destination(profile, SERVICE_YOUTUBE, "key2", ORIENTATION_HORIZONTAL, &enc);
	profile_add_destination(profile, SERVICE_FACEBOOK, "key3", ORIENTATION_HORIZONTAL, &enc);
	profile_add_destination(profile, SERVICE_CUSTOM, "key4", ORIENTATION_HORIZONTAL, &enc);

	/* Bulk enable/disable (profile not active, so no API call) */
	size_t indices[] = {0, 2};
	bool result = profile_bulk_enable_destinations(profile, NULL, indices, 2, false);
	test_assert(result, "Bulk disable should succeed");
	test_assert(profile->destinations[0].enabled == false, "First destination should be disabled");
	test_assert(profile->destinations[1].enabled == true, "Second destination should remain enabled");
	test_assert(profile->destinations[2].enabled == false, "Third destination should be disabled");

	result = profile_bulk_enable_destinations(profile, NULL, indices, 2, true);
	test_assert(result, "Bulk enable should succeed");
	test_assert(profile->destinations[0].enabled == true, "First destination should be enabled");
	test_assert(profile->destinations[2].enabled == true, "Third destination should be enabled");

	/* Bulk update encoding */
	encoding_settings_t new_enc = profile_get_default_encoding();
	new_enc.width = 1280;
	new_enc.height = 720;
	new_enc.bitrate = 3000;

	result = profile_bulk_update_encoding(profile, NULL, indices, 2, &new_enc);
	test_assert(result, "Bulk encoding update should succeed");
	test_assert(profile->destinations[0].encoding.width == 1280, "First dest encoding should be updated");
	test_assert(profile->destinations[2].encoding.width == 1280, "Third dest encoding should be updated");
	test_assert(profile->destinations[1].encoding.width == 0, "Second dest encoding should be unchanged");

	/* Bulk delete (in descending order internally) */
	size_t delete_indices[] = {1, 3};
	result = profile_bulk_delete_destinations(profile, delete_indices, 2);
	test_assert(result, "Bulk delete should succeed");
	test_assert(profile->destination_count == 2, "Should have 2 destinations remaining");

	/* NULL checks */
	result = profile_bulk_enable_destinations(NULL, NULL, indices, 2, true);
	test_assert(!result, "NULL profile should fail");

	result = profile_bulk_enable_destinations(profile, NULL, NULL, 2, true);
	test_assert(!result, "NULL indices should fail");

	result = profile_bulk_enable_destinations(profile, NULL, indices, 0, true);
	test_assert(!result, "Zero count should fail");

	profile_manager_destroy(manager);
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
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(manager, "Health Test");

	encoding_settings_t enc = profile_get_default_encoding();
	profile_add_destination(profile, SERVICE_TWITCH, "key1", ORIENTATION_HORIZONTAL, &enc);

	/* Initial state */
	test_assert(profile->health_monitoring_enabled == false, "Health monitoring should start disabled");

	/* Enable health monitoring */
	profile_set_health_monitoring(profile, true);
	test_assert(profile->health_monitoring_enabled == true, "Health monitoring should be enabled");
	test_assert(profile->health_check_interval_sec == 30, "Default interval should be 30 seconds");
	test_assert(profile->failure_threshold == 3, "Default failure threshold should be 3");
	test_assert(profile->max_reconnect_attempts == 5, "Default max reconnect should be 5");
	test_assert(profile->destinations[0].auto_reconnect_enabled == true, "Destination auto-reconnect should be enabled");

	/* Disable health monitoring */
	profile_set_health_monitoring(profile, false);
	test_assert(profile->health_monitoring_enabled == false, "Health monitoring should be disabled");
	test_assert(profile->destinations[0].auto_reconnect_enabled == false, "Destination auto-reconnect should be disabled");

	/* NULL profile should not crash */
	profile_set_health_monitoring(NULL, true);

	profile_manager_destroy(manager);
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
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(manager, "Preview Test");

	/* Initial state */
	test_assert(profile->preview_mode_enabled == false, "Preview mode should start disabled");
	test_assert(profile->preview_duration_sec == 0, "Preview duration should start at 0");

	/* Test preview timeout check with no preview */
	bool timeout = output_profile_check_preview_timeout(profile);
	test_assert(!timeout, "Should not timeout when preview not enabled");

	/* NULL profile should not crash */
	timeout = output_profile_check_preview_timeout(NULL);
	test_assert(!timeout, "NULL profile should return false");

	/* Test preview functions with NULL */
	bool result = output_profile_start_preview(NULL, "id", 60);
	test_assert(!result, "NULL manager should fail");

	result = output_profile_start_preview(manager, NULL, 60);
	test_assert(!result, "NULL profile_id should fail");

	result = output_profile_preview_to_live(NULL, "id");
	test_assert(!result, "NULL manager should fail preview_to_live");

	result = output_profile_cancel_preview(NULL, "id");
	test_assert(!result, "NULL manager should fail cancel_preview");

	/* Test with non-existent profile */
	result = output_profile_start_preview(manager, "nonexistent", 60);
	test_assert(!result, "Non-existent profile should fail");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Preview Mode Configuration");
	return true;
}

/* Test profile start/stop without API (error paths) */
static bool test_profile_start_stop_errors(void)
{
	test_section_start("Profile Start/Stop Error Paths");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);

	/* Test with NULL manager */
	bool result = output_profile_start(NULL, "id");
	test_assert(!result, "NULL manager should fail start");

	result = output_profile_stop(NULL, "id");
	test_assert(!result, "NULL manager should fail stop");

	/* Test with NULL profile_id */
	profile_manager_t *manager = profile_manager_create(api);
	result = output_profile_start(manager, NULL);
	test_assert(!result, "NULL profile_id should fail start");

	result = output_profile_stop(manager, NULL);
	test_assert(!result, "NULL profile_id should fail stop");

	/* Test with non-existent profile */
	result = output_profile_start(manager, "nonexistent");
	test_assert(!result, "Non-existent profile should fail start");

	result = output_profile_stop(manager, "nonexistent");
	test_assert(!result, "Non-existent profile should fail stop");

	/* Test starting profile with no destinations */
	output_profile_t *profile = profile_manager_create_profile(manager, "Empty Profile");
	result = output_profile_start(manager, profile->profile_id);
	test_assert(!result, "Profile with no enabled destinations should fail start");
	test_assert(profile->status == PROFILE_STATUS_ERROR, "Profile should be in error state");
	test_assert(profile->last_error != NULL, "Profile should have error message");

	/* Test stopping already inactive profile */
	profile->status = PROFILE_STATUS_INACTIVE;
	result = output_profile_stop(manager, profile->profile_id);
	test_assert(result, "Stopping inactive profile should succeed (no-op)");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Start/Stop Error Paths");
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
	profile_manager_t *manager = profile_manager_create(api);

	/* Test get_count with NULL */
	size_t count = profile_manager_get_count(NULL);
	test_assert(count == 0, "NULL manager should return 0 count");

	/* Test get_active_count */
	count = profile_manager_get_active_count(NULL);
	test_assert(count == 0, "NULL manager should return 0 active count");

	count = profile_manager_get_active_count(manager);
	test_assert(count == 0, "Empty manager should have 0 active profiles");

	/* Test start_all and stop_all with NULL */
	bool result = profile_manager_start_all(NULL);
	test_assert(!result, "NULL manager should fail start_all");

	result = profile_manager_stop_all(NULL);
	test_assert(!result, "NULL manager should fail stop_all");

	/* Test with empty manager (should succeed, no-op) */
	result = profile_manager_stop_all(manager);
	test_assert(result, "Empty manager stop_all should succeed");

	profile_manager_destroy(manager);
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
	obs_data_set_string(settings, "name", "Saved Profile");
	obs_data_set_string(settings, "id", "test_id_123");
	obs_data_set_int(settings, "source_orientation", ORIENTATION_HORIZONTAL);
	obs_data_set_bool(settings, "auto_detect_orientation", false);
	obs_data_set_int(settings, "source_width", 1920);
	obs_data_set_int(settings, "source_height", 1080);
	obs_data_set_string(settings, "input_url", "rtmp://custom/input");
	obs_data_set_bool(settings, "auto_start", true);
	obs_data_set_bool(settings, "auto_reconnect", true);
	obs_data_set_int(settings, "reconnect_delay_sec", 15);

	/* Add destinations array */
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
	obs_data_set_array(settings, "destinations", dests_array);
	obs_data_array_release(dests_array);

	/* Load profile from settings */
	output_profile_t *profile = profile_load_from_settings(settings);
	test_assert(profile != NULL, "Should load profile from settings");
	test_assert(strcmp(profile->profile_name, "Saved Profile") == 0, "Name should match");
	test_assert(strcmp(profile->profile_id, "test_id_123") == 0, "ID should match");
	test_assert(profile->source_orientation == ORIENTATION_HORIZONTAL, "Orientation should match");
	test_assert(strcmp(profile->input_url, "rtmp://custom/input") == 0, "Input URL should match");
	test_assert(profile->auto_start == true, "Auto start should match");
	test_assert(profile->reconnect_delay_sec == 15, "Reconnect delay should match");
	test_assert(profile->destination_count == 1, "Should have 1 destination");
	test_assert(profile->status == PROFILE_STATUS_INACTIVE, "Loaded profile should be inactive");

	/* Save profile back to settings */
	obs_data_t *save_settings = obs_data_create();
	profile_save_to_settings(profile, save_settings);

	/* Verify saved values */
	test_assert(strcmp(obs_data_get_string(save_settings, "name"), "Saved Profile") == 0, "Saved name should match");
	test_assert(strcmp(obs_data_get_string(save_settings, "id"), "test_id_123") == 0, "Saved ID should match");

	/* Test NULL handling */
	output_profile_t *null_profile = profile_load_from_settings(NULL);
	test_assert(null_profile == NULL, "NULL settings should return NULL");

	profile_save_to_settings(NULL, save_settings);  /* Should not crash */
	profile_save_to_settings(profile, NULL);  /* Should not crash */

	/* Cleanup */
	obs_data_release(settings);
	obs_data_release(save_settings);

	/* Free profile manually since it wasn't added to a manager */
	bfree(profile->profile_name);
	bfree(profile->profile_id);
	bfree(profile->input_url);
	bfree(profile->last_error);
	bfree(profile->process_reference);
	for (size_t i = 0; i < profile->destination_count; i++) {
		bfree(profile->destinations[i].service_name);
		bfree(profile->destinations[i].stream_key);
		bfree(profile->destinations[i].rtmp_url);
	}
	bfree(profile->destinations);
	bfree(profile);

	test_section_end("Single Profile Persistence");
	return true;
}

/* Test profile restart function */
static bool test_profile_restart(void)
{
	test_section_start("Profile Restart");

	/* Test NULL handling */
	bool result = profile_restart(NULL, "id");
	test_assert(!result, "NULL manager should fail restart");

	restreamer_connection_t conn = {
		.host = "localhost",
		.port = 8080,
		.username = "test",
		.password = "test",
		.use_https = false,
	};

	restreamer_api_t *api = restreamer_api_create(&conn);
	profile_manager_t *manager = profile_manager_create(api);

	result = profile_restart(manager, NULL);
	test_assert(!result, "NULL profile_id should fail restart");

	result = profile_restart(manager, "nonexistent");
	test_assert(!result, "Non-existent profile should fail restart");

	profile_manager_destroy(manager);
	restreamer_api_destroy(api);

	test_section_end("Profile Restart");
	return true;
}

/* Test suite runner */
bool run_output_profile_tests(void)
{
	test_suite_start("Output Profile Tests");

	bool result = true;

	test_start("Profile manager lifecycle");
	result &= test_profile_manager_lifecycle();
	test_end();

	test_start("Profile creation and deletion");
	result &= test_profile_creation();
	test_end();

	test_start("Profile destination management");
	result &= test_profile_destinations();
	test_end();

	test_start("Profile ID generation");
	result &= test_profile_id_generation();
	test_end();

	test_start("Profile settings persistence");
	result &= test_profile_settings_persistence();
	test_end();

	test_start("Profile duplication");
	result &= test_profile_duplication();
	test_end();

	test_start("Profile edge cases");
	result &= test_profile_edge_cases();
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

	test_start("Profile start/stop error paths");
	result &= test_profile_start_stop_errors();
	test_end();

	test_start("Manager operations");
	result &= test_manager_operations();
	test_end();

	test_start("Single profile persistence");
	result &= test_single_profile_persistence();
	test_end();

	test_start("Profile restart");
	result &= test_profile_restart();
	test_end();

	test_suite_end("Output Profile Tests", result);
	return result;
}
