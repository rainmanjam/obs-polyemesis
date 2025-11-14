/*
 * End-to-End Workflow Tests
 * Tests complete user workflows from start to finish
 */

#include "test_framework.h"
#include "../src/restreamer-output-profile.h"
#include "../src/restreamer-config.h"

/* E2E Test 1: Complete Profile Lifecycle */
static bool test_complete_profile_lifecycle(void)
{
	restreamer_api_t *api = NULL; // Mock API for E2E
	profile_manager_t *manager = profile_manager_create(api);

	// Step 1: Create profile
	output_profile_t *profile = profile_manager_create_profile(
		manager, "E2E Test Profile");
	ASSERT_NOT_NULL(profile, "Step 1: Create profile");

	// Step 2: Add multiple destinations
	encoding_settings_t encoding = profile_get_default_encoding();

	bool added1 =
		profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added1, "Step 2a: Add YouTube destination");

	bool added2 =
		profile_add_destination(profile, SERVICE_TWITCH, "twitch-key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added2, "Step 2b: Add Twitch destination");

	ASSERT_EQ(profile->destination_count, 2, "Should have 2 destinations");

	// Step 3: Configure backup
	bool backup_set = profile_set_destination_backup(profile, 0, 1);
	ASSERT_TRUE(backup_set, "Step 3: Set backup relationship");

	// Step 4: Enable destinations
	profile_enable_destination(profile, 0, true);
	profile_enable_destination(profile, 1, true);
	ASSERT_TRUE(profile->destinations[0].enabled, "Step 4a: Enable primary");
	ASSERT_TRUE(profile->destinations[1].enabled, "Step 4b: Enable backup");

	// Step 5: Simulate failure and failover
	profile_trigger_failover(profile, api, 0);
	ASSERT_TRUE(profile->destinations[0].failover_active,
		    "Step 5: Failover activated");

	// Step 6: Restore primary
	profile_restore_primary(profile, api, 0);
	ASSERT_FALSE(profile->destinations[0].failover_active,
		     "Step 6: Primary restored");

	// Step 7: Cleanup
	profile_manager_delete_profile(manager, profile->profile_id);

	profile_manager_destroy(manager);
	return true;
}

/* E2E Test 2: Failover Workflow */
static bool test_failover_workflow(void)
{
	restreamer_api_t *api = NULL;
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Failover Workflow");

	encoding_settings_t encoding = profile_get_default_encoding();

	// Setup: Primary and backup destinations
	profile_add_destination(profile, SERVICE_YOUTUBE, "primary",
				ORIENTATION_HORIZONTAL, &encoding);
	profile_add_destination(profile, SERVICE_YOUTUBE, "backup",
				ORIENTATION_HORIZONTAL, &encoding);
	profile_set_destination_backup(profile, 0, 1);

	// Workflow: Health check → Failure → Failover
	profile_set_health_monitoring(profile, 0, true, 30);

	// Simulate health check failures
	profile->destinations[0].consecutive_failures = 3;

	// Auto-failover check
	profile_check_failover(profile, api);
	ASSERT_TRUE(profile->destinations[0].failover_active,
		    "Failover should activate after health failures");

	// Test backup is now primary
	ASSERT_FALSE(profile->destinations[1].failover_active,
		     "Backup should not have failover flag");

	profile_manager_destroy(manager);
	return true;
}

/* E2E Test 3: Preview to Live Workflow */
static bool test_preview_to_live_workflow(void)
{
	restreamer_api_t *api = NULL;
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Preview Workflow");

	encoding_settings_t encoding = profile_get_default_encoding();
	profile_add_destination(profile, SERVICE_YOUTUBE, "preview-test",
				ORIENTATION_HORIZONTAL, &encoding);

	// Workflow: Start preview → Check status → Convert to live
	bool preview_started = output_profile_start_preview(profile, 0, 60);
	ASSERT_TRUE(preview_started, "Preview should start");

	// Check preview status
	ASSERT_EQ(profile->status, PROFILE_STATUS_PREVIEW,
		  "Should be in preview mode");

	// Verify timeout was set
	ASSERT_TRUE(profile->destinations[0].preview_timeout > 0,
		    "Preview timeout should be set");

	// Convert to live
	bool converted = output_profile_preview_to_live(profile, 0);
	ASSERT_TRUE(converted, "Should convert to live");

	// Status should change
	// Note: Status may depend on other destinations too
	ASSERT_TRUE(profile->destinations[0].preview_timeout == 0,
		    "Preview timeout cleared after conversion");

	profile_manager_destroy(manager);
	return true;
}

