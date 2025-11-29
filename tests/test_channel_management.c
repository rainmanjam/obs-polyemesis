/**
 * Unit Tests for Channel Management
 * Tests channel creation, deletion, output management, and memory safety
 */

#include "test_framework.h"
#include "../src/restreamer-channel.h"
#include "../src/restreamer-api.h"
#include <obs.h>

/* Mock API for testing */
static restreamer_api_t *create_mock_api(void) {
    /* For unit tests, we'll use NULL and test the logic without actual API calls */
    return NULL;
}

/* Test: Profile Manager Creation and Destruction */
static bool test_channel_manager_lifecycle(void) {
    restreamer_api_t *api = create_mock_api();

    /* Create channel manager */
    channel_manager_t *manager = channel_manager_create(api);
    ASSERT_NOT_NULL(manager, "Channel manager should be created");
    ASSERT_EQ(manager->channel_count, 0, "Initial channel count should be 0");
    ASSERT_NOT_NULL(manager->templates, "Templates should be initialized");
    ASSERT_EQ(manager->template_count, 6, "Should have 6 built-in templates");

    /* Destroy profile manager */
    channel_manager_destroy(manager);

    return true;
}

/* Test: Profile Creation */
static bool test_channel_creation(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Create channel */
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");
    ASSERT_NOT_NULL(profile, "Channel should be created");
    ASSERT_STR_EQ(channel->channel_name, "Test Channel", "Channel name should match");
    ASSERT_NOT_NULL(channel->channel_id, "Channel ID should be generated");
    ASSERT_EQ(channel->output_count, 0, "Initial output count should be 0");
    ASSERT_EQ(channel->status, CHANNEL_STATUS_INACTIVE, "Initial status should be INACTIVE");

    /* Verify profile is in manager */
    ASSERT_EQ(manager->channel_count, 1, "Manager should have 1 channel");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Profile Deletion */
static bool test_channel_deletion(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Create channels */
    stream_channel_t *channel1 = channel_manager_create_channel(manager, "Channel 1");
    stream_channel_t *channel2 = channel_manager_create_channel(manager, "Channel 2");
    stream_channel_t *channel3 = channel_manager_create_channel(manager, "Channel 3");

    ASSERT_EQ(manager->channel_count, 3, "Should have 3 profiles");

    /* Delete middle profile */
    bool deleted = channel_manager_delete_channel(manager, channel2->channel_id);
    ASSERT_TRUE(deleted, "Channel deletion should succeed");
    ASSERT_EQ(manager->channel_count, 2, "Should have 2 profiles after deletion");

    /* Verify remaining profiles */
    stream_channel_t *remaining1 = channel_manager_get_channel_at(manager, 0);
    stream_channel_t *remaining2 = channel_manager_get_channel_at(manager, 1);
    ASSERT_NOT_NULL(remaining1, "First channel should exist");
    ASSERT_NOT_NULL(remaining2, "Second channel should exist");

    /* Profiles should be profile1 and profile3 */
    bool has_profile1 = (strcmp(remaining1->channel_name, "Channel 1") == 0 ||
                        strcmp(remaining2->channel_name, "Channel 1") == 0);
    bool has_profile3 = (strcmp(remaining1->channel_name, "Channel 3") == 0 ||
                        strcmp(remaining2->channel_name, "Channel 3") == 0);

    ASSERT_TRUE(has_profile1, "Channel 1 should still exist");
    ASSERT_TRUE(has_profile3, "Channel 3 should still exist");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Output Addition */
static bool test_output_addition(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    /* Add output */
    encoding_settings_t encoding = channel_get_default_encoding();
    encoding.bitrate = 5000;
    encoding.width = 1920;
    encoding.height = 1080;

    bool added = channel_add_output(profile, SERVICE_YOUTUBE, "test-stream-key",
                                        ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_TRUE(added, "Output should be added");
    ASSERT_EQ(channel->output_count, 1, "Should have 1 output");

    /* Verify output properties */
    channel_output_t *dest = &channel->outputs[0];
    ASSERT_EQ(output->service, SERVICE_YOUTUBE, "Service should be YouTube");
    ASSERT_STR_EQ(output->stream_key, "test-stream-key", "Stream key should match");
    ASSERT_EQ(output->encoding.bitrate, 5000, "Bitrate should be 5000");
    ASSERT_EQ(output->encoding.width, 1920, "Width should be 1920");
    ASSERT_EQ(output->encoding.height, 1080, "Height should be 1080");
    ASSERT_TRUE(output->enabled, "Output should be enabled by default");

    /* Verify backup/failover initialization */
    ASSERT_FALSE(output->is_backup, "Should not be a backup");
    ASSERT_EQ(output->primary_index, (size_t)-1, "Primary index should be unset");
    ASSERT_EQ(output->backup_index, (size_t)-1, "Backup index should be unset");
    ASSERT_FALSE(output->failover_active, "Failover should not be active");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Multiple Outputs */
static bool test_multiple_outputs(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Multi-Dest Profile");

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Add multiple outputs */
    channel_add_output(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(profile, SERVICE_TWITCH, "twitch-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(profile, SERVICE_FACEBOOK, "facebook-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(channel->output_count, 3, "Should have 3 outputs");

    /* Verify each output */
    ASSERT_EQ(channel->outputs[0].service, SERVICE_YOUTUBE, "First should be YouTube");
    ASSERT_EQ(channel->outputs[1].service, SERVICE_TWITCH, "Second should be Twitch");
    ASSERT_EQ(channel->outputs[2].service, SERVICE_FACEBOOK, "Third should be Facebook");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Output Removal */
static bool test_output_removal(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Add 3 outputs */
    channel_add_output(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(profile, SERVICE_TWITCH, "twitch-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    channel_add_output(profile, SERVICE_FACEBOOK, "facebook-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(channel->output_count, 3, "Should have 3 outputs");

    /* Remove middle output */
    bool removed = channel_remove_output(profile, 1);
    ASSERT_TRUE(removed, "Output removal should succeed");
    ASSERT_EQ(channel->output_count, 2, "Should have 2 outputs after removal");

    /* Verify remaining outputs */
    ASSERT_EQ(channel->outputs[0].service, SERVICE_YOUTUBE, "First should still be YouTube");
    ASSERT_EQ(channel->outputs[1].service, SERVICE_FACEBOOK, "Second should now be Facebook");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Enable/Disable Output */
static bool test_output_enable_disable(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    encoding_settings_t encoding = channel_get_default_encoding();
    channel_add_output(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_TRUE(channel->outputs[0].enabled, "Output should be enabled initially");

    /* Disable output */
    bool result = channel_set_output_enabled(profile, 0, false);
    ASSERT_TRUE(result, "Disable should succeed");
    ASSERT_FALSE(channel->outputs[0].enabled, "Output should be disabled");

    /* Re-enable output */
    result = channel_set_output_enabled(profile, 0, true);
    ASSERT_TRUE(result, "Enable should succeed");
    ASSERT_TRUE(channel->outputs[0].enabled, "Output should be enabled");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Encoding Settings Update */
static bool test_encoding_update(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    encoding_settings_t encoding = channel_get_default_encoding();
    encoding.bitrate = 5000;

    channel_add_output(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(channel->outputs[0].encoding.bitrate, 5000, "Initial bitrate should be 5000");

    /* Update encoding */
    encoding_settings_t new_encoding = encoding;
    new_encoding.bitrate = 8000;
    new_encoding.width = 2560;
    new_encoding.height = 1440;

    bool updated = channel_update_output_encoding(profile, 0, &new_encoding);
    ASSERT_TRUE(updated, "Encoding update should succeed");

    /* Verify updated values */
    ASSERT_EQ(channel->outputs[0].encoding.bitrate, 8000, "Bitrate should be updated to 8000");
    ASSERT_EQ(channel->outputs[0].encoding.width, 2560, "Width should be updated to 2560");
    ASSERT_EQ(channel->outputs[0].encoding.height, 1440, "Height should be updated to 1440");

    channel_manager_destroy(manager);
    return true;
}

/* Test: Null Pointer Safety */
static bool test_null_pointer_safety(void) {
    /* Test NULL channel manager destruction */
    channel_manager_destroy(NULL); /* Should not crash */

    /* Test NULL channel creation */
    stream_channel_t *channel = channel_manager_create_channel(NULL, "Test");
    ASSERT_NULL(profile, "Should return NULL for NULL manager");

    /* Test NULL channel deletion */
    bool deleted = channel_manager_delete_channel(NULL, "test-id");
    ASSERT_FALSE(deleted, "Should return false for NULL manager");

    /* Test NULL output addition */
    bool added = channel_add_output(NULL, SERVICE_YOUTUBE, "key",
                                        ORIENTATION_HORIZONTAL, NULL);
    ASSERT_FALSE(added, "Should return false for NULL channel");

    return true;
}

/* Test: Boundary Conditions */
static bool test_boundary_conditions(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    encoding_settings_t encoding = channel_get_default_encoding();

    /* Test invalid output index */
    bool removed = channel_remove_output(profile, 999);
    ASSERT_FALSE(removed, "Should fail to remove non-existent output");

    bool enabled = channel_set_output_enabled(profile, 999, false);
    ASSERT_FALSE(enabled, "Should fail to enable/disable non-existent output");

    bool updated = channel_update_output_encoding(profile, 999, &encoding);
    ASSERT_FALSE(updated, "Should fail to update non-existent output");

    /* Test removing from empty profile */
    removed = channel_remove_output(profile, 0);
    ASSERT_FALSE(removed, "Should fail to remove from empty profile");

    channel_manager_destroy(manager);
    return true;
}

BEGIN_TEST_SUITE("Channel Management")
    RUN_TEST(test_channel_manager_lifecycle, "Channel Manager Lifecycle");
    RUN_TEST(test_channel_creation, "Channel Creation");
    RUN_TEST(test_channel_deletion, "Channel Deletion");
    RUN_TEST(test_output_addition, "Output Addition");
    RUN_TEST(test_multiple_outputs, "Multiple Outputs");
    RUN_TEST(test_output_removal, "Output Removal");
    RUN_TEST(test_output_enable_disable, "Enable/Disable Output");
    RUN_TEST(test_encoding_update, "Encoding Settings Update");
    RUN_TEST(test_null_pointer_safety, "Null Pointer Safety");
    RUN_TEST(test_boundary_conditions, "Boundary Conditions");
END_TEST_SUITE()
