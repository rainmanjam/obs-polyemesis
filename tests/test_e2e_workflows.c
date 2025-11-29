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
	channel_manager_t *manager = channel_manager_create(api);

	// Step 1: Create profile
	stream_channel_t *profile = channel_manager_create_channel(
		manager, "E2E Test Channel");
	ASSERT_NOT_NULL(profile, "Step 1: Create profile");

	// Step 2: Add multiple outputs
	encoding_settings_t encoding = channel_get_default_encoding();

	bool added1 =
		channel_add_output(profile, SERVICE_YOUTUBE, "youtube-key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added1, "Step 2a: Add YouTube output");

	bool added2 =
		channel_add_output(profile, SERVICE_TWITCH, "twitch-key",
					ORIENTATION_HORIZONTAL, &encoding);
	ASSERT_TRUE(added2, "Step 2b: Add Twitch output");

	ASSERT_EQ(channel->output_count, 2, "Should have 2 outputs");

	// Step 3: Configure backup
	bool backup_set = channel_set_output_backup(profile, 0, 1);
	ASSERT_TRUE(backup_set, "Step 3: Set backup relationship");

	// Step 4: Enable outputs
	profile_enable_output(profile, 0, true);
	profile_enable_output(profile, 1, true);
	ASSERT_TRUE(channel->outputs[0].enabled, "Step 4a: Enable primary");
	ASSERT_TRUE(channel->outputs[1].enabled, "Step 4b: Enable backup");

	// Step 5: Simulate failure and failover
	channel_trigger_failover(profile, api, 0);
	ASSERT_TRUE(channel->outputs[0].failover_active,
		    "Step 5: Failover activated");

	// Step 6: Restore primary
	channel_restore_primary(profile, api, 0);
	ASSERT_FALSE(channel->outputs[0].failover_active,
		     "Step 6: Primary restored");

	// Step 7: Cleanup
	channel_manager_delete_channel(manager, channel->channel_id);

	channel_manager_destroy(manager);
	return true;
}

/* E2E Test 2: Failover Workflow */
static bool test_failover_workflow(void)
{
	restreamer_api_t *api = NULL;
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Failover Workflow");

	encoding_settings_t encoding = channel_get_default_encoding();

	// Setup: Primary and backup outputs
	channel_add_output(profile, SERVICE_YOUTUBE, "primary",
				ORIENTATION_HORIZONTAL, &encoding);
	channel_add_output(profile, SERVICE_YOUTUBE, "backup",
				ORIENTATION_HORIZONTAL, &encoding);
	channel_set_output_backup(profile, 0, 1);

	// Workflow: Health check → Failure → Failover
	channel_set_health_monitoring(profile, 0, true, 30);

	// Simulate health check failures
	channel->outputs[0].consecutive_failures = 3;

	// Auto-failover check
	channel_check_failover(profile, api);
	ASSERT_TRUE(channel->outputs[0].failover_active,
		    "Failover should activate after health failures");

	// Test backup is now primary
	ASSERT_FALSE(channel->outputs[1].failover_active,
		     "Backup should not have failover flag");

	channel_manager_destroy(manager);
	return true;
}

/* E2E Test 3: Preview to Live Workflow */
static bool test_preview_to_live_workflow(void)
{
	restreamer_api_t *api = NULL;
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Preview Workflow");

	encoding_settings_t encoding = channel_get_default_encoding();
	channel_add_output(profile, SERVICE_YOUTUBE, "preview-test",
				ORIENTATION_HORIZONTAL, &encoding);

	// Workflow: Start preview → Check status → Convert to live
	bool preview_started = stream_channel_start_preview(profile, 0, 60);
	ASSERT_TRUE(preview_started, "Preview should start");

	// Check preview status
	ASSERT_EQ(channel->status, CHANNEL_STATUS_PREVIEW,
		  "Should be in preview mode");

	// Verify timeout was set
	ASSERT_TRUE(channel->outputs[0].preview_timeout > 0,
		    "Preview timeout should be set");

	// Convert to live
	bool converted = stream_channel_preview_to_live(profile, 0);
	ASSERT_TRUE(converted, "Should convert to live");

	// Status should change
	// Note: Status may depend on other outputs too
	ASSERT_TRUE(channel->outputs[0].preview_timeout == 0,
		    "Preview timeout cleared after conversion");

	channel_manager_destroy(manager);
	return true;
}

