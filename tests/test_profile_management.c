/**
 * Unit Tests for Profile Management
 * Tests profile creation, deletion, destination management, and memory safety
 */

#include "test_framework.h"
#include "../src/restreamer-output-profile.h"
#include "../src/restreamer-api.h"
#include <obs.h>

/* Mock API for testing */
static restreamer_api_t *create_mock_api(void) {
    /* For unit tests, we'll use NULL and test the logic without actual API calls */
    return NULL;
}

/* Test: Profile Manager Creation and Destruction */
static bool test_profile_manager_lifecycle(void) {
    restreamer_api_t *api = create_mock_api();

    /* Create profile manager */
    profile_manager_t *manager = profile_manager_create(api);
    ASSERT_NOT_NULL(manager, "Profile manager should be created");
    ASSERT_EQ(manager->profile_count, 0, "Initial profile count should be 0");
    ASSERT_NOT_NULL(manager->templates, "Templates should be initialized");
    ASSERT_EQ(manager->template_count, 6, "Should have 6 built-in templates");

    /* Destroy profile manager */
    profile_manager_destroy(manager);

    return true;
}

/* Test: Profile Creation */
static bool test_profile_creation(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);

    /* Create profile */
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");
    ASSERT_NOT_NULL(profile, "Profile should be created");
    ASSERT_STR_EQ(profile->profile_name, "Test Profile", "Profile name should match");
    ASSERT_NOT_NULL(profile->profile_id, "Profile ID should be generated");
    ASSERT_EQ(profile->destination_count, 0, "Initial destination count should be 0");
    ASSERT_EQ(profile->status, PROFILE_STATUS_INACTIVE, "Initial status should be INACTIVE");

    /* Verify profile is in manager */
    ASSERT_EQ(manager->profile_count, 1, "Manager should have 1 profile");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Profile Deletion */
static bool test_profile_deletion(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);

    /* Create profiles */
    output_profile_t *profile1 = profile_manager_create_profile(manager, "Profile 1");
    output_profile_t *profile2 = profile_manager_create_profile(manager, "Profile 2");
    output_profile_t *profile3 = profile_manager_create_profile(manager, "Profile 3");

    ASSERT_EQ(manager->profile_count, 3, "Should have 3 profiles");

    /* Delete middle profile */
    bool deleted = profile_manager_delete_profile(manager, profile2->profile_id);
    ASSERT_TRUE(deleted, "Profile deletion should succeed");
    ASSERT_EQ(manager->profile_count, 2, "Should have 2 profiles after deletion");

    /* Verify remaining profiles */
    output_profile_t *remaining1 = profile_manager_get_profile_at(manager, 0);
    output_profile_t *remaining2 = profile_manager_get_profile_at(manager, 1);
    ASSERT_NOT_NULL(remaining1, "First profile should exist");
    ASSERT_NOT_NULL(remaining2, "Second profile should exist");

    /* Profiles should be profile1 and profile3 */
    bool has_profile1 = (strcmp(remaining1->profile_name, "Profile 1") == 0 ||
                        strcmp(remaining2->profile_name, "Profile 1") == 0);
    bool has_profile3 = (strcmp(remaining1->profile_name, "Profile 3") == 0 ||
                        strcmp(remaining2->profile_name, "Profile 3") == 0);

    ASSERT_TRUE(has_profile1, "Profile 1 should still exist");
    ASSERT_TRUE(has_profile3, "Profile 3 should still exist");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Destination Addition */
