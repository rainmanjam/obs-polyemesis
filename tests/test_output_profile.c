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

	/* Delete profile */
	bool deleted = profile_manager_delete_profile(manager,
						       profile1->profile_id);
	test_assert(deleted, "Profile deletion should succeed");
	test_assert(manager->profile_count == 1,
		    "Manager should have 1 profile after deletion");

	retrieved =
		profile_manager_get_profile(manager, profile1->profile_id);
	test_assert(retrieved == NULL,
		    "Deleted profile should not be retrievable");

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

	test_suite_end("Output Profile Tests", result);
	return result;
}