/* E2E Test 4: Bulk Operations Workflow */
static bool test_bulk_operations_workflow(void)
{
	restreamer_api_t *api = NULL;
	channel_manager_t *manager = channel_manager_create(api);
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Bulk Ops");

	encoding_settings_t encoding = channel_get_default_encoding();

	// Add 5 outputs
	for (int i = 0; i < 5; i++) {
		char key[32];
		snprintf(key, sizeof(key), "dest-%d", i);
		channel_add_output(profile, SERVICE_YOUTUBE, key,
					ORIENTATION_HORIZONTAL, &encoding);
	}

	ASSERT_EQ(channel->output_count, 5, "Should have 5 outputs");

	// Bulk enable
	size_t indices[] = {0, 1, 2, 3, 4};
	bool enabled =
		channel_bulk_enable_outputs(profile, indices, 5, true);
	ASSERT_TRUE(enabled, "Bulk enable should succeed");

	// Verify all enabled
	for (size_t i = 0; i < 5; i++) {
		ASSERT_TRUE(channel->outputs[i].enabled,
			    "All outputs should be enabled");
	}

	// Bulk disable
	bool disabled =
		channel_bulk_enable_outputs(profile, indices, 5, false);
	ASSERT_TRUE(disabled, "Bulk disable should succeed");

	// Verify all disabled
	for (size_t i = 0; i < 5; i++) {
		ASSERT_FALSE(channel->outputs[i].enabled,
			     "All outputs should be disabled");
	}

	// Bulk delete
	bool deleted = channel_bulk_delete_outputs(profile, indices, 5);
	ASSERT_TRUE(deleted, "Bulk delete should succeed");
	ASSERT_EQ(channel->output_count, 0, "All outputs deleted");

	channel_manager_destroy(manager);
	return true;
}

/* E2E Test 5: Template Application Workflow */
static bool test_template_application_workflow(void)
{
	restreamer_api_t *api = NULL;
	channel_manager_t *manager = channel_manager_create(api);

	// Load built-in templates
	profile_manager_load_builtin_templates(manager);
	ASSERT_TRUE(manager->template_count > 0, "Templates should be loaded");

	// Create profile
	stream_channel_t *profile =
		channel_manager_create_channel(manager, "Template Test");
	channel_add_output(profile, SERVICE_YOUTUBE, "template-dest",
				ORIENTATION_HORIZONTAL, NULL);

	// Find and apply YouTube 1080p60 template
	output_template_t *template = NULL;
	for (size_t i = 0; i < manager->template_count; i++) {
		if (strcmp(manager->templates[i].template_id,
			   "youtube-1080p60") == 0) {
			template = &manager->templates[i];
			break;
		}
	}

	ASSERT_NOT_NULL(template, "YouTube 1080p60 template should exist");

	// Apply template
	bool applied = channel_apply_template(profile, 0, template);
	ASSERT_TRUE(applied, "Template application should succeed");

	// Verify encoding settings match template
	ASSERT_EQ(channel->outputs[0].encoding.width, 1920,
		  "Width should match template");
	ASSERT_EQ(channel->outputs[0].encoding.height, 1080,
		  "Height should match template");
	ASSERT_EQ(channel->outputs[0].encoding.fps_num, 60,
		  "FPS should match template");
	ASSERT_EQ(channel->outputs[0].encoding.bitrate, 6000,
		  "Bitrate should match template");

	channel_manager_destroy(manager);
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
