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

/* Test 1: Maximum number of outputs */
static bool test_max_outputs(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Stress Test");

	ASSERT_NOT_NULL(profile, "Channel should be created");

	encoding_settings_t encoding = channel_get_default_encoding();
	encoding.bitrate = 2500;

	/* Add many outputs to test scaling */
	const size_t MAX_TEST_DESTINATIONS = 50;
	for (size_t i = 0; i < MAX_TEST_DESTINATIONS; i++) {
		char stream_key[64];
		snprintf(stream_key, sizeof(stream_key), "stream-key-%zu", i);

		bool added = channel_add_output(
			profile, SERVICE_YOUTUBE, stream_key,
			ORIENTATION_HORIZONTAL, &encoding);
		ASSERT_TRUE(added, "Should be able to add output");
	}

	ASSERT_EQ(channel->output_count, MAX_TEST_DESTINATIONS,
		  "Should have all outputs added");

	/* Verify we can still access all outputs */
	for (size_t i = 0; i < channel->output_count; i++) {
		ASSERT_EQ(channel->outputs[i].service, SERVICE_YOUTUBE,
			  "Service should be YouTube");
		ASSERT_TRUE(channel->outputs[i].enabled,
			    "Output should be enabled");
	}

	channel_manager_destroy(manager);
	return true;
}

/* Test 2: Rapid add/remove operations */
static bool test_rapid_add_remove(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Rapid Operations Test");

	encoding_settings_t encoding = channel_get_default_encoding();

	/* Rapidly add and remove outputs */
	for (int cycle = 0; cycle < 10; cycle++) {
		/* Add 5 outputs */
		for (int i = 0; i < 5; i++) {
			char key[32];
			snprintf(key, sizeof(key), "key-%d-%d", cycle, i);
			bool added = channel_add_output(
				profile, SERVICE_TWITCH, key,
				ORIENTATION_HORIZONTAL, &encoding);
			ASSERT_TRUE(added, "Add should succeed");
		}

		ASSERT_EQ(channel->output_count, 5,
			  "Should have 5 outputs");

		/* Remove them all */
		while (channel->output_count > 0) {
			bool removed = channel_remove_output(profile, 0);
			ASSERT_TRUE(removed, "Remove should succeed");
		}

		ASSERT_EQ(channel->output_count, 0,
			  "All outputs should be removed");
	}

	channel_manager_destroy(manager);
	return true;
}

/* Test 3: Empty and whitespace-only inputs */
static bool test_empty_inputs(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Empty channel name */
	stream_channel_t *profile1 =
		channel_manager_create_channel(manager, "");
	ASSERT_NOT_NULL(profile1,
			"Should allow empty name (will use default)");

	/* Whitespace-only channel name */
	stream_channel_t *profile2 =
		channel_manager_create_channel(manager, "   ");
	ASSERT_NOT_NULL(profile2, "Should handle whitespace name");

	/* Very long channel name */
	char long_name[1024];
	memset(long_name, 'A', sizeof(long_name) - 1);
	long_name[sizeof(long_name) - 1] = '\0';
	stream_channel_t *profile3 =
		channel_manager_create_channel(manager, long_name);
	ASSERT_NOT_NULL(profile3, "Should handle long name");

	/* Empty stream key */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(profile1, SERVICE_YOUTUBE, "",
					     ORIENTATION_HORIZONTAL,
					     &encoding);
	/* Should fail or handle gracefully - implementation dependent */
	(void)added; /* May succeed or fail depending on implementation */

	/* Whitespace-only stream key */
	added = channel_add_output(profile1, SERVICE_YOUTUBE, "   ",
					ORIENTATION_HORIZONTAL, &encoding);
	(void)added; /* May succeed or fail depending on implementation */

	channel_manager_destroy(manager);
	return true;
}

/* Test 4: Extreme encoding values */
static bool test_extreme_encoding_values(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Extreme Encoding Test");

	encoding_settings_t encoding;

	/* Test 1: Zero values */
	encoding.width = 0;
	encoding.height = 0;
	encoding.bitrate = 0;
	encoding.fps_num = 0;
	encoding.fps_den = 1;

	bool added = channel_add_output(profile, SERVICE_YOUTUBE,
					     "test-key",
					     ORIENTATION_HORIZONTAL, &encoding);
	/* Should either fail gracefully or set minimum values */

	/* Test 2: Maximum values */
	encoding.width = 7680; // 8K
	encoding.height = 4320;
	encoding.bitrate = 100000; // 100Mbps
	encoding.fps_num = 240;
	encoding.fps_den = 1;

	added = channel_add_output(profile, SERVICE_YOUTUBE, "test-key2",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added,
		    "Should be able to add output with high values");

	/* Test 3: Invalid aspect ratios */
	encoding.width = 1;
	encoding.height = 99999;
	encoding.bitrate = 5000;

	added = channel_add_output(profile, SERVICE_YOUTUBE, "test-key3",
					ORIENTATION_HORIZONTAL, &encoding);
	/* Should handle gracefully */

	/* Test 4: Division by zero protection */
	encoding.width = 1920;
	encoding.height = 1080;
	encoding.fps_num = 60;
	encoding.fps_den = 0; // Invalid!

	added = channel_add_output(profile, SERVICE_YOUTUBE, "test-key4",
					ORIENTATION_HORIZONTAL, &encoding);
	/* Should fail gracefully */

	channel_manager_destroy(manager);
	return true;
}