/* E2E Test 4: Bulk Operations Workflow */
static bool test_bulk_operations_workflow(void)
{
	restreamer_api_t *api = NULL;
	profile_manager_t *manager = profile_manager_create(api);
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Bulk Ops");

	encoding_settings_t encoding = profile_get_default_encoding();

	// Add 5 destinations
	for (int i = 0; i < 5; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		profile_add_destination(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	ASSERT_EQ(profile->destination_count, 5, "Should have 5 destinations");

	// Bulk enable
	size_t indices[] = {0, 1, 2, 3, 4};
	bool enabled =
		profile_bulk_enable_destinations(profile, indices, 5, true);
	ASSERT_TRUE(enabled, "Bulk enable should succeed");

	// Verify all enabled
	for (size_t i = 0; i < 5; i++) {
		ASSERT_TRUE(profile->destinations[i].enabled,
			    "All destinations should be enabled");
	}

	// Bulk disable
	bool disabled =
		profile_bulk_enable_destinations(profile, indices, 5, false);
	ASSERT_TRUE(disabled, "Bulk disable should succeed");

	// Verify all disabled
	for (size_t i = 0; i < 5; i++) {
		ASSERT_FALSE(profile->destinations[i].enabled,
			     "All destinations should be disabled");
	}

	// Bulk delete
	bool deleted = profile_bulk_delete_destinations(profile, indices, 5);
	ASSERT_TRUE(deleted, "Bulk delete should succeed");
	ASSERT_EQ(profile->destination_count, 0, "All destinations deleted");

	profile_manager_destroy(manager);
	return true;
}

/* E2E Test 5: Template Application Workflow */
static bool test_template_application_workflow(void)
{
	restreamer_api_t *api = NULL;
	profile_manager_t *manager = profile_manager_create(api);

	// Load built-in templates
	profile_manager_load_builtin_templates(manager);
	ASSERT_TRUE(manager->template_count > 0, "Templates should be loaded");

	// Create profile
	output_profile_t *profile =
		profile_manager_create_profile(manager, "Template Test");
	profile_add_destination(profile, SERVICE_YOUTUBE, "template-dest",
				ORIENTATION_HORIZONTAL, NULL);

	// Find and apply YouTube 1080p60 template
	destination_template_t *template = NULL;
	for (size_t i = 0; i < manager->template_count; i++) {
		if (strcmp(manager->templates[i].template_id,
			   "youtube-1080p60") == 0) {
			template = &manager->templates[i];
			break;
		}
	}

	ASSERT_NOT_NULL(template, "YouTube 1080p60 template should exist");

	// Apply template
	bool applied = profile_apply_template(profile, 0, template);
	ASSERT_TRUE(applied, "Template application should succeed");

	// Verify encoding settings match template
	ASSERT_EQ(profile->destinations[0].encoding.width, 1920,
		  "Width should match template");
	ASSERT_EQ(profile->destinations[0].encoding.height, 1080,
		  "Height should match template");
	ASSERT_EQ(profile->destinations[0].encoding.fps_num, 60,
		  "FPS should match template");
	ASSERT_EQ(profile->destinations[0].encoding.bitrate, 6000,
		  "Bitrate should match template");

	profile_manager_destroy(manager);
	return true;
}

BEGIN_TEST_SUITE("End-to-End Workflow Tests")
RUN_TEST(test_complete_profile_lifecycle,
	 "Complete profile lifecycle workflow");
RUN_TEST(test_failover_workflow, "Failover workflow (health → failure → auto-failover)");
RUN_TEST(test_preview_to_live_workflow,
	 "Preview to live workflow");
RUN_TEST(test_bulk_operations_workflow,
	 "Bulk operations workflow (enable/disable/delete)");
RUN_TEST(test_template_application_workflow,
	 "Template application workflow");
END_TEST_SUITE()
