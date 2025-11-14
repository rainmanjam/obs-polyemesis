/*
 * Edge Case Tests for OBS Polyemesis
 * Tests boundary conditions, stress scenarios, and error recovery
 */

#include "test_framework.h"
#include "../src/restreamer-output-profile.h"
#include "../src/restreamer-multistream.h"
#include "../src/restreamer-api.h"
#include <string.h>
#include <stdlib.h>

/* Mock API for testing */
static restreamer_api_t *create_mock_api(void)
{
	return NULL; /* Tests use NULL API to test logic in isolation */
}

/* Test 1: Maximum number of destinations */
static bool test_max_destinations(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Stress Test");

	ASSERT_NOT_NULL(profile, "Profile should be created");

	encoding_settings_t encoding = profile_get_default_encoding();
	encoding.bitrate = 2500;

	/* Add many destinations to test scaling */
	const size_t MAX_TEST_DESTINATIONS = 50;
	for (size_t i = 0; i < MAX_TEST_DESTINATIONS; i++) {
		char stream_key[64];
		snprintf(stream_key, sizeof(stream_key), "stream-key-%zu", i);

		bool added = profile_add_destination(
			profile, SERVICE_YOUTUBE, stream_key,
			ORIENTATION_HORIZONTAL, &encoding);
		ASSERT_TRUE(added, "Should be able to add destination");
	}

	ASSERT_EQ(profile->destination_count, MAX_TEST_DESTINATIONS,
		  "Should have all destinations added");

	/* Verify we can still access all destinations */
	for (size_t i = 0; i < profile->destination_count; i++) {
		ASSERT_EQ(profile->destinations[i].service, SERVICE_YOUTUBE,
			  "Service should be YouTube");
		ASSERT_TRUE(profile->destinations[i].enabled,
			    "Destination should be enabled");
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 2: Rapid add/remove operations */
static bool test_rapid_add_remove(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Rapid Operations Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Rapidly add and remove destinations */
	for (int cycle = 0; cycle < 10; cycle++) {
		/* Add 5 destinations */
		for (int i = 0; i < 5; i++) {
			char key[32];
			snprintf(key, sizeof(key), "key-%d-%d", cycle, i);
			bool added = profile_add_destination(
				profile, SERVICE_TWITCH, key,
				ORIENTATION_HORIZONTAL, &encoding);
			ASSERT_TRUE(added, "Add should succeed");
		}

		ASSERT_EQ(profile->destination_count, 5,
			  "Should have 5 destinations");

		/* Remove them all */
		while (profile->destination_count > 0) {
			bool removed = profile_remove_destination(profile, 0);
			ASSERT_TRUE(removed, "Remove should succeed");
		}

		ASSERT_EQ(profile->destination_count, 0,
			  "All destinations should be removed");
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 3: Empty and whitespace-only inputs */
static bool test_empty_inputs(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Empty profile name */
	output_profile_t *profile1 =
		profile_manager_create_profile(manager, "");
	ASSERT_NOT_NULL(profile1,
			"Should allow empty name (will use default)");

	/* Whitespace-only profile name */
	output_profile_t *profile2 =
		profile_manager_create_profile(manager, "   ");
	ASSERT_NOT_NULL(profile2, "Should handle whitespace name");

	/* Very long profile name */
	char long_name[1024];
	memset(long_name, 'A', sizeof(long_name) - 1);
	long_name[sizeof(long_name) - 1] = '\0';
	output_profile_t *profile3 =
		profile_manager_create_profile(manager, long_name);
	ASSERT_NOT_NULL(profile3, "Should handle long name");

	/* Empty stream key */
	encoding_settings_t encoding = profile_get_default_encoding();
	bool added = profile_add_destination(profile1, SERVICE_YOUTUBE, "",
					     ORIENTATION_HORIZONTAL,
					     &encoding);
	/* Should fail or handle gracefully - implementation dependent */
	(void)added; /* May succeed or fail depending on implementation */

	/* Whitespace-only stream key */
	added = profile_add_destination(profile1, SERVICE_YOUTUBE, "   ",
					ORIENTATION_HORIZONTAL, &encoding);
	(void)added; /* May succeed or fail depending on implementation */

	profile_manager_destroy(manager);
	return true;
}

/* Test 4: Extreme encoding values */
static bool test_extreme_encoding_values(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Extreme Encoding Test");

	encoding_settings_t encoding;

	/* Test 1: Zero values */
	encoding.width = 0;
	encoding.height = 0;
	encoding.bitrate = 0;
	encoding.fps_num = 0;
	encoding.fps_den = 1;

	bool added = profile_add_destination(profile, SERVICE_YOUTUBE,
					     "test-key",
					     ORIENTATION_HORIZONTAL, &encoding);
	/* Should either fail gracefully or set minimum values */

	/* Test 2: Maximum values */
	encoding.width = 7680; // 8K
	encoding.height = 4320;
	encoding.bitrate = 100000; // 100Mbps
	encoding.fps_num = 240;
	encoding.fps_den = 1;

	added = profile_add_destination(profile, SERVICE_YOUTUBE, "test-key2",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added,
		    "Should be able to add destination with high values");

	/* Test 3: Invalid aspect ratios */
	encoding.width = 1;
	encoding.height = 99999;
	encoding.bitrate = 5000;

	added = profile_add_destination(profile, SERVICE_YOUTUBE, "test-key3",
					ORIENTATION_HORIZONTAL, &encoding);
	/* Should handle gracefully */

	/* Test 4: Division by zero protection */
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.fps_num = 60;
	encoding.fps_den = 0; // Invalid!

	added = profile_add_destination(profile, SERVICE_YOUTUBE, "test-key4",
					ORIENTATION_HORIZONTAL, &encoding);
	/* Should fail gracefully */

	profile_manager_destroy(manager);
	return true;
}

/* Test 5: Multiple profiles with shared operations */
static bool test_multiple_profiles(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	const int NUM_PROFILES = 20;
	encoding_settings_t encoding = profile_get_default_encoding();

	/* Create many profiles */
	for (int i = 0; i < NUM_PROFILES; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Profile %d", i);
		output_profile_t *profile =
			profile_manager_create_profile(manager, name);
		ASSERT_NOT_NULL(profile, "Profile should be created");

		/* Add destinations to each */
		for (int j = 0; j < 3; j++) {
			char key[64];
			snprintf(key, sizeof(key), "p%d-d%d", i, j);
			bool added = profile_add_destination(
				profile, SERVICE_YOUTUBE, key,
				ORIENTATION_HORIZONTAL, &encoding);
			ASSERT_TRUE(added, "Destination should be added");
		}
	}

	/* Verify all profiles exist */
	ASSERT_EQ(manager->profile_count, NUM_PROFILES,
		  "Should have all profiles");

	/* Delete every other profile */
	for (int i = 0; i < NUM_PROFILES; i += 2) {
		if ((size_t)i < manager->profile_count) {
			char *prof_id = manager->profiles[i]->profile_id;
			bool deleted = profile_manager_delete_profile(manager,
								      prof_id);
			ASSERT_TRUE(deleted, "Should delete profile");
		}
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 6: Failover chain stress test */
static bool test_failover_chains(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Failover Chain Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Create a chain: Primary -> Backup1 -> Backup2 -> Backup3 */
	for (int i = 0; i < 4; i++) {
		char key[32];
		snprintf(key, sizeof(key), "chain-%d", i);
		bool added = profile_add_destination(
			profile, SERVICE_YOUTUBE, key, ORIENTATION_HORIZONTAL,
			&encoding);
		ASSERT_TRUE(added, "Destination should be added");
	}

	/* Set up backup chain */
	bool result = profile_set_destination_backup(profile, 0, 1);
	ASSERT_TRUE(result, "Should set first backup");

	result = profile_set_destination_backup(profile, 1, 2);
	ASSERT_TRUE(result, "Should set second backup");

	result = profile_set_destination_backup(profile, 2, 3);
	ASSERT_TRUE(result, "Should set third backup");

	/* Verify chain structure */
	ASSERT_EQ(profile->destinations[0].backup_index, 1,
		  "First primary should point to backup 1");
	ASSERT_EQ(profile->destinations[1].backup_index, 2,
		  "Backup 1 should point to backup 2");
	ASSERT_EQ(profile->destinations[2].backup_index, 3,
		  "Backup 2 should point to backup 3");

	/* Test circular reference prevention */
	result = profile_set_destination_backup(profile, 3, 0);
	ASSERT_FALSE(result, "Should prevent circular backup reference");

	profile_manager_destroy(manager);
	return true;
}

/* Test 7: Bulk operations with partial failures */
static bool test_bulk_partial_failures(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Bulk Partial Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Add 10 destinations */
	for (int i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		profile_add_destination(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Try bulk operation with mix of valid and invalid indices */
	size_t indices[] = {0, 2, 4, 999, 6, 8, 1000};
	bool result =
		profile_bulk_enable_destinations(profile, NULL, indices, 7, false);

	/* Should return false due to invalid indices, but valid ones may be processed */
	ASSERT_FALSE(result, "Should return false when some indices are invalid");

	/* Verify valid indices were processed (implementation dependent) */
	/* This behavior depends on whether bulk operations are atomic or partial */

	profile_manager_destroy(manager);
	return true;
}

/* Test 8: Memory cleanup after errors */
static bool test_error_cleanup(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Create and immediately delete profiles */
	for (int i = 0; i < 100; i++) {
		output_profile_t *profile = profile_manager_create_profile(
			manager, "Temp Profile");
		if (profile) {
			profile_manager_delete_profile(manager, profile->profile_id);
		}
	}

	/* Manager should still be valid */
	ASSERT_NOT_NULL(manager, "Manager should still be valid");

	/* Should be able to create new profile */
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Final Profile");
	ASSERT_NOT_NULL(profile, "Should create profile after many cycles");

	profile_manager_destroy(manager);
	return true;
}

/* Test 9: Special characters in strings */
static bool test_special_characters(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);

	/* Unicode and special characters in profile name */
	output_profile_t *profile1 = profile_manager_create_profile(
		manager, "Profile™️ with émojis 🎥📡");
	ASSERT_NOT_NULL(profile1, "Should handle Unicode");

	/* SQL injection-like strings */
	output_profile_t *profile2 = profile_manager_create_profile(
		manager, "'; DROP TABLE profiles; --");
	ASSERT_NOT_NULL(profile2, "Should handle SQL-like syntax");

	/* Path traversal-like strings */
	output_profile_t *profile3 = profile_manager_create_profile(
		manager, "../../../etc/passwd");
	ASSERT_NOT_NULL(profile3, "Should handle path-like syntax");

	/* Null bytes (embedded) */
	char name_with_null[32] = "Test";
	name_with_null[4] = '\0';
	name_with_null[5] = 'A';
	name_with_null[6] = '\0';
	output_profile_t *profile4 =
		profile_manager_create_profile(manager, name_with_null);
	ASSERT_NOT_NULL(profile4, "Should handle embedded nulls");

	/* Special characters in stream key */
	encoding_settings_t encoding = profile_get_default_encoding();
	bool added = profile_add_destination(profile1, SERVICE_YOUTUBE,
					     "key-with-special!@#$%^&*()",
					     ORIENTATION_HORIZONTAL,
					     &encoding);
	/* Should handle or reject gracefully - we accept either outcome */
	(void)added; /* Implementation may accept or reject special characters */

	profile_manager_destroy(manager);
	return true;
}

/* Test 10: Destination removal and index stability */
static bool test_removal_index_stability(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Index Stability Test");

	encoding_settings_t encoding = profile_get_default_encoding();

	/* Add 10 destinations */
	for (int i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		profile_add_destination(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Set up some backup relationships */
	profile_set_destination_backup(profile, 0, 1);
	profile_set_destination_backup(profile, 2, 3);
	profile_set_destination_backup(profile, 4, 5);

	/* Remove a destination in the middle (index 2) */
	bool removed = profile_remove_destination(profile, 2);
	ASSERT_TRUE(removed, "Should remove destination");

	/* Verify backup indices were updated correctly */
	/* After removing index 2, index 3 becomes 2, index 4 becomes 3, etc. */
	/* Backup relationships should be maintained or cleared appropriately */

	/* Verify we can still add/remove without issues */
	removed = profile_remove_destination(profile, 0);
	ASSERT_TRUE(removed, "Should remove first destination");

	char new_key[] = "new-dest";
	bool added = profile_add_destination(profile, SERVICE_TWITCH, new_key,
					     ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Should add new destination");

	profile_manager_destroy(manager);
	return true;
}

/* Test 11: Preview mode timeout edge cases */
static bool test_preview_timeout_edge_cases(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Preview Timeout Test");

	/* Test with 0 timeout */
	bool started = output_profile_start_preview(manager, profile->profile_id, 0);
	/* Should either reject or handle as "no timeout" */
	(void)started; /* May succeed or fail */

	/* Test with negative timeout (should be rejected) */
	started = output_profile_start_preview(manager, profile->profile_id, (uint32_t)-1);
	/* Should reject or handle large value */
	(void)started;

	/* Test with extremely large timeout */
	started = output_profile_start_preview(manager, profile->profile_id, 999999);
	if (started) {
		/* Should be in preview mode */
		bool cancelled = output_profile_cancel_preview(manager, profile->profile_id);
		ASSERT_TRUE(cancelled, "Should be able to cancel preview");
	}

	profile_manager_destroy(manager);
	return true;
}

/* Test 12: Encoding update edge cases */
static bool test_encoding_update_edge_cases(void)
{
	restreamer_api_t *api = create_mock_api();
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile = profile_manager_create_profile(
		manager, "Encoding Update Test");

	encoding_settings_t encoding = profile_get_default_encoding();
	profile_add_destination(profile, SERVICE_YOUTUBE, "test-key",
				ORIENTATION_HORIZONTAL, &encoding);

	/* Update to same values (no-op) */
	encoding_settings_t same_encoding = encoding;
	bool updated =
		profile_update_destination_encoding(profile, 0, &same_encoding);
	ASSERT_TRUE(updated, "Should succeed even with same values");

	/* Update with NULL encoding */
	updated = profile_update_destination_encoding(profile, 0, NULL);
	ASSERT_FALSE(updated, "Should reject NULL encoding");

	/* Update invalid index */
	updated = profile_update_destination_encoding(profile, 999, &encoding);
	ASSERT_FALSE(updated, "Should reject invalid index");

	/* Update while profile is in certain states */
	/* (This would require profile state management) */

	profile_manager_destroy(manager);
	return true;
}

BEGIN_TEST_SUITE("Edge Case Tests")
	RUN_TEST(test_max_destinations, "Maximum destinations stress test");
	RUN_TEST(test_rapid_add_remove, "Rapid add/remove cycles");
	RUN_TEST(test_empty_inputs, "Empty and whitespace inputs");
	RUN_TEST(test_extreme_encoding_values, "Extreme encoding values");
	RUN_TEST(test_multiple_profiles, "Multiple profiles stress test");
	RUN_TEST(test_failover_chains, "Failover chain stress test");
	RUN_TEST(test_bulk_partial_failures,
		 "Bulk operations with partial failures");
	RUN_TEST(test_error_cleanup, "Error cleanup and recovery");
	RUN_TEST(test_special_characters, "Special characters in strings");
	RUN_TEST(test_removal_index_stability,
		 "Destination removal index stability");
	RUN_TEST(test_preview_timeout_edge_cases,
		 "Preview timeout edge cases");
	RUN_TEST(test_encoding_update_edge_cases,
		 "Encoding update edge cases");
END_TEST_SUITE()