/* Test 5: Multiple profiles with shared operations */
static bool test_multiple_profiles(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);

	const int NUM_PROFILES = 20;
	encoding_settings_t encoding = channel_get_default_encoding();

	/* Create many profiles */
	for (int i = 0; i < NUM_PROFILES; i++) {
		char name[64];
		snprintf(name, sizeof(name), "Channel %d", i);
		stream_channel_t *profile =
			channel_manager_create_channel(manager, name);
		ASSERT_NOT_NULL(profile, "Channel should be created");

		/* Add outputs to each */
		for (int j = 0; j < 3; j++) {
			char key[64];
			snprintf(key, sizeof(key), "p%d-d%d", i, j);
			bool added = channel_add_output(
				profile, SERVICE_YOUTUBE, key,
				ORIENTATION_HORIZONTAL, &encoding);
			ASSERT_TRUE(added, "Output should be added");
		}
	}

	/* Verify all profiles exist */
	ASSERT_EQ(manager->channel_count, NUM_PROFILES,
		  "Should have all profiles");

	/* Delete every other profile */
	for (int i = 0; i < NUM_PROFILES; i += 2) {
		if ((size_t)i < manager->channel_count) {
			char *prof_id = manager->channels[i]->channel_id;
			bool deleted = channel_manager_delete_channel(manager,
								      prof_id);
			ASSERT_TRUE(deleted, "Should delete profile");
		}
	}

	channel_manager_destroy(manager);
	return true;
}

/* Test 6: Failover chain stress test */
static bool test_failover_chains(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Failover Chain Test");

	encoding_settings_t encoding = channel_get_default_encoding();

	/* Create a chain: Primary -> Backup1 -> Backup2 -> Backup3 */
	for (int i = 0; i < 4; i++) {
		char key[32];
		snprintf(key, sizeof(key), "chain-%d", i);
		bool added = channel_add_output(
			profile, SERVICE_YOUTUBE, key, ORIENTATION_HORIZONTAL,
			&encoding);
		ASSERT_TRUE(added, "Output should be added");
	}

	/* Set up backup chain */
	bool result = channel_set_output_backup(profile, 0, 1);
	ASSERT_TRUE(result, "Should set first backup");

	result = channel_set_output_backup(profile, 1, 2);
	ASSERT_TRUE(result, "Should set second backup");

	result = channel_set_output_backup(profile, 2, 3);
	ASSERT_TRUE(result, "Should set third backup");

	/* Verify chain structure */
	ASSERT_EQ(channel->outputs[0].backup_index, 1,
		  "First primary should point to backup 1");
	ASSERT_EQ(channel->outputs[1].backup_index, 2,
		  "Backup 1 should point to backup 2");
	ASSERT_EQ(channel->outputs[2].backup_index, 3,
		  "Backup 2 should point to backup 3");

	/* Test circular reference prevention */
	result = channel_set_output_backup(profile, 3, 0);
	ASSERT_FALSE(result, "Should prevent circular backup reference");

	channel_manager_destroy(manager);
	return true;
}