static bool test_destination_addition(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");

    /* Add destination */
    encoding_settings_t encoding = profile_get_default_encoding();
    encoding.bitrate = 5000;
    encoding.width = 1920;
    encoding.height = 1080;

    bool added = profile_add_destination(profile, SERVICE_YOUTUBE, "test-stream-key",
                                        ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_TRUE(added, "Destination should be added");
    ASSERT_EQ(profile->destination_count, 1, "Should have 1 destination");

    /* Verify destination properties */
    profile_destination_t *dest = &profile->destinations[0];
    ASSERT_EQ(dest->service, SERVICE_YOUTUBE, "Service should be YouTube");
    ASSERT_STR_EQ(dest->stream_key, "test-stream-key", "Stream key should match");
    ASSERT_EQ(dest->encoding.bitrate, 5000, "Bitrate should be 5000");
    ASSERT_EQ(dest->encoding.width, 1920, "Width should be 1920");
    ASSERT_EQ(dest->encoding.height, 1080, "Height should be 1080");
    ASSERT_TRUE(dest->enabled, "Destination should be enabled by default");

    /* Verify backup/failover initialization */
    ASSERT_FALSE(dest->is_backup, "Should not be a backup");
    ASSERT_EQ(dest->primary_index, (size_t)-1, "Primary index should be unset");
    ASSERT_EQ(dest->backup_index, (size_t)-1, "Backup index should be unset");
    ASSERT_FALSE(dest->failover_active, "Failover should not be active");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Multiple Destinations */
static bool test_multiple_destinations(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Multi-Dest Profile");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add multiple destinations */
    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_TWITCH, "twitch-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_FACEBOOK, "facebook-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(profile->destination_count, 3, "Should have 3 destinations");

    /* Verify each destination */
    ASSERT_EQ(profile->destinations[0].service, SERVICE_YOUTUBE, "First should be YouTube");
    ASSERT_EQ(profile->destinations[1].service, SERVICE_TWITCH, "Second should be Twitch");
    ASSERT_EQ(profile->destinations[2].service, SERVICE_FACEBOOK, "Third should be Facebook");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Destination Removal */
static bool test_destination_removal(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add 3 destinations */
    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_TWITCH, "twitch-key",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_FACEBOOK, "facebook-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(profile->destination_count, 3, "Should have 3 destinations");

    /* Remove middle destination */
    bool removed = profile_remove_destination(profile, 1);
    ASSERT_TRUE(removed, "Destination removal should succeed");
    ASSERT_EQ(profile->destination_count, 2, "Should have 2 destinations after removal");

    /* Verify remaining destinations */
    ASSERT_EQ(profile->destinations[0].service, SERVICE_YOUTUBE, "First should still be YouTube");
    ASSERT_EQ(profile->destinations[1].service, SERVICE_FACEBOOK, "Second should now be Facebook");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Enable/Disable Destination */
static bool test_destination_enable_disable(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");

    encoding_settings_t encoding = profile_get_default_encoding();
    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_TRUE(profile->destinations[0].enabled, "Destination should be enabled initially");

    /* Disable destination */
    bool result = profile_set_destination_enabled(profile, 0, false);
    ASSERT_TRUE(result, "Disable should succeed");
    ASSERT_FALSE(profile->destinations[0].enabled, "Destination should be disabled");

    /* Re-enable destination */
    result = profile_set_destination_enabled(profile, 0, true);
    ASSERT_TRUE(result, "Enable should succeed");
    ASSERT_TRUE(profile->destinations[0].enabled, "Destination should be enabled");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Encoding Settings Update */
static bool test_encoding_update(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");

    encoding_settings_t encoding = profile_get_default_encoding();
    encoding.bitrate = 5000;

    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-key",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(profile->destinations[0].encoding.bitrate, 5000, "Initial bitrate should be 5000");

    /* Update encoding */
    encoding_settings_t new_encoding = encoding;
    new_encoding.bitrate = 8000;
    new_encoding.width = 2560;
    new_encoding.height = 1440;

    bool updated = profile_update_destination_encoding(profile, 0, &new_encoding);
    ASSERT_TRUE(updated, "Encoding update should succeed");

    /* Verify updated values */
    ASSERT_EQ(profile->destinations[0].encoding.bitrate, 8000, "Bitrate should be updated to 8000");
    ASSERT_EQ(profile->destinations[0].encoding.width, 2560, "Width should be updated to 2560");
    ASSERT_EQ(profile->destinations[0].encoding.height, 1440, "Height should be updated to 1440");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Null Pointer Safety */
static bool test_null_pointer_safety(void) {
    /* Test NULL profile manager destruction */
    profile_manager_destroy(NULL); /* Should not crash */

    /* Test NULL profile creation */
    output_profile_t *profile = profile_manager_create_profile(NULL, "Test");
    ASSERT_NULL(profile, "Should return NULL for NULL manager");

    /* Test NULL profile deletion */
    bool deleted = profile_manager_delete_profile(NULL, "test-id");
    ASSERT_FALSE(deleted, "Should return false for NULL manager");

    /* Test NULL destination addition */
    bool added = profile_add_destination(NULL, SERVICE_YOUTUBE, "key",
                                        ORIENTATION_HORIZONTAL, NULL);
    ASSERT_FALSE(added, "Should return false for NULL profile");

    return true;
}

/* Test: Boundary Conditions */
static bool test_boundary_conditions(void) {
    restreamer_api_t *api = create_mock_api();
    profile_manager_t *manager = profile_manager_create(api);
    output_profile_t *profile = profile_manager_create_profile(manager, "Test Profile");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Test invalid destination index */
    bool removed = profile_remove_destination(profile, 999);
    ASSERT_FALSE(removed, "Should fail to remove non-existent destination");

    bool enabled = profile_set_destination_enabled(profile, 999, false);
    ASSERT_FALSE(enabled, "Should fail to enable/disable non-existent destination");

    bool updated = profile_update_destination_encoding(profile, 999, &encoding);
    ASSERT_FALSE(updated, "Should fail to update non-existent destination");

    /* Test removing from empty profile */
    removed = profile_remove_destination(profile, 0);
    ASSERT_FALSE(removed, "Should fail to remove from empty profile");

    profile_manager_destroy(manager);
    return true;
}

BEGIN_TEST_SUITE("Profile Management")
    RUN_TEST(test_profile_manager_lifecycle, "Profile Manager Lifecycle");
    RUN_TEST(test_profile_creation, "Profile Creation");
    RUN_TEST(test_profile_deletion, "Profile Deletion");
    RUN_TEST(test_destination_addition, "Destination Addition");
    RUN_TEST(test_multiple_destinations, "Multiple Destinations");
    RUN_TEST(test_destination_removal, "Destination Removal");
    RUN_TEST(test_destination_enable_disable, "Enable/Disable Destination");
    RUN_TEST(test_encoding_update, "Encoding Settings Update");
    RUN_TEST(test_null_pointer_safety, "Null Pointer Safety");
    RUN_TEST(test_boundary_conditions, "Boundary Conditions");
END_TEST_SUITE()