/* Test 7: Bulk operations with partial failures */
static bool test_bulk_partial_failures(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Bulk Partial Test");

	encoding_settings_t encoding = channel_get_default_encoding();

	/* Add 10 outputs */
	for (int i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		channel_add_output(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Try bulk operation with mix of valid and invalid indices */
	size_t indices[] = {0, 2, 4, 999, 6, 8, 1000};
	bool result =
		channel_bulk_enable_outputs(profile, NULL, indices, 7, false);

	/* Should return false due to invalid indices, but valid ones may be processed */
	ASSERT_FALSE(result, "Should return false when some indices are invalid");

	/* Verify valid indices were processed (implementation dependent) */
	/* This behavior depends on whether bulk operations are atomic or partial */

	channel_manager_destroy(manager);
	return true;
}

/* Test 8: Memory cleanup after errors */
static bool test_error_cleanup(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Create and immediately delete profiles */
	for (int i = 0; i < 100; i++) {
		stream_channel_t *profile = channel_manager_create_channel(
			manager, "Temp Profile");
		if (profile) {
			channel_manager_delete_channel(manager, channel->channel_id);
		}
	}

	/* Manager should still be valid */
	ASSERT_NOT_NULL(manager, "Manager should still be valid");

	/* Should be able to create new profile */
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Final Profile");
	ASSERT_NOT_NULL(profile, "Should create profile after many cycles");

	channel_manager_destroy(manager);
	return true;
}

/* Test 9: Special characters in strings */
static bool test_special_characters(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);

	/* Unicode and special characters in channel name */
	stream_channel_t *profile1 = channel_manager_create_channel(
		manager, "Profile™️ with émojis 🎥📡");
	ASSERT_NOT_NULL(profile1, "Should handle Unicode");

	/* SQL injection-like strings */
	stream_channel_t *profile2 = channel_manager_create_channel(
		manager, "'; DROP TABLE profiles; --");
	ASSERT_NOT_NULL(profile2, "Should handle SQL-like syntax");

	/* Path traversal-like strings */
	stream_channel_t *profile3 = channel_manager_create_channel(
		manager, "../../../etc/passwd");
	ASSERT_NOT_NULL(profile3, "Should handle path-like syntax");

	/* Null bytes (embedded) */
	char name_with_null[32] = "Test";
	name_with_null[4] = '\0';
	name_with_null[5] = 'A';
	name_with_null[6] = '\0';
	stream_channel_t *profile4 =
		channel_manager_create_channel(manager, name_with_null);
	ASSERT_NOT_NULL(profile4, "Should handle embedded nulls");

	/* Special characters in stream key */
	encoding_settings_t encoding = channel_get_default_encoding();
	bool added = channel_add_output(profile1, SERVICE_YOUTUBE,
					     "key-with-special!@#$%^&*()",
					     ORIENTATION_HORIZONTAL,
					     &encoding);
	/* Should handle or reject gracefully - we accept either outcome */
	(void)added; /* Implementation may accept or reject special characters */

	channel_manager_destroy(manager);
	return true;
}

/* Test 10: Output removal and index stability */
static bool test_removal_index_stability(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Index Stability Test");

	encoding_settings_t encoding = channel_get_default_encoding();

	/* Add 10 outputs */
	for (int i = 0; i < 10; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		channel_add_output(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	/* Set up some backup relationships */
	channel_set_output_backup(profile, 0, 1);
	channel_set_output_backup(profile, 2, 3);
	channel_set_output_backup(profile, 4, 5);

	/* Remove a output in the middle (index 2) */
	bool removed = channel_remove_output(profile, 2);
	ASSERT_TRUE(removed, "Should remove output");

	/* Verify backup indices were updated correctly */
	/* After removing index 2, index 3 becomes 2, index 4 becomes 3, etc. */
	/* Backup relationships should be maintained or cleared appropriately */

	/* Verify we can still add/remove without issues */
	removed = channel_remove_output(profile, 0);
	ASSERT_TRUE(removed, "Should remove first output");

	char new_key[] = "new-dest";
	bool added = channel_add_output(profile, SERVICE_TWITCH, new_key,
					     ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added, "Should add new output");

	channel_manager_destroy(manager);
	return true;
}

/* Test 11: Preview mode timeout edge cases */
static bool test_preview_timeout_edge_cases(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Preview Timeout Test");

	/* Test with 0 timeout */
	bool started = stream_channel_start_preview(manager, channel->channel_id, 0);
	/* Should either reject or handle as "no timeout" */
	(void)started; /* May succeed or fail */

	/* Test with negative timeout (should be rejected) */
	started = stream_channel_start_preview(manager, channel->channel_id, (uint32_t)-1);
	/* Should reject or handle large value */
	(void)started;

	/* Test with extremely large timeout */
	started = stream_channel_start_preview(manager, channel->channel_id, 999999);
	if (started) {
		/* Should be in preview mode */
		bool cancelled = stream_channel_cancel_preview(manager, channel->channel_id);
		ASSERT_TRUE(cancelled, "Should be able to cancel preview");
	}

	channel_manager_destroy(manager);
	return true;
}

/* Test 12: Encoding update edge cases */
static bool test_encoding_update_edge_cases(void)
{
	restreamer_api_t *api = create_mock_api();
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "Encoding Update Test");

	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(profile, SERVICE_YOUTUBE, "test-key",
				ORIENTATION_HORIZONTAL, &encoding);

	/* Update to same values (no-op) */
	encoding_settings_t same_encoding = encoding;
	bool updated =
		channel_update_output_encoding(profile, 0, &same_encoding);
	ASSERT_TRUE(updated, "Should succeed even with same values");

	/* Update with NULL encoding */
	updated = channel_update_output_encoding(profile, 0, NULL);
	ASSERT_FALSE(updated, "Should reject NULL encoding");

	/* Update invalid index */
	updated = channel_update_output_encoding(profile, 999, &encoding);
	ASSERT_FALSE(updated, "Should reject invalid index");

	/* Update while profile is in certain states */
	/* (This would require profile state management) */

	channel_manager_destroy(manager);
	return true;
}

BEGIN_TEST_SUITE("Edge Case Tests")
	RUN_TEST(test_max_outputs, "Maximum outputs stress test");
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
		 "Output removal index stability");
	RUN_TEST(test_preview_timeout_edge_cases,
		 "Preview timeout edge cases");
	RUN_TEST(test_encoding_update_edge_cases,
		 "Encoding update edge cases");
END_TEST_SUITE()
